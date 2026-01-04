/**
 * @file TemperatureChart.h
 * @brief Temperature chart widget interface
 * 
 * Displays historical temperature data with automatic Y-axis scaling.
 * Enforces minimum 5°C range for better readability. Shows °C unit label.
 */

#ifndef TEMPERATURECHART_H
#define TEMPERATURECHART_H

#include "Chart.h"

/**
 * @brief Temperature Chart widget
 * 
 * Displays historical temperature data in a line chart format
 */
class TemperatureChart : public Chart {
public:
    /**
     * @brief Construct a new Temperature Chart widget
     * 
     * @param x X coordinate of chart's upper left corner
     * @param y Y coordinate of chart's upper left corner
     * @param width Width of the chart area
     * @param height Height of the chart area
     * @param framebuffer Pointer to the framebuffer
     * @param font Font for labels
     */
    TemperatureChart(int x, int y, int width, int height, uint8_t* framebuffer, const GFXfont* font);

    /**
     * @brief Update chart with historical temperature data
     * 
     * @param temperatures Vector of temperature values (Celsius)
     * @param timestamps Vector of timestamps
     */
    void updateTempData(const std::vector<float>& temperatures, const std::vector<uint32_t>& timestamps);

protected:
    /**
     * @brief Draw the chart title
     */
    void drawTitle() override;

    /**
     * @brief Draw Y-axis labels with "°C" unit
     */
    void drawYLabels() override;

    /**
     * @brief Draw X-axis time labels
     */
    void drawXLabels() override;

    /**
     * @brief Calculate auto-scale with minimum 5°C range
     */
    void calculateAutoScale();
};

#endif // TEMPERATURECHART_H
