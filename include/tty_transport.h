/*
 * ***************************************************************************
 * File:        transport/include/tty_transport.h
 * Description: POSIX Terminal (TTY) Transport Interface.
 * Overview:    This header declares the configuration structures and public
 *              symbols for the POSIX terminal (TTY) transport driver.
 *
 *              By implementing the generic transport_ops_s interface, this
 *              driver allows protocol adapters (such as Modbus RTU) to
 *              communicate over physical serial interfaces, virtual serial
 *              ports (PTY), or serial transceivers in a decoupled manner.
 *
 *              The concrete transport dependencies are injected into adapter
 *              instances at runtime, achieving complete separation of
 *              concerns between protocol framing and target operating
 *              system I/O.
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

#ifndef __TRANSPORT_TTY_TRANSPORT_H
#define __TRANSPORT_TTY_TRANSPORT_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "adapter.h"

/****************************************************************************
 * Public Types
 ****************************************************************************/

struct tty_config_s
{
  const char *device;
  int         baudrate;
};

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

#ifdef __cplusplus
extern "C"
{
#endif

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

void *tty_transport_register(const struct tty_config_s *config);

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

#endif /* __TRANSPORT_TTY_TRANSPORT_H */
