/**
 * @file nic_chip_hal.c
 * @brief NIC Chip HAL — ESP-IDF SPI master + GPIO wrapper
 */
#include "nic_chip_hal.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"

#define NIC_SPI_HOST   SPI2_HOST
#define NIC_SPI_MOSI   23
#define NIC_SPI_MISO   25
#define NIC_SPI_SCLK   26
#define NIC_SPI_CS     27
#define NIC_SPI_CLK_HZ 10000000
#define NIC_RESET_PIN  4
#define NIC_ALERT_PIN  5

/*--------------------------------------------------------------------------
 * Module-private state
 *------------------------------------------------------------------------*/
static spi_device_handle_t s_spi_dev = NULL;
static bool s_initialized = false;

/*--------------------------------------------------------------------------
 * Public API
 *------------------------------------------------------------------------*/
rtos_err_t nic_chip_hal_init(void)
{
    spi_bus_config_t bus_cfg = {
        .mosi_io_num   = NIC_SPI_MOSI,
        .miso_io_num   = NIC_SPI_MISO,
        .sclk_io_num   = NIC_SPI_SCLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
    };

    esp_err_t err = spi_bus_initialize(NIC_SPI_HOST, &bus_cfg,
                                       SPI_DMA_CH_AUTO);
    if (err != ESP_OK) { return RTOS_ERR_FAIL; }

    spi_device_interface_config_t dev_cfg = {
        .clock_speed_hz = NIC_SPI_CLK_HZ,
        .mode           = 0,
        .spics_io_num   = NIC_SPI_CS,
        .queue_size     = 4,
    };

    err = spi_bus_add_device(NIC_SPI_HOST, &dev_cfg, &s_spi_dev);
    if (err != ESP_OK) { return RTOS_ERR_FAIL; }

    /* Configure reset GPIO as output */
    gpio_config_t reset_cfg = {
        .pin_bit_mask = (1ULL << NIC_RESET_PIN),
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
    };
    gpio_config(&reset_cfg);

    s_initialized = true;
    return RTOS_OK;
}

rtos_err_t nic_chip_hal_spi_transfer(const uint8_t *tx_buf,
                                     uint8_t       *rx_buf,
                                     uint16_t       length)
{
    if (!s_initialized) { return RTOS_ERR_FAIL; }

    spi_transaction_t txn = {
        .length    = length * 8,
        .tx_buffer = tx_buf,
        .rx_buffer = rx_buf,
    };

    esp_err_t err = spi_device_transmit(s_spi_dev, &txn);
    return (err == ESP_OK) ? RTOS_OK : RTOS_ERR_FAIL;
}

rtos_err_t nic_chip_hal_gpio_set(uint8_t pin_id, bool level)
{
    esp_err_t err = gpio_set_level((gpio_num_t)pin_id, level ? 1 : 0);
    return (err == ESP_OK) ? RTOS_OK : RTOS_ERR_FAIL;
}

bool nic_chip_hal_gpio_get(uint8_t pin_id)
{
    return gpio_get_level((gpio_num_t)pin_id) != 0;
}

rtos_err_t nic_chip_hal_register_alert_isr(uint8_t pin_id,
                                           void (*callback)(void *),
                                           void *arg)
{
    gpio_config_t alert_cfg = {
        .pin_bit_mask = (1ULL << pin_id),
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_ENABLE,
        .intr_type    = GPIO_INTR_NEGEDGE,
    };
    gpio_config(&alert_cfg);
    gpio_install_isr_service(0);
    esp_err_t err = gpio_isr_handler_add((gpio_num_t)pin_id, callback, arg);
    return (err == ESP_OK) ? RTOS_OK : RTOS_ERR_FAIL;
}
