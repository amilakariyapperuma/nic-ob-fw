/**
 * @file nic_gen_mgr.h
 * @brief NIC General Manager — FRU, SEL, reset, FW version
 *
 * Code module only — executes in BMC Mgr thread context as function calls.
 * NOT a separate thread.
 */
#ifndef NIC_GEN_MGR_H
#define NIC_GEN_MGR_H

#include "common_types.h"
#include "rtos_hal.h"

/** Initialize NIC general manager */
rtos_err_t nic_gen_mgr_init(void);

/** Handle FRU inventory query */
rtos_err_t nic_gen_mgr_get_fru(fru_record_t *fru_out);

/** Handle FW version query (OB FW version) */
rtos_err_t nic_gen_mgr_get_ob_fw_version(uint8_t *version_buf,
                                          uint16_t buf_size);

/** Handle NIC ASIC FW version query */
rtos_err_t nic_gen_mgr_get_nic_fw_version(uint8_t *version_buf,
                                           uint16_t buf_size);

/** Handle reset command */
rtos_err_t nic_gen_mgr_reset(uint8_t reset_type);

/** Handle power control command */
rtos_err_t nic_gen_mgr_power_ctrl(uint8_t power_state);

/** Log a system event */
rtos_err_t nic_gen_mgr_log_sel_event(uint8_t  event_type,
                                     uint32_t event_data);

/** Process NIC ASIC event (dispatched from BMC Mgr on NIC event queue) */
rtos_err_t nic_gen_mgr_handle_nic_event(const nic_event_msg_t *event);

#endif /* NIC_GEN_MGR_H */
