// soil_moisture.c - Grove Soil Moisture sensor driver source (stub)
#include "soil_moisture.h"
#include <stdio.h>
#include "driver/adc.h" // Uncomment if using ADC

#define SOIL_MOISTURE_ADC_CHAN ADC1_CHANNEL_6 // Replace with your ADC channel

// Initialization Function
void soil_moisture_init(void) {
    // 12-bit resolution (0-4095)
    adc1_config_width(ADC_WIDTH_BIT_12);

    // 11 dB attenuation = full-scale voltage ~ 0V–3.3V
    adc1_config_channel_atten(SOIL_MOISTURE_ADC_CHAN, ADC_ATTEN_DB_11);

    printf("Soil Moisture sensor initialized (real hardware)!\n");
}

// Convert raw value to "Soil Moisture Index"
void soil_moisture_read(float *soil_moisture) {
    int raw = soil_moisture_read_raw(); // Calls raw value which will range from 0-4095 bits
    *soil_moisture = raw * 10 / 4095.0; // Scales from 0 to 10; this is the arbitrary unit of "Soil Moisture Index" that will range from bone-dry to completely saturated
}
