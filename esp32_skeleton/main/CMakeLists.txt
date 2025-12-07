# 1. Define a variable, "APP_TARGET", that defaults to "satellite"
#    You can override this on the command line.
set(APP_TARGET "satellite" CACHE STRING "Selects the app to build (satellite, middleman, or servo)")

# 2. Conditionally set the source file based on the variable
if(APP_TARGET STREQUAL "satellite")
    # If building for the satellite, use this file and include sensor drivers
    set(SRCS "satellite_main.c")
    message(STATUS "--- Building SATELLITE application with sensors ---")

elseif(APP_TARGET STREQUAL "middleman")
    # If building for the middleman, use this file
    set(SRCS "middleman_main.c")
    message(STATUS "--- Building MIDDLEMAN application ---")

elseif(APP_TARGET STREQUAL "servo")
    # If building for servo testing, use this file
    set(SRCS "servo_test.c")
    message(STATUS "--- Building SERVO TEST application ---")

else()
    # Error if the name is wrong
    message(FATAL_ERROR "Invalid APP_TARGET: ${APP_TARGET}. Choose 'satellite', 'middleman', or 'servo'.")
endif()

# 3. Register the component with the selected source file
if(APP_TARGET STREQUAL "servo")
    # Minimal dependencies for servo testing
    idf_component_register(SRCS "${SRCS}"
                          PRIV_REQUIRES rain_sensor
                          REQUIRES driver
                          INCLUDE_DIRS ".")
else()
    # Full dependencies for satellite/middleman
    idf_component_register(SRCS "${SRCS}"
                          INCLUDE_DIRS "."
                          REQUIRES lora_comm driver esp_adc
                          PRIV_REQUIRES spi_flash nvs_flash esp_netif esp_wifi esp_event log mqtt esp_driver_gpio esp_driver_uart json bme688 as7331 DS18B20 soil_moisture rain_sensor)
endif()
