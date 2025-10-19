/*
 * SPDX-FileCopyrightText: 2022-2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "esp_random.h"
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "driver/uart.h"
// #include "protocol_examples_common.h"
#include "esp_log.h"
#include "cJSON.h"
#include "mqtt_client.h"

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_event.h"

// These macros will use the values you set in menuconfig
#define WIFI_SSID     "NETGEAR31"
#define WIFI_PASS     "Boilerup321"
#define CONFIG_BROKER_URL "mqtt://192.168.1.5:1883"
#define RYLR998_UART_PORT    UART_NUM_1
#define RYLR998_TXD_PIN      GPIO_NUM_17   // ESP32 TX → module RX
#define RYLR998_RXD_PIN      GPIO_NUM_16   // ESP32 RX ← module TX
#define RYLR998_NRST_PIN     GPIO_NUM_4    // (active low) reset pin

static EventGroupHandle_t s_wifi_event_group;
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static int s_retry_num = 0;

#define MM_BUF_SIZE 4096
static const char *TAG = "mqtt_berryWeather";

static void log_error_if_nonzero(const char *message, int error_code)
{
    if (error_code != 0) {
        ESP_LOGE(TAG, "Last error %s: 0x%x", message, error_code);
    }
}

static esp_mqtt5_user_property_item_t user_property_arr[] = {
        {"board", "esp32"},
        {"u", "user"},
        {"p", "password"}
    };

#define USE_PROPERTY_ARR_SIZE   sizeof(user_property_arr)/sizeof(esp_mqtt5_user_property_item_t)

static esp_mqtt5_publish_property_config_t publish_property = {
    .payload_format_indicator = 1,
    .message_expiry_interval = 1000,
    .topic_alias = 0,
    .response_topic = "/topic/test/response",
    .correlation_data = "123456",
    .correlation_data_len = 6,
};

static void print_user_property(mqtt5_user_property_handle_t user_property)
{
    if (user_property) {
        uint8_t count = esp_mqtt5_client_get_user_property_count(user_property);
        if (count) {
            esp_mqtt5_user_property_item_t *item = malloc(count * sizeof(esp_mqtt5_user_property_item_t));
            if (esp_mqtt5_client_get_user_property(user_property, item, &count) == ESP_OK) {
                for (int i = 0; i < count; i ++) {
                    esp_mqtt5_user_property_item_t *t = &item[i];
                    ESP_LOGI(TAG, "key is %s, value is %s", t->key, t->value);
                    free((char *)t->key);
                    free((char *)t->value);
                }
            }
            free(item);
        }
    }
}

static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
    esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
    if (s_retry_num < 5) { // Retry up to 5 times
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
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_sta finished.");

    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "Connected to AP SSID:%s", WIFI_SSID);
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGW(TAG, "Failed to connect to SSID:%s", WIFI_SSID);
    } else {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
    }
}

/*
ACTUAL CODE STARTS HERE: 
*/
/*
Helper functions:
*/

void handle_mqtt_error(esp_mqtt_event_handle_t event)
{
    ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
    print_user_property(event->property->user_property);
    ESP_LOGI(TAG, "MQTT5 return code is %d", event->error_handle->connect_return_code);
    if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
        log_error_if_nonzero("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
        log_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err);
        log_error_if_nonzero("captured as transport's socket errno",  event->error_handle->esp_transport_sock_errno);
        ESP_LOGI(TAG, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));
    }
}

void publish_temperature_discovery_message(esp_mqtt_client_handle_t client)
{
    // Step 1: Define the discovery topic from your slide
    const char *discovery_topic = "homeassistant/sensor/berrystation_1_temperature/config";

    // Step 2: Define the JSON payload from your slide
    // C automatically combines these strings into one.
    const char *discovery_payload =
        "{"
            "\"name\": \"Station 1 Temperature\","
            "\"unique_id\": \"berrystation_1_temperature\","
            "\"stat_t\": \"weather/berrystation_1/state\"," // State topic
            "\"val_tpl\": \"{{ value_json.t }}\","         // Template to get the value
            "\"unit_of_meas\": \"°F\","
            "\"dev_cla\": \"temperature\","
            "\"ic\": \"mdi:thermometer\","
            "\"dev\": {"
                "\"ids\": [\"berrystation_1\"],"
                "\"name\": \"BerryWeather Station 1\","
                "\"mf\": \"ESAP\""
            "}"
        "}";

    // Step 3: Publish the message with the RETAIN flag set to true
    ESP_LOGI(TAG, "Publishing discovery message to topic: %s", discovery_topic);
    int msg_id = esp_mqtt_client_publish(client, 
                                         discovery_topic, 
                                         discovery_payload, 
                                         0,     // length 0 means use strlen
                                         1,     // QoS level 1
                                         true); // RETAIN = true is critical!

    if (msg_id != -1) {
        ESP_LOGI(TAG, "Discovery message sent successfully, msg_id=%d", msg_id);
    } else {
        ESP_LOGE(TAG, "Failed to send discovery message.");
    }
}

