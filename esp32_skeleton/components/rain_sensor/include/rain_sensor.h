#ifndef RAIN_SENSOR_H
#define RAIN_SENSOR_H

void rain_sensor_init(void);
int rain_sensor_read(float *level);
int rain_sensor_get_raw(void);
float rain_sensor_get_normalized(void);

#endif // RAIN_SENSOR_H
