idf_component_register(
    SRCS "src/sensor_mgr.c"
    INCLUDE_DIRS "include"
    REQUIRES common rtos_hal sensor_protocol
)
