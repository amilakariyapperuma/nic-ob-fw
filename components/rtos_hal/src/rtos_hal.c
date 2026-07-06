/**
 * @file rtos_hal.c
 * @brief RTOS abstraction layer — ESP-IDF FreeRTOS implementation
 */
#include "rtos_hal.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/event_groups.h"
#include "freertos/timers.h"
#include "esp_timer.h"

/*--------------------------------------------------------------------------
 * Internal helpers
 *------------------------------------------------------------------------*/
static inline TickType_t ms_to_ticks(uint32_t ms)
{
    return (ms == RTOS_WAIT_FOREVER) ? portMAX_DELAY
                                     : pdMS_TO_TICKS(ms);
}

/*--------------------------------------------------------------------------
 * Task management
 *------------------------------------------------------------------------*/
rtos_err_t rtos_task_create(rtos_task_func_t    func,
                            const char         *name,
                            uint32_t            stack_size,
                            void               *arg,
                            uint32_t            priority,
                            rtos_task_handle_t *handle_out)
{
    BaseType_t ret = xTaskCreatePinnedToCore(
        func, name, stack_size, arg, priority,
        (TaskHandle_t *)handle_out, tskNO_AFFINITY);
    return (ret == pdPASS) ? RTOS_OK : RTOS_ERR_NOMEM;
}

void rtos_task_priority_set(rtos_task_handle_t handle, uint32_t priority)
{
    vTaskPrioritySet((TaskHandle_t)handle, priority);
}

uint32_t rtos_task_priority_get(rtos_task_handle_t handle)
{
    return (uint32_t)uxTaskPriorityGet((TaskHandle_t)handle);
}

/*--------------------------------------------------------------------------
 * Queue management
 *------------------------------------------------------------------------*/
rtos_queue_handle_t rtos_queue_create(uint32_t depth, uint32_t item_size)
{
    return (rtos_queue_handle_t)xQueueCreate(depth, item_size);
}

rtos_err_t rtos_queue_send(rtos_queue_handle_t queue,
                           const void         *item,
                           uint32_t            timeout_ms)
{
    BaseType_t ret = xQueueSend((QueueHandle_t)queue, item,
                                ms_to_ticks(timeout_ms));
    return (ret == pdPASS) ? RTOS_OK : RTOS_ERR_FULL;
}

rtos_err_t rtos_queue_send_from_isr(rtos_queue_handle_t queue,
                                    const void         *item,
                                    bool               *higher_prio_woken)
{
    BaseType_t woken = pdFALSE;
    BaseType_t ret = xQueueSendFromISR((QueueHandle_t)queue, item, &woken);
    if (higher_prio_woken) {
        *higher_prio_woken = (woken == pdTRUE);
    }
    return (ret == pdPASS) ? RTOS_OK : RTOS_ERR_FULL;
}

rtos_err_t rtos_queue_receive(rtos_queue_handle_t queue,
                              void               *item,
                              uint32_t            timeout_ms)
{
    BaseType_t ret = xQueueReceive((QueueHandle_t)queue, item,
                                   ms_to_ticks(timeout_ms));
    return (ret == pdPASS) ? RTOS_OK : RTOS_ERR_TIMEOUT;
}

/*--------------------------------------------------------------------------
 * Semaphore / mutex
 *------------------------------------------------------------------------*/
rtos_sema_handle_t rtos_mutex_create(void)
{
    return (rtos_sema_handle_t)xSemaphoreCreateMutex();
}

rtos_err_t rtos_mutex_take(rtos_sema_handle_t mutex, uint32_t timeout_ms)
{
    BaseType_t ret = xSemaphoreTake((SemaphoreHandle_t)mutex,
                                    ms_to_ticks(timeout_ms));
    return (ret == pdPASS) ? RTOS_OK : RTOS_ERR_TIMEOUT;
}

rtos_err_t rtos_mutex_give(rtos_sema_handle_t mutex)
{
    BaseType_t ret = xSemaphoreGive((SemaphoreHandle_t)mutex);
    return (ret == pdPASS) ? RTOS_OK : RTOS_ERR_FAIL;
}

rtos_sema_handle_t rtos_sema_binary_create(void)
{
    return (rtos_sema_handle_t)xSemaphoreCreateBinary();
}

rtos_err_t rtos_sema_give(rtos_sema_handle_t sema)
{
    BaseType_t ret = xSemaphoreGive((SemaphoreHandle_t)sema);
    return (ret == pdPASS) ? RTOS_OK : RTOS_ERR_FAIL;
}

