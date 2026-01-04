/**
 * @file WeatherData.cpp
 * @brief Historical weather data management implementation
 * 
 * Fetches historical data via HTTP, parses JSON responses, and converts to
 * chart-ready format. Uses static PSRAM buffer for large HTTP responses and
 * caches last successful fetch for resilience against temporary failures.
 */

#include "WeatherData.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <cstdlib>
#include <cstring>

extern "C" {
#include "cJSON.h"
#include "esp_task_wdt.h"
#include "wifi_manager.h"
}

const char* WeatherData::TAG = "WeatherData";

// Static storage for HTTP response
char* WeatherData::http_response_buffer = nullptr;
int WeatherData::http_response_length = 0;

WeatherData& WeatherData::getInstance() {
    static WeatherData instance;
    
    // Allocate buffer on first use (in PSRAM if available)
    if (http_response_buffer == nullptr) {
        http_response_buffer = (char*)heap_caps_malloc(max_response_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        if (http_response_buffer == nullptr) {
            // Fall back to regular heap if PSRAM not available
            http_response_buffer = (char*)malloc(max_response_size);
            ESP_LOGW(TAG, "Allocated HTTP buffer in regular heap");
        } else {
            //ESP_LOGI(TAG, "Allocated HTTP buffer in PSRAM");
        }
    }
    
    return instance;
}

void WeatherData::httpResponseCallback(const char *data, int len) {
    if (http_response_buffer == nullptr) {
        ESP_LOGE(TAG, "HTTP response buffer not allocated");
        return;
    }
    
    // Copy data to our static buffer
    if (len > 0 && len < max_response_size) {
        memcpy(http_response_buffer, data, len);
        http_response_buffer[len] = '\0'; // Null terminate
        http_response_length = len;
        //ESP_LOGI(TAG, "Received and copied HTTP response: %d bytes", len);
    } else if (len >= max_response_size) {
        ESP_LOGE(TAG, "HTTP response too large: %d bytes (max: %d)", len, max_response_size);
        http_response_length = 0;
    } else {
        ESP_LOGW(TAG, "Empty HTTP response");
        http_response_length = 0;
    }
}



void WeatherData::parseHistoryJson(const char* json_data, HistoryData& data) {
    if (json_data == nullptr) {
        ESP_LOGE(TAG, "NULL JSON data provided");
        return;
    }
    
    cJSON *root = cJSON_Parse(json_data);
    if (root == NULL) {
        ESP_LOGE(TAG, "Failed to parse HTTP history JSON");
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) {
            ESP_LOGE(TAG, "JSON Error before: %s", error_ptr);
        }
        return;
    }
    
    if (!cJSON_IsArray(root)) {
        ESP_LOGE(TAG, "HTTP history JSON is not an array");
        cJSON_Delete(root);
        return;
    }
    
    int array_size = cJSON_GetArraySize(root);
    //ESP_LOGI(TAG, "Parsing %d historical data points", array_size);
    
    int point_count = 0;
    
    // Parse each history point
    for (int i = 0; i < array_size; i++) {
        // Reset watchdog periodically during long parsing
        if (i % 50 == 0) {
            esp_task_wdt_reset();
        }
        
        cJSON *item = cJSON_GetArrayItem(root, i);
        if (!cJSON_IsObject(item)) {
            continue;
        }
        
        // Parse timestamp
        cJSON *timestamp = cJSON_GetObjectItem(item, "timestamp");
        uint32_t ts = 0;
        if (cJSON_IsNumber(timestamp)) {
            ts = (uint32_t)timestamp->valueint;
        }
        
        // Parse temperature (use 'temperature' field for combined data, 'value' for individual metric)
        cJSON *temp = cJSON_GetObjectItem(item, "temperature");
        if (!cJSON_IsNumber(temp)) {
            temp = cJSON_GetObjectItem(item, "value"); // Fallback for individual endpoint
        }
        float temperature = 0.0f;
        if (cJSON_IsNumber(temp)) {
            temperature = temp->valuedouble;
        }
        
        // Parse wind speed
        cJSON *wind_speed = cJSON_GetObjectItem(item, "wind_speed");
        float ws = 0.0f;
        if (cJSON_IsNumber(wind_speed)) {
            ws = wind_speed->valuedouble;
        }
        
        // Parse air pressure
        cJSON *pressure = cJSON_GetObjectItem(item, "air_pressure");
        if (!cJSON_IsNumber(pressure)) {
            pressure = cJSON_GetObjectItem(item, "pressure"); // Alternative field name
        }
        float press = 0.0f;
        if (cJSON_IsNumber(pressure)) {
            press = pressure->valuedouble;
        }
        
        // Add to vectors
        data.temperatures.push_back(temperature);
        data.wind_speeds.push_back(ws);
        data.pressures.push_back(press);
        data.timestamps.push_back(ts);
        point_count++;
    }
    
    //ESP_LOGI(TAG, "Successfully parsed and processed %d history points", point_count);
    
    cJSON_Delete(root);
}

