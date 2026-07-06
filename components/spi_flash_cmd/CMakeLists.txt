idf_component_register(
    SRCS "src/spi_flash_cmd.c"
    INCLUDE_DIRS "include"
    REQUIRES common rtos_hal nic_chip_hal
)
