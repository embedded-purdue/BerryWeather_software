#ifndef RAIN_SENSOR_H
#define RAIN_SENSOR_H

#include "esp_err.h"

// Rain sensor functions
void rain_sensor_init(void);
int rain_sensor_read(float *level);
int rain_sensor_get_raw(void);
float rain_sensor_get_normalized(void);

// Servo motor functions (for rain cover/protection)
esp_err_t servo_init(void);
esp_err_t servo_set_angle(float angle);
void servo_test_sweep(void);

#endif // RAIN_SENSOR_H
