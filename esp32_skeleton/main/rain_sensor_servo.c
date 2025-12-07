#include <stdio.h>
#include "driver/ledc.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "SERVO";

// Servo configuration for GPIO14 (D14)
#define SERVO_PIN           GPIO_NUM_14
#define SERVO_FREQ_HZ       50      // 50Hz for standard servo
#define SERVO_TIMER         LEDC_TIMER_0
#define SERVO_MODE          LEDC_LOW_SPEED_MODE
#define SERVO_CHANNEL       LEDC_CHANNEL_0
#define SERVO_RESOLUTION    LEDC_TIMER_16_BIT

// Servo pulse width limits (in microseconds)
#define SERVO_MIN_PULSE_US  500     // 0 degrees
#define SERVO_MAX_PULSE_US  2400    // 180 degrees
#define SERVO_PULSE_RANGE   (SERVO_MAX_PULSE_US - SERVO_MIN_PULSE_US)

// Calculate duty cycle for LEDC
#define SERVO_MAX_DUTY      ((1 << SERVO_RESOLUTION) - 1)
#define PULSE_TO_DUTY(pulse_us) ((pulse_us) * SERVO_MAX_DUTY / (1000000 / SERVO_FREQ_HZ))

esp_err_t servo_init(void) {
    // Configure LEDC timer
    ledc_timer_config_t ledc_timer = {
        .duty_resolution = SERVO_RESOLUTION,
        .freq_hz = SERVO_FREQ_HZ,
        .speed_mode = SERVO_MODE,
        .timer_num = SERVO_TIMER,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    esp_err_t ret = ledc_timer_config(&ledc_timer);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "LEDC timer config failed: %s", esp_err_to_name(ret));
        return ret;
    }

    // Configure LEDC channel
    ledc_channel_config_t ledc_channel = {
        .channel = SERVO_CHANNEL,
        .duty = 0,
        .gpio_num = SERVO_PIN,
        .speed_mode = SERVO_MODE,
        .hpoint = 0,
        .timer_sel = SERVO_TIMER
    };
    ret = ledc_channel_config(&ledc_channel);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "LEDC channel config failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "Servo initialized on GPIO%d", SERVO_PIN);
    return ESP_OK;
}

esp_err_t servo_set_angle(float angle) {
    if (angle < 0 || angle > 180) {
        ESP_LOGE(TAG, "Invalid angle: %.1f (must be 0-180)", angle);
        return ESP_ERR_INVALID_ARG;
    }

    // Calculate pulse width for the angle
    uint32_t pulse_us = SERVO_MIN_PULSE_US + (angle / 180.0) * SERVO_PULSE_RANGE;
    uint32_t duty = PULSE_TO_DUTY(pulse_us);

    esp_err_t ret = ledc_set_duty(SERVO_MODE, SERVO_CHANNEL, duty);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set duty: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = ledc_update_duty(SERVO_MODE, SERVO_CHANNEL);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to update duty: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "Servo angle set to %.1f degrees (pulse: %ld us)", angle, pulse_us);
    return ESP_OK;
}

void servo_test_sweep(void) {
    ESP_LOGI(TAG, "Starting servo sweep test on GPIO%d", SERVO_PIN);
    
    while (1) {
        // Sweep from 0 to 180 degrees
        for (float angle = 0; angle <= 180; angle += 5) {
            servo_set_angle(angle);
            vTaskDelay(pdMS_TO_TICKS(100)); // 100ms delay
        }
        
        vTaskDelay(pdMS_TO_TICKS(500)); // Pause at 180 degrees
        
        // Sweep from 180 to 0 degrees
        for (float angle = 180; angle >= 0; angle -= 5) {
            servo_set_angle(angle);
            vTaskDelay(pdMS_TO_TICKS(100)); // 100ms delay
        }
        
        vTaskDelay(pdMS_TO_TICKS(500)); // Pause at 0 degrees
    }
}