/**
 * @file nic_flash_mgr.h
 * @brief NIC Flash Manager — firmware update thread
 */
#ifndef NIC_FLASH_MGR_H
#define NIC_FLASH_MGR_H

#include "common_types.h"
#include "rtos_hal.h"

/** Initialize flash manager */
rtos_err_t nic_flash_mgr_init(rtos_queue_handle_t cmd_queue);

/** Thread entry point — blocks on command queue */
void nic_flash_mgr_task(void *arg);

#endif /* NIC_FLASH_MGR_H */
