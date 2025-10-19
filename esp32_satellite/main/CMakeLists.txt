idf_component_register(SRCS "mqtt.c" "modular.c" "main.c"
                       PRIV_REQUIRES nvs_flash esp_netif log mqtt spi_flash esp_driver_gpio esp_driver_uart json
                       INCLUDE_DIRS "")
