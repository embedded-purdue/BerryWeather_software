#include "wind_sensor.h"
#include "esp_log.h"
#include "esp_err.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char *TAG = "WIND_SENSOR";

// Hardware configuration 
#define UART_PORT               UART_NUM_1
#define UART_BAUD               4800
#define TXD_PIN                 GPIO_NUM_17  // MAX485 DI
#define RXD_PIN                 GPIO_NUM_16  // MAX485 RO  
#define RTS_PIN                 GPIO_NUM_18  // MAX485 DE+RE
#define BUFFER_SIZE             256

// Modbus settings
#define DEVICE_ADDR             0x01
#define READ_FUNCTION           0x03
#define WIND_REGISTER           0x0000
#define RESPONSE_TIMEOUT        1000

static bool is_initialized = false;

// Modbus CRC calculation
static uint16_t calculate_crc16(const uint8_t *data, size_t length) {
    uint16_t crc = 0xFFFF;
    
    for (size_t i = 0; i < length; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) {
            if (crc & 0x0001) {
                crc = (crc >> 1) ^ 0xA001;
            } else {
                crc >>= 1;
            }
        }
    }
    return crc;
}

esp_err_t wind_sensor_init(void) {
    ESP_LOGI(TAG, "Initializing SN-3000-FSA-N01 Wind Speed Transmitter...");
    
    esp_err_t ret;
    
    // Configure UART for RS485 communication
    uart_config_t config = {
        .baud_rate = UART_BAUD,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    };
    
    // Setup UART driver and configuration
    ret = uart_driver_install(UART_PORT, BUFFER_SIZE, BUFFER_SIZE, 0, NULL, 0);
    ret |= uart_param_config(UART_PORT, &config);
    ret |= uart_set_pin(UART_PORT, TXD_PIN, RXD_PIN, RTS_PIN, UART_PIN_NO_CHANGE);
    ret |= uart_set_mode(UART_PORT, UART_MODE_RS485_HALF_DUPLEX);
    
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "UART setup failed: %s", esp_err_to_name(ret));
        uart_driver_delete(UART_PORT);
        return ret;
    }
    
    uart_flush(UART_PORT);
    
    is_initialized = true;
    ESP_LOGI(TAG, "SN-3000-FSA-N01 Wind sensor ready (Baud: %d, Addr: 0x%02X)", UART_BAUD, DEVICE_ADDR);
    
    return ESP_OK;
}

// Read wind speed register via Modbus
static esp_err_t read_wind_speed_register(uint16_t *value) {
    if (!is_initialized) return ESP_ERR_INVALID_STATE;
    
    // Build Modbus request: [addr][func][reg_h][reg_l][count_h][count_l][crc_l][crc_h]
    uint8_t request[8] = {DEVICE_ADDR, READ_FUNCTION, 0, 0, 0, 1};
    uint16_t crc = calculate_crc16(request, 6);
    request[6] = crc & 0xFF;
    request[7] = crc >> 8;
    
    // Send request and get response
    uart_flush_input(UART_PORT);
    uart_write_bytes(UART_PORT, request, 8);
    uart_wait_tx_done(UART_PORT, pdMS_TO_TICKS(100));
    
    uint8_t response[7];
    int len = uart_read_bytes(UART_PORT, response, 7, pdMS_TO_TICKS(RESPONSE_TIMEOUT));
    
    if (len <= 0) {
        ESP_LOGW(TAG, "No response received - check connections and power");
        return ESP_FAIL;
    }
    
    // Basic validation
    if (len != 7) {
        ESP_LOGW(TAG, "Invalid response length: expected 7, got %d", len);
        return ESP_FAIL;
    }
    
    if (response[0] != DEVICE_ADDR) {
        ESP_LOGW(TAG, "Invalid device address: expected 0x%02X, got 0x%02X", DEVICE_ADDR, response[0]);
        return ESP_FAIL;
    }
    
    if (response[1] != READ_FUNCTION) {
        ESP_LOGW(TAG, "Invalid function code: expected 0x%02X, got 0x%02X", READ_FUNCTION, response[1]);
        return ESP_FAIL;
    }
    
    // Verify CRC
    uint16_t rx_crc = response[5] | (response[6] << 8);
    uint16_t calc_crc = calculate_crc16(response, 5);
    if (rx_crc != calc_crc) {
        ESP_LOGW(TAG, "CRC mismatch: received 0x%04X, calculated 0x%04X", rx_crc, calc_crc);
        return ESP_FAIL;
    }
    
    *value = (response[3] << 8) | response[4];
    return ESP_OK;
}

int wind_sensor_get_speed_raw(void) {
    uint16_t value;
    return (read_wind_speed_register(&value) == ESP_OK) ? value : -1;
}

int wind_sensor_get_direction_raw(void) {
    return -1;  // Not supported
}

esp_err_t wind_sensor_read_speed(float *wind_speed) {
    if (!wind_speed) return ESP_ERR_INVALID_ARG;
    
    uint16_t raw;
    esp_err_t ret = read_wind_speed_register(&raw);
    if (ret != ESP_OK) return ret;
    
    *wind_speed = raw / 10.0f;  // Convert to m/s
    return ESP_OK;
}

esp_err_t wind_sensor_read_direction(float *wind_direction) {
    if (wind_direction) *wind_direction = 0.0f;  // Not supported
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t wind_sensor_read_all(wind_data_t *wind_data) {
    if (!wind_data) return ESP_ERR_INVALID_ARG;
    
    esp_err_t ret = wind_sensor_read_speed(&wind_data->wind_speed);
    wind_data->wind_direction = 0.0f;  // Not supported
    return ret;
}