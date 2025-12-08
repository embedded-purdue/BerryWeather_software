#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_random.h"
#include "esp_log.h"
#include "esp_sleep.h" // For deep sleep
#include "lora_comm.h" 
#include "bme688.h"
#include "soil_moisture.h"
#include "ds18b20.h"
#include "rain_sensor.h"
#include "as7331.h"

// --- Satellite Specific Configuration ---
#define SAT_ADDR 10 // This satellite's address
#define MM_ADDR  1  // The MiddleMan's address

static const char *TAG = "satellite";
//GLOBAL STRUCTS:
struct bme68x_data data;
struct bme68x_dev bme;
AS7331 sensor;
AS7331_Light light;

#define TEST_I2C_PORT I2C_NUM_0
#define I2C_MASTER_SCL_IO 22
#define I2C_MASTER_SDA_IO 21

i2c_master_bus_handle_t main_bus_handle;

i2c_master_bus_config_t i2c_mst_config = {
    .clk_source = I2C_CLK_SRC_DEFAULT,
    .i2c_port = TEST_I2C_PORT,
    .scl_io_num = I2C_MASTER_SCL_IO,
    .sda_io_num = I2C_MASTER_SDA_IO,
    .glitch_ignore_cnt = 7,
    .flags.enable_internal_pullup = true,
};

void i2c_init_shared_bus(void)
{
    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_mst_config, &main_bus_handle));
}

/**
 * @brief Main task for the satellite.
 *
 * This task runs in a loop, reads sensor data, sends it via LoRa,
 * and then sleeps for a configured interval.
 */
