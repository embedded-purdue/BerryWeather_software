// bme688.c - BME688 sensor driver source (stub)
#include "bme68x.h"
#include <stdio.h>
#include <string.h>
#include "driver/i2c_master.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "BME688";

#define BME688_ADDR 0x77 // Replace with your BME688 sensor's I2C address
#define RESET_REG_BME 0xE0
#define RESET_VALUE_BME 0xB6
#define TEST_I2C_PORT I2C_NUM_0
#define I2C_MASTER_SCL_IO 22
#define I2C_MASTER_SDA_IO 21

i2c_master_bus_config_t i2c_mst_config = {
    .clk_source = I2C_CLK_SRC_DEFAULT,
    .i2c_port = TEST_I2C_PORT,
    .scl_io_num = I2C_MASTER_SCL_IO,
    .sda_io_num = I2C_MASTER_SDA_IO,
    .glitch_ignore_cnt = 7,
    .flags.enable_internal_pullup = true,
};

i2c_master_bus_handle_t bus_handle;

i2c_device_config_t dev_cfg = {
    .dev_addr_length = I2C_ADDR_BIT_LEN_7,
    .device_address = BME688_ADDR,  // Use our defined constant
    .scl_speed_hz = 400000  // Increased to 400kHz which BME688 supports
};

i2c_master_dev_handle_t dev_handle;

/* --- Adapter functions for BME68x driver --- */
static void bme68x_delay_us(uint32_t us, void *intf_ptr)
{
    uint64_t start = esp_timer_get_time();
    while ((esp_timer_get_time() - start) < us) {
        ; // busy-wait for short delays
    }
}

static int8_t bme68x_i2c_read(uint8_t reg_addr, uint8_t *reg_data, uint32_t len, void *intf_ptr)
{
    if (!intf_ptr) {
        ESP_LOGE(TAG, "I2C read: null interface pointer");
        return BME68X_E_NULL_PTR;
    }

    i2c_master_dev_handle_t handle = (i2c_master_dev_handle_t)intf_ptr;
    esp_err_t err = i2c_master_transmit_receive(handle, &reg_addr, 1, reg_data, len, 1000);
    
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "I2C read failed: reg=0x%02X, len=%lu, err=%d", reg_addr, (unsigned long)len, err);
        return BME68X_E_COM_FAIL;
    }
    
    ESP_LOGD(TAG, "I2C read: reg=0x%02X, len=%lu, first_byte=0x%02X", 
             reg_addr, (unsigned long)len, reg_data[0]);
    return BME68X_OK;
}

static int8_t bme68x_i2c_write(uint8_t reg_addr, const uint8_t *reg_data, uint32_t len, void *intf_ptr)
{
    if (!intf_ptr) {
        ESP_LOGE(TAG, "I2C write: null interface pointer");
        return BME68X_E_NULL_PTR;
    }

    if (!reg_data && len > 0) {
        ESP_LOGE(TAG, "I2C write: null data pointer");
        return BME68X_E_NULL_PTR;
    }

    i2c_master_dev_handle_t handle = (i2c_master_dev_handle_t)intf_ptr;
    
    uint8_t tx_buf[64];
    size_t tx_len = len + 1;
    
    if (tx_len > sizeof(tx_buf)) {
        ESP_LOGE(TAG, "I2C write: buffer overflow (%u bytes needed)", (unsigned)tx_len);
        return BME68X_E_COM_FAIL;
    }
    
    tx_buf[0] = reg_addr;
    if (len > 0) {
        memcpy(&tx_buf[1], reg_data, len);
    }
    
    esp_err_t err = i2c_master_transmit(handle, tx_buf, tx_len, 1000);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "I2C write failed: reg=0x%02X, len=%lu, err=%d", 
                reg_addr, (unsigned long)len, err);
        return BME68X_E_COM_FAIL;
    }

    ESP_LOGD(TAG, "I2C write: reg=0x%02X, len=%lu, first_byte=%02X", 
             reg_addr, (unsigned long)len, len > 0 ? reg_data[0] : 0);
    return BME68X_OK;
}

