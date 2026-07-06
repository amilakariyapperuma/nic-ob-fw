/**
 * @file sensor_hal.c
 * @brief Sensor I2C master driver — ESP-IDF I2C master wrapper
 */
#include "sensor_hal.h"
#include "driver/i2c.h"

#define SENSOR_I2C_PORT   I2C_NUM_1
#define SENSOR_SDA_PIN    18
#define SENSOR_SCL_PIN    19
#define SENSOR_CLK_HZ     100000

/*--------------------------------------------------------------------------
 * Module-private state
 *------------------------------------------------------------------------*/
static bool s_initialized = false;

/*--------------------------------------------------------------------------
 * Public API
 *------------------------------------------------------------------------*/
rtos_err_t sensor_hal_init(void)
{
    i2c_config_t conf = {
        .mode             = I2C_MODE_MASTER,
        .sda_io_num       = SENSOR_SDA_PIN,
        .scl_io_num       = SENSOR_SCL_PIN,
        .sda_pullup_en    = GPIO_PULLUP_ENABLE,
        .scl_pullup_en    = GPIO_PULLUP_ENABLE,
        .master.clk_speed = SENSOR_CLK_HZ,
    };

    esp_err_t err = i2c_param_config(SENSOR_I2C_PORT, &conf);
    if (err != ESP_OK) { return RTOS_ERR_FAIL; }

    err = i2c_driver_install(SENSOR_I2C_PORT, I2C_MODE_MASTER, 0, 0, 0);
    if (err != ESP_OK) { return RTOS_ERR_FAIL; }

    s_initialized = true;
    return RTOS_OK;
}

rtos_err_t sensor_hal_read_reg(uint8_t  dev_addr,
                               uint8_t  reg_addr,
                               uint8_t *data_out,
                               uint16_t length)
{
    if (!s_initialized) { return RTOS_ERR_FAIL; }

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (dev_addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg_addr, true);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (dev_addr << 1) | I2C_MASTER_READ, true);
    i2c_master_read(cmd, data_out, length, I2C_MASTER_LAST_NACK);
    i2c_master_stop(cmd);

    esp_err_t err = i2c_master_cmd_begin(SENSOR_I2C_PORT, cmd,
                                         pdMS_TO_TICKS(50));
    i2c_cmd_link_delete(cmd);
    return (err == ESP_OK) ? RTOS_OK : RTOS_ERR_FAIL;
}

rtos_err_t sensor_hal_write_reg(uint8_t        dev_addr,
                                uint8_t        reg_addr,
                                const uint8_t *data,
                                uint16_t       length)
{
    if (!s_initialized) { return RTOS_ERR_FAIL; }

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (dev_addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg_addr, true);
    i2c_master_write(cmd, data, length, true);
    i2c_master_stop(cmd);

    esp_err_t err = i2c_master_cmd_begin(SENSOR_I2C_PORT, cmd,
                                         pdMS_TO_TICKS(50));
    i2c_cmd_link_delete(cmd);
    return (err == ESP_OK) ? RTOS_OK : RTOS_ERR_FAIL;
}
