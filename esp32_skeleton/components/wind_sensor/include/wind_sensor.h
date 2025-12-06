#ifndef WIND_SENSOR_H
#define WIND_SENSOR_H

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Wind sensor data structure for SN-3000-FSA-N01
 * Note: This sensor only measures wind speed, not direction
 */
typedef struct {
    float wind_speed;      // Wind speed in m/s (0-60 m/s range)
    float wind_direction;  // Wind direction (not supported by SN-3000-FSA-N01, always 0)
} wind_data_t;

/**
 * @brief Initialize the SN-3000-FSA-N01 Wind Speed Transmitter
 * Configures UART for RS-485 Modbus RTU communication
 * 
 * @return ESP_OK on success, ESP_ERR_* on failure
 */
esp_err_t wind_sensor_init(void);

/**
 * @brief Read wind speed from SN-3000-FSA-N01
 * 
 * @param wind_speed Pointer to store wind speed in m/s (0-60 m/s range)
 * @return ESP_OK on success, ESP_ERR_* on failure
 */
esp_err_t wind_sensor_read_speed(float *wind_speed);

/**
 * @brief Read wind direction (NOT SUPPORTED by SN-3000-FSA-N01)
 * This function always returns ESP_ERR_NOT_SUPPORTED
 * 
 * @param wind_direction Pointer to store wind direction (always set to 0)
 * @return ESP_ERR_NOT_SUPPORTED
 */
esp_err_t wind_sensor_read_direction(float *wind_direction);

/**
 * @brief Read wind data from SN-3000-FSA-N01
 * Only wind speed is measured; direction is always 0
 * 
 * @param wind_data Pointer to wind_data_t structure to store results
 * @return ESP_OK on success, ESP_ERR_* on failure
 */
esp_err_t wind_sensor_read_all(wind_data_t *wind_data);

/**
 * @brief Get raw wind speed value from Modbus register
 * 
 * @return Raw register value (wind speed × 10), or -1 on error
 */
int wind_sensor_get_speed_raw(void);

/**
 * @brief Get raw wind direction value (NOT SUPPORTED)
 * 
 * @return Always returns -1 (not supported)
 */
int wind_sensor_get_direction_raw(void);

#ifdef __cplusplus
}
#endif

#endif // WIND_SENSOR_H