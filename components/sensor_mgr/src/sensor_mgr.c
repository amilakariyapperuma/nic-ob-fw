/**
 * @file sensor_mgr.c
 * @brief Periodic Sensor Manager — owns all sensor hardware access
 */
#include "sensor_mgr.h"
#include "sensor_protocol.h"
#include <string.h>

/*--------------------------------------------------------------------------
 * Module-private state
 *------------------------------------------------------------------------*/
static sensor_cache_t     s_cache          ALIGNED4;
static rtos_sema_handle_t s_cache_mutex    = NULL;
static rtos_queue_handle_t s_alert_queue   = NULL;
static rtos_timer_handle_t s_poll_timer    = NULL;
static volatile bool       s_poll_trigger  = false;

/*--------------------------------------------------------------------------
 * Typed queue wrapper — private to this module.
 *------------------------------------------------------------------------*/
static inline rtos_err_t sensor_alert_queue_receive(nic_event_msg_t *alert,
                                                    uint32_t         timeout_ms)
{
    return rtos_queue_receive(s_alert_queue, alert, timeout_ms);
}

/*--------------------------------------------------------------------------
 * Sensor calibration — private to this module
 *------------------------------------------------------------------------*/
typedef struct ALIGNED4 {
    int16_t  offset;
    uint16_t scale_num;       /* scaled = (raw + offset) * num / den */
    uint16_t scale_den;
    int16_t  upper_critical;
    int16_t  lower_critical;
} sensor_cal_t;

static const sensor_cal_t s_calibration[MAX_SENSORS] = {
    { .offset = 0, .scale_num = 1, .scale_den = 16,
      .upper_critical = 85, .lower_critical = -40 },
    { .offset = 0, .scale_num = 1, .scale_den = 16,
      .upper_critical = 85, .lower_critical = -40 },
    /* TODO: populate remaining sensor calibration */
};

/*--------------------------------------------------------------------------
 * Private helpers
 *------------------------------------------------------------------------*/
static inline int16_t apply_calibration(uint8_t sensor_id, int16_t raw)
{
    const sensor_cal_t *cal = &s_calibration[sensor_id];
    int32_t adjusted = (int32_t)(raw + cal->offset);
    return (int16_t)((adjusted * cal->scale_num) / cal->scale_den);
}

static inline sensor_state_t check_thresholds(uint8_t sensor_id,
                                              int16_t scaled)
{
    const sensor_cal_t *cal = &s_calibration[sensor_id];

    if (scaled >= cal->upper_critical || scaled <= cal->lower_critical) {
        return SENSOR_STATE_CRITICAL;
    }
    /* TODO: add warning thresholds */
    return SENSOR_STATE_OK;
}

