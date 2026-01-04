/**
 * @file wifi_manager.h
 * @brief WiFi connection management interface
 * 
 * Provides functions to initialize WiFi in station mode, connect to configured AP,
 * monitor connection status, and disconnect when needed. Uses FreeRTOS event groups
 * for connection state management and automatic retry logic.
 */

#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include "esp_err.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize WiFi subsystem
 * 
 * Sets up WiFi in station mode, creates event handlers for connection events,
 * and prepares the system for WiFi connection.
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t wifi_manager_init(void);

/**
 * @brief Connect to configured WiFi access point
 * 
 * Attempts to connect to the AP specified in config.h/secrets.h.
 * Will retry up to MAX_RETRY_COUNT times on failure. Disables power
 * saving mode for stable connection during long operations.
 * 
 * @return ESP_OK on successful connection, ESP_FAIL or ESP_ERR_TIMEOUT on failure
 */
esp_err_t wifi_manager_connect(void);

/**
 * @brief Check if WiFi is currently connected
 * 
 * @return true if connected to AP and have IP address, false otherwise
 */
bool wifi_manager_is_connected(void);

/**
 * @brief Disconnect from WiFi access point
 */
void wifi_manager_disconnect(void);

#ifdef __cplusplus
}
#endif

#endif // WIFI_MANAGER_H
