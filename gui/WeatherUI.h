/**
 * @file WeatherUI.h
 * @brief Main weather UI coordinator interface
 * 
 * Singleton class that manages all weather display widgets including:
 * - Windrose and wind data gauges
 * - Thermometer gauge
 * - Wind speed, temperature, and pressure charts
 * - Status bar
 * 
 * Handles UI initialization, data updates, and display refresh coordination.
 */

#ifndef WEATHER_UI_HPP
#define WEATHER_UI_HPP

#include "esp_err.h"
#include "WeatherData.h"
#include "Windrose.h"
#include "Thermometer.h"
#include "WindData.h"
#include "WindSpeedChart.h"
#include "TemperatureChart.h"
#include "PressureChart.h"
#include "Status.h"
#include "epd_driver.h"
#include <memory>

/**
 * @brief WeatherUI class - manages weather UI rendering
 * 
 * This class contains all weather-related widgets and UI components.
 */
class WeatherUI {
public:
    /**
     * @brief Get the singleton instance of WeatherUI
     * @return Reference to the WeatherUI instance
     */
    static WeatherUI& getInstance();

    /**
     * @brief Initialize the weather UI system
     * @return ESP_OK on success, ESP_FAIL on failure
     */
    esp_err_t init();

    /**
     * @brief Update the weather UI with latest data
     */
    void update();

    /**
     * @brief Update the physical display
     */
    void updateDisplay();

    /**
     * @brief Get the Atkinson Hyperlegible 20 font
     * @return Pointer to the font
     */
    const GFXfont* getFont() const;

    /**
     * @brief Get the framebuffer pointer
     * @return Pointer to the framebuffer
     */
    uint8_t* getFramebuffer() { return framebuffer; }
    
    void writeLine(int32_t *x, int32_t *y, const char* str, const GFXfont* font);

    // Delete copy constructor and assignment operator
    WeatherUI(const WeatherUI&) = delete;
    WeatherUI& operator=(const WeatherUI&) = delete;

private:
    WeatherUI();
    ~WeatherUI() = default;

    bool initialized_;
    std::unique_ptr<Windrose> windrose;
    std::unique_ptr<Thermometer> thermometer;
    std::unique_ptr<WindData> winddata;
    std::unique_ptr<WindSpeedChart> windspeed_chart;
    std::unique_ptr<TemperatureChart> temperature_chart;
    std::unique_ptr<PressureChart> pressure_chart;
    std::unique_ptr<Status> status;
    FontProperties font_props;
    uint8_t* framebuffer;
    
    // Cache for comparing with new historical data
    size_t last_history_size;
    
    static const char* TAG;
};

#endif // WEATHER_UI_HPP
