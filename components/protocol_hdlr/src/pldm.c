/**
 * @file pldm.c
 * @brief PLDM application layer — parse requests, build responses
 */
#include "pldm.h"
#include <string.h>

/*--------------------------------------------------------------------------
 * Public API
 *------------------------------------------------------------------------*/
rtos_err_t pldm_init(void)
{
    return RTOS_OK;
}

rtos_err_t pldm_parse(const mctp_packet_t *mctp_pkt, pldm_msg_t *pldm_out)
{
    if (mctp_pkt->payload_len < 3) { return RTOS_ERR_FAIL; }

    pldm_out->instance_id     = mctp_pkt->payload[0];
    pldm_out->pldm_type       = mctp_pkt->payload[1];
    pldm_out->command         = mctp_pkt->payload[2];
    pldm_out->completion_code = 0;

    uint16_t data_len = mctp_pkt->payload_len - 3;
    if (data_len > PLDM_MAX_PAYLOAD) { data_len = PLDM_MAX_PAYLOAD; }

    pldm_out->payload_len = data_len;
    pldm_out->reserved    = 0;

    if (data_len > 0) {
        memcpy(pldm_out->payload, &mctp_pkt->payload[3], data_len);
    }

    return RTOS_OK;
}

rtos_err_t pldm_build_response(const pldm_msg_t *request,
                               const uint8_t    *resp_data,
                               uint16_t          resp_len,
                               uint8_t           completion_code,
                               mctp_packet_t    *mctp_out)
{
    mctp_out->payload[0] = request->instance_id;
    mctp_out->payload[1] = request->pldm_type;
    mctp_out->payload[2] = request->command;
    mctp_out->payload[3] = completion_code;

    if (resp_len > 0 && resp_data != NULL) {
        if (resp_len > MCTP_MAX_PAYLOAD - 4) {
            resp_len = MCTP_MAX_PAYLOAD - 4;
        }
        memcpy(&mctp_out->payload[4], resp_data, resp_len);
    }

    mctp_out->payload_len = 4 + resp_len;
    return RTOS_OK;
}
