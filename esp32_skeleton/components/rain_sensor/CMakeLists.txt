idf_component_register(
    SRCS "rain_sensor.c"
    INCLUDE_DIRS "include"
    REQUIRES driver esp_adc
    PRIV_REQUIRES spi_flash
)