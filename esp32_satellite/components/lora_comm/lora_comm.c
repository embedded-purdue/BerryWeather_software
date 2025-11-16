#include "lora_comm.h"
#include <string.h>
#include <stdio.h>

static const char *TAG = "lora_comm";
void lora_send_cmd_and_print(const char *s) {
    uart_write_bytes(LORA_UART_PORT, s, strlen(s));
    printf("Sent: %s", s);

    uint8_t rx[LORA_UART_BUF_SIZE];
    int len = uart_read_bytes(LORA_UART_PORT, rx, LORA_UART_BUF_SIZE - 1, pdMS_TO_TICKS(1000));
    if (len > 0) { 
        rx[len] = '\0'; 
        printf("Received: %s\n", rx); 
    } else {
        printf("Received: (timeout)\n");
    }
}


/**
 * @brief Waits for any LoRa UART message.
 */
bool lora_wait_for_message(char *buf, size_t len, uint32_t timeout_ms)
{
    int n = uart_read_bytes(LORA_UART_PORT, (uint8_t*)buf, len - 1, pdMS_TO_TICKS(timeout_ms));
    // vTaskDelay(pdMS_TO_TICKS(6000));
    if (n > 0) {
        buf[n] = '\0';
        ESP_LOGI(TAG, "LoRa RX: %s", buf);
        return true;
    }
    return false;
}

/**
 * @brief Handles handshake between Satellite and MiddleMan at startup.
 * @param is_middleman true if this node is the MiddleMan, false if Satellite
 * @param peer_addr The LoRa address of the other device
 * @return true if handshake succeeded, false if timed out
 */
bool lora_boot_handshake(bool is_middleman, uint8_t peer_addr)
{
    char rx_buf[256];
    int attempt = 0;
    const int MAX_ATTEMPTS = 20;

    if (is_middleman) {
        ESP_LOGI(TAG, "[MM] Waiting for satellite boot message...");

        while (attempt < MAX_ATTEMPTS) {
            if (lora_wait_for_message(rx_buf, sizeof(rx_buf), 3000)) {
                if (strstr(rx_buf, "SATELLITE_BOOT_OK")) {
                    ESP_LOGI(TAG, "[MM] Satellite boot message received.");
                    vTaskDelay(pdMS_TO_TICKS(500));
                    lora_send_message(peer_addr, "MM_ACK_BOOT");
                    ESP_LOGI(TAG, "[MM] Sent ACK to satellite.");
                    return true;
                }
            }
            attempt++;
        }

        ESP_LOGW(TAG, "[MM] No boot message received after %d attempts.", MAX_ATTEMPTS);
        return false;
    }
    else {
        ESP_LOGI(TAG, "[SAT] Sending boot message and waiting for ACK...");

        for (attempt = 0; attempt < MAX_ATTEMPTS; attempt++) {
            lora_send_message(peer_addr, "SATELLITE_BOOT_OK");
            ESP_LOGI(TAG, "[SAT] Boot message sent (attempt %d).", attempt + 1);

            if (lora_wait_for_message(rx_buf, sizeof(rx_buf), 3000)) {
                if (strstr(rx_buf, "MM_ACK_BOOT")) {
                    ESP_LOGI(TAG, "[SAT] Received ACK from MiddleMan!");
                    return true;
                }
            }

            ESP_LOGW(TAG, "[SAT] No ACK received, retrying...");
            vTaskDelay(pdMS_TO_TICKS(1000));
        }

        ESP_LOGE(TAG, "[SAT] Handshake failed after %d attempts.", MAX_ATTEMPTS);
        return false;
    }
}

void lora_common_setup(int addr)
{
    vTaskDelay(pdMS_TO_TICKS(500)); // Wait for module to be ready after reset

    const char *commands[] = {
        "AT\r\n",
        "AT+VER?\r\n",
        "AT+UID?\r\n",
        "AT+MODE=0\r\n", // Normal mode
    };
    const int num_commands = sizeof(commands) / sizeof(commands[0]);
    for (int i = 0; i < num_commands; i++) {
        lora_send_cmd_and_print(commands[i]);
    }

    char addr_cmd[32];
    snprintf(addr_cmd, sizeof(addr_cmd), "AT+ADDRESS=%d\r\n", addr);
    lora_send_cmd_and_print(addr_cmd);
}

void lora_uart_config(void)
{
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    uart_param_config(LORA_UART_PORT, &uart_config);
    uart_set_pin(LORA_UART_PORT, LORA_TXD_PIN, LORA_RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(LORA_UART_PORT, LORA_UART_BUF_SIZE * 2, 0, 0, NULL, 0);
}

void lora_reset(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << LORA_NRST_PIN),
        .mode = GPIO_MODE_OUTPUT_OD, // Open-drain
    };
    gpio_config(&io_conf);

    // Perform reset (active low)
    gpio_set_level(LORA_NRST_PIN, 0);
    vTaskDelay(pdMS_TO_TICKS(100));
    gpio_set_level(LORA_NRST_PIN, 1);
    vTaskDelay(pdMS_TO_TICKS(1000)); // Give module time to boot
    uart_flush_input(LORA_UART_PORT);
}

void lora_send_message(uint8_t address, const char* message) {
    char at_command[256];
    int message_length = strlen(message);
    
    // Format: AT+SEND=address,length,message
    snprintf(at_command, sizeof(at_command), "AT+SEND=%d,%d,%s\r\n", 
            address, message_length, message);
    
    // Send the AT command (don't need to read response here, but you could)
    uart_write_bytes(LORA_UART_PORT, at_command, strlen(at_command));
    printf("Sent: %s", at_command);
    
    // Note: This is a "fire-and-forget" send. 
    // For critical data, you'd wait for the "+OK" or "+ERR" response.
}