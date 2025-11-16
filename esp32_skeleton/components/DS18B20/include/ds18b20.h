#ifndef DS18B20_H
#define DS18B20_H

#include <stdio.h> // For float

void ds18b20_init(void);
int ds18b20_read_temperature(float *temperature);

#endif