# BME 688 Sensor
The BME 688 sensor is an integrated sensor [from Adafruit](https://www.adafruit.com/product/5046) that reads the temperature, humidity, barometric pressure, and VOC gas sensing data from the atmosphere. 

For the purpose of the BerryWeather Project, the sensor is responsible for capturing the **temperature**, **humidity**, and **barometric pressure**.

## Implementation
To establish communication between the BME 688 sensor and the ESP32, we utilized the **I2C protocol**. Then, to read the data from the sensor, we utilized the [BME68X Sensor API](https://github.com/boschsensortec/BME68x_SensorAPI/tree/master) developed by Bosch Sensortec.