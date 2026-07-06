idf_component_register(
    SRCS "src/sensor_protocol.c"
    INCLUDE_DIRS "include"
    REQUIRES common rtos_hal sensor_hal
)
