/**
 * @file WindSpeedChart.h
 * @brief Wind speed chart widget interface
 * 
 * Displays historical wind speed data with automatic Y-axis scaling.
 * Enforces minimum 5 m/s range, prevents negative values. Shows m/s unit label.
 */

#ifndef WINDSPEEDCHART_H
#define WINDSPEEDCHART_H

#include "Chart.h"

/**
 * @brief Wind Speed Chart widget
 * 
 * Displays historical wind speed data in a line chart format
 */
class WindSpeedChart : public Chart {
public:
    /**
     * @brief Construct a new Wind Speed Chart widget
     * 
     * @param x X coordinate of chart's upper left corner
     * @param y Y coordinate of chart's upper left corner
     * @param width Width of the chart area
     * @param height Height of the chart area
     * @param framebuffer Pointer to the framebuffer
     * @param font Font for labels
     */
    WindSpeedChart(int x, int y, int width, int height, uint8_t* framebuffer, const GFXfont* font);

    /**
     * @brief Update chart with historical wind speed data
     * 
     * @param wind_speeds Vector of wind speed values (m/s)
     * @param timestamps Vector of timestamps
     */
    void updateWindData(const std::vector<float>& wind_speeds, const std::vector<uint32_t>& timestamps);

protected:
    /**
     * @brief Draw the chart title
     */
    void drawTitle() override;

    /**
     * @brief Draw Y-axis labels with "m/s" unit
     */
    void drawYLabels() override;

    /**
     * @brief Draw X-axis time labels
     */
    void drawXLabels() override;

    /**
     * @brief Calculate auto-scale with minimum 5 m/s range
     */
    void calculateAutoScale();
};

#endif // WINDSPEEDCHART_H
