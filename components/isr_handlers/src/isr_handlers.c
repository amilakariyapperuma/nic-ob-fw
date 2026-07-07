/**
 * @file isr_handlers.c
 * @brief ISR vector — all hardware interrupt handlers
 *
 * ISRs drain hardware buffers and post to RTOS queues.
 * Minimal work in ISR context — no blocking, no mutex.
 */
#include "isr_handlers.h"
#include "bmc_hal.h"
#include "nic_chip_hal.h"

/*--------------------------------------------------------------------------
 * Module-private state — queues set during init
 *------------------------------------------------------------------------*/
static rtos_queue_handle_t s_bmc_rx_queue       = NULL;
static rtos_queue_handle_t s_nic_event_queue    = NULL;
static rtos_queue_handle_t s_sensor_alert_queue = NULL;

/*--------------------------------------------------------------------------
 * Typed ISR queue wrappers — private to this module.
 * ISR variants: non-blocking, no mutex, safe in interrupt context.
 *------------------------------------------------------------------------*/
static inline rtos_err_t IRAM_ATTR
bmc_rx_queue_send_from_isr(const bmc_rx_msg_t *msg, bool *woken)
{
    return rtos_queue_send_from_isr(s_bmc_rx_queue, msg, woken);
}

static inline rtos_err_t IRAM_ATTR
nic_event_queue_send_from_isr(const nic_event_msg_t *evt, bool *woken)
{
    return rtos_queue_send_from_isr(s_nic_event_queue, evt, woken);
}

/*--------------------------------------------------------------------------
 * ISR callbacks — registered with HAL drivers
 *------------------------------------------------------------------------*/

/** I2C slave RX ISR — BMC sent data, drain HW buffer immediately */
static void IRAM_ATTR bmc_i2c_rx_isr(const uint8_t *data, uint16_t length)
{
    if (s_bmc_rx_queue == NULL) { return; }

    bmc_rx_msg_t msg;
    /* Clamp to max SMBus block size */
    msg.length = (length > SMB_MAX_BLOCK_SIZE) ? SMB_MAX_BLOCK_SIZE : length;
    msg.reserved = 0;

    /* Unrolled copy for small fixed-size buffer */
    uint16_t i;
    uint16_t copy4 = msg.length & ~3U;
    for (i = 0; i < copy4; i += 4) {
        msg.data[i]     = data[i];
        msg.data[i + 1] = data[i + 1];
        msg.data[i + 2] = data[i + 2];
        msg.data[i + 3] = data[i + 3];
    }
    for (; i < msg.length; i++) {
        msg.data[i] = data[i];
    }

    bool higher_prio_woken = false;
    bmc_rx_queue_send_from_isr(&msg, &higher_prio_woken);
    rtos_yield_from_isr(higher_prio_woken);
}

/** GPIO alert ISR — NIC ASIC asserted alert line */
static void IRAM_ATTR nic_alert_isr(void *arg)
{
    if (s_nic_event_queue == NULL) { return; }

    nic_event_msg_t evt = {
        .event_type   = NIC_EVENT_HW_ERROR,
        .reserved     = {0},
        .event_data   = 0,
        .timestamp_ms = 0,   /* filled by handler thread if needed */
    };

    bool higher_prio_woken = false;
    nic_event_queue_send_from_isr(&evt, &higher_prio_woken);
    rtos_yield_from_isr(higher_prio_woken);
}

/*--------------------------------------------------------------------------
 * Public API
 *------------------------------------------------------------------------*/
rtos_err_t isr_handlers_init(rtos_queue_handle_t bmc_rx_queue,
                             rtos_queue_handle_t nic_event_queue,
                             rtos_queue_handle_t sensor_alert_queue)
{
    s_bmc_rx_queue       = bmc_rx_queue;
    s_nic_event_queue    = nic_event_queue;
    s_sensor_alert_queue = sensor_alert_queue;

    /* Register BMC I2C RX callback with BMC HAL */
    bmc_hal_register_isr(bmc_i2c_rx_isr);

    /* Register NIC ASIC GPIO alert with NIC Chip HAL */
    nic_chip_hal_register_alert_isr(5, nic_alert_isr, NULL);

    return RTOS_OK;
}
