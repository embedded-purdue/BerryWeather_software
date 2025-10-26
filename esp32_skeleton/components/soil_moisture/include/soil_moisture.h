#ifndef SOIL_MOISTURE_H
#define SOIL_MOISTURE_H
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void soil_moisture_init(void);
void soil_moisture_read(float *soil_moisture);

#endif // SOIL_MOISTURE_H