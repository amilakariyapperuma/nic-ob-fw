idf_component_register(
    SRCS "src/nic_flash_mgr.c"
    INCLUDE_DIRS "include"
    REQUIRES common rtos_hal spi_flash_cmd
)
