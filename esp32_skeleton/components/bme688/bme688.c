// bme688.c - BME688 sensor driver source (stub)
#include "bme68x.h"
#include <stdio.h>
#include <string.h>
#include "driver/i2c.h"

#define BME688_ADDR 0x00 // Replace with your BME688 sensor's I2C address
#define RESET_REG_BME 0xE0
#define RESET_VALUE_BME 0xB6

void bme688_init(struct bme68x_data *data, struct bme68x_dev *bme) {
    uint32_t del_period;
    uint32_t time_ms = 0;
    uint8_t n_fields;
    uint16_t sample_count = 1;

    // 1. (Optional) Initialize I2C bus here if not done elsewhere
    
    // 2. Reset the sensor
    
    uint8_t reset_cmd[2] = {RESET_REG_BME, RESET_VALUE_BME}; 
    i2c_master_write_to_device(I2C_NUM_0, BME688_ADDR, reset_cmd, 2, 1000 / portTICK_PERIOD_MS);
    bme68x_init(bme);

    // 3. Configure oversampling, filter, etc. (see BME688 datasheet for details)
    // Define values of oversampling for each param 
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

