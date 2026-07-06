idf_component_register(
    SRCS "src/protocol_hdlr.c"
         "src/mctp.c"
         "src/pldm.c"
    INCLUDE_DIRS "include"
    PRIV_INCLUDE_DIRS "src"
    REQUIRES common rtos_hal bmc_hal
)
