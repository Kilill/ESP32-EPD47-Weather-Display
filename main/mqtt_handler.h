/**
 * @file mqtt_handler.h
 * @brief MQTT client interface for real-time weather data reception
 * 
 * Provides functions to initialize and manage MQTT connection to weather data broker.
 * Subscribes to multiple topics (temperature, humidity, pressure, wind, rain) and
 * delivers received data via callback function.
 */

#ifndef MQTT_HANDLER_H
#define MQTT_HANDLER_H

#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Callback function type for received MQTT data
 * @param data Pointer to received data string (JSON format)
 * @param len Length of data in bytes
 */
typedef void (*mqtt_data_callback_t)(const char *data, int len);

/**
 * @brief Initialize MQTT client with data callback
 * 
 * Creates MQTT client instance with configuration from config.h/secrets.h.
 * Registers event handlers and prepares for connection.
 * 
 * @param callback Function to call when data is received
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t mqtt_client_init(mqtt_data_callback_t callback);

/**
 * @brief Start MQTT client and connect to broker
 * 
 * Initiates connection to MQTT broker. Actual connection happens asynchronously.
 * 
 * @return ESP_OK on successful start, error code otherwise
 */
esp_err_t mqtt_client_start(void);

/**
 * @brief Stop MQTT client and disconnect
 */
void mqtt_client_stop(void);

/**
 * @brief Check if MQTT client is currently connected
 * 
 * @return true if connected to broker, false otherwise
 */
bool mqtt_client_is_connected(void);

#ifdef __cplusplus
}
#endif

#endif // MQTT_CLIENT_H
