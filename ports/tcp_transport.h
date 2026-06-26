/*
 * ***************************************************************************
 * File:        transport/include/tcp_transport.h
 * Description: POSIX TCP Socket Server Transport Interface.
 * Overview:    This header declares the configuration structures and public
 *   symbols for the POSIX TCP socket server transport driver.
 *
 * By implementing the generic transport_ops_s interface, this
 * driver allows protocol adapters (such as Modbus TCP) to
 * communicate over standard IPv4 networks in a decoupled manner.
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

#ifndef __TRANSPORT_TCP_TRANSPORT_H
#define __TRANSPORT_TCP_TRANSPORT_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "adapter.h"

/****************************************************************************
 * Public Types
 ****************************************************************************/

struct tcp_config_s
{
  const char *ip_address; /* IP address to bind (e.g., "127.0.0.1") */
  uint16_t    port;       /* TCP port to listen on (e.g., 502 for Modbus) */
};

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

#ifdef __cplusplus
extern "C"
{
#endif

/****************************************************************************
 * Name: tcp_transport_register
 *
 * Description:
 * Registers a TCP transport channel by allocating a context from the
 * static pool and configuring it with the provided parameters.
 *
 * Input Parameters:
 * config - Pointer to the TCP server configuration structure.
 *
 * Returned Value:
 * Opaque pointer to the registered transport context, or NULL if the pool
 * is full.
 ****************************************************************************/

struct tcp_context_s *tcp_transport_register(const struct tcp_config_s *config);

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

const struct transport_ops_s *tty_transport_get_ops(void);

#ifdef __cplusplus
}
#endif

#endif /* __TRANSPORT_TCP_TRANSPORT_H */
