# SN-3000-FSA-N01 Wind Speed Transmitter Component

## Overview
This component provides functionality to interface with the SN-3000-FSA-N01 Wind Speed Transmitter, a professional-grade wind sensor with RS-485 Modbus RTU communication.

**Note:** This sensor only measures wind speed, not wind direction.

## Hardware Configuration

### Pin Assignments (RS-485 Interface)
- **RS-485 TX**: GPIO17 (UART2_TX) - Data transmission
- **RS-485 RX**: GPIO16 (UART2_RX) - Data reception
- **RS-485 RTS**: GPIO4 - Direction control for RS-485 transceiver

### Sensor Specifications (SN-3000-FSA-N01)
- **Measuring Range**: 0-60 m/s
- **Resolution**: 0.1 m/s  
- **Accuracy**: ±(0.2 + 0.03 V) m/s @ (0-30 m/s, 25°C), V = wind speed
- **Power Supply**: 5-30V DC (connect externally)
- **Communication**: RS-485 Modbus RTU
- **Default Baud Rate**: 4800 bps
- **Default Device Address**: 0x01

## API Reference

### Initialization
```c
esp_err_t wind_sensor_init(void);
```

### Reading Functions
```c
esp_err_t wind_sensor_read_speed(float *wind_speed);
esp_err_t wind_sensor_read_direction(float *wind_direction);
esp_err_t wind_sensor_read_all(wind_data_t *wind_data);
```

### Raw ADC Functions
```c
int wind_sensor_get_speed_raw(void);
int wind_sensor_get_direction_raw(void);
```

## Wiring Diagram

### Complete Connection: Wind Sensor → MAX485 Module → ESP32

```
      Wind Sensor (SN-3000-FSA-N01)      MAX485 Module                ESP32
   ┌────────────────────────────────┐  ┌─────────────────────┐      ┌──────────────────┐
   │ Brown  → +12V Power Supply     │  │                     │      │                  │
   │ Black  → GND ──────────────────┼──┤ GND ────────────────┼──────┤ GND              │
   │ Yellow → RS485-A ──────────────┼──┤ A                   │      │                  │
   │ Blue   → RS485-B ──────────────┼──┤ B                   │      │                  │
   └────────────────────────────────┘  │                     │      │                  │
                                       │ VCC ────────────────┼──────┤ 3.3V             │
                                       │ RO (Receiver Out) ──┼──────┤ GPIO16 (RX)      │
                                       │ DI (Driver In) ─────┼──────┤ GPIO17 (TX)      │
                                       │ DE+RE (Direction) ──┼──────┤ GPIO18 (RTS)     │
                                       └─────────────────────┘      └──────────────────┘

External 12V Power Supply:
+12V ──→ Wind Sensor Brown wire
GND  ──→ Wind Sensor Black wire + ESP32 GND + MAX485 GND (common ground)
```

### MAX485 Module Pin Functions
| MAX485 Pin | Function | Description | ESP32 Connection |
|------------|----------|-------------|------------------|
| VCC | Power | Module power (3.3V or 5V) | 3.3V |
| GND | Ground | Common ground | GND |
| A | RS485-A | Differential data + | Wind sensor Yellow |
| B | RS485-B | Differential data - | Wind sensor Blue |
| RO | Receiver Out | Data output to microcontroller | GPIO16 (UART1 RX) |
| DI | Driver In | Data input from microcontroller | GPIO17 (UART1 TX) |
| RE | Receiver Enable | Receive mode control | GPIO18 (tied with DE) |
| DE | Driver Enable | Transmit mode control | GPIO18 (tied with RE) |

### Key Points
- **DE and RE pins are tied together** and connected to GPIO18 for automatic direction control
- **External 12V power supply required** for the wind sensor (do not power from ESP32)
- **Common ground connection** between all devices is essential
- **RS485 is differential signaling** - A and B lines carry the data signal

## Usage Example

```c
#include "wind_sensor.h"

void app_main() {
    // Initialize wind sensor
    esp_err_t ret = wind_sensor_init();
    if (ret != ESP_OK) {
        ESP_LOGE("MAIN", "Failed to initialize wind sensor: %s", esp_err_to_name(ret));
        return;
    }
    
    while(1) {
        float wind_speed;
        
        // Read wind speed only (direction not supported)
        if (wind_sensor_read_speed(&wind_speed) == ESP_OK) {
            printf("Wind Speed: %.1f m/s\n", wind_speed);
        } else {
            printf("Failed to read wind speed\n");
        }
        
        vTaskDelay(pdMS_TO_TICKS(2000)); // Read every 2 seconds
    }
}
```

## Communication Protocol

### Modbus RTU Settings
- **Baud Rate**: 4800 bps (default, configurable)
- **Data Bits**: 8
- **Parity**: None  
- **Stop Bits**: 1
- **Device Address**: 0x01 (default, configurable)

### Register Map
| Register | Address | Content | Access |
|----------|---------|---------|---------|
| 40001 | 0x0000 | Wind speed × 10 | Read-only |

### Example Communication
**Request** (read wind speed from device 0x01):
```
01 03 00 00 00 01 84 0A
```

**Response** (wind speed = 8.6 m/s):
```
01 03 02 00 56 38 7A
```
Calculation: 0x0056 = 86 decimal → 86/10 = 8.6 m/s

## Troubleshooting

### Common Issues
1. **No Response**: Check wiring, power supply, and device address
2. **CRC Errors**: Verify RS-485 connections and cable quality  
3. **Timeout**: Ensure proper baud rate and response timing
4. **Invalid Data**: Check sensor power supply (5-30V DC required)

## Notes
- External power supply (5-30V DC) required for sensor operation
- RS-485 transceiver module needed for ESP32 interface
- Sensor supports multiple units on same bus (up to 254 devices)
- Use 120Ω termination resistors for long cable runs
- Minimum response time: 2 seconds (sensor specification)