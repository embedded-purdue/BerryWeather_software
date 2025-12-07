#include "ds18b20.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "esp_rom_sys.h" // For esp_rom_delay_us

#define ONEWIRE_GPIO 18
static const char *TAG = "DS18B20_DRIVER";

// --- Private 1-Wire Functions ---
static void onewire_set_output() { 
    gpio_set_direction(ONEWIRE_GPIO, GPIO_MODE_OUTPUT); 
}
static void ds18b20_set_resolution(uint8_t resolution_config);
static void onewire_set_input() { 
    gpio_set_direction(ONEWIRE_GPIO, GPIO_MODE_INPUT); 
}
static void onewire_high() { 
    onewire_set_input(); 
}
static void onewire_low() { 
    onewire_set_output(); gpio_set_level(ONEWIRE_GPIO, 0); 
}
static int onewire_read_level() { return gpio_get_level(ONEWIRE_GPIO); }
//1 wire reset process
static bool onewire_reset(void) {
    onewire_low();
    esp_rom_delay_us(480);
    onewire_high();
    esp_rom_delay_us(70);
    bool presence = (onewire_read_level() == 0);
    esp_rom_delay_us(410);
    return presence;
}
// Writes one byte to the 1-Wire bus, LSB first.
static void onewire_write_bit(bool bit) {
    onewire_low();
    esp_rom_delay_us(bit ? 6 : 60);
    onewire_high();
    esp_rom_delay_us(bit ? 64 : 10);
}
//Reads one byte from the 1-Wire bus, LSB first

static bool onewire_read_bit(void) {
    onewire_low();
    esp_rom_delay_us(6);
    onewire_high();
    esp_rom_delay_us(9);
    bool bit = (onewire_read_level() == 1);
    esp_rom_delay_us(55);
    return bit;
}

static void onewire_write_byte(uint8_t byte) {
    for (int i = 0; i < 8; i++) {
        onewire_write_bit(byte & 0x01);
        byte >>= 1;
    }
}

static uint8_t onewire_read_byte(void) {
    uint8_t byte = 0;
    for (int i = 0; i < 8; i++) {
        byte >>= 1;
        if (onewire_read_bit()) {
            byte |= 0x80;
        }
    }
    return byte;
}

// --- Public Functions (called from main.c) ---
void ds18b20_init(void) {
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << ONEWIRE_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = true,
        .pull_down_en = false,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);

    // 0x5F for 11-bit, 0x7F for 12-bit
    ds18b20_set_resolution(0x5F);
    ESP_LOGI(TAG, "DS18B20 driver initialized on GPIO %d", ONEWIRE_GPIO);
}
static void ds18b20_set_resolution(uint8_t resolution_config) {
    if (!onewire_reset()) {
        ESP_LOGE(TAG, "Failed to set resolution, no device found.");
        return;
    }
    onewire_write_byte(0xCC); // Skip ROM
    onewire_write_byte(0x4E); // Write Scratchpad command
    onewire_write_byte(0x00); // TH Register (not used)
    onewire_write_byte(0x00); // TL Register (not used)
    onewire_write_byte(resolution_config); //Configuration Register
}

int ds18b20_read_temperature(float *temperature) {
    if (!onewire_reset()) {
        ESP_LOGE(TAG, "No device found.");
        return -1;
    }
    onewire_write_byte(0xCC); // Skip ROM
    onewire_write_byte(0x44); // Convert T
    vTaskDelay(pdMS_TO_TICKS(400));

    if (!onewire_reset()) {
        ESP_LOGE(TAG, "No device found after conversion.");
        return -1;
    }
    onewire_write_byte(0xCC); // Skip ROM
    onewire_write_byte(0xBE); // Read Scratchpad

    uint8_t lsb = onewire_read_byte();
    uint8_t msb = onewire_read_byte();
    // combines MSB and LSB to a 16-bit signed integer
    int16_t raw_temp = (msb << 8) | lsb;
    //Sensor provides data in 1/16th degree C increments
    *temperature = (float)raw_temp / 16.0f;

    return 0; // Success
}