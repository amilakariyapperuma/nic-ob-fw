/**
 * @file nic_chip_hal.h
 * @brief NIC Chip HAL+driver — SPI for flash, GPIO for control
 */
#ifndef NIC_CHIP_HAL_H
#define NIC_CHIP_HAL_H

#include "common_types.h"
#include "rtos_hal.h"

/** Initialize SPI master and GPIO for NIC chip interface */
rtos_err_t nic_chip_hal_init(void);

/** SPI transfer — full duplex */
rtos_err_t nic_chip_hal_spi_transfer(const uint8_t *tx_buf,
                                     uint8_t       *rx_buf,
                                     uint16_t       length);

/** Assert GPIO control line (reset, power seq) */
rtos_err_t nic_chip_hal_gpio_set(uint8_t pin_id, bool level);

/** Read GPIO input (status, alert) */
bool nic_chip_hal_gpio_get(uint8_t pin_id);

/** Register GPIO ISR for NIC ASIC alert lines */
rtos_err_t nic_chip_hal_register_alert_isr(uint8_t pin_id,
                                           void (*callback)(void *),
                                           void *arg);

#endif /* NIC_CHIP_HAL_H */