rtos_err_t rtos_sema_give_from_isr(rtos_sema_handle_t sema,
                                   bool              *higher_prio_woken)
{
    BaseType_t woken = pdFALSE;
    BaseType_t ret = xSemaphoreGiveFromISR((SemaphoreHandle_t)sema, &woken);
    if (higher_prio_woken) {
        *higher_prio_woken = (woken == pdTRUE);
    }
    return (ret == pdPASS) ? RTOS_OK : RTOS_ERR_FAIL;
}

rtos_err_t rtos_sema_take(rtos_sema_handle_t sema, uint32_t timeout_ms)
{
    BaseType_t ret = xSemaphoreTake((SemaphoreHandle_t)sema,
                                    ms_to_ticks(timeout_ms));
    return (ret == pdPASS) ? RTOS_OK : RTOS_ERR_TIMEOUT;
}

/*--------------------------------------------------------------------------
 * Event groups
 *------------------------------------------------------------------------*/
rtos_event_group_handle_t rtos_event_group_create(void)
{
    return (rtos_event_group_handle_t)xEventGroupCreate();
}

uint32_t rtos_event_group_wait(rtos_event_group_handle_t group,
                               uint32_t                  bits_to_wait,
                               bool                      clear_on_exit,
                               bool                      wait_all,
                               uint32_t                  timeout_ms)
{
    return (uint32_t)xEventGroupWaitBits(
        (EventGroupHandle_t)group,
        (EventBits_t)bits_to_wait,
        clear_on_exit ? pdTRUE : pdFALSE,
        wait_all      ? pdTRUE : pdFALSE,
        ms_to_ticks(timeout_ms));
}

rtos_err_t rtos_event_group_set(rtos_event_group_handle_t group,
                                uint32_t                  bits)
{
    xEventGroupSetBits((EventGroupHandle_t)group, (EventBits_t)bits);
    return RTOS_OK;
}

rtos_err_t rtos_event_group_set_from_isr(rtos_event_group_handle_t group,
                                         uint32_t                  bits,
                                         bool                     *higher_prio_woken)
{
    BaseType_t woken = pdFALSE;
    BaseType_t ret = xEventGroupSetBitsFromISR(
        (EventGroupHandle_t)group, (EventBits_t)bits, &woken);
    if (higher_prio_woken) {
        *higher_prio_woken = (woken == pdTRUE);
    }
    return (ret == pdPASS) ? RTOS_OK : RTOS_ERR_FAIL;
}

/*--------------------------------------------------------------------------
 * Software timer
 *------------------------------------------------------------------------*/
rtos_timer_handle_t rtos_timer_create(const char    *name,
                                      uint32_t       period_ms,
                                      bool           auto_reload,
                                      rtos_timer_cb_t callback)
{
    return (rtos_timer_handle_t)xTimerCreate(
        name, pdMS_TO_TICKS(period_ms),
        auto_reload ? pdTRUE : pdFALSE,
        NULL, (TimerCallbackFunction_t)callback);
}

rtos_err_t rtos_timer_start(rtos_timer_handle_t timer)
{
    BaseType_t ret = xTimerStart((TimerHandle_t)timer, 0);
    return (ret == pdPASS) ? RTOS_OK : RTOS_ERR_FAIL;
}

rtos_err_t rtos_timer_stop(rtos_timer_handle_t timer)
{
    BaseType_t ret = xTimerStop((TimerHandle_t)timer, 0);
    return (ret == pdPASS) ? RTOS_OK : RTOS_ERR_FAIL;
}

/*--------------------------------------------------------------------------
 * Utility
 *------------------------------------------------------------------------*/
uint32_t rtos_get_tick_ms(void)
{
    return (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);
}

void rtos_delay_ms(uint32_t ms)
{
    vTaskDelay(pdMS_TO_TICKS(ms));
}

void rtos_yield_from_isr(bool higher_prio_woken)
{
    if (higher_prio_woken) {
        portYIELD_FROM_ISR();
    }
}

/*--------------------------------------------------------------------------
 * Critical section
 *------------------------------------------------------------------------*/
void rtos_enter_critical(void)
{
    taskENTER_CRITICAL(&((portMUX_TYPE)portMUX_INITIALIZER_UNLOCKED));
}

void rtos_exit_critical(void)
{
    taskEXIT_CRITICAL(&((portMUX_TYPE)portMUX_INITIALIZER_UNLOCKED));
}
