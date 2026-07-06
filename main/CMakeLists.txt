idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "."
    REQUIRES
        common
        rtos_hal
        bmc_hal
        sensor_hal
        nic_chip_hal
        isr_handlers
        bmc_mgr
        sensor_mgr
        nic_flash_mgr
)
