/**
 * @file protocol_hdlr.h
 * @brief Protocol handler — facade over MCTP + PLDM stack
 *
 * Public interface for BMC Mgr. Exposes:
 * - Process raw bytes → parsed PLDM message
 * - Send PLDM response back to BMC
 * - PLDM type/command constants needed for dispatch
 * - Inline helpers to inspect PLDM messages
 *
 * MCTP and PLDM internals are hidden behind this facade.
 */
#ifndef PROTOCOL_HDLR_H
#define PROTOCOL_HDLR_H

#include "common_types.h"
#include "rtos_hal.h"

/*--------------------------------------------------------------------------
 * PLDM constants — exposed for BMC Mgr dispatch logic
 *------------------------------------------------------------------------*/

/* PLDM type codes */
#define PLDM_TYPE_SENSOR_MONITORING  2
#define PLDM_TYPE_FW_UPDATE          5

/* PLDM sensor commands */
#define PLDM_CMD_GET_SENSOR_READING  0x11
#define PLDM_CMD_SET_THRESHOLD       0x12

/* PLDM FW update commands */
#define PLDM_CMD_REQUEST_UPDATE      0x10
#define PLDM_CMD_PASS_COMPONENT      0x06
#define PLDM_CMD_ACTIVATE_FW         0x1A

/* Completion codes */
#define PLDM_CC_SUCCESS              0x00
#define PLDM_CC_ERROR                0x01
#define PLDM_CC_INVALID_CMD          0x04

/*--------------------------------------------------------------------------
 * PLDM message inspection — inline helpers for dispatch
 *------------------------------------------------------------------------*/
static inline uint8_t pldm_get_type(const pldm_msg_t *msg)
{
    return msg->pldm_type;
}

static inline uint8_t pldm_get_command(const pldm_msg_t *msg)
{
    return msg->command;
}

/*--------------------------------------------------------------------------
 * Protocol handler API
 *------------------------------------------------------------------------*/

/** Initialize the full protocol stack (MCTP + PLDM) */
rtos_err_t protocol_hdlr_init(void);

/**
 * Process raw BMC receive data through MCTP -> PLDM.
 * Returns true if a complete PLDM message is ready.
 */
bool protocol_hdlr_process_rx(const bmc_rx_msg_t *raw, pldm_msg_t *pldm_out);

/** Build and transmit a PLDM response back to BMC */
rtos_err_t protocol_hdlr_send_response(const pldm_msg_t *request,
                                       const uint8_t    *resp_data,
                                       uint16_t          resp_len,
                                       uint8_t           completion_code);

#endif /* PROTOCOL_HDLR_H */
