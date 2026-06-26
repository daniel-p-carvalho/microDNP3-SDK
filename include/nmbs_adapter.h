/*
 * ***************************************************************************
 * File:        adapters/include/nmbs_adapter.h
 * Description: Public Interface for the nanoMODBUS RTU Server Adapter.
 * Author:      Daniel P. Carvalho <danieloak@gmail.com>
 * --------------------------------------------------------------------------
 * License: BSD-3-Clause
 *
 * Copyright (c) 2026, Daniel P. Carvalho. All rights reserved.
 * ***************************************************************************
 */

#ifndef __ADAPTERS_NMBS_ADAPTER_H
#define __ADAPTERS_NMBS_ADAPTER_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "adapter.h"

#include "nanomodbus.h"

/****************************************************************************
 * Public Types
 ****************************************************************************/

/* Modbus adaptation types for analog mapping. */

enum modbus_adapt_type_e
{
  MODBUS_ADAPT_UINT16 = 0,
  MODBUS_ADAPT_UINT32,
  MODBUS_ADAPT_FLOAT
};

/****************************************************************************
 * Name: struct modbus_adapter_instance_s
 *
 * Description:
 * Persistent runtime context holding the operational state, configuration
 * clones, and backend stack instances for a concrete Modbus adapter.
 *
 * This structure is allocated by the application to guarantee zero
 * dynamic memory allocation at runtime, but its fields are populated 
 * interning during the binding and initialization phases.
 ****************************************************************************/

struct modbus_adapter_instance_s
{
  struct adapter_s              base;         /* Base adapter handle */
  struct transport_s           *transport;    /* OS-specific I/O. */
  const struct point_mapping_s *mappings;     /* Mapping array. */
  uint16_t                      num_mappings; /* Number of mappings. */
  uint8_t                       mode;         /* Modbus mode (RTU / TCP) */
  uint8_t                       slave_id;     /* Modbus server ID. */
  nmbs_t                        nmbs;         /* nanoMODBUS context. */
  bool                          initialized;  /* Initialization flag. */
};


/****************************************************************************
 * Name: struct modbus_adapter_config_s
 *
 * Description:
 * Ephemeral configuration payload containing initialization parameters,
 * rules of engagement, and telemetry map references for a Modbus adapter.
 *
 * This transient structure is passed by the application to the abstract
 * ops->init() routine, allowing parameters to be cloned or referenced
 * safely before this payload is discarded from the stack.
 ****************************************************************************/

struct modbus_adapter_config_s
{
  const struct point_mapping_s *mappings;     /* Telemetry mappings. */
  uint16_t                      num_mappings; /* Number of mappings. */
  uint8_t                       mode;         /* Modbus mode (RTU / TCP). */
  uint8_t                       slave_id;     /* Modbus slave ID. */
};

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

#ifdef __cplusplus
extern "C"
{
#endif

/****************************************************************************
 * Name: nmbs_adapter_bind
 *
 * Description:
 * Binds an abstract transport layer channel to a specific, application-
 * allocated Modbus adapter instance. 
 *
 * This function couples the generic protocol framing state machine with 
 * the concrete hardware or operating system I/O implementation hooks. 
 * It clears the supplied instance context memory to guarantee a 
 * predictable clean state before mapping the abstract operation handlers.
 *
 * Input Parameters:
 * instance  - Pointer to the uninitialized 'modbus_adapter_instance_s' 
 * structure allocated by the application (typically with 
 * static or global lifetime scope).
 * transport - Pointer to the generic 'transport_s' structure containing 
 * the concrete platform I/O operations and driver context.
 *
 * Returned Value:
 * A pointer to the initialized generic 'adapter_s' base interface handle 
 * on success, which can be safely used for polymorphic downcasting; 
 * NULL if either the instance or transport pointer is invalid.
 ****************************************************************************/

struct adapter_s *nmbs_adapter_bind(struct modbus_adapter_instance_s *instance,
                                    struct transport_s *transport);

#ifdef __cplusplus
}
#endif

#endif /* __ADAPTERS_NMBS_ADAPTER_H */
