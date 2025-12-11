#ifndef RAIN_SENSOR_H
#define RAIN_SENSOR_H

typedef enum {
    RAIN_NONE = 0,
    RAIN_LIGHT,
    RAIN_MODERATE,
    RAIN_HEAVY
} rain_level_t;

void rain_sensor_init(void);
int rain_sensor_read(float *level);
int rain_sensor_get_raw(void);
float rain_sensor_get_normalized(void);

#endif // RAIN_SENSOR_H