void publish_humidity_discovery_message(esp_mqtt_client_handle_t client)
{
    // Topic from your slide
    const char *discovery_topic = "homeassistant/sensor/berrystation_1_humidity/config";

    // JSON payload from your slide
    const char *discovery_payload =
        "{"
            "\"name\": \"Station 1 Humidity\","
            "\"unique_id\": \"berrystation_1_humidity\","
            "\"stat_t\": \"weather/berrystation_1/state\","
            "\"val_tpl\": \"{{ value_json.h }}\"," // Extracts the 'h' field
            "\"unit_of_meas\": \"%\","
            "\"dev_cla\": \"humidity\","
            "\"ic\": \"mdi:water-percent\","
            "\"dev\": {"
                "\"ids\": [\"berrystation_1\"],"
                "\"name\": \"BerryWeather Station 1\","
                "\"mf\": \"ESAP\""
            "}"
        "}";

    ESP_LOGI(TAG, "Publishing humidity discovery message...");
    esp_mqtt_client_publish(client, discovery_topic, discovery_payload, 0, 1, true);
}

void publish_pressure_discovery_message(esp_mqtt_client_handle_t client)
{
    const char *discovery_topic = "homeassistant/sensor/berrystation_1_pressure/config";

    const char *discovery_payload =
        "{"
            "\"name\": \"Station 1 Pressure\","
            "\"unique_id\": \"berrystation_1_pressure\","
            "\"stat_t\": \"weather/berrystation_1/state\","
            "\"val_tpl\": \"{{ value_json.p }}\"," // Extracts the 'p' field
            "\"unit_of_meas\": \"hPa\","           // Unit: hectopascals
            "\"dev_cla\": \"pressure\","
            "\"ic\": \"mdi:gauge\","               // Icon: a pressure gauge
            "\"dev\": {"
                "\"ids\": [\"berrystation_1\"],"
                "\"name\": \"BerryWeather Station 1\","
                "\"mf\": \"ESAP\""
            "}"
        "}";

    ESP_LOGI(TAG, "Publishing pressure discovery message...");
    esp_mqtt_client_publish(client, discovery_topic, discovery_payload, 0, 1, true);
}

void publish_uv_discovery_message(esp_mqtt_client_handle_t client)
{
    const char *discovery_topic = "homeassistant/sensor/berrystation_1_uv/config";

    const char *discovery_payload =
        "{"
            "\"name\": \"Station 1 UV Index\","
            "\"unique_id\": \"berrystation_1_uv\","
            "\"stat_t\": \"weather/berrystation_1/state\","
            "\"val_tpl\": \"{{ value_json.uv }}\","  // Extracts the 'uv' field
            "\"unit_of_meas\": \"UV Index\","
            // No device class for UV index, so we omit it.
            "\"ic\": \"mdi:sun-wireless\","          // Icon: sun with rays
            "\"dev\": {"
                "\"ids\": [\"berrystation_1\"],"
                "\"name\": \"BerryWeather Station 1\","
                "\"mf\": \"ESAP\""
            "}"
        "}";

    ESP_LOGI(TAG, "Publishing UV discovery message...");
    esp_mqtt_client_publish(client, discovery_topic, discovery_payload, 0, 1, true);
}

/*
Constantly listens for updates on LoRA.
As soon as there's an update -> Decodes and Understands what the update is -> Publishes a state topic 

*/
void listen_task(void *arg)
{
    esp_mqtt_client_handle_t client = (esp_mqtt_client_handle_t) arg;

    // listen for incoming messages
    uint8_t data[MM_BUF_SIZE];
    while (1) {
        int len = uart_read_bytes(RYLR998_UART_PORT, data, MM_BUF_SIZE - 1, pdMS_TO_TICKS(1000));
        if (len > 0) {
            data[len] = '\0';
            printf("Received: %s\n", data);
        }

        //Parse the message
        cJSON *root = cJSON_Parse((char*)data);
        if (!root) continue;

        // --- Always publish the state update ---
        const char *state_topic = "weather/berrystation_1/state";
        
        esp_mqtt_client_publish(client, state_topic, (const char *)data, 0, 0, false);        
        cJSON_Delete(root);
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}



static void mqtt5_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%" PRIi32, base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;

    ESP_LOGD(TAG, "free heap size is %" PRIu32 ", minimum %" PRIu32, esp_get_free_heap_size(), esp_get_minimum_free_heap_size());
    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
    
            //SENDING DISCOVERY TOPICS:
            publish_temperature_discovery_message(client);
            publish_humidity_discovery_message(client);
            publish_pressure_discovery_message(client);
            publish_uv_discovery_message(client);

            //SWITCHING TO SENSOR UPDATE MODE:
            xTaskCreate(listen_task, "listen_task", 4096, client, 5, NULL);
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
            print_user_property(event->property->user_property);
            break;
        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
            print_user_property(event->property->user_property);
            esp_mqtt5_client_set_publish_property(client, &publish_property);
            msg_id = esp_mqtt_client_publish(client, "/topic/qos0", "data", 0, 0, 0);
            ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
            break;
        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            print_user_property(event->property->user_property);
            break;
        case MQTT_EVENT_ERROR:
            handle_mqtt_error(event);
            break;
        //MQTT_EVENT_DATA -> ADD if need to receive msgs back from home assistant to middleman.
        default:
            ESP_LOGI(TAG, "Other event id:%d", event->event_id);
            break;
    }
}

static void mqtt5_app_start(void)
{
    ////////////CONFIG///////////////////
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
    /////////////////////////////
    
    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt5_cfg);
    /* The last argument may be used to pass data to the event handler, in this example mqtt_event_handler */
    //Can pass special structs ("tools") that the event handler can use.
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt5_event_handler, NULL); //registering the event handler before client starts.
    //START CLIENT:
    esp_mqtt_client_start(client);
}

/*
init_log: initializes logs at startup
*/
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

/*
init_checks: initializes and checks NVS, TCP/IP, WiFi connection, and creates default event loop at startup
*/
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
    init_log();
    init_checks();
    mqtt5_app_start();
}