void bme688_init(struct bme68x_data *data, struct bme68x_dev *bme) {
    esp_err_t err;
    int8_t rslt;

    ESP_LOGI(TAG, "Initializing BME688 sensor...");

    // Set debug log level for detailed I2C communication logs
    esp_log_level_set(TAG, ESP_LOG_DEBUG);

    // Initialize I2C bus
    err = i2c_new_master_bus(&i2c_mst_config, &bus_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create I2C master bus: %d", err);
        return;
    }

    // Add device with known address 0x77
    dev_cfg.device_address = BME688_ADDR;  // Using our defined address (0x77)
    err = i2c_master_bus_add_device(bus_handle, &dev_cfg, &dev_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add I2C device at address 0x%02X: %d", BME688_ADDR, err);
        return;
    }
    
    // Test communication by trying to read the chip ID
    uint8_t chip_id = 0;
    err = i2c_master_transmit_receive(dev_handle, (uint8_t[]){0xD0}, 1, &chip_id, 1, 1000);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read chip ID: %d", err);
        return;
    }
    
    ESP_LOGI(TAG, "Connected to BME688 at address 0x%02X, Chip ID: 0x%02X", BME688_ADDR, chip_id);

    // Set up bme68x_dev interface
    bme->intf = BME68X_I2C_INTF;
    bme->intf_ptr = (void *)dev_handle;
    bme->read = bme68x_i2c_read;
    bme->write = (int8_t (*)(uint8_t, const uint8_t *, uint32_t, void *))bme68x_i2c_write;
    bme->delay_us = bme68x_delay_us;
    bme->amb_temp = 25; // Ambient temperature in deg C

    // Initialize BME688 sensor
    rslt = bme68x_init(bme);
    if (rslt != BME68X_OK) {
        ESP_LOGE(TAG, "Failed to initialize BME688: %d", rslt);
        return;
    }

    // Configure TPH settings
    struct bme68x_conf conf;
    conf.filter = BME68X_FILTER_OFF;
    conf.odr = BME68X_ODR_NONE;
    conf.os_hum = BME68X_OS_1X;
    conf.os_pres = BME68X_OS_16X;
    conf.os_temp = BME68X_OS_2X;
    
    rslt = bme68x_set_conf(&conf, bme);
    if (rslt != BME68X_OK) {
        ESP_LOGE(TAG, "Failed to set TPH configuration: %d", rslt);
        return;
    }

    // Setup heater configuration
    struct bme68x_heatr_conf heatr_conf;
    heatr_conf.enable = BME68X_ENABLE;
    heatr_conf.heatr_temp = 300;
    heatr_conf.heatr_dur = 100;
    
    rslt = bme68x_set_heatr_conf(BME68X_FORCED_MODE, &heatr_conf, bme);
    if (rslt != BME68X_OK) {
        ESP_LOGE(TAG, "Failed to set heater configuration: %d", rslt);
        return;
    }

    // Set operation mode to forced mode
    rslt = bme68x_set_op_mode(BME68X_FORCED_MODE, bme);
    if (rslt != BME68X_OK) {
        ESP_LOGE(TAG, "Failed to set operation mode: %d", rslt);
        return;
    }

    ESP_LOGI(TAG, "BME688 initialized successfully");
}



int8_t bme688_read_temperature(float *temp, struct bme68x_data *data, struct bme68x_dev *bme)
{
    if (!temp || !data || !bme) return BME68X_E_NULL_PTR;

    uint8_t n_data = 0;
    *temp = 0.0f;
    memset(data, 0, sizeof(*data));

    // Set operation mode to forced mode to trigger a measurement
    int8_t rslt = bme68x_set_op_mode(BME68X_FORCED_MODE, bme);
    if (rslt != BME68X_OK) {
        ESP_LOGE(TAG, "Failed to set forced mode: %d", rslt);
        return rslt;
    }

    // Create a configuration structure for getting measurement duration
    struct bme68x_conf conf;
    conf.os_hum = BME68X_OS_16X;
    conf.os_pres = BME68X_OS_1X;
    conf.os_temp = BME68X_OS_2X;
    
    // Get the delay time for the measurement
    uint32_t delay_period = bme68x_get_meas_dur(BME68X_FORCED_MODE, &conf, bme);
    
    ESP_LOGI(TAG, "Measurement delay: %lu ms", delay_period / 100);
    
    // Wait for the measurement to complete (convert microseconds to milliseconds)
    vTaskDelay(pdMS_TO_TICKS((delay_period / 100) + 1));

    // Get the data
    rslt = bme68x_get_data(BME68X_FORCED_MODE, data, &n_data, bme);
    if (rslt != BME68X_OK) {
        ESP_LOGE(TAG, "Failed to get temperature data: %d", rslt);
        return rslt;
    }
    if (n_data == 0) {
        ESP_LOGW(TAG, "No new temperature data available");
        return BME68X_W_NO_NEW_DATA;
    }

    *temp = data->temperature;
    ESP_LOGI(TAG, "Temperature reading: %.2f°C", *temp);
    return BME68X_OK;
}

