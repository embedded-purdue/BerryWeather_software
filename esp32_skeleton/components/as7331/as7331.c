// as7331.c - AS7331 sensor driver source (stub)
#include "as7331.h"
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define AS7331_ADDR 0x74 // page 42: AS7331 sensor's I2C address- Can run 4 AS7331s on the same I2C bus with A1 and A0 pins defining two lowest-order bits (high or low) - Assuming both A1 and A0 tied to GND
#define OPERATIONAL_STATE_REG_AS7331 0x00 // page 48: OSR is at 0x00
#define RESET_VALUE_AS7331 0x08 // page 49: Set SW_RES bit (bit 3) to 1
#define CONFIG_VALUE_AS7331 0x02 // page 49: 010 sets operational state to configuration
#define MEASUREMENT_VALUE_AS7331 0x03 // page 49: 011 sets operational state to measurement
#define CREG3_AS7331 0x08 // page 48: config register for clock frequency
#define CREG3_CCLK_VALUE 0x00 // page 56: 00 sets internal clock frequency to 1.024 MHz
#define CREG1_AS7331 0x06 // page 48: config register for gain and time
#define CREG1_TIME_GAIN_VALUE_AS7331 0xA7 // page 51: set gain to 2x and time to 128ms
#define CREG2_AS7331 0x07 // Configuration Register 2
#define CREG2_VALUE_AS7331 0x00 // Default configuration
#define OUTCONV_REG_AS7331 0x05 // OUTCONV register
#define I2C_MASTER_SCL 22 // ESP32's SCL pin
#define I2C_MASTER_SDA 21 // ESP32's SDA pin
#define UV_MEASUREMENT_START_REG 0x02 // page 59: MRES1 register - ONLY IN MEASUREMENT MODE
#define UV_MEASUREMENT_TRIGGER_VALUE 0x83 // Start bit for measurement
#define UV_MEASUREMENT_STATUS_REG 0x0A   // Status register for measurement
#define I2C_PORT_DEFAULT I2C_NUM_0 // I2C port
#define I2C_MASTER_TX_BUF_DISABLE 0
#define I2C_MASTER_RX_BUF_DISABLE 0
#define STATUS_REG 0x00

esp_err_t as7331_init(AS7331 *dev) {

    if (!dev) return ESP_ERR_INVALID_ARG;

    dev->port = I2C_PORT_DEFAULT;
    dev->addr = AS7331_ADDR;

    // 1. (Optional) Initialize I2C bus here if not done elsewhere
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA, 
        .scl_io_num = I2C_MASTER_SCL, 
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 100000, // Slow down to 100 kHz for better reliability
        .clk_flags = 0,
        };
    
    i2c_param_config(I2C_NUM_0, &conf);
    i2c_driver_install(I2C_NUM_0, conf.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);

    // Read status register to verify I2C communication
    uint8_t test_status;
    ESP_ERROR_CHECK(i2c_master_write_read_device(I2C_NUM_0, AS7331_ADDR, (uint8_t[]){OPERATIONAL_STATE_REG_AS7331}, 1, &test_status, 1, 1000 / portTICK_PERIOD_MS));
    printf("Initial I2C test successful, can communicate with sensor at address 0x%02X\n", AS7331_ADDR);
    
    // Send reset command
    uint8_t reset_cmd[2] = {OPERATIONAL_STATE_REG_AS7331, RESET_VALUE_AS7331};
    ESP_ERROR_CHECK(i2c_master_write_to_device(I2C_NUM_0, AS7331_ADDR, reset_cmd, 2, 1000 / portTICK_PERIOD_MS));
    vTaskDelay(pdMS_TO_TICKS(100));  // Wait for reset to complete

    // Enter CONFIG mode afer reset
    uint8_t config_cmd[2] = {OPERATIONAL_STATE_REG_AS7331, CONFIG_VALUE_AS7331};
    ESP_ERROR_CHECK(i2c_master_write_to_device(I2C_NUM_0, AS7331_ADDR, config_cmd, 2, 1000/portTICK_PERIOD_MS));
    vTaskDelay(pdMS_TO_TICKS(20));

    // Configure measurement parameters
    uint8_t meas_config[2] = {CREG1_AS7331, CREG1_TIME_GAIN_VALUE_AS7331};
    ESP_ERROR_CHECK(i2c_master_write_to_device(dev->port, dev->addr, meas_config, 2, 1000 / portTICK_PERIOD_MS));
    vTaskDelay(pdMS_TO_TICKS(20));
    
    // Configure CREG2 settings (default)
    uint8_t creg2_cmd[2] = {CREG2_AS7331, CREG2_VALUE_AS7331};
    ESP_ERROR_CHECK(i2c_master_write_to_device(I2C_NUM_0, AS7331_ADDR, creg2_cmd, 2, 1000 / portTICK_PERIOD_MS));
    vTaskDelay(pdMS_TO_TICKS(20));
    
    // Configure clock frequency
    uint8_t clock_cmd[2] = {CREG3_AS7331, CREG3_CCLK_VALUE};
    ESP_ERROR_CHECK(i2c_master_write_to_device(I2C_NUM_0, AS7331_ADDR, clock_cmd, 2, 1000 / portTICK_PERIOD_MS));
    vTaskDelay(pdMS_TO_TICKS(20));
    
    // 3. Switch to MEASUREMENT mode
    uint8_t meas_cmd[2] = {OPERATIONAL_STATE_REG_AS7331, MEASUREMENT_VALUE_AS7331};
    ESP_ERROR_CHECK(i2c_master_write_to_device(dev->port, dev->addr, meas_cmd, 2, 1000 / portTICK_PERIOD_MS));
    vTaskDelay(pdMS_TO_TICKS(10));

    printf("AS7331 initialized (real hardware)!\n");
    return ESP_OK;
}

