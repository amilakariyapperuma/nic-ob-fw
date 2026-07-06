/**
 * @file nic_chip_protocol.h
 * @brief NIC chip protocol stub — communication with NIC ASIC
 *
 * TODO: Implement actual protocol (NC-SI or vendor-specific)
 */
#ifndef NIC_CHIP_PROTOCOL_H
#define NIC_CHIP_PROTOCOL_H

#include "common_types.h"
#include "rtos_hal.h"

/** Initialize NIC chip protocol */
rtos_err_t nic_chip_protocol_init(void);

/** Request FW version from NIC ASIC */
rtos_err_t nic_chip_protocol_get_fw_version(uint8_t *version_buf,
                                            uint16_t buf_size);

/** Request link status from NIC ASIC */
rtos_err_t nic_chip_protocol_get_link_status(uint8_t *status_out);

/** Send reset command to NIC ASIC */
rtos_err_t nic_chip_protocol_reset(uint8_t reset_type);

#endif /* NIC_CHIP_PROTOCOL_H */
