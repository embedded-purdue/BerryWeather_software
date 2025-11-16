idf_component_register(SRCS "main.c" "as7331.c" "ds18b20.c" "rain_sensor.c" "soil_moisture.c"
                       PRIV_REQUIRES spi_flash bme688
                       REQUIRES driver
                       INCLUDE_DIRS ".")
