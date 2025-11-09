#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_log.h"
#include "cJSON.h"
#include "mqtt_client.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"

// --- Include our new common component ---
#include "lora_comm.h" 
// --- MiddleMan Specific Configuration ---
#define MM_ADDR 1 // This MiddleMan's address
#define SAT_ADDR 10 // This satellite's address


// Set to 1 to build the minimal LoRa-only listener
// Set to 0 to build the full Wi-Fi + MQTT application
#define LORA_TEST_ONLY 0

// List of KNOWN satellite addresses.
// We will publish discovery topics for each of these.
static const int KNOWN_SATELLITE_ADDRESSES[] = { 10 }; // Add more, e.g. {10, 11, 12}
static const int NUM_KNOWN_SATELLITES = sizeof(KNOWN_SATELLITE_ADDRESSES) / sizeof(KNOWN_SATELLITE_ADDRESSES[0]);


// --- Wi-Fi/MQTT Configuration ---
#define WIFI_SSID     "NETGEAR31"
#define WIFI_PASS     "Boilerup321"
#define CONFIG_BROKER_URL "mqtt://192.168.1.5:1883"

static EventGroupHandle_t s_wifi_event_group;
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1
static int s_retry_num = 0;
static const char *TAG = "mqtt_berryWeather";


// --- Minimal LoRa-only Listen Task ---
void lora_simple_listen_task(void *arg)
{
    char rx_buf[256];
    int attempt = 0;
    const int MAX_ATTEMPTS = 20;
    
    while (attempt < MAX_ATTEMPTS) {
        ESP_LOGI(TAG, "in new attempt<max_attempts wait for message, simple listen lora");
        if (lora_wait_for_message(rx_buf, sizeof(rx_buf), 3000)) {
            if (strstr((char*)rx_buf, "+RCV=")) {
                int sender_addr = 10; // extract dynamically if multiple satellites
                // After parsing JSON
                ESP_LOGI(TAG, "Data received from satellite %d: %s", sender_addr, rx_buf);
                lora_send_message(sender_addr, "MM_ACK_DATA");
                ESP_LOGI(TAG, "ACK sent to satellite %d.", sender_addr);
            }
        }
        attempt++;
    }

}

/*
 *
 * --- Wi-Fi Connection Logic (Unchanged from your mqtt.c) ---
 *
 */
static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < 5) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "Retrying to connect to the AP");
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(TAG,"Connect to the AP fail");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "Got IP address:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

void wifi_init_sta(void)
{
    s_wifi_event_group = xEventGroupCreate();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL));
    wifi_config_t wifi_config = {
        .sta = { .ssid = WIFI_SSID, .password = WIFI_PASS },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_LOGI(TAG, "wifi_init_sta finished.");
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT, pdFALSE, pdFALSE, portMAX_DELAY);
    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "Connected to AP SSID:%s", WIFI_SSID);
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGW(TAG, "Failed to connect to SSID:%s", WIFI_SSID);
    } else {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
    }
}


/*
 *
 * --- MQTT Logic (Now with dynamic discovery) ---
 *
 */

/**
 * @brief Publishes all 4 discovery messages for a single satellite.
 * This is now dynamic, based on the satellite's address.
 */
