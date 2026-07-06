/**
 * @file mctp.c
 * @brief MCTP transport — reassembly and framing
 */
#include "mctp.h"
#include <string.h>

/*--------------------------------------------------------------------------
 * Module-private reassembly state
 *------------------------------------------------------------------------*/
static uint8_t  s_reasm_buf[PLDM_MAX_PAYLOAD] ALIGNED4;
static uint16_t s_reasm_offset = 0;
static uint8_t  s_expected_seq = 0;
static bool     s_reasm_active = false;

/*--------------------------------------------------------------------------
 * Private helpers
 *------------------------------------------------------------------------*/
static inline uint8_t extract_seq(uint8_t flags_seq)
{
    return (flags_seq & MCTP_SEQ_MASK) >> 4;
}

static inline bool is_som(uint8_t flags_seq)
{
    return (flags_seq & MCTP_FLAG_SOM) != 0;
}

static inline bool is_eom(uint8_t flags_seq)
{
    return (flags_seq & MCTP_FLAG_EOM) != 0;
}

/*--------------------------------------------------------------------------
 * Public API
 *------------------------------------------------------------------------*/
rtos_err_t mctp_init(void)
{
    s_reasm_offset  = 0;
    s_expected_seq  = 0;
    s_reasm_active  = false;
    memset(s_reasm_buf, 0, sizeof(s_reasm_buf));
    return RTOS_OK;
}

bool mctp_process_rx(const bmc_rx_msg_t *raw, mctp_packet_t *pkt_out)
{
    if (raw->length < 4) { return false; }   /* minimum MCTP header */

    uint8_t hdr_ver   = raw->data[0];
    uint8_t dest_eid  = raw->data[1];
    uint8_t src_eid   = raw->data[2];
    uint8_t flags_seq = raw->data[3];

    const uint8_t *payload = &raw->data[4];
    uint16_t payload_len   = raw->length - 4;

    if (is_som(flags_seq)) {
        /* Start of message — reset reassembly */
        s_reasm_offset = 0;
        s_reasm_active = true;
        s_expected_seq = extract_seq(flags_seq);
    }

    if (!s_reasm_active) { return false; }

    /* Sequence check */
    if (extract_seq(flags_seq) != s_expected_seq) {
        mctp_reset();
        return false;
    }

    /* Append payload to reassembly buffer */
    if (s_reasm_offset + payload_len > sizeof(s_reasm_buf)) {
        mctp_reset();
        return false;
    }
    memcpy(&s_reasm_buf[s_reasm_offset], payload, payload_len);
    s_reasm_offset += payload_len;
    s_expected_seq  = (s_expected_seq + 1) & 0x03;

    if (is_eom(flags_seq)) {
        /* Complete message — populate output */
        pkt_out->hdr_version = hdr_ver;
        pkt_out->dest_eid    = dest_eid;
        pkt_out->src_eid     = src_eid;
        pkt_out->flags_seq   = flags_seq;
        pkt_out->payload_len = s_reasm_offset;
        pkt_out->reserved    = 0;
        memcpy(pkt_out->payload, s_reasm_buf, s_reasm_offset);

        s_reasm_active = false;
        s_reasm_offset = 0;
        return true;
    }

    return false;   /* fragment received, waiting for more */
}

rtos_err_t mctp_frame_tx(const mctp_packet_t *pkt,
                         uint8_t             *tx_buf,
                         uint16_t            *tx_len)
{
    tx_buf[0] = pkt->hdr_version;
    tx_buf[1] = pkt->dest_eid;
    tx_buf[2] = pkt->src_eid;
    tx_buf[3] = MCTP_FLAG_SOM | MCTP_FLAG_EOM;   /* single-packet msg */
    memcpy(&tx_buf[4], pkt->payload, pkt->payload_len);
    *tx_len = 4 + pkt->payload_len;
    return RTOS_OK;
}

void mctp_reset(void)
{
    s_reasm_offset = 0;
    s_expected_seq = 0;
    s_reasm_active = false;
}
