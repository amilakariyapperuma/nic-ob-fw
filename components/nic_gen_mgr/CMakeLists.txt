idf_component_register(
    SRCS "src/nic_gen_mgr.c"
    INCLUDE_DIRS "include"
    REQUIRES common rtos_hal nic_chip_protocol nic_chip_hal
)
