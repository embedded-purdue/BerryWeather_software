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
#include "protocol_examples_common.h"
#include "esp_log.h"
#include "mqtt_client.h"

static const char *TAG = "mqtt5_example";

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

static esp_mqtt5_subscribe_property_config_t subscribe_property = {
    .subscribe_id = 25555,
    .no_local_flag = false,
    .retain_as_published_flag = false,
    .retain_handle = 0,
    .is_share_subscribe = true,
    .share_name = "group1",
};

static esp_mqtt5_subscribe_property_config_t subscribe1_property = {
    .subscribe_id = 25555,
    .no_local_flag = true,
    .retain_as_published_flag = false,
    .retain_handle = 0,
};

static esp_mqtt5_unsubscribe_property_config_t unsubscribe_property = {
    .is_share_subscribe = true,
    .share_name = "group1",
};

static esp_mqtt5_disconnect_property_config_t disconnect_property = {
    .session_expiry_interval = 60,
    .disconnect_reason = 0,
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


/*
Helper functions:
*/
void handle_mqtt_event_data(esp_mqtt_event_handle_t event)
{
    ESP_LOGI(TAG, "MQTT_EVENT_DATA");
    print_user_property(event->property->user_property);
    ESP_LOGI(TAG, "payload_format_indicator is %d", event->property->payload_format_indicator);
    ESP_LOGI(TAG, "response_topic is %.*s", event->property->response_topic_len, event->property->response_topic);
    ESP_LOGI(TAG, "correlation_data is %.*s", event->property->correlation_data_len, event->property->correlation_data);
    ESP_LOGI(TAG, "content_type is %.*s", event->property->content_type_len, event->property->content_type);
    ESP_LOGI(TAG, "TOPIC=%.*s", event->topic_len, event->topic);
    ESP_LOGI(TAG, "DATA=%.*s", event->data_len, event->data);
}

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
ACTUAL CODE STARTS HERE: 
*/
static void temperature_task(void *arg)
{
    esp_mqtt_client_handle_t client = (esp_mqtt_client_handle_t) arg;
    char payload[64];
    int counter = 0;

    while (1) {
        // Simulate data (replace with real sensor read later)
        float temp = 25.0 + (esp_random() % 100) / 10.0;     // 25.0 – 34.9 °C
        float humidity = 50.0 + (esp_random() % 200) / 10.0; // 50.0 – 69.9 %

        // Format as CSV: timestamp,temp,humidity
        snprintf(payload, sizeof(payload), "%d,%.2f,%.2f", counter++, temp, humidity);

        // Publish to topic
        int msg_id = esp_mqtt_client_publish(client, "/sensor/temperature", payload, 0, 1, 0);
        ESP_LOGI(TAG, "Published CSV: %s (msg_id=%d)", payload, msg_id);

        vTaskDelay(pdMS_TO_TICKS(5000)); // send every 5 seconds
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
        xTaskCreate(temperature_task, "temperature_task", 4096, client, 5, NULL);
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
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        print_user_property(event->property->user_property);
        esp_mqtt5_client_set_user_property(&disconnect_property.user_property, user_property_arr, USE_PROPERTY_ARR_SIZE);
        esp_mqtt5_client_set_disconnect_property(client, &disconnect_property);
        esp_mqtt5_client_delete_user_property(disconnect_property.user_property);
        disconnect_property.user_property = NULL;
        esp_mqtt_client_disconnect(client);
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        print_user_property(event->property->user_property);
        break;
    case MQTT_EVENT_DATA:
        handle_mqtt_event_data(event);
        break;
    case MQTT_EVENT_ERROR:
        handle_mqtt_error(event);
        break;
    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
}

static void mqtt5_app_start(void)
{
    ////////////CONFIG///////////////////
    esp_mqtt5_connection_property_config_t connect_property = {
        .session_expiry_interval = 10,
        .maximum_packet_size = 1024,
        .receive_maximum = 65535,
        .topic_alias_maximum = 2,
        .request_resp_info = true,
        .request_problem_info = true,
        .will_delay_interval = 10,
        .payload_format_indicator = true,
        .message_expiry_interval = 10,
        .response_topic = "/test/response",
        .correlation_data = "123456",
        .correlation_data_len = 6,
    };

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

    //Sets the connection and user properties -> mallocs memory.
    esp_mqtt5_client_set_user_property(&connect_property.user_property, user_property_arr, USE_PROPERTY_ARR_SIZE);
    esp_mqtt5_client_set_user_property(&connect_property.will_user_property, user_property_arr, USE_PROPERTY_ARR_SIZE);
    
    //Makes a copy and sets those properties to the client
    esp_mqtt5_client_set_connect_property(client, &connect_property);

    //Now the properties are set, so need to free the memory allocated for connect property
    esp_mqtt5_client_delete_user_property(connect_property.user_property);
    esp_mqtt5_client_delete_user_property(connect_property.will_user_property);

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
    ESP_ERROR_CHECK(example_connect());
}

void app_main(void)
{
    init_log();
    init_checks();
    mqtt5_app_start();
}
