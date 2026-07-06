/**
 * @file rtos_hal.h
 * @brief RTOS abstraction layer wrapping ESP-IDF FreeRTOS APIs
 */
#ifndef RTOS_HAL_H
#define RTOS_HAL_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/*--------------------------------------------------------------------------
 * Opaque handle types
 *------------------------------------------------------------------------*/
typedef void *rtos_task_handle_t;
typedef void *rtos_queue_handle_t;
typedef void *rtos_sema_handle_t;
typedef void *rtos_event_group_handle_t;
typedef void *rtos_timer_handle_t;
typedef void (*rtos_task_func_t)(void *arg);
typedef void (*rtos_timer_cb_t)(rtos_timer_handle_t timer);

/*--------------------------------------------------------------------------
 * Return codes
 *------------------------------------------------------------------------*/
typedef enum {
    RTOS_OK = 0,
    RTOS_ERR_TIMEOUT,
    RTOS_ERR_FULL,
    RTOS_ERR_EMPTY,
    RTOS_ERR_NOMEM,
    RTOS_ERR_FAIL
} rtos_err_t;

/*--------------------------------------------------------------------------
 * Special timeout values
 *------------------------------------------------------------------------*/
#define RTOS_WAIT_FOREVER   0xFFFFFFFFU
#define RTOS_NO_WAIT        0U

/*--------------------------------------------------------------------------
 * Task management
 *------------------------------------------------------------------------*/
rtos_err_t rtos_task_create(rtos_task_func_t func,
                            const char      *name,
                            uint32_t         stack_size,
                            void            *arg,
                            uint32_t         priority,
                            rtos_task_handle_t *handle_out);

void rtos_task_priority_set(rtos_task_handle_t handle, uint32_t priority);

uint32_t rtos_task_priority_get(rtos_task_handle_t handle);

/*--------------------------------------------------------------------------
 * Queue management
 *------------------------------------------------------------------------*/
rtos_queue_handle_t rtos_queue_create(uint32_t depth, uint32_t item_size);

rtos_err_t rtos_queue_send(rtos_queue_handle_t queue,
                           const void         *item,
                           uint32_t            timeout_ms);

rtos_err_t rtos_queue_send_from_isr(rtos_queue_handle_t queue,
                                    const void         *item,
                                    bool               *higher_prio_woken);

rtos_err_t rtos_queue_receive(rtos_queue_handle_t queue,
                              void               *item,
                              uint32_t            timeout_ms);

/*--------------------------------------------------------------------------
 * Semaphore / mutex
 *------------------------------------------------------------------------*/
rtos_sema_handle_t rtos_mutex_create(void);

rtos_err_t rtos_mutex_take(rtos_sema_handle_t mutex, uint32_t timeout_ms);

rtos_err_t rtos_mutex_give(rtos_sema_handle_t mutex);

rtos_sema_handle_t rtos_sema_binary_create(void);

rtos_err_t rtos_sema_give(rtos_sema_handle_t sema);

rtos_err_t rtos_sema_give_from_isr(rtos_sema_handle_t sema,
                                   bool              *higher_prio_woken);

rtos_err_t rtos_sema_take(rtos_sema_handle_t sema, uint32_t timeout_ms);

/*--------------------------------------------------------------------------
 * Event groups — for multi-queue wait pattern
 *------------------------------------------------------------------------*/
rtos_event_group_handle_t rtos_event_group_create(void);

uint32_t rtos_event_group_wait(rtos_event_group_handle_t group,
                               uint32_t                  bits_to_wait,
                               bool                      clear_on_exit,
                               bool                      wait_all,
                               uint32_t                  timeout_ms);

rtos_err_t rtos_event_group_set(rtos_event_group_handle_t group,
                                uint32_t                  bits);

rtos_err_t rtos_event_group_set_from_isr(rtos_event_group_handle_t group,
                                         uint32_t                  bits,
                                         bool                     *higher_prio_woken);

/*--------------------------------------------------------------------------
 * Software timer — for periodic sensor polling
 *------------------------------------------------------------------------*/
rtos_timer_handle_t rtos_timer_create(const char    *name,
                                      uint32_t       period_ms,
                                      bool           auto_reload,
                                      rtos_timer_cb_t callback);

rtos_err_t rtos_timer_start(rtos_timer_handle_t timer);

rtos_err_t rtos_timer_stop(rtos_timer_handle_t timer);

/*--------------------------------------------------------------------------
 * Utility
 *------------------------------------------------------------------------*/
uint32_t rtos_get_tick_ms(void);

void rtos_delay_ms(uint32_t ms);

void rtos_yield_from_isr(bool higher_prio_woken);

/*--------------------------------------------------------------------------
 * Critical section — for ISR-safe shared variable access
 *------------------------------------------------------------------------*/
void rtos_enter_critical(void);

void rtos_exit_critical(void);

#endif /* RTOS_HAL_H */
