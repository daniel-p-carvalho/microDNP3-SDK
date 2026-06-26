/*
 *****************************************************************************
 * File:        core/include/adapter.h
 * Description: Generic Protocol Adapter Interface Abstraction.
 * Overview:    This header defines a generic, abstract interface structure
 *   based on function pointer tables for all protocol adapters (e.g.,
 *   Modbus, DNP3, MQTT).
 *
 * By decoupling the main execution loop from concrete protocol-specific
 * implementations, this interface allows the core system to manage an array
 * of adapters through an abstract handle and execute their state machines
 * with standardized functions.
 *
 * Memory Model:
 * Adapters adhere strictly to a Zero Dynamic Allocation paradigm. Concrete
 * drivers must allocate their private runtime contexts statically. Type
 * abstraction is achieved via predictable structural alignment, allowing
 * safe pointer conversions (upcasting/downcasting) between the generic base
 * structure and the private driver memory block.
 *
 * Lifecycle & Execution Flow:
 * 1. init  - Configures logical parameters and opens drivers.
 * 2. run   - Executed periodically with active time injection.
 * 3. close - Shuts down the adapter and resets static states.
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

#ifndef __CORE_INCLUDE_ADAPTER_H
#define __CORE_INCLUDE_ADAPTER_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdint.h>
#include <stddef.h>

#include "transport.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Public Types
 ****************************************************************************/

/* Telemetry point types supported in the Data Core. */

enum point_type_e
{
  POINT_TYPE_DIN = 0,  /* Digital Input (Data Core). */
  POINT_TYPE_DOUT,     /* Digital Output / Coil (Data Core). */
  POINT_TYPE_AIN,      /* Analog Input (Data Core). */
  POINT_TYPE_AOUT      /* Analog Output / Holding Register (Data Core). */
};

/* Defines the conversion and mapping rule of a telemetry data point. */

struct point_mapping_s
{
  uint16_t          protocol_address; /* Address in the protocol. */
  enum point_type_e core_point_type;  /* Corresponding type in Data Core. */
  uint16_t          core_point_index; /* Index in the Data Core. */
  uint8_t           adapt_type;       /* Protocol-specific format. */
  float             scale;            /* Scaling factor for conversion. */
  float             offset;           /* Linear offset. */
};

struct adapter_s;

/* Generic table of interface operations defining the core lifecycle callbacks
 * (init, run, close) that every concrete protocol adapter driver must
 * implement to hook into the main loop.
 */

struct adapter_ops_s
{
  /**************************************************************************
   * Name: init
   *
   * Description:
   *   Initializes the protocol-specific adapter instance. This callback
   *   opens communication channels (e.g., serial ports, network sockets),
   *   resets internal state variables, and registers callbacks.
   *
   * This function acts as the glue code between the generic system loop
   * and the protocol driver.
   *  - The 'instance' pointer allows the callback to perform a safe downcast
   *    to recover its own context.
   *  - The 'config' parameter serves as a type-agnostic channel to convey 
   *    initialization parameters, and its memory lifetime requirements
   *    depend strictly on the specific adapter implementation. Scalar
   *    variables (e.g., station IDs, port settings) can be cloned into the 
   *    driver's permanent static runtime state. However, complex
   *    configurations or heavy arrays (e.g., Data Core telemetry mapping 
   *    tables) may be stored by reference to save RAM. Therefore, the 
   *    documentation of each specific adapter driver MUST be consulted to 
   *    determine the required scope and lifetime of the config structure.
   *
   * Input Parameters:
   *   instance - Pointer to the generic adapter structure.
   *   config   - Pointer to the protocol-specific public config structure.
   *
   * Returned Value:
   *   0 on success; a negative error code on failure.
   **************************************************************************/

  int (*init)(struct adapter_s *instance, const void *config);

  /**************************************************************************
   * Name: run
   *
   * Description:
   *   Periodically executes the protocol's communication state machine.
   *   This callback must be strictly non-blocking.
   *
   * Industrial protocols (such as DNP3) rely heavily on active timers to
   * manage link-layer timeouts (inter-character gaps, link-layer ACKs) and
   * application-layer windows (SBO select-to-operate timeouts, MQTT keep-
   * alives). Directly calling hardware-specific timers (e.g., MCU hardware
   * TIMers) or kernel-dependent OS services inside the protocol drivers
   * would break the system main architectural design pillars: hardware
   * agnosticism and strict target portability.
   *
   * By implementing a "Time Injection" model, this callback shifts the
   * time-tracking responsibility entirely to the host application loop (or
   * RTOS dedicated task). The architecture remains clean, deterministic,
   * and fully operational across bare-metal environments and RTOS,
   * while maintaining a zero-dynamic-allocation (malloc-free) static
   * footprint.
   *
   * Input Parameters:
   *   instance        - Pointer to the generic adapter class structure.
   *   current_time_ms - Monotonic system time injected in milliseconds. Used
   *     locally by the protocol engines to evaluate logical timeouts via
   *     non-blocking delta comparison.
   *
   * Returned Value:
   *   None.
   **************************************************************************/

  void (*run)(struct adapter_s *instance, uint32_t current_time_ms);

  /**************************************************************************
   * Name: close
   *
   * Description:
   *   Cleanly shuts down the protocol adapter, releasing sockets or OS
   *   handles and resetting state variables to their initial values.
   *   Does not perform any dynamic heap deallocation.
   *
   * Input Parameters:
   *   instance - Pointer to the generic adapter class.
   *
   * Returned Value:
   *   None.
   **************************************************************************/

  void (*close)(struct adapter_s *instance);
};

/* Generic polymorphic base structure. Concrete drivers must place this as
 * their first member to enable type-safe downcasting in C.
 */

struct adapter_s
{
  const struct adapter_ops_s *ops;           /* Operations VTable pointer. */
};

#endif /* __CORE_INCLUDE_ADAPTER_H */
