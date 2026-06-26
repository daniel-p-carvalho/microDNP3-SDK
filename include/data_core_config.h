/*
 ****************************************************************************
 * File:        core/include/data_core_config.h
 * Description: Data Core Static Configuration Limits.
 * Overview:    Defines the static dimensioning limits and feature flags for
 *              the Data Core process image database.
 *
 *              All configurations are protected by guard checks and can be
 *              overridden at compilation time without modifying this file:
 *              - CMake: use target_compile_definitions(target PRIVATE
 *                MACRO=val) or set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}
 *                -DMACRO=val").
 *              - Make/Shell: pass the definitions via compiler flags (e.g.,
 *                CFLAGS="-DCONFIG_UDNP3_NUM_DI=64").
 *              - IDEs: add definitions to the compiler preprocessor
 *                settings.
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

#ifndef __CORE_INCLUDE_DATA_CORE_CONFIG_H
#define __CORE_INCLUDE_DATA_CORE_CONFIG_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Number of I/O channels (static limits). See header for override options. */

#ifndef CONFIG_UDNP3_NUM_DI
#  define CONFIG_UDNP3_NUM_DI 32
#endif

#ifndef CONFIG_UDNP3_NUM_DO
#  define CONFIG_UDNP3_NUM_DO 32
#endif

#ifndef CONFIG_UDNP3_NUM_AI
#  define CONFIG_UDNP3_NUM_AI 16
#endif

#ifndef CONFIG_UDNP3_NUM_AO
#  define CONFIG_UDNP3_NUM_AO 16
#endif

/* Dimensioning of bit-packed binary arrays. */

#define DC_DI_BYTES ((CONFIG_UDNP3_NUM_DI + 7) / 8)
#define DC_DO_BYTES ((CONFIG_UDNP3_NUM_DO + 7) / 8)

/* Data Core integration model selection. See header for override options. */

/****************************************************************************
 * CONFIG_UDNP3_ENABLE_POINTER_MODEL
 *
 * Description:
 *   Enables the Zero-Copy Pointer Model API subset.
 *
 * Details:
 *   When enabled, allows external protocol adapters to retrieve direct
 *   pointers to the raw bytes (DIs/DOs) and float arrays (AIs/AOs) in the
 *   Data Core Process Image (using dc_get_*_ptr APIs).
 *   - Pros: Maximum RAM efficiency (no duplicate tables) and immediate
 *     state reflection.
 *   - Cons: Requires synchronization (mutexes) if the adapter runs in an
 *     asynchronous concurrent task relative to the PLC logic loop.
 *
 * Value:
 *   1 = Enabled (default)
 *   0 = Disabled
 ****************************************************************************/

#ifndef CONFIG_UDNP3_ENABLE_POINTER_MODEL
#  define CONFIG_UDNP3_ENABLE_POINTER_MODEL 1
#endif

/****************************************************************************
 * CONFIG_UDNP3_ENABLE_COPY_MODEL
 *
 * Description:
 *   Enables the Double-Buffering Copy Model API subset.
 *
 * Details:
 *   When enabled, provides APIs (dc_sync_to_buffer and dc_sync_from_buffer)
 *   to synchronize the entire Process Image database as a block copy
 *   operation (using memcpy).
 *   - Pros: Complete memory isolation between the PLC task and network
 *     protocol threads, avoiding concurrency issues and lock contention.
 *   - Cons: Requires duplicate memory allocation in the protocol adapter
 *     database to hold local tables.
 *
 * Value:
 *   1 = Enabled (default)
 *   0 = Disabled
 ****************************************************************************/

#ifndef CONFIG_UDNP3_ENABLE_COPY_MODEL
#  define CONFIG_UDNP3_ENABLE_COPY_MODEL 1
#endif

/****************************************************************************
 * CONFIG_UDNP3_ENABLE_WALL_CLOCK
 *
 * Description:
 * Enables the absolute Wall-Clock (Real-Time) Hardware Abstraction Layer.
 *
 * Details:
 * When enabled, injects the agnostic dc_clock_gettime() and dc_clock_settime()
 * API signatures into the Data Core. This enables external protocol stacks 
 * (such as DNP3) to synchronize the system's absolute time reference or
 * allows internal loggers to capture UTC wall-clock snapshots.
 * - Pros: Provides unified platform-independent hooks for real-time tracking.
 * - Cons: Requires target BSP or RTOS bridge implementation to bind the hooks.
 *
 * Value:
 * 1 = Enabled
 * 0 = Disabled (default)
 ****************************************************************************/

#ifndef CONFIG_UDNP3_ENABLE_WALL_CLOCK
#  define CONFIG_UDNP3_ENABLE_WALL_CLOCK 1
#endif

/****************************************************************************
 * CONFIG_UDNP3_ENABLE_SOE
 *
 * Description:
 *   Enables the Sequence of Events (SOE) event-oriented subsystem.
 *
 * Value:
 *   1 = Enabled (default)
 *   0 = Disabled
 ****************************************************************************/

#ifndef CONFIG_UDNP3_ENABLE_SOE
#  define CONFIG_UDNP3_ENABLE_SOE 0
#endif

/****************************************************************************
 * CONFIG_UDNP3_SOE_QUEUE_SIZE
 *
 * Description:
 *   Sizing of the SOE queue buffer (must be a power of 2).
 *
 * Value:
 *   128 = Default size (only evaluated when CONFIG_UDNP3_ENABLE_SOE == 1)
 ****************************************************************************/

#if CONFIG_UDNP3_ENABLE_SOE
#  ifndef CONFIG_UDNP3_SOE_QUEUE_SIZE
#    define CONFIG_UDNP3_SOE_QUEUE_SIZE 128
#  endif
#endif

/****************************************************************************
 * Configuration Sanity Checks & Architectural Dependency Validation
 ****************************************************************************/

/* The Sequence of Events (SOE) subsystem fundamentally relies on absolute 
 * wall-clock timestamps to align events in chronological order. Enforce 
 * compile-time validation to prevent out-of-sync builds.
 */

#if defined(CONFIG_UDNP3_ENABLE_SOE) && (CONFIG_UDNP3_ENABLE_SOE > 0)
#  if !defined(CONFIG_UDNP3_ENABLE_WALL_CLOCK) || \
              (CONFIG_UDNP3_ENABLE_WALL_CLOCK == 0)
#    error "CONFIG_UDNP3_ENABLE_SOE requires CONFIG_UDNP3_ENABLE_WALL_CLOCK."
#  endif
#endif

#endif /* __CORE_INCLUDE_DATA_CORE_CONFIG_H */
