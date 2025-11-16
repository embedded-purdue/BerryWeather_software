idf_component_register(SRCS "main.c" "ds18b20.c" "soil_moisture.c"
                       PRIV_REQUIRES spi_flash bme688 as7331 DS18B20 soil_moisture rain_sensor
                       REQUIRES driver esp_adc
                       INCLUDE_DIRS ".")
