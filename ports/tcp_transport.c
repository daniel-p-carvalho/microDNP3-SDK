/*
 * ***************************************************************************
 * File:        transport/src/tcp_transport.c
 * Description: POSIX TCP Socket Server Transport Implementation.
 * Overview:    This driver implements non-blocking TCP server communication
 *   using the standard BSD socket framework.
 *
 * It maps standard network system calls (socket, bind, listen,
 * accept, send, recv, select) to the generic transport operations
 * defined in adapter.h. It handles incoming client connections
 * and configures file descriptors to perform non-blocking I/O.
 *
 * By utilizing select-based non-blocking wait times, it ensures
 * that read and write calls respect timeout parameters without
 * stalling the execution thread of the protocol adapter.
 *
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

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>

#include "tcp_transport.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#ifndef CONFIG_TCP_NUM_INSTANCES
#  define CONFIG_TCP_NUM_INSTANCES 1
#endif

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct tcp_context_s
{
  struct transport_s  base;      /* Base abstract transport object. */
  int                 server_fd; /* Server passive listening socket descriptor. */
  int                 client_fd; /* Active accepted client socket descriptor. */
  struct tcp_config_s config;    /* Cloned network configuration. */
  bool                in_use;    /* Static pool allocation flag. */
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static int tcp_open(void *context);
static int tcp_send(void *context, const uint8_t *buffer, size_t length);
static int tcp_recv(void *context, uint8_t *buffer, size_t max_length,
                    uint32_t timeout_ms);
static int tcp_close(void *context);

/****************************************************************************
 * Private Data
 ****************************************************************************/

#ifndef CONFIG_TTY_NUM_INSTANCES
#  define CONFIG_TTY_NUM_INSTANCES 1
#endif

/* Statically allocated pool to enforce zero dynamic memory allocation. */

static struct tcp_context_s g_tcp_instances[CONFIG_TCP_NUM_INSTANCES];

/* Global interface operations table for the TCP driver. */

