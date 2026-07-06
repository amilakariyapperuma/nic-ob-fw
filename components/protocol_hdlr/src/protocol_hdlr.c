/**
 * @file protocol_hdlr.c
 * @brief Protocol handler — chains MCTP and PLDM as function calls
 */
#include "protocol_hdlr.h"
#include "mctp.h"
#include "pldm.h"
#include "bmc_hal.h"

/*--------------------------------------------------------------------------
 * Public API
 *------------------------------------------------------------------------*/
rtos_err_t protocol_hdlr_init(void)
{
    rtos_err_t err;

    err = mctp_init();
    if (err != RTOS_OK) { return err; }

    err = pldm_init();
    if (err != RTOS_OK) { return err; }

    return RTOS_OK;
}

bool protocol_hdlr_process_rx(const bmc_rx_msg_t *raw, pldm_msg_t *pldm_out)
{
    mctp_packet_t mctp_pkt;

    /* MCTP reassembly — may take multiple calls to complete */
    if (!mctp_process_rx(raw, &mctp_pkt)) {
        return false;   /* fragment, waiting for more */
    }

    /* Complete MCTP message — parse PLDM */
    if (pldm_parse(&mctp_pkt, pldm_out) != RTOS_OK) {
        return false;
    }

    return true;
}

rtos_err_t protocol_hdlr_send_response(const pldm_msg_t *request,
                                       const uint8_t    *resp_data,
                                       uint16_t          resp_len,
                                       uint8_t           completion_code)
{
    mctp_packet_t mctp_resp;
    uint8_t tx_buf[SMB_MAX_BLOCK_SIZE];
    uint16_t tx_len;

    rtos_err_t err = pldm_build_response(request, resp_data, resp_len,
                                         completion_code, &mctp_resp);
    if (err != RTOS_OK) { return err; }

    err = mctp_frame_tx(&mctp_resp, tx_buf, &tx_len);
    if (err != RTOS_OK) { return err; }

    return bmc_hal_transmit(tx_buf, tx_len);
}