// INCLUDE THE REST OF THE FUNCTIONS BELOW
esp_err_t AS7331_read_registers(AS7331 *dev, uint8_t reg_addr, uint8_t *data, size_t length) {

    if (!dev || !data || !length) return ESP_ERR_INVALID_ARG; // stops function if called with invalid arguments

    return i2c_master_write_read_device(dev->port, dev->addr, &reg_addr, 1, data, length, 1000/portTICK_PERIOD_MS);
}

esp_err_t as7331_read_light(AS7331 *dev, AS7331_Light *light) {
    if (!dev || !light) return ESP_ERR_INVALID_ARG;

    // Trigger single measurement
    uint8_t trig_cmd[2] = {OPERATIONAL_STATE_REG_AS7331, UV_MEASUREMENT_TRIGGER_VALUE };
    ESP_ERROR_CHECK(i2c_master_write_to_device(dev->port, dev->addr, trig_cmd, 2, 1000/portTICK_PERIOD_MS));
    vTaskDelay(pdMS_TO_TICKS(150));

    // Read measurement registers
    uint8_t buf[6];
    ESP_ERROR_CHECK(AS7331_read_registers(dev, UV_MEASUREMENT_START_REG, buf, sizeof(buf)));

    uint16_t raw_uva = ((uint16_t)buf[1] << 8) | buf[0];
    uint16_t raw_uvb = ((uint16_t)buf[3] << 8) | buf[2];
    uint16_t raw_uvc = ((uint16_t)buf[5] << 8) | buf[4];

    // Store raw values
    dev->light_reading_raw[0] = raw_uva;
    dev->light_reading_raw[1] = raw_uvb;
    dev->light_reading_raw[2] = raw_uvc;

    // Get integration time in seconds
    uint8_t outconv_buf[2];
    ESP_ERROR_CHECK(AS7331_read_registers(dev, OUTCONV_REG_AS7331, outconv_buf, sizeof(outconv_buf)));
    uint16_t outconv = ((uint16_t)outconv_buf[1] << 8) | outconv_buf[0];
    float t_int = outconv / 1.024e6f; // clock frequency 1.024 MHz
    
    // Get responsivity values for each UV channel
    float t_ref = 0.064f; // reference conversion time (given on datasheet page 12)
    float gain_factor = 2.0f; // gain = 2x (datasheet values reflect gain = 1x and integration time = 64ms)
    float time_factor = t_int / t_ref;
    float respA = 0.205f * gain_factor * time_factor; // datasheet page 12
    float respB = 0.157f * gain_factor * time_factor; // datasheet page 12
    float respC = 0.326f * gain_factor * time_factor; // datasheet page 12
    
    // Convert to physical values (µW/cm²)
    light->uva = (float)raw_uva / respA;
    light->uvb = (float)raw_uvb / respB;
    light->uvc = (float)raw_uvc / respC;

    return ESP_OK; // successful transaction
}