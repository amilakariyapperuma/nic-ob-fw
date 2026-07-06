idf_component_register(
    SRCS "src/isr_handlers.c"
    INCLUDE_DIRS "include"
    REQUIRES common rtos_hal bmc_hal nic_chip_hal
)
