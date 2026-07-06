/**
 * @file nic_chip_protocol.c
 * @brief NIC chip protocol — stub implementation
 *
 * TODO: Implement NC-SI or vendor-specific NIC ASIC protocol
 */
#include "nic_chip_protocol.h"
#include "nic_chip_hal.h"

rtos_err_t nic_chip_protocol_init(void)
{
    /* TODO: protocol-level initialization with NIC ASIC */
    return RTOS_OK;
}

rtos_err_t nic_chip_protocol_get_fw_version(uint8_t *version_buf,
                                            uint16_t buf_size)
{
    /* TODO: send version query via NIC chip protocol */
    (void)version_buf;
    (void)buf_size;
    return RTOS_OK;
}

rtos_err_t nic_chip_protocol_get_link_status(uint8_t *status_out)
{
    /* TODO: send link status query */
    (void)status_out;
    return RTOS_OK;
}

rtos_err_t nic_chip_protocol_reset(uint8_t reset_type)
{
    /* TODO: send reset via protocol, then assert GPIO */
    (void)reset_type;
    return RTOS_OK;
}
