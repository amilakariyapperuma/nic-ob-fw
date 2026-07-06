/**
 * @file sensor_mgr.h
 * @brief Periodic Sensor Manager — polls sensors, maintains cache
 */
#ifndef SENSOR_MGR_H
#define SENSOR_MGR_H

#include "common_types.h"
#include "rtos_hal.h"

/** Initialize sensor manager (creates timer, not thread — main creates thread) */
rtos_err_t sensor_mgr_init(rtos_queue_handle_t alert_queue);

/** Thread entry point */
void sensor_mgr_task(void *arg);

/**
 * Read cached sensor value — called from BMC Mgr thread context.
 * Thread-safe via mutex.
 */
rtos_err_t sensor_mgr_get_cached(uint8_t          sensor_id,
                                 sensor_record_t *record_out);

/** Get pointer to entire cache for bulk read — mutex must be held */
rtos_err_t sensor_mgr_get_cache_snapshot(sensor_cache_t *snapshot_out);

#endif /* SENSOR_MGR_H */
