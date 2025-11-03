// bme688.c - BME688 sensor driver source (stub)
#include "bme68x.h"
#include <stdio.h>
#include <string.h>
#include "driver/i2c_master.h"
#include "esp_err.h"

#define BME688_ADDR 0x77 // Replace with your BME688 sensor's I2C address
#define RESET_REG_BME 0xE0
#define RESET_VALUE_BME 0xB6
#define TEST_I2C_PORT I2C_NUM_0
#define I2C_MASTER_SCL_IO 22
#define I2C_MASTER_SDA_IO 21

i2c_master_bus_config_t i2c_mst_config = {
    .clk_source = I2C_CLK_SRC_DEFAULT,
    .i2c_port = TEST_I2C_PORT,
    .scl_io_num = I2C_MASTER_SCL_IO,
    .sda_io_num = I2C_MASTER_SDA_IO,
    .glitch_ignore_cnt = 7,
    .flags.enable_internal_pullup = true,
};

i2c_master_bus_handle_t bus_handle;

i2c_device_config_t dev_cfg = {
    .dev_addr_length = I2C_ADDR_BIT_LEN_7,
    .device_address = 0x77,  
    .scl_speed_hz = 100000,
};

i2c_master_dev_handle_t dev_handle;

void bme688_init(struct bme68x_data *data, struct bme68x_dev *bme) {
    uint32_t del_period;
    uint32_t time_ms = 0;
    uint8_t n_fields;
    uint16_t sample_count = 1;
    

    // 1. (Optional) Initialize I2C bus here if not done elsewhere
    
    // 2. Reset the sensor
    
    uint8_t reset_cmd[2] = {RESET_REG_BME, RESET_VALUE_BME}; 
    // Initialize I2C bus and add device
    esp_err_t err = i2c_new_master_bus(&i2c_mst_config, &bus_handle);
    if (err != ESP_OK) {
        printf("Failed to create I2C master bus: %d\n", err);
        return;
    }

    err = i2c_master_bus_add_device(bus_handle, &dev_cfg, &dev_handle);
    if (err != ESP_OK) {
        printf("Failed to add I2C device: %d\n", err);
        return;
    }

    // Try both possible I2C addresses
    uint8_t who_am_i[1] = {0xD0}; // BME688 chip ID register
    uint8_t chip_id[1];
    
    err = i2c_master_transmit_receive(dev_handle, who_am_i, 1, chip_id, 1, 1000);
    if (err != ESP_OK) {
        printf("Failed to read chip ID: %d. Trying alternate address...\n", err);
        // Try alternate address
        dev_cfg.device_address = 0x76;
        i2c_master_bus_rm_device(dev_handle);
        err = i2c_master_bus_add_device(bus_handle, &dev_cfg, &dev_handle);
        if (err != ESP_OK) {
            printf("Failed to add I2C device with alternate address: %d\n", err);
            return;
        }
    }

    // Transmit reset command
    err = i2c_master_transmit(dev_handle, reset_cmd, 2, 1000);
    if (err != ESP_OK) {
        printf("Failed to send reset command: %d\n", err);
        return;
    }
    //i2c_master_bus_rm_device(I2C_NUM_0, BME688_ADDR, reset_cmd, 2, 1000 / );
    bme68x_init(bme);

    struct bme68x_conf config;
    config.filter = BME68X_FILTER_OFF;
    config.odr = BME68X_ODR_NONE;
    config.os_hum = BME68X_OS_16X;
    config.os_pres = BME68X_OS_1X;
    config.os_temp = BME68X_OS_2X;
    
    bme68x_set_conf(&config, bme);

    //setup heater(pressure)
    struct bme68x_heatr_conf heatr_conf;
    heatr_conf.enable = BME68X_ENABLE;
    heatr_conf.heatr_temp = 300;
    heatr_conf.heatr_dur = 100;
    bme68x_set_heatr_conf(BME68X_FORCED_MODE, &heatr_conf, bme);
    bme68x_set_op_mode(BME68X_FORCED_MODE, bme);
    
    // Example: Write to a configuration register (replace reg and value)
    // uint8_t config_cmd[2] = {0x74, 0xB6}; // 0x74 = config reg, 0xB6 = config value
    // i2c_master_write_to_device(I2C_NUM_0, BME688_ADDR, config_cmd, 2, 1000 / portTICK_PERIOD_MS);

    // Add more configuration as needed

    printf("BME688 initialized (real hardware)!\n");
}



int8_t bme688_read_temperature(float *temp, struct bme68x_data *data, struct bme68x_dev *bme)
{
    if (!temp || !data || !bme) return BME68X_E_NULL_PTR;

    uint8_t n_data = 0;
    *temp = 0.0f;
    memset(data, 0, sizeof(*data)); // okay for forced mode

    int8_t rslt = bme68x_get_data(BME68X_FORCED_MODE, data, &n_data, bme);
    if (rslt < 0) return rslt;           // hard error from bus/driver
    if (n_data == 0) return BME68X_W_NO_NEW_DATA;

    *temp = data->temperature;
    return BME68X_OK;
}

int8_t bme688_read_pressure(float *pres, struct bme68x_data *data, struct bme68x_dev *bme)
{
    if (!pres || !data || !bme) return BME68X_E_NULL_PTR;

    uint8_t n_data = 0;
    *pres = 0.0f;
    memset(data, 0, sizeof(*data));

    int8_t rslt = bme68x_get_data(BME68X_FORCED_MODE, data, &n_data, bme);
    if (rslt < 0) return rslt;  
    if (n_data == 0) return BME68X_W_NO_NEW_DATA;

    *pres = data->pressure;
    return BME68X_OK;
}

int8_t bme688_read_humidity(float *hum, struct bme68x_data *data, struct bme68x_dev *bme)
{
    if (!hum || !data || !bme) return BME68X_E_NULL_PTR;

    uint8_t n_data = 0;
    *hum = 0.0f;
    memset(data, 0, sizeof(*data)); 

    int8_t rslt = bme68x_get_data(BME68X_FORCED_MODE, data, &n_data, bme);
    if (rslt < 0) return rslt;  
    if (n_data == 0) return BME68X_W_NO_NEW_DATA;

    *hum = data->humidity;
    return BME68X_OK;
}

