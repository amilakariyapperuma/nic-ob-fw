/**
 * @file sensor_protocol.c
 * @brief Sensor protocol — stub implementation
 *
 * TODO: Implement PMBus commands or direct register map
 *       depending on actual sensor hardware.
 */
#include "sensor_protocol.h"
#include "sensor_hal.h"

/*--------------------------------------------------------------------------
 * Module-private sensor address table
 *------------------------------------------------------------------------*/
typedef struct ALIGNED4 {
    uint8_t i2c_addr;
    uint8_t reg_addr;
    uint8_t sensor_type;
    uint8_t reserved;
} sensor_hw_map_t;

static const sensor_hw_map_t s_sensor_map[MAX_SENSORS] = {
    { .i2c_addr = 0x48, .reg_addr = 0x00, .sensor_type = SENSOR_TYPE_TEMP },
    { .i2c_addr = 0x49, .reg_addr = 0x00, .sensor_type = SENSOR_TYPE_TEMP },
    /* TODO: populate remaining sensor entries */
};

/*--------------------------------------------------------------------------
 * Public API
 *------------------------------------------------------------------------*/
rtos_err_t sensor_protocol_init(void)
{
    /* TODO: initialize PMBus if applicable */
    return RTOS_OK;
}

rtos_err_t sensor_protocol_read(uint8_t sensor_id, int16_t *raw_value_out)
{
    if (sensor_id >= MAX_SENSORS) { return RTOS_ERR_FAIL; }

    const sensor_hw_map_t *map = &s_sensor_map[sensor_id];
    uint8_t raw[2];

    rtos_err_t err = sensor_hal_read_reg(map->i2c_addr, map->reg_addr,
                                         raw, 2);
    if (err != RTOS_OK) { return err; }

    /* Big-endian 16-bit typical for temp sensors */
    *raw_value_out = (int16_t)((raw[0] << 8) | raw[1]);
    return RTOS_OK;
}

rtos_err_t sensor_protocol_set_threshold(uint8_t sensor_id,
                                         int16_t upper_critical,
                                         int16_t lower_critical)
{
    /* TODO: implement threshold register writes */
    (void)sensor_id;
    (void)upper_critical;
    (void)lower_critical;
    return RTOS_OK;
}
