/**
 * @file sensor_protocol.h
 * @brief Sensor protocol abstraction — stub for PMBus or direct register
 */
#ifndef SENSOR_PROTOCOL_H
#define SENSOR_PROTOCOL_H

#include "common_types.h"
#include "rtos_hal.h"

/** Initialize sensor protocol layer */
rtos_err_t sensor_protocol_init(void);

/** Read sensor value through protocol abstraction */
rtos_err_t sensor_protocol_read(uint8_t sensor_id,
                                int16_t *raw_value_out);

/** Configure sensor threshold (if supported by protocol) */
rtos_err_t sensor_protocol_set_threshold(uint8_t sensor_id,
                                         int16_t upper_critical,
                                         int16_t lower_critical);

#endif /* SENSOR_PROTOCOL_H */
