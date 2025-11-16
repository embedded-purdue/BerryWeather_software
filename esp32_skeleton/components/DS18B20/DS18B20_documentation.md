BerryWeather DS18B20 Implementation

Data sheet:
Most of the information that was used to implement the sensor into this application was obtained from the data sheet, which can be found at: https://www.analog.com/media/en/technical-documentation/data-sheets/DS18B20.pdf 

Files

This driver uses two files to define how the ESP32 microcontroller communicates with the DS18B20 sensor:

ds18b20.h: The header file that exposes the public functions available to other parts of the application. This includes ds18b20_init() for setting up the sensor and ds18b20_read_temperature() for getting a reading.

ds18b20.c: The source file containing the low-level implementation. It handles the 1-Wire communication protocol, including sending reset pulses, writing bits and bytes, and reading data from the sensor.

Initialization

The ds18b20_init() function prepares the sensor for use.

GPIO Configuration: It configures the ONEWIRE_GPIO (GPIO 18) pin. The 1-Wire protocol is emulated by switching the pin between an input (with pull-up) to "read" or let the line float high, and an output to "write" or pull the line low.

Set Sensor Resolution: It calls the internal ds18b20_set_resolution() function. This function:

Sends a onewire_reset() to find the device.

Sends the Skip ROM command (0xCC), which addresses all devices on the 1-Wire bus .

Sends the Write Scratchpad command (0x4E).

Writes 0x00 to the TH and TL registers (not used in this driver).

Writes the Configuration Register. The value 0x5F is used, which sets the sensor's resolution to 11-bit. This provides a precision of 0.125°C and has a maximum conversion time of 375ms.