idf_component_register(SRCS "lora_comm.c"
                      INCLUDE_DIRS "."
                      REQUIRES esp_driver_uart esp_driver_gpio)