/*
 ****************************************************************************
 * File:        core/include/data_core.h
 * Description: Data Core API and state definitions.
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

#ifndef __CORE_INCLUDE_DATA_CORE_H
#define __CORE_INCLUDE_DATA_CORE_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdbool.h>
#include <stdint.h>
#include "data_core_config.h"

/****************************************************************************
 * Public Types
 ****************************************************************************/

#if CONFIG_UDNP3_ENABLE_WALL_CLOCK

/****************************************************************************
 * Name: struct dc_time_s
 *
 * Description:
 * Agnostic container for absolute Wall-Clock representations (UTC time).
 *
 * Details:
 * Designed as a lightweight, platform-independent substitute for POSIX
 * time structures. Avoids layout size variations between 32-bit and 
 * 64-bit hardware architectures.
 ****************************************************************************/

struct dc_time_s
{
  uint64_t tv_sec;  /* Absolute time in seconds since Unix Epoch. */
  uint16_t tv_msec; /* Fractional millisecond component [0..999]. */
};

#endif /* CONFIG_UDNP3_ENABLE_WALL_CLOCK */

#if CONFIG_UDNP3_ENABLE_SOE

/* Point types supported by the SOE queue. */

enum dc_type_e
{
  DC_TYPE_DI = 0,
  DC_TYPE_DO,
  DC_TYPE_AI,
  DC_TYPE_AO
};

/* Individual event structure. */

struct dc_event_s
{
  uint64_t timestamp;   /* UTC timestamp in ms (8-byte aligned). */
  union
  {
    bool     bi;        /* Binary value (1 Byte). */
    float    an;        /* Analog value (4 Bytes). */
  } value;              /* Value union (4 Bytes). */

  uint16_t index;       /* Point index (2 Bytes). */
  uint8_t  type;        /* Point type (enum dc_type_e - 1 Byte). */
  uint8_t  quality;     /* Quality flags (1 Byte). */
};

/* Sequence of Events (SOE) circular queue. */

struct dc_soe_queue_s
{
  struct dc_event_s *events;
  uint16_t          size; /* Queue capacity (must be a power of 2). */
  uint16_t          head; /* Monotonic write index (2 Bytes). */
};

#endif /* CONFIG_UDNP3_ENABLE_SOE */

/* Data Core state container configuration structure. */

struct dc_config_s
{
  uint8_t  *din_buf;          /* Bit-packed Digital Inputs buffer pointer. */
  uint16_t  num_di;           /* Maximum number of Digital Inputs. */
  uint8_t  *dout_buf;         /* Bit-packed Digital Outputs buffer pointer. */
  uint16_t  num_do;           /* Maximum number of Digital Outputs. */
  float    *ain_buf;          /* Float Analog Inputs array pointer. */
  uint16_t  num_ai;           /* Maximum number of Analog Inputs. */
  float    *aout_buf;         /* Float Analog Outputs array pointer. */
  uint16_t  num_ao;           /* Maximum number of Analog Outputs. */

#if CONFIG_UDNP3_ENABLE_SOE
  struct dc_event_s *soe_events;     /* SOE events array pointer. */
  uint16_t           soe_queue_size; /* SOE queue capacity (power of 2). */
#endif
};

/* Data Core state container representing the Process Image. */

struct data_core_s
{
  uint8_t  *din;              /* Bit-packed Digital Inputs. */
  uint8_t  *dout;             /* Bit-packed Digital Outputs. */
  float    *ain;              /* Float Analog Inputs. */
  float    *aout;             /* Float Analog Outputs. */

  uint16_t  num_di;           /* Configured number of Digital Inputs. */
  uint16_t  num_do;           /* Configured number of Digital Outputs. */
  uint16_t  num_ai;           /* Configured number of Analog Inputs. */
  uint16_t  num_ao;           /* Configured number of Analog Outputs. */

#if CONFIG_UDNP3_ENABLE_SOE
  struct dc_soe_queue_s soe;         /* Circular event queue (SOE). */
#endif
};

