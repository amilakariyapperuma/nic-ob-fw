/**
 * @file sensor_hal.h
 * @brief Sensor I2C interface HAL+driver — master mode for sensor reads
 */
#ifndef SENSOR_HAL_H
#define SENSOR_HAL_H

#include "common_types.h"
#include "rtos_hal.h"

/** Initialize I2C master for sensor bus */
rtos_err_t sensor_hal_init(void);

/** Read raw sensor register — blocking, microsecond-level */
rtos_err_t sensor_hal_read_reg(uint8_t  dev_addr,
                               uint8_t  reg_addr,
                               uint8_t *data_out,
                               uint16_t length);

/** Write sensor register (e.g., threshold configuration) */
rtos_err_t sensor_hal_write_reg(uint8_t       dev_addr,
                                uint8_t       reg_addr,
                                const uint8_t *data,
                                uint16_t      length);

#endif /* SENSOR_HAL_H */
