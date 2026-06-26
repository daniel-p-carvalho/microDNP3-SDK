/*
 *****************************************************************************
 * File:        core/include/transport.h
 * Description: Generic I/O Transport Interface Abstraction.
 * Overview:    This header defines a generic, abstract interface structure
 *   based on function pointer tables for all physical and logical
 *   communication channels (e.g., Linux POSIX termios/serial, BSD network
 *   sockets, RTOS target drivers, or virtual PTY loops).
 *
 * By decoupling the protocol engine from concrete I/O hardware or underlying
 * operating system APIs, this interface ensures strict hardware agnosticism
 * and absolute target portability.
 *
 * Memory Model:
 * Transports adhere strictly to a Zero Dynamic Allocation paradigm. Concrete
 * drivers must allocate their private runtime contexts statically. Type
 * abstraction is achieved via a mandatory structured layout that enables
 * predictable runtime pointer conversions (upcasting/downcasting) between the
 * generic base structure and the private driver memory block.
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

#ifndef __CORE_INCLUDE_TRANSPORT_H
#define __CORE_INCLUDE_TRANSPORT_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdint.h>
#include <stddef.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Helper macros for clean, type-safe transport operations access */
#define TRANSPORT_OPEN(t)           ((t)->ops->open(t))
#define TRANSPORT_SEND(t, buf, len) ((t)->ops->send(t, buf, len))
#define TRANSPORT_CLOSE(t)          ((t)->ops->close(t))

#define TRANSPORT_RECV(t, buf, max_len, timeout_ms) \
  ((t)->ops->recv(t, buf, max_len, timeout_ms))

/****************************************************************************
 * Public Types
 ****************************************************************************/

/* Forward declarations */
struct transport_ops_s;

/* Generic polymorphic base structure for transport channels. Concrete drivers 
 * must place this as their first member to enable type-safe downcasting in C.
 */
struct transport_s
{
  const struct transport_ops_s *ops;  /* VTable of physical I/O callbacks */
};

/* Table of generic transport operations (Virtual Method Table). Every concrete
 * hardware or OS-specific communication driver must implement these routines.
 */
struct transport_ops_s
{
  /**************************************************************************
   * Name: open
   *
   * Description:
   * Initializes and opens the physical or logical communication channel 
   * (e.g., configures tty serial baudrates, binds ports, or allocates
   * static RTOS peripheral descriptors).
   *
   * Input Parameters:
   * context - Pointer to the generic transport instance (struct transport_s).
   * The concrete driver implementation performs a safe static 
   * downcast on this pointer to recover its private static RAM context.
   *
   * Returned Value:
   * 0 on success; a negative error code on failure.
   **************************************************************************/
  int (*open)(void *context);

  /**************************************************************************
   * Name: send
   *
   * Description:
   * Transmits a raw block of bytes over the active communication medium.
   * This call should be non-blocking or respect internal driver timeouts.
   *
   * Input Parameters:
   * context - Pointer to the generic transport instance.
   * buffer  - Pointer to the source data buffer to be transmitted.
   * length  - Total number of bytes to write.
   *
   * Returned Value:
   * The number of bytes successfully written on success; a negative error 
   * code on failure.
   **************************************************************************/
  int (*send)(void *context, const uint8_t *buffer, size_t length);

  /**************************************************************************
   * Name: recv
   *
   * Description:
   * Receives a raw block of bytes from the communication medium. This 
   * callback implements a strict, non-blocking or timed-bounded operation.
   *
   * Input Parameters:
   * context    - Pointer to the generic transport instance.
   * buffer     - Pointer to the destination memory buffer.
   * max_length - Maximum number of bytes to copy into the buffer.
   * timeout_ms - Maximum window to wait for incoming characters, matching 
   * the non-blocking requirements of the master protocol loop.
   *
   * Returned Value:
   * The number of bytes successfully read on success; 0 on timeout; 
   * a negative error code on failure.
   **************************************************************************/
  int (*recv)(void *context, uint8_t *buffer, size_t max_length,
              uint32_t timeout_ms);

  /**************************************************************************
   * Name: close
   *
   * Description:
   * Cleanly closes the communication link, releasing system handles, 
   * closing file descriptors, and flushing hardware registers back to safe 
   * initial values without performing any dynamic heap deallocation.
   *
   * Input Parameters:
   * context - Pointer to the generic transport instance.
   *
   * Returned Value:
   * 0 on success; a negative error code on failure.
   **************************************************************************/
  int (*close)(void *context);
};

#endif /* __CORE_INCLUDE_TRANSPORT_H */