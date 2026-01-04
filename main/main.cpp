/**
 * @file main.cpp
 * @brief Main application entry point for ESP32 weather station display
 * 
 * This file initializes all system components including WiFi, HTTP client,
 * SNTP time sync, and the WeatherUI display system. It manages the main application
 * loop which periodically updates the display and maintains network connections.
 * 
 * @author Weather Station Project
 * @date 2026
 */

#include <cstdio>
#include <cstring>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_task_wdt.h"
#include "esp_sleep.h"
#include "nvs_flash.h"
#include "config.h"

#include "wifi_manager.h"
#include "http_client.h"
#include "esp_sntp.h"
#include "../gui/Status.h"
#include "../gui/WeatherUI.h"
#include "../gui/WeatherData.h"
#include "../gui/Fonts/mplus_rounded_1c_medium_20.h"
#define theFont MPLUSRounded1c_Medium_20
static const char *TAG = "main";

static TaskHandle_t s_main_task_handle = nullptr;
uint8_t * fb = nullptr;

/**
 * @brief Main application logic and initialization sequence
 * 
 * Initializes all system components in order:
 * 1. Weather data storage
 * 2. WeatherUI and Status display
 * 3. WiFi connection
 * 4. SNTP time synchronization
 * 5. HTTP client for weather data
 * 
 * After initialization, enters main loop which:
 * - Updates status display
 * - Monitors WiFi connection
 * - Periodically fetches weather data via WeatherData class
 * - Updates main UI display
 * - Resets watchdog timer
 */
