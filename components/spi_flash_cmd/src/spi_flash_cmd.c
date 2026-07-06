/**
 * @file spi_flash_cmd.c
 * @brief SPI Flash JEDEC command layer implementation
 */
#include "spi_flash_cmd.h"
#include "nic_chip_hal.h"
#include <string.h>

/*--------------------------------------------------------------------------
 * Private helpers
 *------------------------------------------------------------------------*/
static inline rtos_err_t send_cmd_with_addr(uint8_t cmd, uint32_t addr)
{
    uint8_t tx[4] = {
        cmd,
        (uint8_t)(addr >> 16),
        (uint8_t)(addr >> 8),
        (uint8_t)(addr)
    };
    return nic_chip_hal_spi_transfer(tx, NULL, 4);
}

static inline rtos_err_t read_status(uint8_t *status_out)
{
    uint8_t tx[2] = { FLASH_CMD_READ_STATUS, 0x00 };
    uint8_t rx[2] = { 0 };
    rtos_err_t err = nic_chip_hal_spi_transfer(tx, rx, 2);
    if (err == RTOS_OK) { *status_out = rx[1]; }
    return err;
}

/*--------------------------------------------------------------------------
 * Public API
 *------------------------------------------------------------------------*/
rtos_err_t spi_flash_cmd_init(void)
{
    /* Verify flash is responding by reading JEDEC ID */
    uint8_t mfr, dev, cap;
    return spi_flash_cmd_read_id(&mfr, &dev, &cap);
}

rtos_err_t spi_flash_cmd_read(uint32_t addr, uint8_t *buf, uint32_t len)
{
    uint8_t cmd[4] = {
        FLASH_CMD_READ_DATA,
        (uint8_t)(addr >> 16),
        (uint8_t)(addr >> 8),
        (uint8_t)(addr)
    };

    /* Send command, then clock in data */
    rtos_err_t err = nic_chip_hal_spi_transfer(cmd, NULL, 4);
    if (err != RTOS_OK) { return err; }

    return nic_chip_hal_spi_transfer(NULL, buf, (uint16_t)len);
}

rtos_err_t spi_flash_cmd_page_program(uint32_t       addr,
                                      const uint8_t *data,
                                      uint16_t       len)
{
    if (len > FLASH_PAGE_SIZE) { return RTOS_ERR_FAIL; }

    uint8_t tx[4 + FLASH_PAGE_SIZE];
    tx[0] = FLASH_CMD_PAGE_PROGRAM;
    tx[1] = (uint8_t)(addr >> 16);
    tx[2] = (uint8_t)(addr >> 8);
    tx[3] = (uint8_t)(addr);
    memcpy(&tx[4], data, len);

    return nic_chip_hal_spi_transfer(tx, NULL, 4 + len);
}

rtos_err_t spi_flash_cmd_sector_erase(uint32_t addr)
{
    return send_cmd_with_addr(FLASH_CMD_SECTOR_ERASE, addr);
}

rtos_err_t spi_flash_cmd_write_enable(void)
{
    uint8_t cmd = FLASH_CMD_WRITE_ENABLE;
    return nic_chip_hal_spi_transfer(&cmd, NULL, 1);
}

rtos_err_t spi_flash_cmd_wait_ready(uint32_t timeout_ms)
{
    uint32_t start = rtos_get_tick_ms();
    uint8_t status;

    do {
        rtos_err_t err = read_status(&status);
        if (err != RTOS_OK) { return err; }

        if ((status & FLASH_STATUS_BUSY) == 0) {
            return RTOS_OK;
        }

        rtos_delay_ms(1);
    } while ((rtos_get_tick_ms() - start) < timeout_ms);

    return RTOS_ERR_TIMEOUT;
}

rtos_err_t spi_flash_cmd_read_id(uint8_t *mfr, uint8_t *dev_type,
                                 uint8_t *capacity)
{
    uint8_t tx[4] = { FLASH_CMD_READ_JEDEC_ID, 0, 0, 0 };
    uint8_t rx[4] = { 0 };

    rtos_err_t err = nic_chip_hal_spi_transfer(tx, rx, 4);
    if (err != RTOS_OK) { return err; }

    *mfr      = rx[1];
    *dev_type = rx[2];
    *capacity = rx[3];
    return RTOS_OK;
}
