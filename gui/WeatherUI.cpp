/**
 * @file WeatherUI.cpp
 * @brief Main weather UI coordinator implementation
 * 
 * Manages the complete weather display system including:
 * - EPD driver initialization and framebuffer management
 * - Widget creation and positioning
 * - Periodic data fetching and display updates
 * - Watchdog timer management during long operations
 * 
 * Uses singleton pattern to ensure single UI instance.
 */

#include "WeatherUI.h"
#include "WeatherData.h"
#include "Windrose.h"
#include "WindData.h"
#include "WindSpeedChart.h"
#include "TemperatureChart.h"
#include "PressureChart.h"
#include "Status.h"
#include "wifi_manager.h"
#include "esp_log.h"
#include "esp_task_wdt.h"
#include <cmath>
#include <cstdio>
#include <cstring>

// Include EPDiy font for text rendering - font is defined in the header
//#include "../gui/Fonts/atkinson_hyperlegible_mono_20_bold.h"
#include "../gui/Fonts/mplus_rounded_1c_medium_20.h"
#include "../gui/Fonts/mplus_rounded_1c_medium_12.h"
// #include "../gui/Fonts/mplus_rounded_1c_regular_20.h"
#define theFont MPLUSRounded1c_Medium_20
#define theFontSmal MPLUSRounded1c_Medium_12

const char* WeatherUI::TAG = "WeatherUI";

WeatherUI::WeatherUI() 
    : initialized_(false) 
    , font_props ({.fg_color = 0, .bg_color = 15, .fallback_glyph = 0, .flags = 0 })
    , framebuffer(nullptr)
    , last_history_size(0)
{
    // Initialize the EPD driver
    epd_init();
    
    // Allocate framebuffer in PSRAM
    framebuffer = (uint8_t*)heap_caps_malloc(EPD_WIDTH * EPD_HEIGHT / 2, MALLOC_CAP_SPIRAM);
    if (framebuffer == nullptr) {
        ESP_LOGE(TAG, "Failed to allocate framebuffer in constructor");
    } else {
        // Clear framebuffer to white
        memset(framebuffer, 0xFF, EPD_WIDTH * EPD_HEIGHT / 2);
    }
}

WeatherUI& WeatherUI::getInstance() {
    static WeatherUI instance;
    return instance;
}

esp_err_t WeatherUI::init() {
    if (initialized_) {
        ESP_LOGW(TAG, "WeatherUI already initialized");
        return ESP_OK;
    }

    //ESP_LOGI(TAG, "Initializing weather UI");
    
    if (framebuffer == nullptr) {
        ESP_LOGE(TAG, "Framebuffer is null, cannot initialize");
        return ESP_FAIL;
    }
   
    //ESP_LOGI(TAG, "LilyGO-EPD47 initialized successfully");
    //ESP_LOGI(TAG, "Display dimensions (width x height): %dx%d", EPD_WIDTH, EPD_HEIGHT);
    //ESP_LOGI(TAG, "Framebuffer at %p, size %d bytes", framebuffer, EPD_WIDTH * EPD_HEIGHT / 2);
    
    // Do a full clear on first run
    epd_poweron();
    epd_clear();
    //ESP_LOGI(TAG, "Initial clear complete");

   
    // Initialize Windrose widget
    windrose = std::make_unique<Windrose>(15, 5, EPD_HEIGHT/2+10, &theFont);
    
    // Initialize WindData widget
    winddata =  std::make_unique<WindData>(10, EPD_HEIGHT/2 + 10,  &theFont);
    
    // Initialize Thermometer widget
    thermometer = std::make_unique<Thermometer>(EPD_HEIGHT/2+40 ,40, 50, EPD_HEIGHT-65, &theFont);
    
    // Initialize charts on right side with adjusted heights to fit all three
    // Total available height: 520px (540 - 20 top margin)
    // 3 charts at 160px each with 20px spacing = 520px
    const int chart_x = EPD_HEIGHT/2 + 200;
    const int chart_width = EPD_WIDTH - EPD_HEIGHT/2 - 220;
    const int chart_height = 160;
    const int chart_spacing = 20;
    
    // WindSpeedChart at top
    windspeed_chart = std::make_unique<WindSpeedChart>(
        chart_x, 20,
        chart_width, chart_height,
        framebuffer, &theFontSmal);
    
    // TemperatureChart in middle
    temperature_chart = std::make_unique<TemperatureChart>(
        chart_x, 20 + chart_height + chart_spacing,
        chart_width, chart_height,
        framebuffer, &theFontSmal);
    
    // PressureChart at bottom
    pressure_chart = std::make_unique<PressureChart>(
        chart_x, 20 + 2 * (chart_height + chart_spacing),
        chart_width, chart_height,
        framebuffer, &theFontSmal);
    
    // Initialize Status widget in lower left corner with time/date display (90px height, 308px width)
    status = std::make_unique<Status>(10, EPD_HEIGHT - 110, 308, 90, framebuffer);
    
    //ESP_LOGI(TAG, "Battery monitoring via ADC (GPIO 36)");
    
    //ESP_LOGI(TAG, "Updating e-paper...");
    
    Rect_t update_area = {
        .x = 0,
        .y = 0,
        .width = EPD_WIDTH,
        .height = EPD_HEIGHT
    };
    
    epd_draw_grayscale_image(update_area, framebuffer);
    epd_poweroff();

    initialized_ = true;
    //ESP_LOGI(TAG, "Weather UI initialized");
    return ESP_OK;
}

