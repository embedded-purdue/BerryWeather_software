# Soil Moisture Sensor
## Grove - Capacitive Soil Moisture Sensor (Corrosion Resistant)

### Description
The Grove Soil Moisture Sensor uses a capacitor which contains a material with a dielectric constant that varies based on its moisture level. Based on its level of moisture, it will output a value ranging from 0-4095, which this code converts to a “Soil Moisture Index” (SMI) which is arbitrary with reference points for easier interpretation.

### Component Connections
| Sensor Pin | ESP32 Pin |
|------------|-----------|
| GND        | GND       |
| Vcc        | 3v3       |
| SIG        | GPIO      |
*Note: The 3v3 pin provides voltage, so connecting Vcc to the 5V pin on the ESP32 is will also work, but it's output may be calibrated differently.

In `soil_moisture.c`, line 6 defines the ADC channel being used. For the GPIO pin selected, the ADC channel in the code should match according to the table below:

|    ADC Channel    |    GPIO     |
|-------------------|-------------|
| ADC1_CHANNEL_0    | GPIO36 (VP) |
| ADC1_CHANNEL_1    | GPIO37 (VN) |
| ADC1_CHANNEL_4    | GPIO32      |
| ADC1_CHANNEL_5    | GPIO33      |
| ADC1_CHANNEL_6    | GPIO34      |
| ADC1_CHANNEL_7    | GPIO35      |

### Unit Output
The arbitrary unit the soil_moisture.c code outputs is called "Soil Moisture Index" or SMI for short. It corresponds to a percentage of saturation in water by weight. The sensor 
| SMI Range | % By Weight |
|-----------|-------------|
|  SMI < 3  |     N/A     |
| (3, 4)    |    ~100%    |
| (4, 4.5)  | ~67% - ~100%|
| (4.5, 5)  | ~34% - ~66% |
| (5, 5.5)  | ~0% - ~33%  |
| (5.5, 6.5)|     ~0%     |