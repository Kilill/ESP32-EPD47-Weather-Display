/**
 * @file Chart.h
 * @brief Base class for time-series chart widgets
 * 
 * Provides common functionality for displaying time-series data on e-paper display.
 * Handles data management, auto-scaling, axes drawing, grid lines, and labels.
 * Derived classes implement specific chart types (line, bar, etc.).
 */

#ifndef CHART_H
#define CHART_H

#include <cstdint>
#include <vector>
#include "epd_driver.h"

/**
 * @brief Base Chart class for displaying time-series data
 * 
 * This is an abstract base class that provides common functionality
 * for all chart types (line charts, bar charts, etc.)
 */
class Chart {
public:
    /**
     * @brief Data point structure for chart data
     */
    struct DataPoint {
        float value;
        uint32_t timestamp;  // Unix timestamp or tick count
    };

    /**
     * @brief Construct a new Chart widget
     * 
     * @param x X coordinate of chart's upper left corner
     * @param y Y coordinate of chart's upper left corner
     * @param width Width of the chart area
     * @param height Height of the chart area
     * @param framebuffer Pointer to the framebuffer
     * @param font Font for labels
     */
    Chart(int x, int y, int width, int height, uint8_t* framebuffer, const GFXfont* font);

    /**
     * @brief Virtual destructor
     */
    virtual ~Chart() = default;

    /**
     * @brief Set the data for the chart
     * 
     * @param data Vector of data points
     */
    void setData(const std::vector<DataPoint>& data);

    /**
     * @brief Clear all data points
     */
    void clearData();

    /**
     * @brief Set the Y-axis range
     * 
     * @param min Minimum value
     * @param max Maximum value
     */
    void setYRange(float min, float max);

    /**
     * @brief Enable/disable auto-scaling of Y-axis
     * 
     * @param enable True to enable auto-scaling
     */
    void setAutoScale(bool enable);

    /**
     * @brief Draw the complete chart
     */
    virtual void draw();

    /**
     * @brief Update the chart with new data and redraw
     * 
     * @param data Vector of data points
     */
    void update(const std::vector<DataPoint>& data);

protected:
    /**
     * @brief Draw the chart axes
     */
    void drawAxes();

    /**
     * @brief Draw grid lines
     */
    void drawGrid();

    /**
     * @brief Draw alternating day backgrounds
     */
    void drawDayBackgrounds();

    /**
     * @brief Draw the data line/curve
     * 
     * This method should be overridden by derived classes for custom rendering
     */
    virtual void drawData();

    /**
     * @brief Draw Y-axis labels
     * 
     * Override this to customize label formatting
     */
    virtual void drawYLabels();

    /**
     * @brief Draw X-axis labels (time labels)
     * 
     * Override this to customize time label formatting
     */
    virtual void drawXLabels();

    /**
     * @brief Draw the chart title
     * 
     * Override this to customize title
     */
    virtual void drawTitle();

    /**
     * @brief Convert data value to Y pixel coordinate
     * 
     * @param value Data value
     * @return Y pixel coordinate
     */
    int valueToY(float value) const;

    /**
     * @brief Convert data point index to X pixel coordinate
     * 
     * @param index Data point index
     * @return X pixel coordinate
     */
    int indexToX(int index) const;

    /**
     * @brief Calculate auto-scale range from data
     */
    void calculateAutoScale();

    // Chart dimensions and position
    int x_pos;
    int y_pos;
    int chart_width;
    int chart_height;
    
    // Drawing area (excluding margins for labels)
    int plot_x;
    int plot_y;
    int plot_width;
    int plot_height;
    
    // Data
    std::vector<DataPoint> data_points;
    
    // Y-axis range
    float y_min;
    float y_max;
    bool auto_scale;
    
    // Display
    uint8_t* fb;
    const GFXfont* font;
    
    // Margins for labels
    static constexpr int MARGIN_LEFT = 40;
    static constexpr int MARGIN_RIGHT = 10;
    static constexpr int MARGIN_TOP = 30;
    static constexpr int MARGIN_BOTTOM = 25;
    
    // Grid settings
    static constexpr int GRID_DIVISIONS_Y = 5;
    static constexpr int GRID_DIVISIONS_X = 6;
};

#endif // CHART_H
