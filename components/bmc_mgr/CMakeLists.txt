idf_component_register(
    SRCS "src/bmc_mgr.c"
    INCLUDE_DIRS "include"
    REQUIRES common rtos_hal protocol_hdlr nic_gen_mgr sensor_mgr
)
