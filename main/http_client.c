/**
 * @file http_client.c
 * @brief HTTPS client implementation for historical data fetching
 * 
 * Implements HTTPS client using ESP-IDF esp_http_client with TLS support.
 * Handles chunked transfer encoding and accumulates complete response before
 * invoking callback. Includes watchdog timer resets during long operations.
 */

#include "http_client.h"
#include "config.h"
#include "esp_http_client.h"
#include "esp_crt_bundle.h"
#include "esp_log.h"
#include "esp_task_wdt.h"
#include "esp_timer.h"
#include <string.h>
#include <stdlib.h>

static const char *TAG = "http_client";

#define MAX_HTTP_OUTPUT_BUFFER HTTP_BUFFER_SIZE

typedef struct {
    char *buffer;
    int buffer_len;
    int max_buffer_size;
    bool truncated;
    http_data_callback_t callback;
} http_client_context_t;

/**
 * @brief HTTP event handler callback
 * 
 * Processes HTTP client events:
 * - ERROR: Logs error
 * - ON_CONNECTED: Resets watchdog after TLS handshake
 * - HEADER_SENT: Resets watchdog after sending request
 * - ON_HEADER: Logs response headers (debug)
 * - ON_DATA: Accumulates response data in buffer, resets watchdog
 * - ON_FINISH: Null-terminates buffer and calls user callback
 * - DISCONNECTED: Logs disconnection
 * 
 * @param evt HTTP client event structure
 * @return ESP_OK
 */
static esp_err_t http_event_handler(esp_http_client_event_t *evt) {
    http_client_context_t *ctx = (http_client_context_t *)evt->user_data;
    
    switch(evt->event_id) {
        case HTTP_EVENT_ERROR:
            ESP_LOGE(TAG, "HTTP_EVENT_ERROR");
            break;
            
        case HTTP_EVENT_ON_CONNECTED:
            //ESP_LOGI(TAG, "HTTP_EVENT_ON_CONNECTED (TLS handshake complete)");
            esp_task_wdt_reset();  // Reset watchdog after connection established
            break;
            
        case HTTP_EVENT_HEADER_SENT:
            //ESP_LOGI(TAG, "HTTP_EVENT_HEADER_SENT");
            esp_task_wdt_reset();  // Reset watchdog after headers sent
            break;
            
        case HTTP_EVENT_ON_HEADER:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", 
                     evt->header_key, evt->header_value);
            break;
            
        case HTTP_EVENT_ON_DATA:
            //ESP_LOGI(TAG, "HTTP_EVENT_ON_DATA, len=%d, total=%d/%d", 
                     //evt->data_len, ctx->buffer_len + evt->data_len, ctx->max_buffer_size);
            esp_task_wdt_reset();  // Reset watchdog as data is received
            // Accumulate data in buffer (handle both chunked and non-chunked responses)
            if (ctx->buffer_len + evt->data_len <= ctx->max_buffer_size) {
                memcpy(ctx->buffer + ctx->buffer_len, evt->data, evt->data_len);
                ctx->buffer_len += evt->data_len;
            } else {
                ESP_LOGE(TAG, "Response buffer overflow! Attempting to write %d bytes, buffer %d/%d",
                         evt->data_len, ctx->buffer_len, ctx->max_buffer_size);
                ctx->truncated = true;
            }
            break;
            
        case HTTP_EVENT_ON_FINISH:
            //ESP_LOGI(TAG, "HTTP_EVENT_ON_FINISH, buffer_len=%d, truncated=%d", ctx->buffer_len, ctx->truncated);
            // Null-terminate the buffer
            if (ctx->buffer_len < ctx->max_buffer_size) {
                ctx->buffer[ctx->buffer_len] = '\0';
            }
            // Log response content for debugging 404 errors
            if (ctx->buffer_len > 0 && ctx->buffer_len < 500) {
                ESP_LOGW(TAG, "Response content: %s", ctx->buffer);
            }
            // Only call callback if data is complete
            if (ctx->truncated) {
                ESP_LOGE(TAG, "Response was truncated, skipping callback to prevent parsing incomplete JSON");
            } else if (ctx->callback != NULL && ctx->buffer_len > 0) {
                //ESP_LOGI(TAG, "Calling callback at %p with data at %p, len=%d", 
                         //ctx->callback, ctx->buffer, ctx->buffer_len);
                ctx->callback(ctx->buffer, ctx->buffer_len);
                //ESP_LOGI(TAG, "Callback completed");
            }
            break;
            
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGD(TAG, "HTTP_EVENT_DISCONNECTED");
            break;
            
        default:
            break;
    }
    return ESP_OK;
}

esp_err_t http_client_init(void) {
    //ESP_LOGI(TAG, "HTTP client initialized");
    return ESP_OK;
}