void publish_discovery_for_satellite(esp_mqtt_client_handle_t client, int sat_addr)
{
    ESP_LOGI(TAG, "Publishing discovery messages for satellite address %d", sat_addr);

    char discovery_topic[512];
    char discovery_payload[1024];
    char state_topic[128];
    char unique_id[256];
    char device_name[128];
    char device_id[128];

    // Common device info
    snprintf(state_topic, sizeof(state_topic), "weather/berrystation_%d/state", sat_addr);
    snprintf(device_id, sizeof(device_id), "berrystation_%d", sat_addr);
    snprintf(device_name, sizeof(device_name), "BerryWeather Station %d", sat_addr);

    // 1. Temperature
    snprintf(unique_id, sizeof(unique_id), "%s_temperature", device_id);
    snprintf(discovery_topic, sizeof(discovery_topic), "homeassistant/sensor/%s/config", unique_id);
    snprintf(discovery_payload, sizeof(discovery_payload),
        "{"
            "\"name\": \"%s Temperature\","
            "\"unique_id\": \"%s\","
            "\"stat_t\": \"%s\","
            "\"val_tpl\": \"{{ value_json.t }}\","
            "\"unit_of_meas\": \"°F\"," // You may want to send in °C
            "\"dev_cla\": \"temperature\","
            "\"ic\": \"mdi:thermometer\","
            "\"dev\": {\"ids\": [\"%s\"],\"name\": \"%s\",\"mf\": \"ESAP\"}"
        "}",
        device_name, unique_id, state_topic, device_id, device_name);
    esp_mqtt_client_publish(client, discovery_topic, discovery_payload, 0, 1, true);

    // 2. Humidity
    snprintf(unique_id, sizeof(unique_id), "%s_humidity", device_id);
    snprintf(discovery_topic, sizeof(discovery_topic), "homeassistant/sensor/%s/config", unique_id);
    snprintf(discovery_payload, sizeof(discovery_payload),
        "{"
            "\"name\": \"%s Humidity\","
            "\"unique_id\": \"%s\","
            "\"stat_t\": \"%s\","
            "\"val_tpl\": \"{{ value_json.h }}\","
            "\"unit_of_meas\": \"%%\","
            "\"dev_cla\": \"humidity\","
            "\"ic\": \"mdi:water-percent\","
            "\"dev\": {\"ids\": [\"%s\"],\"name\": \"%s\",\"mf\": \"ESAP\"}"
        "}",
        device_name, unique_id, state_topic, device_id, device_name);
    esp_mqtt_client_publish(client, discovery_topic, discovery_payload, 0, 1, true);

    // 3. Pressure
    snprintf(unique_id, sizeof(unique_id), "%s_pressure", device_id);
    snprintf(discovery_topic, sizeof(discovery_topic), "homeassistant/sensor/%s/config", unique_id);
    snprintf(discovery_payload, sizeof(discovery_payload),
        "{"
            "\"name\": \"%s Pressure\","
            "\"unique_id\": \"%s\","
            "\"stat_t\": \"%s\","
            "\"val_tpl\": \"{{ value_json.p }}\","
            "\"unit_of_meas\": \"hPa\","
            "\"dev_cla\": \"pressure\","
            "\"ic\": \"mdi:gauge\","
            "\"dev\": {\"ids\": [\"%s\"],\"name\": \"%s\",\"mf\": \"ESAP\"}"
        "}",
        device_name, unique_id, state_topic, device_id, device_name);
    esp_mqtt_client_publish(client, discovery_topic, discovery_payload, 0, 1, true);

    // 4. UV Index
    snprintf(unique_id, sizeof(unique_id), "%s_uv", device_id);
    snprintf(discovery_topic, sizeof(discovery_topic), "homeassistant/sensor/%s/config", unique_id);
    snprintf(discovery_payload, sizeof(discovery_payload),
        "{"
            "\"name\": \"%s UV Index\","
            "\"unique_id\": \"%s\","
            "\"stat_t\": \"%s\","
            "\"val_tpl\": \"{{ value_json.uv }}\","
            "\"unit_of_meas\": \"UV Index\","
            "\"ic\": \"mdi:sun-wireless\","
            "\"dev\": {\"ids\": [\"%s\"],\"name\": \"%s\",\"mf\": \"ESAP\"}"
        "}",
        device_name, unique_id, state_topic, device_id, device_name);
    esp_mqtt_client_publish(client, discovery_topic, discovery_payload, 0, 1, true);

    vTaskDelay(pdMS_TO_TICKS(250)); // Small delay to avoid flooding the broker
}


/**
 * @brief Listens for LoRa messages, parses them, and publishes to MQTT.
 *
 * This task is started once the MQTT client is connected.
 */
