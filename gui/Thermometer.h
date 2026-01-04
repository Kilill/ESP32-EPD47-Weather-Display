/**
 * @file Thermometer.h
 * @brief Vertical thermometer gauge widget interface
 * 
 * Displays temperature as a classic thermometer visualization with:
 * - Vertical bar with scale markings
 * - Bulb at bottom
 * - Temperature range: -15°C to +40°C
 * - Tick marks every 10 degrees
 * - Current temperature label
 */

#ifndef THERMOMETER_HPP
#define THERMOMETER_HPP

#include <cstdint>
#include "epd_driver.h"

// Forward declaration
class Display;

/**
 * @brief Thermometer widget class
 * 
 * This class manages a vertical thermometer widget that displays temperature
 * from -15°C to +40°C with tick marks every 10 degrees.
 */
class Thermometer {
public:
    /**
     * @brief Construct a new Thermometer widget
     * 
     * @param x X coordinate of the thermometer's left edge
     * @param y Y coordinate of the thermometer's top edge
     * @param width Width of the thermometer bar
     * @param height Total height of the widget (includes bar, bulb, and labels)
     * @param font Pointer to the font for temperature labels
     */
    Thermometer(int x, int y, int width, int height, const GFXfont* font);

    /**
     * @brief Draw the thermometer widget
     * 
     * @param temperature Temperature in degrees Celsius (-15 to +40)
     */
    void draw(float temperature);

    /**
     * @brief Update the temperature and redraw the widget
     * 
     * @param temperature New temperature in degrees Celsius
     */
    void update(float temperature);

    /**
     * @brief Get the current temperature
     * 
     * @return Current temperature in degrees Celsius
     */
    float getTemperature() const { return temperature_; }

    /**
     * @brief Get the width
     */
    int getWidth() const { return width_; }

    /**
     * @brief Get the height
     */
    int getHeight() const { return height_; }

private:
    int x_;
    int y_;
    int width_;
    int height_;          // Total widget height
    int bar_height_;      // Height of just the thermometer bar
    float temperature_;
    const GFXfont* font_;
    
    // Temperature range
    static constexpr float MIN_TEMP = -15.0f;
    static constexpr float MAX_TEMP = 40.0f;
    static constexpr float TEMP_RANGE = MAX_TEMP - MIN_TEMP;  // 55 degrees
    
    // Bulb size at bottom
    static constexpr int BULB_RADIUS = 30;

    void drawScale();
    void drawBar(float temperature);
    void drawBulb();
    void drawTemperatureLabel(float temperature);
    int temperatureToY(float temp) const;

    #ifdef THERMOMETER_DEBUG
    static const char* TAG;
    #endif
};

#endif // THERMOMETER_HPP
