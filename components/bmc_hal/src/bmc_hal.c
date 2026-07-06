/**
 * @file bmc_hal.c
 * @brief BMC I2C interface — ESP-IDF I2C slave driver wrapper
 */
#include "bmc_hal.h"
#include "driver/i2c.h"

#define BMC_I2C_PORT    I2C_NUM_0
#define BMC_SDA_PIN     21
#define BMC_SCL_PIN     22
#define BMC_RX_BUF_SIZE 256
#define BMC_TX_BUF_SIZE 256

/*--------------------------------------------------------------------------
 * Module-private state
 *------------------------------------------------------------------------*/
static void (*s_rx_callback)(const uint8_t *, uint16_t) = NULL;
static bool  s_initialized = false;

/*--------------------------------------------------------------------------
 * Public API
 *------------------------------------------------------------------------*/
rtos_err_t bmc_hal_init(uint8_t slave_addr)
{
    i2c_config_t conf = {
        .mode             = I2C_MODE_SLAVE,
        .sda_io_num       = BMC_SDA_PIN,
        .scl_io_num       = BMC_SCL_PIN,
        .sda_pullup_en    = GPIO_PULLUP_ENABLE,
        .scl_pullup_en    = GPIO_PULLUP_ENABLE,
        .slave.addr_10bit_en = 0,
        .slave.slave_addr = slave_addr,
    };

    esp_err_t err = i2c_param_config(BMC_I2C_PORT, &conf);
    if (err != ESP_OK) { return RTOS_ERR_FAIL; }

    err = i2c_driver_install(BMC_I2C_PORT, I2C_MODE_SLAVE,
                             BMC_RX_BUF_SIZE, BMC_TX_BUF_SIZE, 0);
    if (err != ESP_OK) { return RTOS_ERR_FAIL; }

    s_initialized = true;
    return RTOS_OK;
}

rtos_err_t bmc_hal_transmit(const uint8_t *data, uint16_t length)
{
    if (!s_initialized) { return RTOS_ERR_FAIL; }

    int written = i2c_slave_write_buffer(BMC_I2C_PORT, data, length,
                                         pdMS_TO_TICKS(100));
    return (written > 0) ? RTOS_OK : RTOS_ERR_FAIL;
}

rtos_err_t bmc_hal_register_isr(void (*rx_callback)(const uint8_t *, uint16_t))
{
    s_rx_callback = rx_callback;
    return RTOS_OK;
}
