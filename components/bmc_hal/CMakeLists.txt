idf_component_register(
    SRCS "src/bmc_hal.c"
    INCLUDE_DIRS "include"
    REQUIRES common rtos_hal
)
