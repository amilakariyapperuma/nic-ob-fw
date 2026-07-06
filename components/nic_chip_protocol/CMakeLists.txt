idf_component_register(
    SRCS "src/nic_chip_protocol.c"
    INCLUDE_DIRS "include"
    REQUIRES common rtos_hal nic_chip_hal
)
