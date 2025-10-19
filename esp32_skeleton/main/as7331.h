// as7331.h - AS7331 sensor driver header (stub)
#include "driver/i2c.h"

#ifndef AS7331_H
#define AS7331_H

// INCLUDE THE REST OF THE FUNCTIONS BELOW
typedef struct {
    
    i2c_port_t port;
    uint8_t addr;
    uint16_t light_reading_raw[3]; // UV wavelength Data raw counts (UVA, UVB, UVC)

} AS7331;

typedef struct {

    float uva;
    float uvb;
    float uvc;

} AS7331_Light;

// Initializes the AS7331 sensor
esp_err_t as7331_init(AS7331 *dev);

// Read UV data function
esp_err_t as7331_read_light(AS7331 *dev, AS7331_Light *light);

// Low level function
esp_err_t AS7331_read_registers(AS7331 *dev, uint8_t reg_addr, uint8_t *data, size_t length);

#endif // AS7331_H