/**
 * @file bmc_hal.h
 * @brief BMC I2C interface HAL+driver — slave mode toward BMC
 */
#ifndef BMC_HAL_H
#define BMC_HAL_H

#include "common_types.h"
#include "rtos_hal.h"

/** Initialize I2C slave for BMC communication */
rtos_err_t bmc_hal_init(uint8_t slave_addr);

/** Transmit response to BMC — called from BMC Mgr thread context */
rtos_err_t bmc_hal_transmit(const uint8_t *data, uint16_t length);

/** Register ISR callback — called during init */
rtos_err_t bmc_hal_register_isr(void (*rx_callback)(const uint8_t *data,
                                                     uint16_t       length));

#endif /* BMC_HAL_H */
