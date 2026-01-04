/**
 * @file PressureChart.h
 * @brief Atmospheric pressure chart widget interface
 * 
 * Displays historical atmospheric pressure data with automatic Y-axis scaling.
 * Typically ranges 980-1040 hPa. Shows hPa/mb unit label.
 */

#ifndef PRESSURECHART_H
#define PRESSURECHART_H

#include "Chart.h"

/**
 * @brief Atmospheric Pressure Chart widget
 * 
 * Displays historical atmospheric pressure data in a line chart format
 */
class PressureChart : public Chart {
public:
    /**
     * @brief Construct a new Pressure Chart widget
     * 
     * @param x X coordinate of chart's upper left corner
     * @param y Y coordinate of chart's upper left corner
     * @param width Width of the chart area
     * @param height Height of the chart area
     * @param framebuffer Pointer to the framebuffer
     * @param font Font for labels
     */
    PressureChart(int x, int y, int width, int height, uint8_t* framebuffer, const GFXfont* font);

    /**
     * @brief Update chart with historical pressure data
     * 
     * @param pressures Vector of pressure values (millibars/hPa)
     * @param timestamps Vector of timestamps
     */
    void updatePressureData(const std::vector<float>& pressures, const std::vector<uint32_t>& timestamps);

protected:
    /**
     * @brief Draw the chart title
     */
    void drawTitle() override;

    /**
     * @brief Draw Y-axis labels with "hPa" or "mb" unit
     */
    void drawYLabels() override;

    /**
     * @brief Draw X-axis time labels
     */
    void drawXLabels() override;
};

#endif // PRESSURECHART_H
