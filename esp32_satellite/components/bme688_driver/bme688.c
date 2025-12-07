// bme688.c - BME688 sensor driver source (stub)
#include "bme68x.h"
#include <stdio.h>
#include <string.h>
#include "driver/i2c.h"

#define BME688_ADDR 0x00 // Replace with your BME688 sensor's I2C address
#define RESET_REG_BME 0xE0
#define RESET_VALUE_BME 0xB6

void bme688_init(void) {
    struct bme68x_data data;
    uint32_t del_period;
    uint32_t time_ms = 0;
    uint8_t n_fields;
    uint16_t sample_count = 1;

    // 1. (Optional) Initialize I2C bus here if not done elsewhere
    
    // 2. Reset the sensor
    struct bme68x_dev bme;
    uint8_t reset_cmd[2] = {RESET_REG_BME, RESET_VALUE_BME}; 
    i2c_master_write_to_device(I2C_NUM_0, BME688_ADDR, reset_cmd, 2, 1000 / portTICK_PERIOD_MS);
    bme68x_init(&bme);

    // 3. Configure oversampling, filter, etc. (see BME688 datasheet for details)
    // Define values of oversampling for each param 
    struct bme68x_conf config;
    config.filter = BME68X_FILTER_OFF;
    config.odr = BME68X_ODR_NONE;
    config.os_hum = BME68X_OS_16X;
    config.os_pres = BME68X_OS_1X;
    config.os_temp = BME68X_OS_2X;
    
    bme68x_set_conf(&config, &bme);

    //setup heater(pressure)
    struct bme68x_heatr_conf heatr_conf;
    heatr_conf.enable = BME68X_ENABLE;
    heatr_conf.heatr_temp = 300;
    heatr_conf.heatr_dur = 100;
    bme68x_set_heatr_conf(BME68X_FORCED_MODE, &heatr_conf, &bme);
    bme68x_set_op_mode(BME68X_FORCED_MODE, &bme);
    
    // Example: Write to a configuration register (replace reg and value)
    // uint8_t config_cmd[2] = {0x74, 0xB6}; // 0x74 = config reg, 0xB6 = config value
    // i2c_master_write_to_device(I2C_NUM_0, BME688_ADDR, config_cmd, 2, 1000 / portTICK_PERIOD_MS);

    // Add more configuration as needed

    printf("BME688 initialized (real hardware)!\n");
}

int8_C bme688_read_temperature(float *temp, struct bme68x_data *data, struct bme68x_dev *bme){
    /*
    I see that in main.c this function is used as a void return type, so should we get rid of the 
    status codes that we are returning? Or should we change main.c to be able to utilize these codes?
    For now I am leaving the status codes in.
    */ 

    // makes sure pointers we passed are good
    if (!temp || !data || !bme) return BME68X_E_NULL_PTR;

    /*
    Maybe get rid of this error checking in the two lines below since it is done in the init?
    Do we need to check every time we read a value? I think that might reduce performance potentially? idrk
    */ 

    // makes sure operating mode is set properly (again i see this is done in init, do we need to do it here as well?)
    int8_t rslt = bme68x_set_op_mode(BME68X_FORCED_MODE, bme);
    if (rslt != BME68X_OK) return rslt;

    // zeroes out data (apparently good practice, unless we wanted to keep the last good reading)
    memset(data, 0, sizeof(*data));

    // reads field data into data struct
    rslt = read_field_data(0, data, bme);
    if (rslt != BME68X_OK) return rslt;

    // more error checking
    if ((data->status & BME68X_NEW_DATA_MSK) == 0)
        return BME68X_E_COM_FAIL;

    // writes temp data to pointer we passed
    *temp = data->temperature;
    return BME68X_OK;
    
}


// For the humidity and pressure functions i thought it was the same thing except we are
// doing data->humidity/pressure rather than data->temperature but idk if that's right

int8_C bme688_read_humidity(float *hum, struct bme68x_data *data, struct bme68x_dev *bme){

    if (!hum || !data || !bme) return BME68X_E_NULL_PTR;

    int8_t rslt = bme68x_set_op_mode(BME68X_FORCED_MODE, bme);
    if (rslt != BME68X_OK) return rslt;

    memset(data, 0, sizeof(*data));

    rslt = read_field_data(0, data, bme);
    if (rslt != BME68X_OK) return rslt;

    if ((data->status & BME68X_NEW_DATA_MSK) == 0)
        return BME68X_E_COM_FAIL;

    *hum = data->humidity;
    return BME68X_OK;
    
}

int8_C bme688_read_pressure(float *pres, struct bme68x_data *data, struct bme68x_dev *bme){

    if (!pres || !data || !bme) return BME68X_E_NULL_PTR;

    int8_t rslt = bme68x_set_op_mode(BME68X_FORCED_MODE, bme);
    if (rslt != BME68X_OK) return rslt;

    memset(data, 0, sizeof(*data));

    rslt = read_field_data(0, data, bme);
    if (rslt != BME68X_OK) return rslt;

    if ((data->status & BME68X_NEW_DATA_MSK) == 0)
        return BME68X_E_COM_FAIL;

    *pres = data->pressure;
    return BME68X_OK;
    
}