static const struct transport_ops_s g_tcp_transport =
{
  .open  = tcp_open,
  .send  = tcp_send,
  .recv  = tcp_recv,
  .close = tcp_close
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: tcp_open
 *
 * Description:
 * Creates a passive TCP listening socket, configures address reuse options,
 * binds to the requested IP/port, and places the socket in listen mode.
 *
 * Input Parameters:
 * context - Pointer to the private transport context structure.
 *
 * Returned Value:
 * 0 on success; a negative error code on failure.
 ****************************************************************************/

static int tcp_open(void *context)
{
  if (!context)
    {
      return -1;
    }

  struct tcp_context_s *ctx = (struct tcp_context_s *)context;

  /* Instantiate an IPv4 stream socket over TCP. */
  ctx->server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (ctx->server_fd == -1)
    {
      return -1;
    }

  /* Allow immediate port reuse to avoid 'Address already in use' errors on restarts. */
  int opt = 1;
  if (setsockopt(ctx->server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
      close(ctx->server_fd);
      ctx->server_fd = -1;
      return -1;
    }

  /* Set up the IPv4 target network address structure. */
  struct sockaddr_in address;
  memset(&address, 0, sizeof(address));
  address.sin_family = AF_INET;
  address.sin_port = htons(ctx->config.port);

  if (!ctx->config.ip_address || strlen(ctx->config.ip_address) == 0)
    {
      address.sin_addr.s_addr = INADDR_ANY;
    }
  else
    {
      if (inet_pton(AF_INET, ctx->config.ip_address, &address.sin_addr) <= 0)
        {
          close(ctx->server_fd);
          ctx->server_fd = -1;
          return -1;
        }
    }

  /* Bind the socket to the network interface. */
  if (bind(ctx->server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
      close(ctx->server_fd);
      ctx->server_fd = -1;
      return -1;
    }

  /* Enter listening mode with a standard connection backlog queue. */
  if (listen(ctx->server_fd, 3) < 0)
    {
      close(ctx->server_fd);
      ctx->server_fd = -1;
      return -1;
    }

  /* Initialize the client descriptor as disconnected. */
  ctx->client_fd = -1;
  return 0;
}

/****************************************************************************
 * Name: tcp_send
 *
 * Description:
 * Dispatches raw data over the active client socket. Uses non-blocking 
 * select checks to ensure network flow control does not stall execution.
 *
 * Input Parameters:
 * context - Pointer to the private transport context structure.
 * buffer  - Destination source data byte stream.
 * length  - Total number of bytes to transmit.
 *
 * Returned Value:
 * Number of bytes successfully written, or a negative error code on failure.
 ****************************************************************************/

static int tcp_send(void *context, const uint8_t *buffer, size_t length)
{
  if (!context)
    {
      return -1;
    }

  struct tcp_context_s *ctx = (struct tcp_context_s *)context;
  
  /* Fail immediately if there is no active client connected. */
  int fd = ctx->client_fd;
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

      /* Wait until the socket is writable or a 1-second timeout expires. */
      int ret = select(fd + 1, NULL, &wfds, NULL, &tv);
      if (ret < 0)
        {
          return -1;
        }
      else if (ret == 0)
        {
          return (int)total; /* Socket buffer full, return partial bytes sent. */
        }

      /* Transmit the remaining chunk of data. MSG_NOSIGNAL prevents crashes if client drops. */
      ssize_t w = send(fd, buffer + total, length - total, MSG_NOSIGNAL);
      if (w <= 0)
        {
          /* Connection broken or closed by peer. Clean up descriptor. */
          close(ctx->client_fd);
          ctx->client_fd = -1;
          return -1;
        }
      total += w;
    }

  return (int)total;
}

/****************************************************************************
 * Name: tcp_recv
 *
 * Description:
 * Handles incoming client management and read processing. If no client is
 * connected, it performs a non-blocking poll on the listening server socket
 * to accept a connection. Once connected, it reads incoming bytes into 
 * the target buffer using select timeouts.
 *
 * Input Parameters:
 * context    - Pointer to the private transport context structure.
 * buffer     - Destination buffer for incoming data.
 * max_length - Maximum number of bytes requested by the protocol stack.
 * timeout_ms - Read timeout limit in milliseconds.
 *
 * Returned Value:
 * Number of bytes read, or a negative error code on failure.
 ****************************************************************************/

static int tcp_recv(void *context, uint8_t *buffer, size_t max_length,
                    uint32_t timeout_ms)
{
  if (!context)
    {
      return -1;
    }

  struct tcp_context_s *ctx = (struct tcp_context_s *)context;

  /* Step 1: Connection Management. If no active client exists, try to accept one. */

  if (ctx->client_fd == -1)
    {
      if (ctx->server_fd == -1)
        {
          return -1;
        }

      fd_set accept_fds;
      FD_ZERO(&accept_fds);
      FD_SET(ctx->server_fd, &accept_fds);

      /* Use a rapid 1ms non-blocking check on the listening socket during polling loops. */

      struct timeval accept_tv;
      accept_tv.tv_sec = 0;
      accept_tv.tv_usec = 1000;

      int accept_sel = select(ctx->server_fd + 1, &accept_fds, NULL, NULL, &accept_tv);
      if (accept_sel > 0)
        {
          struct sockaddr_in client_addr;
          socklen_t client_len = sizeof(client_addr);
          
          ctx->client_fd = accept((ctx->server_fd), (struct sockaddr *)&client_addr, &client_len);
          if (ctx->client_fd != -1)
            {
              /* Configure the newly accepted socket to enforce strict non-blocking operations */

              int flags = fcntl(ctx->client_fd, F_GETFL, 0);
              fcntl(ctx->client_fd, F_SETFL, flags | O_NONBLOCK);
            }
        }

      /* No client is active yet; return 0 bytes read to maintain non-blocking stack execution. */

      if (ctx->client_fd == -1)
        {
          return 0;
        }
    }

  /* Step 2: Read processing for the connected client. */

  int fd = ctx->client_fd;
  size_t total = 0;

  while (total < max_length)
    {
      fd_set rfds;
      FD_ZERO(&rfds);
      FD_SET(fd, &rfds);

      struct timeval tv;
      tv.tv_sec = timeout_ms / 1000;
      tv.tv_usec = (timeout_ms % 1000) * 1000;

      /* Wait until data is physically ready on the network socket wire.
       * This guarantees that our subsequent non-blocking recv will not 
       * trigger busy-waiting cycles on EAGAIN.
       */

      int ret = select(fd + 1, &rfds, NULL, NULL, &tv);
      if (ret < 0)
        {
          return -1; /* Select system call failure */
        }
      else if (ret == 0)
        {
          return (int)total; /* Timeout expired, return accumulated bytes to the stack */
        }

      /* Read a transparent chunk of the stream into the target buffer. 
       * The nanoMODBUS library natively manages the 7-byte MBAP envelope parsing.
       */

      ssize_t r = recv(fd, buffer + total, max_length - total, 0);
      if (r < 0)
        {
          /* If transient non-blocking conditions occur, yield and re-evaluate via select */
          if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
              continue;
            }
          return -1; /* General socket/network read error */
        }
      else if (r == 0)
        {
          /* Connection closed cleanly by the remote test client. Reset descriptor state. */
          close(ctx->client_fd);
          ctx->client_fd = -1;
          return (int)total;
        }

      total += (size_t)r;
    }

  return (int)total;
}

/****************************************************************************
 * Name: tcp_close
 *
 * Description:
 * Safely terminates active client connections and shuts down the core
 * passive listening server socket interface.
 *
 * Input Parameters:
 * context - Pointer to the private transport context structure.
 *
 * Returned Value:
 * 0 on success; a negative error code on failure.
 ****************************************************************************/

static int tcp_close(void *context)
{
  if (!context)
    {
      return -1;
    }

  struct tcp_context_s *ctx = (struct tcp_context_s *)context;

  /* Sever active client pipes. */
  if (ctx->client_fd != -1)
    {
      close(ctx->client_fd);
      ctx->client_fd = -1;
    }

  /* Shutdown passive network listeners. */
  if (ctx->server_fd != -1)
    {
      close(ctx->server_fd);
      ctx->server_fd = -1;
    }

  return 0;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: tcp_transport_register
 *
 * Description:
 * Allocates and binds a persistent context from the static pool, avoiding
 * heap transactions and satisfying zero-dynamic allocation architectures.
 *
 * Input Parameters:
 * config - Pointer to the initialization network properties.
 *
 * Returned Value:
 * Opaque handle pointer to the registered transport base.
 ****************************************************************************/

struct tcp_context_s *tcp_transport_register(const struct tcp_config_s *config)
{
  if (!config)
    {
      return NULL;
    }

  for (uint8_t i = 0; i < CONFIG_TCP_NUM_INSTANCES; i++)
    {
      if (!g_tcp_instances[i].in_use)
        {
          g_tcp_instances[i].base.ops = &g_tcp_transport; /* Inject operations mapping. */
          g_tcp_instances[i].server_fd = -1;
          g_tcp_instances[i].client_fd = -1;
          g_tcp_instances[i].config    = *config;          /* Clone configuration properties. */
          g_tcp_instances[i].in_use    = true;
          return &g_tcp_instances[i];
        }
    }

  return NULL;
}