void run_app() {
    // Store main task handle for notifications
    s_main_task_handle = xTaskGetCurrentTaskHandle();
    
    WeatherUI& ui = WeatherUI::getInstance();
    Status status(EPD_WIDTH - 328, 0, 308, 90, ui.getFramebuffer());
    esp_task_wdt_add(nullptr);
    GFXfont* font = (GFXfont*)&theFont;    

    //ESP_LOGI(TAG, "Weather UI instance retrieved. Initializing...");
    if (ui.init() != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize Weather UI");
        return;
    }
    fb = ui.getFramebuffer();

    int32_t tx=5, ty=font->ascender + font->descender + 5;
    
    // Initialize WiFi

    // need to reset x and back up y a bit after writing lines since they advance  x and y
    ESP_ERROR_CHECK(wifi_manager_init());
    //ESP_LOGI(TAG, "%d,%d Connecting WiFi... ,",tx,ty);
    ty-=10;
    int oldy=ty;
    ui.writeLine(&tx, &ty, "Wifi init,  ", &theFont); 
    ESP_ERROR_CHECK(wifi_manager_connect());
    ty=oldy;
    // Wait for WiFi connection and IP address
    //ESP_LOGI(TAG, "%d,%d Waiting for WiFi connection...",tx,ty);
    ui.writeLine(&tx, &ty, "Connecting, ", &theFont);
    
    while (!wifi_manager_is_connected()) {
        vTaskDelay(pdMS_TO_TICKS(500));
        esp_task_wdt_reset();
    }
    ty=oldy;
    //ESP_LOGI(TAG, "WiFi connected, IP address obtained");
    ui.writeLine(&tx, &ty, " Connected\n", &theFont);
    vTaskDelay(pdMS_TO_TICKS(1000));
      
    // Initialize SNTP for time synchronization
    //ESP_LOGI(TAG, "SNTP initializing...");
    tx=5;
    ty-=10;
    oldy=ty;
    ui.writeLine(&tx, &ty, "Syncing time...: ", &theFont);
    
    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, SNTP_SERVER);
    esp_sntp_init();
    
    // Set timezone from configuration
    setenv("TZ", TIMEZONE_STRING, 1);
    tzset();
    
    // Wait for time to be synchronized
    time_t now = 0;
    struct tm timeinfo;
    memset(&timeinfo, 0, sizeof(timeinfo));
    int retry = 0;
    int retry_count = 10;
    while (timeinfo.tm_year < (2024 - 1900) && ++retry < retry_count) {
        //ESP_LOGI(TAG, "Waiting for system time to be set... (%d/%d)", retry, retry_count);
        vTaskDelay(pdMS_TO_TICKS(2000));
        time(&now);
        localtime_r(&now, &timeinfo);
        esp_task_wdt_reset();
    }
    
    ty=oldy;
    if (timeinfo.tm_year >= (2024 - 1900)) {
        char strftime_buf[64];
        char buf[128];
        strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
        snprintf(buf, sizeof(buf), "%s\n", strftime_buf);
        //ESP_LOGI(TAG, "Time synchronized: %s\n", strftime_buf);
        ui.writeLine(&tx, &ty, buf, &theFont); 
    } else {
        ESP_LOGW(TAG, "Failed to synchronize time");
        ui.writeLine(&tx, &ty, "Failed \n", &theFont); 
    }
    esp_task_wdt_reset();
     // Initialize HTTP client
    tx=5;
    ty-=10;
    oldy=ty;
    ui.writeLine(&tx, &ty, "HTTP Client initializing\n", &theFont); 
    //ESP_LOGI(TAG, "%d,%d Http client init... ,",tx,ty);
    ESP_ERROR_CHECK(http_client_init());
    esp_task_wdt_reset();
    
    tx=5;
    ty-=10;
    ui.writeLine(&tx, &ty, "System Ready", &theFont);
    
    // Get WeatherData instance for fetching current and historical data
    WeatherData& weather = WeatherData::getInstance();
    
    // Fetch initial current weather data
    //ESP_LOGI(TAG, "Fetching initial current weather data");
    esp_task_wdt_reset();
    weather.fetchCurrentWeather();
    esp_task_wdt_reset();
    
    // Initial display update (WeatherUI will fetch historical data)
    //ESP_LOGI(TAG, "Performing initial display update");
    esp_task_wdt_reset();
    
    epd_clear();
    
    // Main loop - monitor connections and update display
    
    while (1) {
        // Reset watchdog
        esp_task_wdt_reset();

        status.updateFromSystem(); 
        status.draw();
        esp_task_wdt_reset();  // Reset after status draw
        
        // WiFi reconnection is handled automatically by the event handler
        // Wait for WiFi to reconnect before attempting to fetch data
        ESP_LOGW(TAG, "Wakeup wifi check");
        if (!wifi_manager_is_connected()) {
            ESP_LOGW(TAG, "WiFi disconnected, waiting for automatic reconnection...");
            while (!wifi_manager_is_connected()) {
                vTaskDelay(pdMS_TO_TICKS(1000));  // Wait 1 second between checks
                esp_task_wdt_reset();  // Reset watchdog while waiting
            }
            ESP_LOGI(TAG, "WiFi reconnected successfully");
            // Give network stack time to fully initialize after reconnection
            vTaskDelay(pdMS_TO_TICKS(500));
        }


        vTaskDelay(pdMS_TO_TICKS(1000));
        ESP_LOGW(TAG, "wifi ok, proceeding to fetch weather data");
        //ESP_LOGI(TAG, "Fetching current weather data");
        esp_task_wdt_reset();
        weather.fetchCurrentWeather();
        esp_task_wdt_reset();
    
        // Update UI (WeatherUI will fetch historical data as needed)
        ui.update();
        epd_poweroff();
        esp_task_wdt_reset();  // Reset watchdog after UI update completes

        // Enter light sleep (preserves WiFi connection)
        // Configure sleep to keep WiFi powered
        esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);
        esp_sleep_enable_timer_wakeup(SLEEP_DURATION_SECONDS * 1000000ULL);
        
        // Enter light sleep - WiFi connection maintained
        esp_light_sleep_start();
        epd_poweron();
        
        // Give WiFi time to resume after sleep
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}


/**
 * @brief Application entry point (called by ESP-IDF framework)
 * 
 * Initializes NVS (Non-Volatile Storage) which is required for WiFi operation,
 * then launches the main application logic.
 */
extern "C" void app_main(void) {
    // Initialize NVS (required for WiFi)
    //ESP_LOGI(TAG, "Initializing NVS...");
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        //ESP_LOGI(TAG, "Erasing NVS flash...");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    run_app();    
}
