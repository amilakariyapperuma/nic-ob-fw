/**
 * @file mctp.h
 * @brief MCTP transport layer — framing, fragmentation, reassembly
 *
 * Called as library functions within BMC Mgr thread context.
 * Not a separate thread.
 */
#ifndef MCTP_H
#define MCTP_H

#include "common_types.h"
#include "rtos_hal.h"

/* MCTP header flag masks */
#define MCTP_FLAG_SOM   0x80
#define MCTP_FLAG_EOM   0x40
#define MCTP_SEQ_MASK   0x30
#define MCTP_TAG_MASK   0x07

/** Initialize MCTP module state */
rtos_err_t mctp_init(void);

/**
 * Process raw SMBus bytes into MCTP packet.
 * Returns true if a complete MCTP packet is ready.
 */
bool mctp_process_rx(const bmc_rx_msg_t *raw, mctp_packet_t *pkt_out);

/** Frame a response into raw bytes for BMC HAL transmit */
rtos_err_t mctp_frame_tx(const mctp_packet_t *pkt,
                         uint8_t             *tx_buf,
                         uint16_t            *tx_len);

/** Reset reassembly state (e.g., on timeout or error) */
void mctp_reset(void);

#endif /* MCTP_H */