void periodic_sensor_task(void *arg)
{
    // Define the send interval (e.g., 30 minutes)
    // For testing, you can set this to a shorter duration, like 30 seconds:
    // const int SEND_INTERVAL_SECONDS = 30;
    const int SEND_INTERVAL_MINUTES = 1;
    const uint64_t SEND_INTERVAL_US = (uint64_t)SEND_INTERVAL_MINUTES * 60 * 1000 * 1000;
    
    printf("Starting periodic sensor task. Sending every %d minutes.\n", SEND_INTERVAL_MINUTES);

    // 1. Read data from sensors
    float temp, hum, pres;
    float soil_temp;
    float rain_level;
    float soil_moisture;
    // float temperature = get_temp_data();
    bme688_read_temperature(&temp, &data, &bme);
    bme688_read_humidity(&hum, &data, &bme);
    bme688_read_pressure(&pres, &data, &bme);
    ESP_LOGI(TAG, "BME688 -> Temp: %.2f °C, Hum: %.2f %%, Pres: %.2f hPa", temp, hum, pres);

    ds18b20_read_temperature(&soil_temp);
    ESP_LOGI(TAG, "DS18B20 -> Soil Temp: %.2f °C", soil_temp);

    rain_level = rain_sensor_get_normalized();
    ESP_LOGI(TAG, "Rain Sensor -> Level: %.2f", rain_level);

    soil_moisture_read(&soil_moisture);
    ESP_LOGI(TAG, "Soil Moisture -> Value: %.2f", soil_moisture);

    as7331_read_light(&sensor, &light);
    ESP_LOGI(TAG, "AS7331 Scaled -> UVA: %.2f, UVB: %.2f, UVC: %.2f uW/cm²", light.uva, light.uvb, light.uvc);

    float uv_index = light.uva;

    // 2. Create the JSON payload
    char json_payload[256];

        // snprintf(json_payload, sizeof(json_payload),
        // "{"
        // "\"t\":%.2f,"      // air temp (°C)
        // "\"h\":%.2f,"      // air humidity (%%)
        // "\"p\":%.2f,"      // air pressure (hPa)
        // "\"st\":%.2f,"     // soil temp (°C)
        // "\"sm\":%.2f,"     // soil moisture (normalized/ADC)
        // "\"rain\":%.2f"   // rain level (normalized)
        // // "\"uv\":%.2f,"     // combined/derived UV index
        // // "\"uva\":%.2f,"
        // // "\"uvb\":%.2f,"
        // // "\"uvc\":%.2f"
        // "}",
        // temp, hum, pres/1000,
        // soil_temp, soil_moisture, rain_level);

        snprintf(json_payload, sizeof(json_payload),
            "{"
            "\"t\":%.2f,"      // air temp (°C)
            "\"h\":%.2f,"      // air humidity (%%)
            "\"p\":%.2f,"      // air pressure (hPa)
            "\"st\":%.2f,"     // soil temp (°C)
            "\"sm\":%.2f,"     // soil moisture (normalized/ADC)
            "\"rain\":%.2f,"   // rain level (normalized)
            "\"uv\":%.2f,"     // combined/derived UV index
            "\"uva\":%.2f,"
            "\"uvb\":%.2f,"
            "\"uvc\":%.2f"
            "}",
            temp, hum, pres/100,
            soil_temp, soil_moisture, rain_level,
            uv_index,
            light.uva, light.uvb, light.uvc);

    printf("----------------------------------\n");
    printf("Reading sensors and sending data...\n");
    printf("Payload: %s\n", json_payload);
    
    lora_send_message(MM_ADDR, json_payload);
    // uart_flush_input(LORA_UART_PORT);
    printf("Waiting for data ACK from MiddleMan...\n");
    
    char rx_buf[128];
    bool ack_received = false;
    
    for (int attempt = 0; attempt < 25; attempt++) {
        if (lora_wait_for_message(rx_buf, sizeof(rx_buf), 6000)) {
            if (strstr(rx_buf, "MM_ACK_DATA")) {
                printf("ACK received from MiddleMan!\n");
                ack_received = true;
                break;
            }
        }
        printf("No ACK, retrying send (%d/3)...\n", attempt + 1);
        lora_send_message(MM_ADDR, json_payload);
    }
    
    // 4. Sleep if ACK or after max retries
    if (!ack_received)
        printf("No ACK from MiddleMan, going to sleep anyway.\n");
    
    uart_wait_tx_done(LORA_UART_PORT, pdMS_TO_TICKS(1000));
    vTaskDelay(pdMS_TO_TICKS(500));
    
    esp_sleep_enable_timer_wakeup(SEND_INTERVAL_US);
    esp_deep_sleep_start();

    /*
    // --- OR ---
    // If you don't want deep sleep, you can use vTaskDelay.
    // This keeps the ESP32 powered on but pauses the task.
    // It uses much more power than deep sleep.
    
    printf("Data sent. Delaying task for %d minutes.\n", SEND_INTERVAL_MINUTES);
    printf("----------------------------------\n");
    vTaskDelay(pdMS_TO_TICKS(SEND_INTERVAL_MINUTES * 60 * 1000));
    */
}

void app_main(void)
{
    printf("--- Satellite Device Booting ---\n");

    // 1. Initialize LoRa UART and Reset Module
    lora_uart_config();
    lora_reset();

    vTaskDelay(pdMS_TO_TICKS(500));
    // uart_flush_input(LORA_UART_PORT);

    // 2. Perform common LoRa setup
    printf("Setting up LoRa module...\n");
    lora_common_setup(SAT_ADDR); 

    //INITIALIZE SENSORS:

    //INIT BUS:
    i2c_init_shared_bus();

    bme688_init(&data, &bme, main_bus_handle);
    ds18b20_init();
    rain_sensor_init();
    soil_moisture_init();
    as7331_init(&sensor, main_bus_handle);


    printf("Performing LoRa boot handshake...\n");
    if (lora_boot_handshake(false, MM_ADDR)) {
        printf("Handshake successful! Starting periodic task.\n");
    } else {
        printf("Handshake failed. Continuing anyway.\n");
    }
    xTaskCreate(periodic_sensor_task, "periodic_sensor_task", 4096, NULL, 5, NULL);

    // If using deep sleep, the code in periodic_sensor_task will run,
    // and then the device will sleep and reboot.
}