WeatherData::HistoryData WeatherData::generateSampleData(int points) {
    HistoryData sample_data;
    
    // Get current weather data for baseline values
    const CurrentWeather& current = getCurrentWeather();
    
    if (!current.valid) {
        ESP_LOGW(TAG, "No valid current weather data, using defaults for sample data");
        // Use default values
        float base_temp = 20.0f;
        float base_wind = 5.0f;
        float base_pressure = 1013.0f;
        uint32_t current_time = xTaskGetTickCount() / configTICK_RATE_HZ;
        
        for (int i = 0; i < points; i++) {
            sample_data.temperatures.push_back(base_temp + (rand() % 10 - 5) / 2.0f);
            sample_data.wind_speeds.push_back(base_wind + (rand() % 30 - 15) / 10.0f);
            sample_data.pressures.push_back(base_pressure + (rand() % 20 - 10) / 2.0f);
            sample_data.timestamps.push_back(current_time - (points - i) * 3600);
        }
    } else {
        uint32_t current_time = xTaskGetTickCount() / configTICK_RATE_HZ;
        
        for (int i = 0; i < points; i++) {
            sample_data.temperatures.push_back(current.temperature + (rand() % 10 - 5) / 2.0f);
            sample_data.wind_speeds.push_back(current.wind_speed + (rand() % 30 - 15) / 10.0f);
            sample_data.pressures.push_back(current.pressure + (rand() % 20 - 10) / 2.0f);
            sample_data.timestamps.push_back(current_time - (points - i) * 3600);
        }
    }
    
    //ESP_LOGI(TAG, "Generated %d sample data points", points);
    return sample_data;
}

WeatherData::HistoryData WeatherData::fetchHistoricalData() {
    //ESP_LOGI(TAG, "Fetching historical weather data from HTTP API...");
    
    HistoryData history_data;
    
    // Clear response buffer and length
    http_response_length = 0;
    if (http_response_buffer != nullptr) {
        http_response_buffer[0] = '\0';
    }
    
    // Fetch data from HTTP API
    esp_err_t ret = http_client_fetch_history(httpResponseCallback);
    
    //ESP_LOGI(TAG, "HTTP fetch returned: %s, buffer=%p, length=%d", 
             //esp_err_to_name(ret), http_response_buffer, http_response_length);
    
    if (ret == ESP_OK && http_response_length > 0) {
        //ESP_LOGI(TAG, "Successfully fetched historical data, parsing JSON...");
        
        // Parse the JSON response
        parseHistoryJson(http_response_buffer, history_data);
        
        if (history_data.isEmpty()) {
            ESP_LOGW(TAG, "Parsing resulted in no data points, keeping cached data");
            // Return cached data if available, otherwise empty
            return cached_data;
        } else {
            //ESP_LOGI(TAG, "Successfully loaded %zu historical data points", history_data.size());
            // Update cache with new data
            cached_data = history_data;
            return history_data;
        }
    } else {
        ESP_LOGE(TAG, "Failed to fetch historical data: %s (length=%d)", 
                 esp_err_to_name(ret), http_response_length);
        //ESP_LOGI(TAG, "Returning cached data (%zu points)", cached_data.size());
        // Return cached data (may be empty if this is the first fetch)
        return cached_data;
    }
}