void WeatherUI::update() {
    esp_task_wdt_reset();  // Reset watchdog at the start of update cycle
    
    // Get current weather data from WeatherData singleton
    WeatherData& weather = WeatherData::getInstance();
    const CurrentWeather& current = weather.getCurrentWeather();

    memset(framebuffer, 0xFF, EPD_WIDTH * EPD_HEIGHT / 2);
    
    // Draw gauges if we have valid current data
    if (current.valid) {
        windrose->draw(current.wind_direction);
        thermometer->draw(current.temperature);
        winddata->draw(current.wind_direction, current.wind_speed, current.wind_speed_max);
        
        esp_task_wdt_reset();  // Reset watchdog after drawing gauges
        
        //ESP_LOGI(TAG, "Gauges drawn: valid=%d temp=%.1f rain=%.1f wind=%.1f dir=%d pressure=%.1f", 
                 //current.valid, current.temperature, current.rainfall, 
                 //current.wind_speed, current.wind_direction, current.pressure);
    } else {
        ESP_LOGW(TAG, "Skipping gauge display - no valid current weather data");
    }
    
    // Always update status and fetch historical data
    status->updateFromSystem();
    
    // Fetch and update charts with real historical data
    WeatherData& weather_data = WeatherData::getInstance();
        //ESP_LOGI(TAG, "Fetching historical data...");
        esp_task_wdt_reset();  // Reset watchdog before HTTP fetch
        
        // Get cached data before fetch for comparison
        size_t old_size = weather_data.getCachedData().size();
        
        // Fetch new data (will return cached if fetch fails)
        WeatherData::HistoryData history = weather_data.fetchHistoricalData();
        
        esp_task_wdt_reset();  // Reset watchdog after HTTP fetch completes
        
        //ESP_LOGI(TAG, "Historical data received: %zu points (was %zu), isEmpty=%d", 
                 //history.size(), old_size, history.isEmpty());
        
        // Only update charts if we have new data
        bool data_changed = (history.size() != old_size);
        
        if (!history.isEmpty()) {
            if (data_changed) {
                //ESP_LOGI(TAG, "New data received, updating charts with %zu data points", history.size());
            } else {
                //ESP_LOGI(TAG, "Data unchanged, redrawing charts with cached data");
            }
            // Always draw charts (they update data if changed, or just redraw if unchanged)
            windspeed_chart->updateWindData(history.wind_speeds, history.timestamps);
            temperature_chart->updateTempData(history.temperatures, history.timestamps);
            pressure_chart->updatePressureData(history.pressures, history.timestamps);
            esp_task_wdt_reset();  // Reset watchdog after updating chart data
            //ESP_LOGI(TAG, "Charts drawn successfully");
            status->updateError(Status::ErrorStatus::NO_ERROR);
        } else {
            ESP_LOGW(TAG, "Historical data is empty");
            if (old_size == 0) {
                // First fetch failed, draw empty charts
                ESP_LOGW(TAG, "Drawing empty charts");
                windspeed_chart->updateWindData(history.wind_speeds, history.timestamps);
                temperature_chart->updateTempData(history.temperatures, history.timestamps);
                pressure_chart->updatePressureData(history.pressures, history.timestamps);
            } else {
                //ESP_LOGI(TAG, "Keeping previous chart data");
            }
            // Set error status to indicate data fetch failure
            status->updateError(Status::ErrorStatus::ERROR);
        }
        
        // Draw status after error state is updated
        status->draw();
        esp_task_wdt_reset();  // Reset watchdog after status widget drawing
    
    //ESP_LOGI(TAG, "Preparing display update...");
    esp_task_wdt_reset();  // Reset watchdog before display operations
    
    Rect_t update_area = {
        .x = 0,
        .y = 0,
        .width = EPD_WIDTH,
        .height = EPD_HEIGHT
    };
    
    epd_poweron();
    esp_task_wdt_reset();  // Reset before clear operation
    epd_clear_area_cycles(update_area, 1, 50);
    esp_task_wdt_reset();  // Reset before draw operation
    epd_draw_grayscale_image(update_area, framebuffer);
    epd_poweroff();
    
    //ESP_LOGI(TAG, "Display update complete");
}

const EpdFont* WeatherUI::getFont() const {
    return &theFont;
}

void WeatherUI::writeLine(int32_t *x, int32_t *y, const char* str, const GFXfont* font) {
    
    Rect_t update_area = {
        .x = 0,
        .y = 0,
        .width = EPD_WIDTH,
        .height = EPD_HEIGHT
    };

    write_string( font, str, x, y, framebuffer);
    epd_poweron();
    epd_draw_grayscale_image(update_area, framebuffer);
    epd_poweroff();
}
 