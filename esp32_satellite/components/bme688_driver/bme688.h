#ifndef BME688_H
#define BME688_H
#include "bme68x.h"

// Initializes the BME688 sensor
void bme688_init(void);

int8_C bme688_read_temperature(float *temp, struct bme68x_data *data, struct bme68x_dev *bme);
int8_C bme688_read_humidity(float *hum, struct bme68x_data *data, struct bme68x_dev *bme);
int8_C bme688_read_pressure(float *pres, struct bme68x_data *data, struct bme68x_dev *bme);

#endif // BME688_H
