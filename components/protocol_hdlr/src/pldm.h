/**
 * @file pldm.h
 * @brief PLDM internal header — parse and build implementation
 *
 * INTERNAL to protocol_hdlr component. Not visible to other components.
 * Constants and dispatch helpers are in protocol_hdlr.h (public).
 */
#ifndef PLDM_INTERNAL_H
#define PLDM_INTERNAL_H

#include "common_types.h"
#include "rtos_hal.h"

/** Initialize PLDM module */
rtos_err_t pldm_init(void);

/** Parse MCTP payload into PLDM message */
rtos_err_t pldm_parse(const mctp_packet_t *mctp_pkt, pldm_msg_t *pldm_out);

/** Build PLDM response and pack into MCTP payload */
rtos_err_t pldm_build_response(const pldm_msg_t *request,
                               const uint8_t    *resp_data,
                               uint16_t          resp_len,
                               uint8_t           completion_code,
                               mctp_packet_t    *mctp_out);

#endif /* PLDM_INTERNAL_H */
