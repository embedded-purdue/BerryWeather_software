# idf_component_register(SRCS "rain_sensor_servo.c" "rain_sensor.c" "as7331.c" "main.c"
#                        PRIV_REQUIRES spi_flash
#                        INCLUDE_DIRS "")

idf_component_register(
    SRCS "rain_sensor.c"
    INCLUDE_DIRS "include"
    REQUIRES driver esp_adc
    PRIV_REQUIRES spi_flash
)