idf_component_register(SRCS "servo_test.c"
                       PRIV_REQUIRES rain_sensor
                       REQUIRES driver
                       INCLUDE_DIRS ".")
