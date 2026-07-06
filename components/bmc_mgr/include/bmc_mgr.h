/**
 * @file bmc_mgr.h
 * @brief BMC Manager — highest priority service thread
 *
 * Owns the BMC relationship: receives PLDM commands via protocol stack,
 * dispatches to service managers, handles liveness.
 * Blocks on two queues via event flags: BMC RX queue + NIC event queue.
 */
#ifndef BMC_MGR_H
#define BMC_MGR_H

#include "common_types.h"
#include "rtos_hal.h"

/** Initialize BMC manager */
rtos_err_t bmc_mgr_init(rtos_queue_handle_t bmc_rx_queue,
                         rtos_queue_handle_t nic_event_queue,
                         rtos_queue_handle_t flash_cmd_queue);

/** Thread entry point */
void bmc_mgr_task(void *arg);

#endif /* BMC_MGR_H */
