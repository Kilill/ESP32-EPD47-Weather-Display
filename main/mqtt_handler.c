/**
 * @file mqtt_handler.c
 * @brief MQTT client implementation for weather data reception
 * 
 * Manages MQTT connection lifecycle, subscribes to weather-related topics,
 * and processes incoming messages by wrapping them with topic information
 * before calling the registered callback.
 */

#include "mqtt_handler.h"
#include "config.h"
#include "esp_log.h"
#include "esp_event.h"
#include "mqtt_client.h"  // ESP MQTT component header
#include <string.h>

static const char *TAG = "mqtt_client";

static esp_mqtt_client_handle_t s_mqtt_client = NULL;
static mqtt_data_callback_t s_data_callback = NULL;
static bool s_is_connected = false;

/**
 * @brief MQTT event handler callback
 * 
 * Processes MQTT events:
 * - CONNECTED: Subscribes to all weather topics
 * - DISCONNECTED: Updates connection status
 * - SUBSCRIBED: Logs subscription confirmation
 * - DATA: Wraps data with topic name in JSON and calls user callback
 * - ERROR: Logs error details
 * 
 * @param handler_args User arguments (unused)
 * @param base Event base
 * @param event_id Specific event ID
 * @param event_data Event-specific data
 */
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, 
                               int32_t event_id, void *event_data) {
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;
    
    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_CONNECTED:
            //ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
            s_is_connected = true;
            // Subscribe to all weather topics
            esp_mqtt_client_subscribe(s_mqtt_client, MQTT_TOPIC_TEMP, MQTT_QOS_LEVEL);
            //ESP_LOGI(TAG, "Subscribed to: %s", MQTT_TOPIC_TEMP);
            
            esp_mqtt_client_subscribe(s_mqtt_client, MQTT_TOPIC_HUMIDITY, MQTT_QOS_LEVEL);
            //ESP_LOGI(TAG, "Subscribed to: %s", MQTT_TOPIC_HUMIDITY);
            
            esp_mqtt_client_subscribe(s_mqtt_client, MQTT_TOPIC_PRESSURE, MQTT_QOS_LEVEL);
            //ESP_LOGI(TAG, "Subscribed to: %s", MQTT_TOPIC_PRESSURE);
            
            esp_mqtt_client_subscribe(s_mqtt_client, MQTT_TOPIC_WIND, MQTT_QOS_LEVEL);
            //ESP_LOGI(TAG, "Subscribed to: %s", MQTT_TOPIC_WIND);
            
            esp_mqtt_client_subscribe(s_mqtt_client, MQTT_TOPIC_RAIN, MQTT_QOS_LEVEL);
            //ESP_LOGI(TAG, "Subscribed to: %s", MQTT_TOPIC_RAIN);
            break;
            
        case MQTT_EVENT_DISCONNECTED:
            //ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
            s_is_connected = false;
            break;
            
        case MQTT_EVENT_SUBSCRIBED:
            //ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
            break;
            
        case MQTT_EVENT_DATA:
            ESP_LOGD(TAG, "MQTT_EVENT_DATA");
            ESP_LOGD(TAG, "TOPIC=%.*s", event->topic_len, event->topic);
            ESP_LOGD(TAG, "DATA=%.*s", event->data_len, event->data);
            
            // Call the data callback if registered
            if (s_data_callback != NULL && event->data_len > 0 && event->topic_len > 0) {
                // Create null-terminated strings for topic and data
                char *topic_str = malloc(event->topic_len + 1);
                char *data_str = malloc(event->data_len + 1);
                
                if (topic_str && data_str) {
                    memcpy(topic_str, event->topic, event->topic_len);
                    topic_str[event->topic_len] = '\0';
                    
                    memcpy(data_str, event->data, event->data_len);
                    data_str[event->data_len] = '\0';
                    
                    // Pass both topic and data to callback
                    // Allocate enough space for JSON wrapper: {"topic":"...","data":...}
                    size_t combined_len = strlen(topic_str) + strlen(data_str) + 30;
                    char *combined = malloc(combined_len);
                    if (combined) {
                        snprintf(combined, combined_len, "{\"topic\":\"%s\",\"data\":%s}", topic_str, data_str);
                        ESP_LOGD(TAG, "Combined JSON: %s", combined);
                        s_data_callback(combined, strlen(combined));
                        free(combined);
                    }
                    
                    free(topic_str);
                    free(data_str);
                }
            }
            break;
            
        case MQTT_EVENT_ERROR:
            ESP_LOGE(TAG, "MQTT_EVENT_ERROR");
            if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
                ESP_LOGE(TAG, "Last error code reported from esp-tls: 0x%x", 
                         event->error_handle->esp_tls_last_esp_err);
                ESP_LOGE(TAG, "Last tls stack error number: 0x%x", 
                         event->error_handle->esp_tls_stack_err);
            }
            break;
            
        default:
            ESP_LOGD(TAG, "Other event id:%d", event->event_id);
            break;
    }
}

esp_err_t mqtt_client_init(mqtt_data_callback_t callback) {
    s_data_callback = callback;
    
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = MQTT_BROKER_URI,
        .broker.address.port = MQTT_BROKER_PORT,
        .task.stack_size = 6144,  // Increase stack size from default 4096
        .task.priority = 5,
        .buffer.size = 2048,  // Reduced to save memory for HTTPS/TLS
        .buffer.out_size = 1024,
    };
    
    // Add username/password if configured
    if (strlen(MQTT_USERNAME) > 0) {
        mqtt_cfg.credentials.username = MQTT_USERNAME;
    }
    if (strlen(MQTT_PASSWORD) > 0) {
        mqtt_cfg.credentials.authentication.password = MQTT_PASSWORD;
    }
    
    s_mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    if (s_mqtt_client == NULL) {
        ESP_LOGE(TAG, "Failed to initialize MQTT client");
        return ESP_FAIL;
    }
    
    ESP_ERROR_CHECK(esp_mqtt_client_register_event(s_mqtt_client, ESP_EVENT_ANY_ID, 
                                                   mqtt_event_handler, NULL));
    
    //ESP_LOGI(TAG, "MQTT client initialized");
    return ESP_OK;
}

esp_err_t mqtt_client_start(void) {
    if (s_mqtt_client == NULL) {
        ESP_LOGE(TAG, "MQTT client not initialized");
        return ESP_ERR_INVALID_STATE;
    }
    
    esp_err_t ret = esp_mqtt_client_start(s_mqtt_client);
    if (ret == ESP_OK) {
        //ESP_LOGI(TAG, "MQTT client started");
    } else {
        ESP_LOGE(TAG, "Failed to start MQTT client");
    }
    return ret;
}

void mqtt_client_stop(void) {
    if (s_mqtt_client != NULL) {
        esp_mqtt_client_stop(s_mqtt_client);
        s_is_connected = false;
        //ESP_LOGI(TAG, "MQTT client stopped");
    }
}

bool mqtt_client_is_connected(void) {
    return s_is_connected;
}
