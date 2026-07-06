/**
 * @file nic_gen_mgr.c
 * @brief NIC General Manager — called in BMC Mgr thread context
 */
#include "nic_gen_mgr.h"
#include "nic_chip_protocol.h"
#include "nic_chip_hal.h"
#include <string.h>

/*--------------------------------------------------------------------------
 * Module-private state
 *------------------------------------------------------------------------*/
static const uint8_t s_ob_fw_version[] = "1.0.0";

static fru_record_t s_fru ALIGNED4 = {
    .board_serial = "SN000000000001",
    .part_number  = "NIC-OB-FW-001",
    .mac_addr     = {0x00, 0x1A, 0x2B, 0x3C, 0x4D, 0x5E, 0x00, 0x00},
    .hw_revision  = 1,
    .reserved     = {0},
};

/*--------------------------------------------------------------------------
 * SEL storage — simple circular log
 *------------------------------------------------------------------------*/
#define SEL_MAX_ENTRIES 32

typedef struct ALIGNED4 {
    uint8_t  event_type;
    uint8_t  reserved[3];
    uint32_t event_data;
    uint32_t timestamp_ms;
} sel_entry_t;

static sel_entry_t s_sel_log[SEL_MAX_ENTRIES] ALIGNED4;
static uint8_t     s_sel_head = 0;
static uint8_t     s_sel_count = 0;

/*--------------------------------------------------------------------------
 * Public API
 *------------------------------------------------------------------------*/
rtos_err_t nic_gen_mgr_init(void)
{
    memset(s_sel_log, 0, sizeof(s_sel_log));
    s_sel_head  = 0;
    s_sel_count = 0;
    return nic_chip_protocol_init();
}

rtos_err_t nic_gen_mgr_get_fru(fru_record_t *fru_out)
{
    *fru_out = s_fru;
    return RTOS_OK;
}

rtos_err_t nic_gen_mgr_get_ob_fw_version(uint8_t *version_buf,
                                          uint16_t buf_size)
{
    uint16_t copy_len = sizeof(s_ob_fw_version);
    if (copy_len > buf_size) { copy_len = buf_size; }
    memcpy(version_buf, s_ob_fw_version, copy_len);
    return RTOS_OK;
}

rtos_err_t nic_gen_mgr_get_nic_fw_version(uint8_t *version_buf,
                                           uint16_t buf_size)
{
    return nic_chip_protocol_get_fw_version(version_buf, buf_size);
}

rtos_err_t nic_gen_mgr_reset(uint8_t reset_type)
{
    nic_gen_mgr_log_sel_event(MSG_TYPE_RESET_CMD, reset_type);
    return nic_chip_protocol_reset(reset_type);
}

rtos_err_t nic_gen_mgr_power_ctrl(uint8_t power_state)
{
    /* TODO: implement power sequencing via GPIO */
    (void)power_state;
    return RTOS_OK;
}

rtos_err_t nic_gen_mgr_log_sel_event(uint8_t  event_type,
                                     uint32_t event_data)
{
    sel_entry_t *entry = &s_sel_log[s_sel_head];
    entry->event_type   = event_type;
    entry->event_data   = event_data;
    entry->timestamp_ms = rtos_get_tick_ms();

    s_sel_head = (s_sel_head + 1) % SEL_MAX_ENTRIES;
    if (s_sel_count < SEL_MAX_ENTRIES) { s_sel_count++; }

    return RTOS_OK;
}

rtos_err_t nic_gen_mgr_handle_nic_event(const nic_event_msg_t *event)
{
    nic_gen_mgr_log_sel_event(event->event_type, event->event_data);
    /* TODO: take protective action based on event type */
    return RTOS_OK;
}
