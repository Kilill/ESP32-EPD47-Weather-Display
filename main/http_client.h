/**
 * @file http_client.h
 * @brief HTTPS client interface for fetching historical weather data
 * 
 * Provides functions to fetch historical weather data from HTTP(S) API endpoint.
 * Supports large responses (up to 64KB) with chunked transfer encoding.
 */

#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Callback function type for received HTTP data
 * @param data Pointer to complete response data
 * @param len Length of data in bytes
 */
typedef void (*http_data_callback_t)(const char *data, int len);

/**
 * @brief Initialize HTTP client subsystem
 * 
 * @return ESP_OK (currently always succeeds)
 */
esp_err_t http_client_init(void);

/**
 * @brief Fetch historical weather data from configured endpoint
 * 
 * Performs HTTPS GET request to HTTP_HISTORY_ENDPOINT (configured in config.h).
 * Accumulates entire response in memory before calling callback. Includes
 * watchdog resets during download to prevent timeout on large responses.
 * 
 * @param callback Function to call with complete response data
 * @return ESP_OK on successful fetch, error code otherwise
 */
esp_err_t http_client_fetch_history(http_data_callback_t callback);

/**
 * @brief Fetch current weather data from configured endpoint
 * 
 * Performs HTTPS GET request to HTTP_CURRENT_ENDPOINT (configured in config.h).
 * Accumulates entire response in memory before calling callback.
 * 
 * @param callback Function to call with complete response data
 * @return ESP_OK on successful fetch, error code otherwise
 */
esp_err_t http_client_fetch_current(http_data_callback_t callback);

#ifdef __cplusplus
}
#endif

#endif // HTTP_CLIENT_H
