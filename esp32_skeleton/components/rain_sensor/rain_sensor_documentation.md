# BerryWeather Rain Sensor Implementation
Sensor: DC 3V-5V Rain Water Level Sensor Module

## Data Sheet:
Product Page:  
https://www.elecbee.com/en/product-detail/dc-3v-5v-20ma-rain-water-level-sensor-module-detection-liquid-surface-depth-height-for-arduino_25843

## Overview:
The rain water level sensor is a simple analog sensor that detects the presence and approximates the level of water through exposed copper traces. When water bridges these conductors, it creates a variable resistance that can be measured through an analog input. The sensor outputs a voltage proportional to the water coverage area on the sensor surface.

## Files: 
In this application, there are two files that define how the microcontroller communicates with the rain sensor:

- `rain_sensor.h`: Header file containing function declarations and pin definitions
- `rain_sensor.c`: Implementation file containing the sensor driver code

## Communication:
The ESP32 microcontroller communicates with the rain sensor through analog voltage readings. 

The sensor is read through the ESP32's built-in Analog-to-Digital Converter (ADC). The sensor outputs a reading from 0.0 - 3.3 V.

## Initialization

The initialization process configures the GPIO and the ADC subsystems:

 **1. Configure Power Control GPIO:**
```
#define RAIN_SENSOR_POWER_PIN GPIO_NUM_4 

gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << RAIN_SENSOR_POWER_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };

gpio_set_level(RAIN_SENSOR_POWER_PIN, 0);
```
 - Output mode
 - No pull-up or pull-down resistors
 - No interrupts
 - Sensor is initially powered off

 **2. Configure ADC:**
```
#define RAIN_SENSOR_ADC_CHAN ADC1_CHANNEL_0 

adc1_config_width(ADC_WIDTH_BIT_12);
adc1_config_channel_atten(RAIN_SENSOR_ADC_CHAN, ADC_ATTEN_DB_12);
```
 - Set up ADC1_CHANNEL_0 for analog readings
 - ADC Width: 12-bit resolution (0-4095 range)
 - Attenuation: 12dB attenuation for full 0-3.3V measurement range

## Water Level Reading
**Overview:**
1. Power on the sensor via GPIO
2. Wait for voltage stabilization (10ms)
3. Read the ADC value
4. Power off the sensor immediately



## Sensor Characteristics

**Operating Voltage:** 3V-5V

**Operating Current:** ~20mA  

**Output:** Analog voltage (0V to supply voltage)  

## Other Considerations

**Measurement Accuracy**
- Resolution: 12-bit ADC provides 0.8mV resolution across 3.3V range
- Stabilization Time: 10ms delay ensures consistent readings

**Power Management:** To extend the lifespan of the sensor, it is only powered on during active measurements. This is controlled through a GPIO pin that switches power on and off.

## Code
