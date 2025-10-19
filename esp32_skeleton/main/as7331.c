// as7331.c - AS7331 sensor driver source (stub)
#include "as7331.h"
#include <stdio.h>
#include "driver/i2c.h"

#define AS7331_ADDR 0x74 // page 42: AS7331 sensor's I2C address- Can run 4 AS7331s on the same I2C bus with A1 and A0 pins defining two lowest-order bits (high or low) - Assuming both A1 and A0 tied to GND
#define OPERATIONAL_STATE_REG_AS7331 0x00 // page 48: OSR is at 0x00
#define RESET_VALUE_AS7331 0x08 // page 49: Set SW_RES bit (bit 3) to 1
#define CONFIG_VALUE_AS7331 0x02 // page 49: 010 sets operational state to configuration
#define MEASUREMENT_VALUE_AS7331 0x03 // page 49: 011 sets operational state to measurement
#define CREG3_AS7331 0x08 // page 48: config register for clock frequency
#define CREG3_CCLK_VALUE 0x00 // page 56: 00 sets internal clock frequency to 1.024 MHz
#define CREG1_AS7331 0x06 // page 48: config register for gain and time
#define CREG1_TIME_GAIN_VALUE_AS7331 0xB3 // page 51: set gain to 1x and time to 8ms
#define I2C_MASTER_SCL 22 // ESP32's SCL pin
#define I2C_MASTER_SDA 21 // ESP32's SDA pin
#define UV_MEASUREMENT_START_REG 0x02 // page 59: MRES1 register - ONLY IN MEASUREMENT MODE
#define I2C_PORT_DEFAULT I2C_NUM_0 // I2C port

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
        .master.clk_speed = 400000, // 400 kHz
        .clk_flags = 0,
        };
        
    // 2. Reset the sensor
    uint8_t reset_cmd[2] = {OPERATIONAL_STATE_REG_AS7331, RESET_VALUE_AS7331};
    i2c_master_write_to_device(I2C_NUM_0, AS7331_ADDR, reset_cmd, 2, 1000 / portTICK_PERIOD_MS);

    // 3. Configure integration time, gain, etc. (see AS7331 datasheet for details)
    // Example: Write to a configuration register (replace reg and value)
    // uint8_t config_cmd[2] = {0xXX, 0xXX}; // 0xXX = config reg, 0xXX = config value
    // i2c_master_write_to_device(I2C_NUM_0, AS7331_ADDR, config_cmd, 2, 1000 / portTICK_PERIOD_MS);
    uint8_t OSR_set_mode_config_cmd[2] = {OPERATIONAL_STATE_REG_AS7331, CONFIG_VALUE_AS7331}; // set OSR to configuration state
    i2c_master_write_to_device(I2C_NUM_0, AS7331_ADDR, OSR_set_mode_config_cmd, 2, 1000 / portTICK_PERIOD_MS);
    uint8_t config_cmd[2] = {CREG1_AS7331, CREG1_TIME_GAIN_VALUE_AS7331}; // set time to 8ms and gain to 1x
    i2c_master_write_to_device(I2C_NUM_0, AS7331_ADDR, config_cmd, 2, 1000 / portTICK_PERIOD_MS);

    // Add more configuration as needed
    uint8_t OSR_set_mode_measurement_cmd[2] = {OPERATIONAL_STATE_REG_AS7331, MEASUREMENT_VALUE_AS7331}; // set OSR to measurement state
    i2c_master_write_to_device(I2C_NUM_0, AS7331_ADDR, OSR_set_mode_measurement_cmd, 2, 1000 / portTICK_PERIOD_MS);

    printf("AS7331 initialized (real hardware)!\n");
    return ESP_OK;
}

// INCLUDE THE REST OF THE FUNCTIONS BELOW
esp_err_t AS7331_read_registers(AS7331 *dev, uint8_t reg_addr, uint8_t *data, size_t length) {

    if (!dev || !data || !length) return ESP_ERR_INVALID_ARG; // stops function if called with invalid arguments

    return i2c_master_write_read_device(dev->port, dev->addr, &reg_addr, 1, data, length, 1000/portTICK_PERIOD_MS);
}

esp_err_t as7331_read_light(AS7331 *dev, AS7331_Light *light) {

    uint8_t buf[6]; // 6 byte array for storing UV readings - each reading is 2 bytes long
    esp_err_t err = AS7331_read_registers(dev, UV_MEASUREMENT_START_REG, buf, sizeof(buf)); // read 6 bytes starting from start register

    if (err != ESP_OK) return err; // failed i2c transaction

    uint16_t raw_uva = ((uint16_t) buf[1] << 8) | buf[0]; // bitwise or to create 16 bit UVA reading (page 59)
    uint16_t raw_uvb = ((uint16_t) buf[3] << 8) | buf[2]; // bitwise or to create 16 bit UVA reading (page 59)
    uint16_t raw_uvc = ((uint16_t) buf[5] << 8) | buf[4]; // bitwise or to create 16 bit UVA reading (page 59)

    dev->light_reading_raw[0] = raw_uva;
    dev->light_reading_raw[1] = raw_uvb;
    dev->light_reading_raw[2] = raw_uvc;

    const float SCALE_UVA = 0.012f; // placeholder conversion factor k - not yet measured with known irradiance E (k = E / counts)
    const float SCALE_UVB = 0.014f; // placeholder conversion factor k - not yet measured with known irradiance E (k = E / counts)
    const float SCALE_UVC = 0.016f; // placeholder conversion factor k - not yet measured with known irradiance E (k = E / counts)

    light->uva = raw_uva * SCALE_UVA;
    light->uvb = raw_uvb * SCALE_UVB;
    light->uvc = raw_uvc * SCALE_UVC;

    return ESP_OK; // successful transaction
}