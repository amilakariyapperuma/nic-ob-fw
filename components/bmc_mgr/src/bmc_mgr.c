/**
 * @file bmc_mgr.c
 * @brief BMC Manager — main dispatch thread
 *
 * Event-flag based multi-queue wait:
 *   BIT_BMC_RX    — raw bytes from BMC arrived
 *   BIT_NIC_EVENT — NIC ASIC event arrived
 *
 * BMC requests always drained first (higher priority within thread).
 */
#include "bmc_mgr.h"
#include "protocol_hdlr.h"
#include "nic_gen_mgr.h"
#include "sensor_mgr.h"

#define BIT_BMC_RX     (1U << 0)
#define BIT_NIC_EVENT  (1U << 1)

/*--------------------------------------------------------------------------
 * Module-private state
 *------------------------------------------------------------------------*/
static rtos_queue_handle_t       s_bmc_rx_queue    = NULL;
static rtos_queue_handle_t       s_nic_event_queue = NULL;
static rtos_queue_handle_t       s_flash_cmd_queue = NULL;
static rtos_event_group_handle_t s_events          = NULL;

/*--------------------------------------------------------------------------
 * Typed queue wrappers — private to this module.
 * Compiler enforces correct struct type at every call site.
 * Zero overhead — inlined by compiler.
 *------------------------------------------------------------------------*/
static inline rtos_err_t bmc_rx_queue_receive(bmc_rx_msg_t *msg,
                                              uint32_t      timeout_ms)
{
    return rtos_queue_receive(s_bmc_rx_queue, msg, timeout_ms);
}

static inline rtos_err_t nic_event_queue_receive(nic_event_msg_t *evt,
                                                 uint32_t         timeout_ms)
{
    return rtos_queue_receive(s_nic_event_queue, evt, timeout_ms);
}

static inline rtos_err_t flash_cmd_queue_send(const flash_cmd_t *cmd,
                                              uint32_t           timeout_ms)
{
    return rtos_queue_send(s_flash_cmd_queue, cmd, timeout_ms);
}

/*--------------------------------------------------------------------------
 * Liveness — subcomponent within BMC Mgr
 *------------------------------------------------------------------------*/
static uint32_t s_last_bmc_seen_ms   = 0;
static uint32_t s_liveness_timeout_ms = 5000;

static inline void liveness_update(void)
{
    s_last_bmc_seen_ms = rtos_get_tick_ms();
}

static inline bool liveness_check(void)
{
    return (rtos_get_tick_ms() - s_last_bmc_seen_ms) < s_liveness_timeout_ms;
}

/*--------------------------------------------------------------------------
 * PLDM command dispatch — called in this thread context
 *------------------------------------------------------------------------*/
static void dispatch_pldm(const pldm_msg_t *msg)
{
    uint8_t resp_buf[64] ALIGNED4;
    uint16_t resp_len = 0;
    uint8_t cc = PLDM_CC_SUCCESS;

    switch (pldm_get_type(msg)) {

    case PLDM_TYPE_SENSOR_MONITORING:
        switch (pldm_get_command(msg)) {
        case PLDM_CMD_GET_SENSOR_READING: {
            uint8_t sensor_id = (msg->payload_len > 0) ? msg->payload[0] : 0;
            sensor_record_t rec;

            if (sensor_mgr_get_cached(sensor_id, &rec) == RTOS_OK) {
                /* Pack sensor record into response */
                memcpy(resp_buf, &rec, sizeof(rec));
                resp_len = sizeof(rec);
            } else {
                cc = PLDM_CC_ERROR;
            }
            break;
        }
        default:
            cc = PLDM_CC_INVALID_CMD;
            break;
        }
        break;

    case PLDM_TYPE_FW_UPDATE:
        switch (pldm_get_command(msg)) {
        case PLDM_CMD_REQUEST_UPDATE: {
            /* Async dispatch to Flash Mgr — do not block */
            flash_cmd_t cmd = {
                .cmd_type         = FLASH_CMD_UPDATE_START,
                .target_partition = 0,
                .reserved         = 0,
                .image_offset     = 0,
                .image_size       = 0,
            };

            /* Extract offset/size from PLDM payload if present */
            if (msg->payload_len >= 8) {
                memcpy(&cmd.image_offset, &msg->payload[0], 4);
                memcpy(&cmd.image_size,   &msg->payload[4], 4);
            }

            if (flash_cmd_queue_send(&cmd, RTOS_NO_WAIT) != RTOS_OK) {
                cc = PLDM_CC_ERROR;   /* queue full — update in progress */
            }
            break;
        }
        default:
            cc = PLDM_CC_INVALID_CMD;
            break;
        }
        break;

    default:
        cc = PLDM_CC_INVALID_CMD;
        break;
    }

    /* Send response back to BMC */
    protocol_hdlr_send_response(msg, resp_buf, resp_len, cc);
}

/*--------------------------------------------------------------------------
 * NIC ASIC event handling — also in this thread context
 *------------------------------------------------------------------------*/
static void handle_nic_event(const nic_event_msg_t *event)
{
    /* Delegate to NIC Gen Mgr (function call, same thread) */
    nic_gen_mgr_handle_nic_event(event);

    /* TODO: if event warrants it, push SEL/alert to BMC proactively */
}

/*--------------------------------------------------------------------------
 * Public API
 *------------------------------------------------------------------------*/
rtos_err_t bmc_mgr_init(rtos_queue_handle_t bmc_rx_queue,
                         rtos_queue_handle_t nic_event_queue,
                         rtos_queue_handle_t flash_cmd_queue)
{
    s_bmc_rx_queue    = bmc_rx_queue;
    s_nic_event_queue = nic_event_queue;
    s_flash_cmd_queue = flash_cmd_queue;

    s_events = rtos_event_group_create();
    if (s_events == NULL) { return RTOS_ERR_NOMEM; }

    rtos_err_t err = protocol_hdlr_init();
    if (err != RTOS_OK) { return err; }

    err = nic_gen_mgr_init();
    if (err != RTOS_OK) { return err; }

    liveness_update();
    return RTOS_OK;
}

void bmc_mgr_task(void *arg)
{
    (void)arg;

    bmc_rx_msg_t    bmc_msg;
    nic_event_msg_t nic_evt;

    while (1) {
        /* Wait for either queue to have data */
        uint32_t bits = rtos_event_group_wait(
            s_events,
            BIT_BMC_RX | BIT_NIC_EVENT,
            true,     /* clear on exit */
            false,    /* wait for ANY bit */
            1000);    /* 1s timeout for liveness check */

        /* Always drain BMC queue first — higher priority within thread */
        while (bmc_rx_queue_receive(&bmc_msg, RTOS_NO_WAIT) == RTOS_OK) {
            liveness_update();

            pldm_msg_t pldm;
            if (protocol_hdlr_process_rx(&bmc_msg, &pldm)) {
                dispatch_pldm(&pldm);
            }
            /* else: MCTP fragment, waiting for more */
        }

        /* Then drain NIC event queue */
        while (nic_event_queue_receive(&nic_evt, RTOS_NO_WAIT) == RTOS_OK) {
            handle_nic_event(&nic_evt);
        }

        /* Periodic liveness check */
        if (!liveness_check()) {
            /* BMC has gone silent — log event, prepare for recovery */
            nic_gen_mgr_log_sel_event(MSG_TYPE_LIVENESS, 0);
            /* TODO: enter degraded mode or trigger watchdog */
        }
    }
}
