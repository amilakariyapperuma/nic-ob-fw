/**
 * @file nic_flash_mgr.c
 * @brief NIC Flash Manager — lowest priority thread, long-running ops
 */
#include "nic_flash_mgr.h"
#include "spi_flash_cmd.h"
#include <string.h>

/*--------------------------------------------------------------------------
 * Module-private state
 *------------------------------------------------------------------------*/
static rtos_queue_handle_t s_cmd_queue   = NULL;
static bool                s_update_active = false;

/*--------------------------------------------------------------------------
 * Module-private flash update state machine
 *------------------------------------------------------------------------*/
typedef enum {
    FW_STATE_IDLE = 0,
    FW_STATE_RECEIVING,
    FW_STATE_VERIFYING,
    FW_STATE_ACTIVATING,
    FW_STATE_ERROR
} fw_update_state_t;

static fw_update_state_t s_fw_state = FW_STATE_IDLE;

/*--------------------------------------------------------------------------
 * Private helpers
 *------------------------------------------------------------------------*/
static rtos_err_t write_image_to_staging(uint32_t       offset,
                                         const uint8_t *data,
                                         uint32_t       size)
{
    uint32_t addr = offset;
    uint32_t remaining = size;

    while (remaining > 0) {
        /* Erase sector if at sector boundary */
        if ((addr % FLASH_SECTOR_SIZE) == 0) {
            rtos_err_t err = spi_flash_cmd_write_enable();
            if (err != RTOS_OK) { return err; }

            err = spi_flash_cmd_sector_erase(addr);
            if (err != RTOS_OK) { return err; }

            /* Wait for erase — this is the long-running part.
             * Thread is preemptable here by higher-priority tasks. */
            err = spi_flash_cmd_wait_ready(3000);
            if (err != RTOS_OK) { return err; }
        }

        /* Write one page */
        uint16_t chunk = (remaining > FLASH_PAGE_SIZE)
                         ? FLASH_PAGE_SIZE
                         : (uint16_t)remaining;

        rtos_err_t err = spi_flash_cmd_write_enable();
        if (err != RTOS_OK) { return err; }

        err = spi_flash_cmd_page_program(addr, data, chunk);
        if (err != RTOS_OK) { return err; }

        err = spi_flash_cmd_wait_ready(100);
        if (err != RTOS_OK) { return err; }

        addr      += chunk;
        data      += chunk;
        remaining -= chunk;
    }

    return RTOS_OK;
}

static rtos_err_t handle_update_start(const flash_cmd_t *cmd)
{
    s_fw_state     = FW_STATE_RECEIVING;
    s_update_active = true;

    /* TODO: read image from source (BMC-provided staging area)
     *       and write to flash staging partition.
     *       ERoT will validate after write completes. */

    rtos_err_t err = write_image_to_staging(cmd->image_offset,
                                            NULL, /* TODO: source */
                                            cmd->image_size);
    if (err != RTOS_OK) {
        s_fw_state = FW_STATE_ERROR;
        return err;
    }

    s_fw_state = FW_STATE_VERIFYING;
    /* ERoT validates autonomously at HW level — we just wrote the image */

    s_fw_state      = FW_STATE_IDLE;
    s_update_active = false;
    return RTOS_OK;
}

static rtos_err_t handle_update_abort(void)
{
    s_fw_state      = FW_STATE_IDLE;
    s_update_active = false;
    return RTOS_OK;
}

/*--------------------------------------------------------------------------
 * Public API
 *------------------------------------------------------------------------*/
rtos_err_t nic_flash_mgr_init(rtos_queue_handle_t cmd_queue)
{
    s_cmd_queue = cmd_queue;
    s_fw_state  = FW_STATE_IDLE;

    return spi_flash_cmd_init();
}

void nic_flash_mgr_task(void *arg)
{
    (void)arg;

    flash_cmd_t cmd;

    while (1) {
        /* Block until BMC Mgr dispatches a command */
        if (rtos_queue_receive(s_cmd_queue, &cmd,
                               RTOS_WAIT_FOREVER) != RTOS_OK) {
            continue;
        }

        switch (cmd.cmd_type) {
        case FLASH_CMD_UPDATE_START:
            handle_update_start(&cmd);
            break;
        case FLASH_CMD_UPDATE_ABORT:
            handle_update_abort();
            break;
        case FLASH_CMD_VERIFY:
            /* TODO: trigger verification read-back */
            break;
        case FLASH_CMD_ACTIVATE:
            /* TODO: signal ERoT to activate staged image */
            break;
        default:
            break;
        }
    }
}