#ifdef __cplusplus
extern "C"
{
#endif

/****************************************************************************
 * Public Data
 ****************************************************************************/

/* Global instance of the central Data Core. */

extern struct data_core_s g_data_core;

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/* Initialization API. */

/****************************************************************************
 * Name: dc_init
 *
 * Description:
 *   Initializes the central Data Core memory to zero.
 *
 * Details:
 *   Resets the global process image state structure using memset.
 *   This must be called at startup before any other Data Core
 *   API is invoked.
 *
 * Input Parameters:
 *   config - Pointer to configuration struct with buffers and point counts.
 *
 * Returned Value:
 *   None.
 *
 ****************************************************************************/

void dc_init(const struct dc_config_s *config);

/* Active Read/Write APIs for Process Image. */

/****************************************************************************
 * Name: dc_read_din
 *
 * Description:
 *   Reads a single bit-packed Digital Input value from the Process Image.
 *
 * Details:
 *   Evaluates the point index boundaries against CONFIG_UDNP3_NUM_DI.
 *   Divides the index by 8 to find the target byte offset, and computes
 *   the bit offset using modulo 8. Operates directly on the packed
 *   bit-vector.
 *
 * Input Parameters:
 *   index - Index of the DI channel.
 *   value - Pointer to store the read boolean state.
 *
 * Returned Value:
 *   True if the index is valid, false otherwise.
 ****************************************************************************/

bool dc_read_din(uint16_t index, bool *value);

/****************************************************************************
 * Name: dc_write_din
 *
 * Description:
 *   Writes a single bit-packed Digital Input value into the Process Image.
 *
 * Details:
 *   Evaluates the point index boundaries against CONFIG_UDNP3_NUM_DI.
 *   Divides the index by 8 to find the byte offset, and computes the
 *   bit offset using modulo 8. Updates the bit-vector directly.
 *
 * Input Parameters:
 *   index - Index of the DI channel.
 *   value - Boolean value to be written.
 *
 * Returned Value:
 *   True if the index is valid, false otherwise.
 ****************************************************************************/

bool dc_write_din(uint16_t index, bool value);

/****************************************************************************
 * Name: dc_read_dout
 *
 * Description:
 *   Reads a single bit-packed Digital Output value from the Process Image.
 *
 * Details:
 *   Evaluates the point index boundaries against CONFIG_UDNP3_NUM_DO.
 *   Divides the index by 8 to find the target byte offset, and computes
 *   the bit offset using modulo 8. Operates directly on the packed
 *   bit-vector.
 *
 * Input Parameters:
 *   index - Index of the DO channel.
 *   value - Pointer to store the read boolean state.
 *
 * Returned Value:
 *   True if the index is valid, false otherwise.
 ****************************************************************************/

bool dc_read_dout(uint16_t index, bool *value);

/****************************************************************************
 * Name: dc_write_dout
 *
 * Description:
 *   Writes a single bit-packed Digital Output value into the Process Image.
 *
 * Details:
 *   Evaluates the point index boundaries against CONFIG_UDNP3_NUM_DO.
 *   Divides the index by 8 to find the byte offset, and computes the
 *   bit offset using modulo 8. Updates the bit-vector directly.
 *
 * Input Parameters:
 *   index - Index of the DO channel.
 *   value - Boolean value to be written.
 *
 * Returned Value:
 *   True if the index is valid, false otherwise.
 ****************************************************************************/

bool dc_write_dout(uint16_t index, bool value);

/****************************************************************************
 * Name: dc_read_ain
 *
 * Description:
 *   Reads a single Analog Input value from the Process Image.
 *
 * Details:
 *   Performs bounds check against CONFIG_UDNP3_NUM_AI. Accesses the
 *   float element in the array directly.
 *
 * Input Parameters:
 *   index - Index of the AI channel.
 *   value - Pointer to store the read float value.
 *
 * Returned Value:
 *   True if the index is valid, false otherwise.
 ****************************************************************************/

bool dc_read_ain(uint16_t index, float *value);

/****************************************************************************
 * Name: dc_write_ain
 *
 * Description:
 *   Writes a single Analog Input value into the Process Image.
 *
 * Details:
 *   Performs bounds check against CONFIG_UDNP3_NUM_AI. Updates the
 *   float element in the array directly.
 *
 * Input Parameters:
 *   index - Index of the AI channel.
 *   value - Float value to write.
 *
 * Returned Value:
 *   True if the index is valid, false otherwise.
 ****************************************************************************/

bool dc_write_ain(uint16_t index, float value);

/****************************************************************************
 * Name: dc_read_aout
 *
 * Description:
 *   Reads a single Analog Output value from the Process Image.
 *
 * Details:
 *   Performs bounds check against CONFIG_UDNP3_NUM_AO. Accesses the
 *   float element in the array directly.
 *
 * Input Parameters:
 *   index - Index of the AO channel.
 *   value - Pointer to store the read float value.
 *
 * Returned Value:
 *   True if the index is valid, false otherwise.
 ****************************************************************************/

bool dc_read_aout(uint16_t index, float *value);

/****************************************************************************
 * Name: dc_write_aout
 *
 * Description:
 *   Writes a single Analog Output value into the Process Image.
 *
 * Details:
 *   Performs bounds check against CONFIG_UDNP3_NUM_AO. Updates the
 *   float element in the array directly.
 *
 * Input Parameters:
 *   index - Index of the AO channel.
 *   value - Float value to write.
 *
 * Returned Value:
 *   True if the index is valid, false otherwise.
 ****************************************************************************/

bool dc_write_aout(uint16_t index, float value);

#if CONFIG_UDNP3_ENABLE_POINTER_MODEL

/* Pointer Model APIs (Zero-Copy access to bytes and analog values). */

/****************************************************************************
 * Name: dc_get_din_byte_ptr
 *
 * Description:
 *   Pointer model access to a single byte of compact Digital Inputs.
 *
 * Details:
 *   Part of the Zero-Copy Pointer Model. Returns a direct pointer to
 *   a specific byte of the packed digital inputs array. Allows external
 *   stacks to reference raw bytes directly, saving RAM. The caller must
 *   lock access to g_data_core if accessed concurrently.
 *
 * Input Parameters:
 *   byte_index - Byte offset.
 *
 * Returned Value:
 *   Pointer to the byte, or NULL if out of bounds.
 ****************************************************************************/

uint8_t *dc_get_din_byte_ptr(uint16_t byte_index);

/****************************************************************************
 * Name: dc_get_dout_byte_ptr
 *
 * Description:
 *   Pointer model access to a single byte of compact Digital Outputs.
 *
 * Details:
 *   Part of the Zero-Copy Pointer Model. Returns a direct pointer to
 *   a specific byte of the packed digital outputs array. The caller must
 *   lock access to g_data_core if accessed concurrently.
 *
 * Input Parameters:
 *   byte_index - Byte offset.
 *
 * Returned Value:
 *   Pointer to the byte, or NULL if out of bounds.
 ****************************************************************************/

uint8_t *dc_get_dout_byte_ptr(uint16_t byte_index);

/****************************************************************************
 * Name: dc_get_ain_ptr
 *
 * Description:
 *   Pointer model access to a single Analog Input float channel.
 *
 * Details:
 *   Part of the Zero-Copy Pointer Model. Returns a direct pointer to
 *   a specific float variable in g_data_core. The caller must lock
 *   access to g_data_core if accessed concurrently.
 *
 * Input Parameters:
 *   index - Index of the AI channel.
 *
 * Returned Value:
 *   Pointer to the float value, or NULL if out of bounds.
 ****************************************************************************/

float *dc_get_ain_ptr(uint16_t index);

/****************************************************************************
 * Name: dc_get_aout_ptr
 *
 * Description:
 *   Pointer model access to a single Analog Output float channel.
 *
 * Details:
 *   Part of the Zero-Copy Pointer Model. Returns a direct pointer to
 *   a specific float variable in g_data_core. The caller must lock
 *   access to g_data_core if accessed concurrently.
 *
 * Input Parameters:
 *   index - Index of the AO channel.
 *
 * Returned Value:
 *   Pointer to the float value, or NULL if out of bounds.
 ****************************************************************************/

float *dc_get_aout_ptr(uint16_t index);

#endif /* CONFIG_UDNP3_ENABLE_POINTER_MODEL */

#if CONFIG_UDNP3_ENABLE_COPY_MODEL

/* Copy Model APIs (Double Buffering synchronization). */

/****************************************************************************
 * Name: dc_sync_to_buffer
 *
 * Description:
 *   Copies the entire Process Image state to a dest buffer.
 *
 * Details:
 *   Part of the Double-Buffering Copy Model. Performs a block copy
 *   using memcpy. Useful for threads that need to query the database
 *   snapshot without holding locks for extended durations.
 *
 * Input Parameters:
 *   dest - Pointer to destination buffer struct.
 *
 * Returned Value:
 *   None.
 ****************************************************************************/

void dc_sync_to_buffer(struct data_core_s *dest);

/****************************************************************************
 * Name: dc_sync_from_buffer
 *
 * Description:
 *   Copies the entire Process Image state from a src buffer.
 *
 * Details:
 *   Part of the Double-Buffering Copy Model. Performs a block copy
 *   using memcpy. Useful for writing back local buffers into the global
 *   Data Core.
 *
 * Input Parameters:
 *   src - Pointer to source buffer struct.
 *
 * Returned Value:
 *   None.
 ****************************************************************************/

void dc_sync_from_buffer(const struct data_core_s *src);

#endif /* CONFIG_UDNP3_ENABLE_COPY_MODEL */

/* **************************************************************************
 * Absolute Wall-Clock (HAL Interface) APIs
 * **************************************************************************
 * Overview:
 * This subsystem provides an agnostic Hardware Abstraction Layer (HAL) for
 * absolute time tracking (Wall-Clock) inside the central Data Core.
 *
 * Architecture & Portability:
 * Inspired by the standard POSIX clock_gettime/settime framework, these 
 * functions bypass architecture-dependent data layouts (such as standard
 * 32-bit vs 64-bit struct timespec variations) to maintain a static memory
 * footprint compliant with strict embedded constraints.
 *
 * Inter-Submodule Dependencies:
 * - The Sequence of Events (SOE) subsystem depends directly on this interface
 * to extract 64-bit linear UTC millisecond records for event logging.
 * - If CONFIG_UDNP3_ENABLE_SOE is enabled, CONFIG_UDNP3_ENABLE_WALL_CLOCK must be
 * enforced at compile-time to guarantee timestamp synchronization integrity.
 * - This interface remains decoupled from the protocol's monotonic delta
 * timers (32-bit), ensuring that external wall-clock synchronization jumps
 * (e.g., via DNP3 Write Time commands) do not disrupt active link-layer or
 * application-layer timeouts.
 *
 * Platform Porting Requirement:
 * The platform board support package (BSP) or RTOS bridge must provide the
 * low-level implementation for these signatures (e.g., binding them to a
 * hardware RTC peripheral peripheral or an OS system clock service).
 */

#if defined(CONFIG_UDNP3_ENABLE_WALL_CLOCK) && \
           (CONFIG_UDNP3_ENABLE_WALL_CLOCK > 0)
   

/****************************************************************************
 * Name: dc_clock_gettime
 *
 * Description:
 * Retrieves the current absolute Wall-Clock time from the system.
 *
 * Details:
 * Agnostic wrapper inspired by POSIX clock_gettime(). It populates the 
 * dc_time_s structure with UTC time. The underlying implementation must
 * be provided by the platform ports (RTOS hook or hardware RTC read).
 *
 * Input Parameters:
 * time - Pointer to the target structure to be populated.
 *
 * Returned Value:
 * 0 on success; a negative error code on failure.
 ****************************************************************************/

int dc_clock_gettime(struct dc_time_s *time);

/****************************************************************************
 * Name: dc_clock_settime
 *
 * Description:
 * Sets the device's absolute Wall-Clock time.
 *
 * Details:
 * Agnostic wrapper inspired by POSIX clock_settime(). This function is
 * invoked by protocol adapters (like DNP3) when a time synchronization
 * command forces a clock update. The platform implementation will typically
 * update the hardware RTC or the internal RTOS Unix Epoch offset.
 *
 * Input Parameters:
 * time - Pointer to the structure containing the new absolute time.
 *
 * Returned Value:
 * 0 on success; a negative error code on failure.
 ****************************************************************************/

int dc_clock_settime(const struct dc_time_s *time);

#endif /* defined(CONFIG_UDNP3_ENABLE_WALL_CLOCK) &&
                 (CONFIG_UDNP3_ENABLE_WALL_CLOCK > 0)
        */

#ifdef __cplusplus
}
#endif

#endif /* __CORE_INCLUDE_DATA_CORE_H */
