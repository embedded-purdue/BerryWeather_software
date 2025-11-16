BerryWeather DS18B20 Implementation

Data sheet:
Most of the information that was used to implement the sensor into this application was obtained from the data sheet, which can be found at: https://www.analog.com/media/en/technical-documentation/data-sheets/DS18B20.pdf 

Files

This driver uses two files to define how the ESP32 microcontroller communicates with the DS18B20 sensor:

ds18b20.h: The header file that exposes the public functions available to other parts of the application. This includes ds18b20_init() for setting up the sensor and ds18b20_read_temperature() for getting a reading.

ds18b20.c: The source file containing the low-level implementation. It handles the 1-Wire communication protocol, including sending reset pulses, writing bits and bytes, and reading data from the sensor.

