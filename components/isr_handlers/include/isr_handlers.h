/**
 * @file isr_handlers.h
 * @brief ISR vector — central interrupt handlers for all HW sources
 */
#ifndef ISR_HANDLERS_H
#define ISR_HANDLERS_H

#include "common_types.h"
#include "rtos_hal.h"

/** Initialize all ISR handlers and register with HW */
rtos_err_t isr_handlers_init(rtos_queue_handle_t bmc_rx_queue,
                             rtos_queue_handle_t nic_event_queue,
                             rtos_queue_handle_t sensor_alert_queue);

#endif /* ISR_HANDLERS_H */
