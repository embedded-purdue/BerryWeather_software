# idf_component_register(SRCS "mqtt.c" "modular.c"
#                        PRIV_REQUIRES nvs_flash esp_netif esp_wifi esp_event log mqtt spi_flash esp_driver_gpio esp_driver_uart json
#                        INCLUDE_DIRS "")



# 1. Define a variable, "APP_TARGET", that defaults to "satellite"
#    You can override this on the command line.
set(APP_TARGET "satellite" CACHE STRING "Selects the app to build (satellite or middleman)")

# 2. Conditionally set the source file based on the variable
if(APP_TARGET STREQUAL "satellite")
    # If building for the satellite, use this file
    set(SRCS "satellite_main.c")
    message(STATUS "--- Building SATELLITE application ---")

elseif(APP_TARGET STREQUAL "middleman")
    # If building for the middleman, use this file
    set(SRCS "middleman_main.c")
    message(STATUS "--- Building MIDDLEMAN application ---")

else()
    # Error if the name is wrong
    message(FATAL_ERROR "Invalid APP_TARGET: ${APP_TARGET}. Choose 'satellite' or 'middleman'.")
endif()

# 3. Register the component with the selected source file
idf_component_register(SRCS "${SRCS}"
                      INCLUDE_DIRS "."
                      REQUIRES lora_comm 
                      PRIV_REQUIRES nvs_flash esp_netif esp_wifi esp_event log mqtt spi_flash esp_driver_gpio esp_driver_uart json)