void lora_listen_to_mqtt_task(void *arg)
{
    esp_mqtt_client_handle_t client = (esp_mqtt_client_handle_t) arg;

    ESP_LOGI(TAG, "Starting LoRa listen task. Forwarding to MQTT.");
    
    char rx_buf[256];
    int attempt = 0;
    const int MAX_ATTEMPTS = 20;
    
    while (attempt < MAX_ATTEMPTS) {
        ESP_LOGI(TAG, "in mqtt listen task while loop");
        if (lora_wait_for_message(rx_buf, sizeof(rx_buf), 3000)) {
            if (strstr((char*)rx_buf, "+RCV=")) {
                int sender_addr = 10; // extract dynamically if multiple satellites
                // After parsing JSON
                ESP_LOGI(TAG, "Data received from satellite %d: %s", sender_addr, rx_buf);
                lora_send_message(sender_addr, "MM_ACK_DATA");
                ESP_LOGI(TAG, "ACK sent to satellite %d.", sender_addr);

                if (strstr((char*)rx_buf, "+RCV="))  {
                    ESP_LOGI(TAG, "INSIDE PARSING IT");
                    int rssi, snr; // <-- Removed payload_len
                    char *json_payload = NULL;
                    char *token;
                    
                    // Use strtok to parse the comma-separated string
                    // Note: This modifies the 'data' buffer in-place
                    token = strtok((char*)rx_buf + 5, ","); // Skip "+RCV="
                    if (token == NULL) continue;
                    sender_addr = atoi(token);
                    
                    // DELETED THE BLOCK FOR payload_len
            
                    token = strtok(NULL, ","); // This is the start of the payload
                    if (token == NULL) continue;
                    json_payload = token;
    
                    // The payload might contain commas, so we can't use strtok anymore.
                    // We know the length, but the LoRa module doesn't include the
                    // last two fields (RSSI, SNR) if the payload has commas.
                    // Let's assume for now the payload is the rest of the string
                    // until the next comma (which would be RSSI).
                    // A safer way is to find the Nth comma.
                    // For now, let's find the *last* two commas.
    
                    char *rssi_str = strrchr(json_payload, ',');
                    if (rssi_str == NULL) continue;
                    *rssi_str = '\0'; // Terminate the JSON payload string
                    rssi = atoi(rssi_str + 1);
    
                    char *snr_str = strrchr(json_payload, ',');
                    if (snr_str == NULL) continue;
                    *snr_str = '\0'; // Terminate the JSON payload string
                    snr = atoi(snr_str + 1);
    
                    // Now, 'json_payload' points to the clean JSON string
                    ESP_LOGI(TAG, "Parsed message from Address %d (RSSI: %d, SNR: %d)", sender_addr, rssi, snr);
                    ESP_LOGI(TAG, "Payload: %s", json_payload);
    
                    // --- Validate the JSON ---
                    cJSON *root = cJSON_Parse(json_payload);
                    if (root == NULL) {
                        ESP_LOGE(TAG, "Received invalid JSON. Discarding.");
                        continue; // Wait for the next message
                    }
                    cJSON_Delete(root); // We just wanted to check if it's valid
    
                    // --- Dynamically create the state topic ---
                    char state_topic[128];
                    snprintf(state_topic, sizeof(state_topic), "weather/berrystation_%d/state", sender_addr);
    
                    // --- Publish to MQTT ---
                    ESP_LOGI(TAG, "Publishing to MQTT topic: %s", state_topic);
                    esp_mqtt_client_publish(client, state_topic, json_payload, 0, 0, false);
    
                    
                }
            }
        }
        attempt++;
    }
    
}


static void mqtt5_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%" PRIi32, base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;

    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
    
            // --- SENDING DYNAMIC DISCOVERY TOPICS ---
            ESP_LOGI(TAG, "Publishing discovery topics for %d known satellites...", NUM_KNOWN_SATELLITES);
            for (int i = 0; i < NUM_KNOWN_SATELLITES; i++) {
                publish_discovery_for_satellite(client, KNOWN_SATELLITE_ADDRESSES[i]);
            }
            ESP_LOGI(TAG, "Discovery topics published.");

            // --- START THE LORA LISTENER TASK ---
            // We only start this *after* MQTT is connected.
            xTaskCreate(lora_listen_to_mqtt_task, "lora_listen_task", 4096, client, 5, NULL);
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
            //DELETE THIS: ONLY FOR TESTING WITHOUT MQTT:
            // xTaskCreate(lora_listen_to_mqtt_task, "lora_listen_task", 4096, client, 5, NULL);
            // Note: If you get disconnected, you may want to stop/restart the lora_listen_task
            break;
        // ... other event handlers (SUBSCRIBED, PUBLISHED, ERROR) ...
        case MQTT_EVENT_ERROR:
             ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
             // (Your error handling code)
            break;
        default:
            ESP_LOGI(TAG, "Other event id:%d", event->event_id);
            break;
    }
}

