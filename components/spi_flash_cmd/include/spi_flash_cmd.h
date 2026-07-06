/**
 * @file spi_flash_cmd.h
 * @brief SPI Flash JEDEC command layer
 */
#ifndef SPI_FLASH_CMD_H
#define SPI_FLASH_CMD_H

#include "common_types.h"
#include "rtos_hal.h"

/* Standard JEDEC SPI Flash commands */
#define FLASH_CMD_WRITE_ENABLE   0x06
#define FLASH_CMD_WRITE_DISABLE  0x04
#define FLASH_CMD_READ_STATUS    0x05
#define FLASH_CMD_PAGE_PROGRAM   0x02
#define FLASH_CMD_SECTOR_ERASE   0x20
#define FLASH_CMD_READ_DATA      0x03
#define FLASH_CMD_READ_JEDEC_ID  0x9F

/* Status register bits */
#define FLASH_STATUS_BUSY        0x01
#define FLASH_STATUS_WEL         0x02

/** Initialize flash command layer */
rtos_err_t spi_flash_cmd_init(void);

/** Read flash data at given address */
rtos_err_t spi_flash_cmd_read(uint32_t addr, uint8_t *buf, uint32_t len);

/** Write a page (up to 256 bytes) — caller must write-enable first */
rtos_err_t spi_flash_cmd_page_program(uint32_t addr,
                                      const uint8_t *data,
                                      uint16_t len);

/** Erase a 4KB sector */
rtos_err_t spi_flash_cmd_sector_erase(uint32_t addr);

/** Enable writes */
rtos_err_t spi_flash_cmd_write_enable(void);

/** Poll status register until not busy, with timeout */
rtos_err_t spi_flash_cmd_wait_ready(uint32_t timeout_ms);

/** Read JEDEC manufacturer/device ID */
rtos_err_t spi_flash_cmd_read_id(uint8_t *mfr, uint8_t *dev_type,
                                 uint8_t *capacity);

#endif /* SPI_FLASH_CMD_H */
