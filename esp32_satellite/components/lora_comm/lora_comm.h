#ifndef LORA_COMM_H
#define LORA_COMM_H

#include "driver/uart.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// --- Configuration ---
#define LORA_UART_PORT      UART_NUM_1
#define LORA_TXD_PIN        GPIO_NUM_17
#define LORA_RXD_PIN        GPIO_NUM_16
#define LORA_NRST_PIN       GPIO_NUM_4
#define LORA_UART_BUF_SIZE  2048

// --- Function Prototypes ---

/**
 * @brief Configures the ESP32's UART peripheral to talk to the LoRa module.
 */
void lora_uart_config(void);

/**
 * @brief Performs a hardware reset of the LoRa module via the NRST pin.
 */
void lora_reset(void);

/**
 * @brief Sends an AT command, prints it, and waits for/prints the response.
 * @param cmd The AT command string to send (should end in \r\n).
 */
void lora_send_cmd_and_print(const char *cmd);

/**
 * @brief Performs the common setup sequence for the LoRa module.
 * @param addr The LoRa address to assign to this device.
 */
void lora_common_setup(int addr);

/**
 * @brief Formats and sends a LoRa message using the AT+SEND command.
 * @param address The destination LoRa address.
 * @param message The data payload to send.
 */
void lora_send_message(uint8_t address, const char* message);

#endif // LORA_COMM_H