static void mqtt5_app_start(void)
{
    esp_mqtt_client_config_t mqtt5_cfg = {
        .broker.address.uri = CONFIG_BROKER_URL,
        .session.protocol_ver = MQTT_PROTOCOL_V_5,
        .network.disable_auto_reconnect = true,
        .credentials.username = "berrymqtt",
        .credentials.authentication.password = "Boilerup321mqtt",
        .session.last_will.topic = "/topic/will",
        .session.last_will.msg = "i will leave",
        .session.last_will.msg_len = 12,
        .session.last_will.qos = 1,
        .session.last_will.retain = true,
    };
    
    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt5_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt5_event_handler, NULL);
    esp_mqtt_client_start(client);
}

// (Your init_log and init_checks functions remain the same)
void init_log()
{
    ESP_LOGI(TAG, "[APP] Startup..");
    ESP_LOGI(TAG, "[APP] Free memory: %" PRIu32 " bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "[APP] IDF version: %s", esp_get_idf_version());

    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("mqtt_client", ESP_LOG_VERBOSE);
    esp_log_level_set("mqtt_example", ESP_LOG_VERBOSE);
    esp_log_level_set("transport_base", ESP_LOG_VERBOSE);
    esp_log_level_set("esp-tls", ESP_LOG_VERBOSE);
    esp_log_level_set("transport", ESP_LOG_VERBOSE);
    esp_log_level_set("outbox", ESP_LOG_VERBOSE);
}

void init_checks()
{
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    /* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
     * Read "Establishing Wi-Fi or Ethernet Connection" section in
     * examples/protocols/README.md for more information about this function.
     */
    // ESP_ERROR_CHECK(example_connect());

    // Create the default Wi-Fi station network interface
    esp_netif_create_default_wifi_sta();

    // Call your new Wi-Fi connection function
    wifi_init_sta();
}

void app_main(void)
{
#if LORA_TEST_ONLY == 1
    // --- LORA TEST MODE ---
    // This code will run if LORA_TEST_ONLY is 1

    ESP_LOGI(TAG, "--- MiddleMan Device Booting (LoRa TEST MODE) ---");

    // 1. Initialize LoRa UART and Reset Module
    ESP_LOGI(TAG, "Initializing LoRa Module...");
    lora_uart_config();
    lora_reset();

    vTaskDelay(pdMS_TO_TICKS(500));
    uart_flush_input(LORA_UART_PORT);

    // 2. Perform common LoRa setup
    ESP_LOGI(TAG, "Setting up LoRa module...\n");
    lora_common_setup(MM_ADDR); 

    ESP_LOGI(TAG, "Performing LoRa boot handshake...");
    if (lora_boot_handshake(true, SAT_ADDR)) {
        ESP_LOGI(TAG, " Handshake successful with satellite 10!");
    } else {
        ESP_LOGW(TAG, " Handshake timeout. Starting listen mode anyway.");
    }
    xTaskCreate(lora_simple_listen_task, "lora_simple_listen_task", 4096, NULL, 5, NULL);
    ESP_LOGI(TAG, "Listening on UART %d", LORA_UART_PORT);

#else
    // --- FULL WI-FI + MQTT MODE ---
    // This original code will run if LORA_TEST_ONLY is 0

    ESP_LOGI(TAG, "--- MiddleMan Device Booting (FULL MODE) ---");

    // 1. Initialize NVS, Netif, and Wi-Fi
    init_log();
    init_checks(); // This blocks until Wi-Fi is connected

    // 2. Initialize LoRa UART and Reset Module
    ESP_LOGI(TAG, "Initializing LoRa Module...");
    lora_uart_config();
    lora_reset();

    // 3. Perform common LoRa setup
    ESP_LOGI(TAG, "Setting up LoRa module...\n");
    lora_common_setup(MM_ADDR); 

    ESP_LOGI(TAG, "Performing LoRa boot handshake...");
    if (lora_boot_handshake(true, SAT_ADDR)) {
        ESP_LOGI(TAG, " Handshake successful with satellite 10!");
    } else {
        ESP_LOGW(TAG, " Handshake timeout. Starting listen mode anyway.");
    }

    // 4. Start the MQTT Client
    ESP_LOGI(TAG, "Starting MQTT Client...");
    mqtt5_app_start();

#endif
}