esp_err_t http_client_fetch_history(http_data_callback_t callback) {
    esp_err_t ret = ESP_OK;
    const int max_retries = 5;
    const int retry_delay_ms = 1000;
    
    // Log available memory before attempting connection
    //ESP_LOGI(TAG, "Free heap before HTTPS: %lu bytes (internal: %lu, PSRAM: %lu)",
             //esp_get_free_heap_size(),
             //heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
             //heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
    
    // Allocate buffer for response
    char *buffer = malloc(MAX_HTTP_OUTPUT_BUFFER);
    if (buffer == NULL) {
        ESP_LOGE(TAG, "Failed to allocate response buffer");
        return ESP_ERR_NO_MEM;
    }
    
    http_client_context_t context = {
        .buffer = buffer,
        .buffer_len = 0,
        .max_buffer_size = MAX_HTTP_OUTPUT_BUFFER,
        .truncated = false,
        .callback = callback
    };
    
    //int64_t start_time = esp_timer_get_time();
    
    // Build full URL with API path
    char url[256];
    snprintf(url, sizeof(url), "%s%s", WEBSERVER_URL, HTTP_HISTORY_ENDPOINT);
    
    //ESP_LOGI(TAG, "Fetching historical data from: %s", url);
    
    esp_http_client_config_t config = {
        .url = url,
        .method = HTTP_METHOD_GET,
        .event_handler = http_event_handler,
        .user_data = &context,
        .timeout_ms = HTTP_TIMEOUT_MS,
        .is_async = false,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client == NULL) {
        ESP_LOGE(TAG, "Failed to initialize HTTP client");
        free(buffer);
        return ESP_FAIL;
    }
    
    // Retry loop with backoff
    for (int retry = 0; retry < max_retries; retry++) {
        if (retry > 0) {
            ESP_LOGW(TAG, "Retry attempt %d/%d for historical data after %dms delay", 
                     retry + 1, max_retries, retry_delay_ms);
            vTaskDelay(pdMS_TO_TICKS(retry_delay_ms));
            esp_task_wdt_reset();
            // Reset context for retry
            context.buffer_len = 0;
            context.truncated = false;
        }
        
        //int64_t init_time = esp_timer_get_time();
        //ESP_LOGI(TAG, "Client init took %lld ms", (init_time - start_time) / 1000);
        
        //ESP_LOGI(TAG, "Starting HTTPS request...");
        ret = esp_http_client_perform(client);
        
        //int64_t perform_time = esp_timer_get_time();
        //ESP_LOGI(TAG, "Request took %lld ms", (perform_time - init_time) / 1000);
        if (ret == ESP_OK) {
            int status = esp_http_client_get_status_code(client);
            //int length = esp_http_client_get_content_length(client);
            //ESP_LOGI(TAG, "HTTP GET Status = %d, content_length = %d", status, length);
            
            if (status == 200) {
                // Success - break out of retry loop
                break;
            } else {
                ESP_LOGE(TAG, "HTTP request failed with status: %d", status);
                ret = ESP_FAIL;
            }
        } else {
            ESP_LOGE(TAG, "HTTP GET request failed: %s", esp_err_to_name(ret));
        }
    }
    
    esp_http_client_cleanup(client);
    free(buffer);
    
    return ret;
}

esp_err_t http_client_fetch_current(http_data_callback_t callback) {
    esp_err_t ret = ESP_OK;
    const int max_retries = 5;
    const int retry_delay_ms = 1000;
    
    // Allocate buffer for current weather
    // Note: This endpoint may return more data than expected - increase if needed
    const int current_buffer_size = 32768;  // 32KB - endpoint returns historical data
    char *buffer = malloc(current_buffer_size);
    if (buffer == NULL) {
        ESP_LOGE(TAG, "Failed to allocate response buffer");
        return ESP_ERR_NO_MEM;
    }
    
    http_client_context_t context = {
        .buffer = buffer,
        .buffer_len = 0,
        .max_buffer_size = current_buffer_size,
        .truncated = false,
        .callback = callback
    };
    
    // Build full URL with API path
    char url[256];
    snprintf(url, sizeof(url), "%s%s", WEBSERVER_URL, HTTP_CURRENT_ENDPOINT);
    
    //ESP_LOGI(TAG, "Fetching current weather data from: %s", url);
    
    esp_http_client_config_t config = {
        .url = url,
        .method = HTTP_METHOD_GET,
        .event_handler = http_event_handler,
        .user_data = &context,
        .timeout_ms = 10000,  // Shorter timeout for current data
        .is_async = false,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client == NULL) {
        ESP_LOGE(TAG, "Failed to initialize HTTP client");
        free(buffer);
        return ESP_FAIL;
    }
    
    // Retry loop with backoff
    for (int retry = 0; retry < max_retries; retry++) {
        if (retry > 0) {
            ESP_LOGW(TAG, "Retry attempt %d/%d for current weather after %dms delay", 
                     retry + 1, max_retries, retry_delay_ms);
            vTaskDelay(pdMS_TO_TICKS(retry_delay_ms));
            esp_task_wdt_reset();
            // Reset context for retry
            context.buffer_len = 0;
            context.truncated = false;
        }
        
        ret = esp_http_client_perform(client);
        
        if (ret == ESP_OK) {
            int status = esp_http_client_get_status_code(client);
            //ESP_LOGI(TAG, "HTTP GET Status = %d", status);
            
            if (status == 200) {
                // Success - break out of retry loop
                break;
            } else {
                ESP_LOGE(TAG, "HTTP request failed with status: %d", status);
                ret = ESP_FAIL;
            }
        } else {
            ESP_LOGE(TAG, "HTTP GET request failed: %s", esp_err_to_name(ret));
        }
    }
    
    esp_http_client_cleanup(client);
    free(buffer);
    
    return ret;
}
