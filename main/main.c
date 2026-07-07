/**
 * @file main.c
 * @brief NIC Out-of-Band Firmware — system initialization and task creation
 *
 * Architecture:
 *   3 threads  — BMC Manager (high), Sensor Manager (medium), Flash Manager (low)
 *   1 module   — NIC Gen Manager (runs in BMC Mgr context)
 *   ISR layer  — I2C RX (BMC), GPIO alert (NIC ASIC)
 *
 * Thread split principle: priority drives thread boundaries.
 * Code modularity drives module boundaries (separate concern).
 */
#include "common_types.h"
#include "rtos_hal.h"

/* HAL drivers */
#include "bmc_hal.h"
#include "sensor_hal.h"
#include "nic_chip_hal.h"

/* ISR handlers */
#include "isr_handlers.h"

/* Service managers */
#include "bmc_mgr.h"
#include "sensor_mgr.h"
#include "nic_flash_mgr.h"

#include "esp_log.h"

#define TAG "NIC_OB_FW"

#define BMC_I2C_SLAVE_ADDR  0x20

/*--------------------------------------------------------------------------
 * Typed queue creation macro.
 * Locks item_size to sizeof(type) at the creation site — ensures the
 * queue's internal buffer matches the concrete struct used by senders
 * and receivers. Eliminates the risk of size mismatch between creation
 * and use.
 *
 * Usage: QUEUE_CREATE_TYPED(depth, bmc_rx_msg_t)
 *------------------------------------------------------------------------*/
#define QUEUE_CREATE_TYPED(depth, type) \
    rtos_queue_create((depth), sizeof(type))

/*--------------------------------------------------------------------------
 * System queues — created here, shared across modules via init params
 *------------------------------------------------------------------------*/
static rtos_queue_handle_t s_bmc_rx_queue       = NULL;
static rtos_queue_handle_t s_nic_event_queue    = NULL;
static rtos_queue_handle_t s_sensor_alert_queue = NULL;
static rtos_queue_handle_t s_flash_cmd_queue    = NULL;

/*--------------------------------------------------------------------------
 * Task handles
 *------------------------------------------------------------------------*/
static rtos_task_handle_t s_bmc_mgr_task    = NULL;
static rtos_task_handle_t s_sensor_mgr_task = NULL;
static rtos_task_handle_t s_flash_mgr_task  = NULL;

/*--------------------------------------------------------------------------
 * Initialization sequence
 *------------------------------------------------------------------------*/
static rtos_err_t init_queues(void)
{
    s_bmc_rx_queue = QUEUE_CREATE_TYPED(QUEUE_DEPTH_BMC, bmc_rx_msg_t);
    if (s_bmc_rx_queue == NULL) { return RTOS_ERR_NOMEM; }

    s_nic_event_queue = QUEUE_CREATE_TYPED(QUEUE_DEPTH_NIC_GEN, nic_event_msg_t);
    if (s_nic_event_queue == NULL) { return RTOS_ERR_NOMEM; }

    s_sensor_alert_queue = QUEUE_CREATE_TYPED(QUEUE_DEPTH_SENSOR, nic_event_msg_t);
    if (s_sensor_alert_queue == NULL) { return RTOS_ERR_NOMEM; }

    s_flash_cmd_queue = QUEUE_CREATE_TYPED(QUEUE_DEPTH_FLASH, flash_cmd_t);
    if (s_flash_cmd_queue == NULL) { return RTOS_ERR_NOMEM; }

    return RTOS_OK;
}

static rtos_err_t init_hal(void)
{
    rtos_err_t err;

    err = bmc_hal_init(BMC_I2C_SLAVE_ADDR);
    if (err != RTOS_OK) {
        ESP_LOGE(TAG, "BMC HAL init failed");
        return err;
    }

    err = sensor_hal_init();
    if (err != RTOS_OK) {
        ESP_LOGE(TAG, "Sensor HAL init failed");
        return err;
    }

    err = nic_chip_hal_init();
    if (err != RTOS_OK) {
        ESP_LOGE(TAG, "NIC Chip HAL init failed");
        return err;
    }

    return RTOS_OK;
}

static rtos_err_t init_services(void)
{
    rtos_err_t err;

    err = bmc_mgr_init(s_bmc_rx_queue, s_nic_event_queue,
                        s_flash_cmd_queue);
    if (err != RTOS_OK) {
        ESP_LOGE(TAG, "BMC Mgr init failed");
        return err;
    }

    err = sensor_mgr_init(s_sensor_alert_queue);
    if (err != RTOS_OK) {
        ESP_LOGE(TAG, "Sensor Mgr init failed");
        return err;
    }

    err = nic_flash_mgr_init(s_flash_cmd_queue);
    if (err != RTOS_OK) {
        ESP_LOGE(TAG, "Flash Mgr init failed");
        return err;
    }

    return RTOS_OK;
}

static rtos_err_t init_isr(void)
{
    return isr_handlers_init(s_bmc_rx_queue, s_nic_event_queue,
                             s_sensor_alert_queue);
}

static rtos_err_t create_tasks(void)
{
    rtos_err_t err;

    /* BMC Manager — highest priority service thread */
    err = rtos_task_create(bmc_mgr_task, "BMC_Mgr",
                           TASK_STACK_BMC, NULL,
                           PRI_BMC_MGR, &s_bmc_mgr_task);
    if (err != RTOS_OK) {
        ESP_LOGE(TAG, "BMC Mgr task create failed");
        return err;
    }

    /* Sensor Manager — periodic polling */
    err = rtos_task_create(sensor_mgr_task, "Sensor_Mgr",
                           TASK_STACK_SENSOR, NULL,
                           PRI_SENSOR_MGR, &s_sensor_mgr_task);
    if (err != RTOS_OK) {
        ESP_LOGE(TAG, "Sensor Mgr task create failed");
        return err;
    }

    /* Flash Manager — lowest priority, long-running */
    err = rtos_task_create(nic_flash_mgr_task, "Flash_Mgr",
                           TASK_STACK_FLASH, NULL,
                           PRI_FLASH_MGR, &s_flash_mgr_task);
    if (err != RTOS_OK) {
        ESP_LOGE(TAG, "Flash Mgr task create failed");
        return err;
    }

    return RTOS_OK;
}

/*--------------------------------------------------------------------------
 * ESP-IDF entry point
 *------------------------------------------------------------------------*/
void app_main(void)
{
    ESP_LOGI(TAG, "NIC Out-of-Band Firmware starting...");

    /* Init order matters:
     * 1. Queues    — must exist before ISR or tasks reference them
     * 2. HAL       — hardware initialized before services use it
     * 3. Services  — modules initialized with queue handles
     * 4. ISR       — registered after queues and HAL are ready
     * 5. Tasks     — started last, begin processing immediately
     */

    rtos_err_t err;

    err = init_queues();
    if (err != RTOS_OK) { ESP_LOGE(TAG, "Queue init failed"); return; }

    err = init_hal();
    if (err != RTOS_OK) { ESP_LOGE(TAG, "HAL init failed"); return; }

    err = init_services();
    if (err != RTOS_OK) { ESP_LOGE(TAG, "Service init failed"); return; }

    err = init_isr();
    if (err != RTOS_OK) { ESP_LOGE(TAG, "ISR init failed"); return; }

    err = create_tasks();
    if (err != RTOS_OK) { ESP_LOGE(TAG, "Task creation failed"); return; }

    ESP_LOGI(TAG, "All systems initialized. Threads running.");

    /* app_main returns — FreeRTOS scheduler is already running on ESP-IDF */
}
