/*
 * ***************************************************************************
 * File:        transport/src/tty_transport.c
 * Description: POSIX Terminal (TTY) Transport Implementation.
 * Overview:    This driver implements non-blocking serial communication
 *              using the POSIX termios framework.
 *
 *              It maps standard POSIX system calls (open, close, read, write,
 *              select) to the generic transport operations defined in
 *              adapter.h. It configures the TTY descriptor in raw mode
 *              with 8N1 framing (8 data bits, no parity, 1 stop bit) and
 *              supports standard baud rates.
 *
 *              By utilizing select-based non-blocking wait times, it ensures
 *              that read and write calls respect timeout parameters without
 *              stalling the execution thread of the protocol adapter.
 * Author:      Daniel P. Carvalho <danieloak@gmail.com>
 * --------------------------------------------------------------------------
 * License: BSD-3-Clause
 *
 * Copyright (c) 2026, Daniel P. Carvalho. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 ****************************************************************************
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/

/* Define _DEFAULT_SOURCE to expose POSIX/BSD extensions (e.g. termios,
 * select) when compiling under the strict C99 standard.
 */
#define _DEFAULT_SOURCE

#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <sys/select.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "tty_transport.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#ifndef CONFIG_TTY_NUM_INSTANCES
#  define CONFIG_TTY_NUM_INSTANCES 1
#endif

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct tty_context_s
{
  struct transport_s  base;
  int                 tty_fd;
  struct tty_config_s config;
  bool                in_use;
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static int tty_open(void *context);
static int tty_send(void *context, const uint8_t *buffer, size_t length);
static int tty_recv(void *context, uint8_t *buffer, size_t max_length,
                    uint32_t timeout_ms);
static int tty_close(void *context);

/****************************************************************************
 * Private Data
 ****************************************************************************/

static struct tty_context_s g_tty_instances[CONFIG_TTY_NUM_INSTANCES];

static const struct transport_ops_s g_tty_transport =
{
  .open  = tty_open,
  .send  = tty_send,
  .recv  = tty_recv,
  .close = tty_close
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: tty_open
 *
 * Description:
 *   Opens the serial port device file descriptor and configures terminal
 *   options in raw mode with the requested baudrate (8N1 configuration).
 *
 * Input Parameters:
 *   context - Pointer to the generic transport context (points to
 *             struct tty_context_s).
 *
 * Returned Value:
 *   0 on success; a negative error code on failure.
 ****************************************************************************/

static int tty_open(void *context)
{
  if (!context)
    {
      return -1;
    }

  struct tty_context_s *ctx = (struct tty_context_s *)context;

  ctx->tty_fd = open(ctx->config.device, O_RDWR | O_NOCTTY | O_NDELAY);
  if (ctx->tty_fd == -1)
    {
      return -1;
    }

  fcntl(ctx->tty_fd, F_SETFL, 0);

  struct termios options;
  if (tcgetattr(ctx->tty_fd, &options) != 0)
    {
      close(ctx->tty_fd);
      ctx->tty_fd = -1;
      return -1;
    }

  speed_t speed;
  switch (ctx->config.baudrate)
    {
    case 9600:   speed = B9600;   break;
    case 19200:  speed = B19200;  break;
    case 38400:  speed = B38400;  break;
    case 57600:  speed = B57600;  break;
    case 115200: speed = B115200; break;
    default:     speed = B115200; break;
    }

  cfsetispeed(&options, speed);
  cfsetospeed(&options, speed);

  options.c_cflag &= ~PARENB;
  options.c_cflag &= ~CSTOPB;
  options.c_cflag &= ~CSIZE;
  options.c_cflag |= CS8;
  options.c_cflag |= (CLOCAL | CREAD);

  options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
  options.c_oflag &= ~OPOST;
  options.c_iflag &= ~(IXON | IXOFF | IXANY);
  options.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP |
                       INLCR | IGNCR | ICRNL);

  options.c_cc[VMIN] = 0;
  options.c_cc[VTIME] = 1;

  if (tcsetattr(ctx->tty_fd, TCSANOW, &options) != 0)
    {
      close(ctx->tty_fd);
      ctx->tty_fd = -1;
      return -1;
    }

  return 0;
}

/****************************************************************************
 * Name: tty_send
 *
 * Description:
 *   Sends a buffer of bytes through the open serial port descriptor.
 *   Uses select to perform a non-blocking wait.
 *
 * Input Parameters:
 *   context - Pointer to the generic transport context.
 *   buffer  - Pointer to the source data buffer to write.
 *   length  - Number of bytes to write.
 *
 * Returned Value:
 *   Number of bytes successfully written, or a negative error code on
 *   failure.
 ****************************************************************************/

static int tty_send(void *context, const uint8_t *buffer,
                    size_t length)
{
  if (!context)
    {
      return -1;
    }

  struct tty_context_s *ctx = (struct tty_context_s *)context;
  int fd = ctx->tty_fd;

  if (fd == -1)
    {
      return -1;
    }

  size_t total = 0;
  while (total < length)
    {
      fd_set wfds;
      FD_ZERO(&wfds);
      FD_SET(fd, &wfds);

      struct timeval tv;
      tv.tv_sec = 1;
      tv.tv_usec = 0;

      int ret = select(fd + 1, NULL, &wfds, NULL, &tv);
      if (ret < 0)
        {
          return -1;
        }
      else if (ret == 0)
        {
          return (int)total;
        }

      ssize_t w = write(fd, buffer + total, length - total);
      if (w <= 0)
        {
          return -1;
        }
      total += w;
    }

  return (int)total;
}

/****************************************************************************
 * Name: tty_recv
 *
 * Description:
 *   Receives incoming bytes from the serial port into a buffer.
 *   Uses select to handle the timeout-based wait.
 *
 * Input Parameters:
 *   context    - Pointer to the generic transport context.
 *   buffer     - Pointer to the destination buffer.
 *   max_length - Maximum number of bytes to read.
 *   timeout_ms - Read timeout limit in milliseconds.
 *
 * Returned Value:
 *   Number of bytes successfully read, or a negative error code on
 *   failure.
 ****************************************************************************/

static int tty_recv(void *context, uint8_t *buffer,
                    size_t max_length, uint32_t timeout_ms)
{
  if (!context)
    {
      return -1;
    }

  struct tty_context_s *ctx = (struct tty_context_s *)context;
  int fd = ctx->tty_fd;

  if (fd == -1)
    {
      return -1;
    }

  size_t total = 0;
  while (total < max_length)
    {
      fd_set rfds;
      FD_ZERO(&rfds);
      FD_SET(fd, &rfds);

      struct timeval tv;
      tv.tv_sec = timeout_ms / 1000;
      tv.tv_usec = (timeout_ms % 1000) * 1000;

      int ret = select(fd + 1, &rfds, NULL, NULL, &tv);
      if (ret < 0)
        {
          return -1;
        }
      else if (ret == 0)
        {
          return (int)total;
        }

      ssize_t r = read(fd, buffer + total, max_length - total);
      if (r < 0)
        {
          return -1;
        }
      else if (r == 0)
        {
          return (int)total;
        }
      total += (size_t)r;
    }

  return (int)total;
}

/****************************************************************************
 * Name: tty_close
 *
 * Description:
 *   Closes the open serial port device file descriptor.
 *
 * Input Parameters:
 *   context - Pointer to the generic transport context.
 *
 * Returned Value:
 *   0 on success; a negative error code on failure.
 ****************************************************************************/

static int tty_close(void *context)
{
  if (!context)
    {
      return -1;
    }

  struct tty_context_s *ctx = (struct tty_context_s *)context;

  if (ctx->tty_fd != -1)
    {
      close(ctx->tty_fd);
      ctx->tty_fd = -1;
    }

  return 0;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: tty_transport_register
 *
 * Description:
 *   Registers a TTY transport channel by allocating a context from the
 *   static pool and configuring it with the provided parameters.
 *
 * Input Parameters:
 *   config - Pointer to the TTY configuration structure.
 *
 * Returned Value:
 *   Opaque pointer to the registered transport context, or NULL if the pool
 *   is full.
 ****************************************************************************/

void *tty_transport_register(const struct tty_config_s *config)
{
  if (!config)
    {
      return NULL;
    }

  for (uint8_t i = 0; i < CONFIG_TTY_NUM_INSTANCES; i++)
    {
      if (!g_tty_instances[i].in_use)
        {
          g_tty_instances[i].base.ops = &g_tty_transport;
          g_tty_instances[i].tty_fd = -1;
          g_tty_instances[i].config = *config;
          g_tty_instances[i].in_use = true;
          return &g_tty_instances[i];
        }
    }

  return NULL;
}

/****************************************************************************
 * Name: tty_transport_get_ops
 *
 * Description:
 *   Retrieves the public operations VTable for the TTY transport.
 *
 * Input Parameters:
 *   None.
 *
 * Returned Value:
 *   Pointer to the global transport operations structure.
 ****************************************************************************/

const struct transport_ops_s *tty_transport_get_ops(void)
{
  return &g_tty_transport;
}