int8_t bme688_read_pressure(float *pres, struct bme68x_data *data, struct bme68x_dev *bme)
{
    if (!pres || !data || !bme) return BME68X_E_NULL_PTR;

    uint8_t n_data = 0;
    *pres = 0.0f;
    memset(data, 0, sizeof(*data));

    // Set operation mode to forced mode to trigger a measurement
    int8_t rslt = bme68x_set_op_mode(BME68X_FORCED_MODE, bme);
    if (rslt != BME68X_OK) {
        ESP_LOGE(TAG, "Failed to set forced mode: %d", rslt);
        return rslt;
    }

    // Create a configuration structure for getting measurement duration
    struct bme68x_conf conf;
    conf.os_hum = BME68X_OS_16X;
    conf.os_pres = BME68X_OS_1X;
    conf.os_temp = BME68X_OS_2X;
    
    // Get the delay time for the measurement
    uint32_t delay_period = bme68x_get_meas_dur(BME68X_FORCED_MODE, &conf, bme);
    
    ESP_LOGI(TAG, "Pressure measurement delay: %lu ms", delay_period / 100);
    
    // Wait for the measurement to complete (convert microseconds to milliseconds)
    vTaskDelay(pdMS_TO_TICKS((delay_period / 100) + 1));

    // Get the data
    rslt = bme68x_get_data(BME68X_FORCED_MODE, data, &n_data, bme);
    if (rslt != BME68X_OK) {
        ESP_LOGE(TAG, "Failed to get pressure data: %d", rslt);
        return rslt;
    }
    if (n_data == 0) {
        ESP_LOGW(TAG, "No new pressure data available");
        return BME68X_W_NO_NEW_DATA;
    }

    *pres = data->pressure;
    ESP_LOGI(TAG, "Pressure reading: %.2f hPa", *pres);
    return BME68X_OK;
}

int8_t bme688_read_humidity(float *hum, struct bme68x_data *data, struct bme68x_dev *bme)
{
    if (!hum || !data || !bme) return BME68X_E_NULL_PTR;

    uint8_t n_data = 0;

    memset(data, 0, sizeof(*data));

    // Set operation mode to forced mode to trigger a measurement
    int8_t rslt = bme68x_set_op_mode(BME68X_FORCED_MODE, bme);
    if (rslt != BME68X_OK) {
        ESP_LOGE(TAG, "Failed to set forced mode: %d", rslt);
        return rslt;
    }

    // Create a configuration structure for getting measurement duration
    struct bme68x_conf conf;
    conf.os_hum = BME68X_OS_1X;
    conf.os_pres = BME68X_OS_16X;
    conf.os_temp = BME68X_OS_2X;
    
    // Get the delay time for the measurement
    uint32_t delay_period = bme68x_get_meas_dur(BME68X_FORCED_MODE, &conf, bme);
    
    ESP_LOGI(TAG, "Humidity measurement delay: %lu ms", delay_period / 100);
    
    // Wait for the measurement to complete (convert microseconds to milliseconds)
    vTaskDelay(pdMS_TO_TICKS((delay_period / 100) + 1));

    // Get the data
    rslt = bme68x_get_data(BME68X_FORCED_MODE, data, &n_data, bme);
   // if (n_data)
        *hum = data->humidity;
    if (rslt == BME68X_W_NO_NEW_DATA) {
        ESP_LOGW(TAG, "No new humidity data available");
    }
    ESP_LOGI(TAG, "Humidity reading: %.2f%%", *hum);
    return BME68X_OK;
}

int8_t bme688_read_gas_resistance(float *gas_resistance, struct bme68x_data *data, struct bme68x_dev *bme)
{
    if (!gas_resistance || !data || !bme) return BME68X_E_NULL_PTR;

    uint8_t n_data = 0;
    *gas_resistance = 0.0f;
    memset(data, 0, sizeof(*data));

    int8_t rslt = bme68x_get_data(BME68X_FORCED_MODE, data, &n_data, bme);
    if (rslt < 0) return rslt;
    if (n_data == 0) return BME68X_W_NO_NEW_DATA;
    
    // Check if gas measurement is valid
    if (data->status & BME68X_GASM_VALID_MSK) {
        *gas_resistance = data->gas_resistance;
        return BME68X_OK;
    }
    
    return BME68X_W_NO_NEW_DATA;
}