bool WeatherData::fetchCurrentWeather() {
    //ESP_LOGI(TAG, "Fetching current weather data from HTTP API...");
    
    // Clear response buffer and length
    http_response_length = 0;
    if (http_response_buffer != nullptr) {
        http_response_buffer[0] = '\0';
    }
    
    // Fetch data from HTTP API
    esp_err_t ret = http_client_fetch_current(httpResponseCallback);
    
    //ESP_LOGI(TAG, "HTTP fetch returned: %s, buffer=%p, length=%d", 
             //esp_err_to_name(ret), http_response_buffer, http_response_length);
    
    if (ret == ESP_OK && http_response_length > 0) {
        //ESP_LOGI(TAG, "Successfully fetched current weather data, parsing JSON...");
        
        // Parse the JSON response
        bool parsed = parseCurrentJson(http_response_buffer);
        
        if (!parsed) {
            ESP_LOGE(TAG, "Failed to parse current weather JSON");
            return false;
        }
        
        //ESP_LOGI(TAG, "Current weather: T=%.1f°C H=%.1f%% P=%.1fmb WS=%.1fm/s WD=%d°",
                 //current_weather.temperature, current_weather.humidity, 
                 //current_weather.pressure, current_weather.wind_speed,
                 //current_weather.wind_direction);
        return true;
    } else {
        ESP_LOGE(TAG, "Failed to fetch current weather data: %s (length=%d)", 
                 esp_err_to_name(ret), http_response_length);
        return false;
    }
}

bool WeatherData::parseCurrentJson(const char* json_data) {
    if (json_data == nullptr) {
        ESP_LOGE(TAG, "NULL JSON data provided");
        return false;
    }
    
    ESP_LOGD(TAG, "Parsing current weather JSON: %s", json_data);
    
    cJSON *root = cJSON_Parse(json_data);
    if (root == NULL) {
        ESP_LOGE(TAG, "Failed to parse JSON: %s", json_data);
        return false;
    }
    
    // Parse according to API spec: /current returns a single object
    // {
    //   "timestamp": 1733313982,
    //   "temperature": 7.0,
    //   "humidity": 92.2,
    //   "pressure": 1002.7,
    //   "wind_speed": 0.0,
    //   "wind_speed_max": 0.0,
    //   "wind_speed_min": 0.0,
    //   "wind_direction": 275,
    //   "rainfall": 0.0
    // }
    
    bool success = false;
    
    cJSON *timestamp = cJSON_GetObjectItem(root, "timestamp");
    cJSON *temp = cJSON_GetObjectItem(root, "temperature");
    cJSON *humidity = cJSON_GetObjectItem(root, "humidity");
    cJSON *pressure = cJSON_GetObjectItem(root, "pressure");
    cJSON *wind_speed = cJSON_GetObjectItem(root, "wind_speed");
    cJSON *wind_speed_max = cJSON_GetObjectItem(root, "wind_speed_max");
    cJSON *wind_speed_min = cJSON_GetObjectItem(root, "wind_speed_min");
    cJSON *wind_dir = cJSON_GetObjectItem(root, "wind_direction");
    cJSON *rainfall = cJSON_GetObjectItem(root, "rainfall");
    
    if (cJSON_IsNumber(timestamp) && cJSON_IsNumber(temp) && 
        cJSON_IsNumber(humidity) && cJSON_IsNumber(pressure)) {
        
        current_weather.timestamp = (time_t)timestamp->valueint;
        current_weather.temperature = temp->valuedouble;
        current_weather.humidity = humidity->valuedouble;
        current_weather.pressure = pressure->valuedouble;
        
        if (cJSON_IsNumber(wind_speed)) {
            current_weather.wind_speed = wind_speed->valuedouble;
        }
        if (cJSON_IsNumber(wind_speed_max)) {
            current_weather.wind_speed_max = wind_speed_max->valuedouble;
        }
        if (cJSON_IsNumber(wind_speed_min)) {
            current_weather.wind_speed_min = wind_speed_min->valuedouble;
        }
        if (cJSON_IsNumber(wind_dir)) {
            current_weather.wind_direction = (uint16_t)wind_dir->valueint;
        }
        if (cJSON_IsNumber(rainfall)) {
            current_weather.rainfall = rainfall->valuedouble;
        }
        
        current_weather.valid = true;
        success = true;
    } else {
        ESP_LOGE(TAG, "Missing required fields in current weather JSON");
    }
    
    cJSON_Delete(root);
    return success;
}