static void poll_all_sensors(void)
{
    uint8_t count = s_cache.count;
    uint8_t i;

    /* Unrolled: process 4 sensors per iteration where possible */
    uint8_t unrolled = count & ~3U;
    for (i = 0; i < unrolled; i += 4) {
        /* Sensor i */
        int16_t raw;
        if (sensor_protocol_read(i, &raw) == RTOS_OK) {
            int16_t scaled = apply_calibration(i, raw);
            rtos_mutex_take(s_cache_mutex, RTOS_WAIT_FOREVER);
            s_cache.records[i].raw_value    = raw;
            s_cache.records[i].scaled_value = scaled;
            s_cache.records[i].state        = check_thresholds(i, scaled);
            s_cache.records[i].timestamp_ms = rtos_get_tick_ms();
            rtos_mutex_give(s_cache_mutex);
        }
        /* Sensor i+1 */
        if (sensor_protocol_read(i + 1, &raw) == RTOS_OK) {
            int16_t scaled = apply_calibration(i + 1, raw);
            rtos_mutex_take(s_cache_mutex, RTOS_WAIT_FOREVER);
            s_cache.records[i + 1].raw_value    = raw;
            s_cache.records[i + 1].scaled_value = scaled;
            s_cache.records[i + 1].state        = check_thresholds(i + 1, scaled);
            s_cache.records[i + 1].timestamp_ms = rtos_get_tick_ms();
            rtos_mutex_give(s_cache_mutex);
        }
        /* Sensor i+2 */
        if (sensor_protocol_read(i + 2, &raw) == RTOS_OK) {
            int16_t scaled = apply_calibration(i + 2, raw);
            rtos_mutex_take(s_cache_mutex, RTOS_WAIT_FOREVER);
            s_cache.records[i + 2].raw_value    = raw;
            s_cache.records[i + 2].scaled_value = scaled;
            s_cache.records[i + 2].state        = check_thresholds(i + 2, scaled);
            s_cache.records[i + 2].timestamp_ms = rtos_get_tick_ms();
            rtos_mutex_give(s_cache_mutex);
        }
        /* Sensor i+3 */
        if (sensor_protocol_read(i + 3, &raw) == RTOS_OK) {
            int16_t scaled = apply_calibration(i + 3, raw);
            rtos_mutex_take(s_cache_mutex, RTOS_WAIT_FOREVER);
            s_cache.records[i + 3].raw_value    = raw;
            s_cache.records[i + 3].scaled_value = scaled;
            s_cache.records[i + 3].state        = check_thresholds(i + 3, scaled);
            s_cache.records[i + 3].timestamp_ms = rtos_get_tick_ms();
            rtos_mutex_give(s_cache_mutex);
        }
    }
    /* Remaining sensors */
    for (; i < count; i++) {
        int16_t raw;
        if (sensor_protocol_read(i, &raw) == RTOS_OK) {
            int16_t scaled = apply_calibration(i, raw);
            rtos_mutex_take(s_cache_mutex, RTOS_WAIT_FOREVER);
            s_cache.records[i].raw_value    = raw;
            s_cache.records[i].scaled_value = scaled;
            s_cache.records[i].state        = check_thresholds(i, scaled);
            s_cache.records[i].timestamp_ms = rtos_get_tick_ms();
            rtos_mutex_give(s_cache_mutex);
        }
    }
}

/** Timer callback — runs in timer daemon context, sets flag only */
static void sensor_poll_timer_cb(rtos_timer_handle_t timer)
{
    (void)timer;
    s_poll_trigger = true;
}

/*--------------------------------------------------------------------------
 * Public API
 *------------------------------------------------------------------------*/
rtos_err_t sensor_mgr_init(rtos_queue_handle_t alert_queue)
{
    memset(&s_cache, 0, sizeof(s_cache));
    s_cache.count = 2;   /* TODO: configure based on HW */

    s_cache_mutex = rtos_mutex_create();
    if (s_cache_mutex == NULL) { return RTOS_ERR_NOMEM; }

    s_alert_queue = alert_queue;

    s_poll_timer = rtos_timer_create("sensor_poll",
                                     SENSOR_POLL_INTERVAL_MS,
                                     true, sensor_poll_timer_cb);
    if (s_poll_timer == NULL) { return RTOS_ERR_NOMEM; }

    return sensor_protocol_init();
}

void sensor_mgr_task(void *arg)
{
    (void)arg;

    rtos_timer_start(s_poll_timer);

    while (1) {
        /* Wait for poll trigger or alert event */
        if (s_poll_trigger) {
            s_poll_trigger = false;
            poll_all_sensors();
        }

        /* Check for hardware threshold alerts */
        nic_event_msg_t alert;
        if (sensor_alert_queue_receive(&alert,
                                       SENSOR_POLL_INTERVAL_MS / 2) == RTOS_OK) {
            /* TODO: handle threshold alert — log SEL, notify BMC */
        }
    }
}

rtos_err_t sensor_mgr_get_cached(uint8_t          sensor_id,
                                 sensor_record_t *record_out)
{
    if (sensor_id >= s_cache.count) { return RTOS_ERR_FAIL; }

    rtos_mutex_take(s_cache_mutex, RTOS_WAIT_FOREVER);
    *record_out = s_cache.records[sensor_id];
    rtos_mutex_give(s_cache_mutex);

    return RTOS_OK;
}

rtos_err_t sensor_mgr_get_cache_snapshot(sensor_cache_t *snapshot_out)
{
    rtos_mutex_take(s_cache_mutex, RTOS_WAIT_FOREVER);
    memcpy(snapshot_out, &s_cache, sizeof(sensor_cache_t));
    rtos_mutex_give(s_cache_mutex);
    return RTOS_OK;
}
