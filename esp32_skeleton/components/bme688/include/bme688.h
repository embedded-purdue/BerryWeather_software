#ifndef BME688_H
#define BME688_H

#include <stdint.h>
#include "bme68x.h"
#include "driver/i2c_master.h"

void bme688_init(struct bme68x_data *data,
                 struct bme68x_dev *bme, i2c_master_bus_handle_t bus_handle);

/* Read temperature (°C) in FORCED mode.
 * Returns: BME68X_OK on success,
 *          BME68X_W_NO_NEW_DATA if nothing fresh,
 *          <0 on error.
 */
int8_t bme688_read_temperature(float *temp,
                               struct bme68x_data *data,
                               struct bme68x_dev *bme);

/* Read pressure (Pa) in FORCED mode. */
int8_t bme688_read_pressure(float *pres,
                            struct bme68x_data *data,
                            struct bme68x_dev *bme);

/* Read relative humidity (%) in FORCED mode. */
int8_t bme688_read_humidity(float *hum,
                            struct bme68x_data *data,
                            struct bme68x_dev *bme);

/* Read gas resistance (ohm) in FORCED mode. */
int8_t bme688_read_gas_resistance(float *gas_resistance,
                                 struct bme68x_data *data,
                                 struct bme68x_dev *bme);

/* Usage notes:
 * - These helpers assume BME68X_FORCED_MODE.
 * - `data` must point to one struct for forced mode.
 * - If BME68X_W_NO_NEW_DATA, trigger a new forced measurement and retry.
 * - For PARALLEL/SEQUENTIAL, call bme68x_get_data() with an array[3].
 */

#endif /* BME688_H */
