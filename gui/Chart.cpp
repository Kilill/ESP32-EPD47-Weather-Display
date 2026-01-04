/**
 * @file Chart.cpp
 * @brief Base chart class implementation
 * 
 * Implements common chart functionality including coordinate transformations,
 * auto-scaling, grid/axes drawing, and label rendering.
 */

#include "Chart.h"
#include "epd_driver.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <ctime>
#include <vector>

Chart::Chart(int x, int y, int width, int height, uint8_t* framebuffer, const GFXfont* font)
    : x_pos(x), y_pos(y), chart_width(width), chart_height(height),
      plot_x(0), plot_y(0), plot_width(0), plot_height(0),
      y_min(0.0f), y_max(100.0f), auto_scale(true),
      fb(framebuffer), font(font) {
    
    // Calculate plot area (chart area minus margins)
    plot_x = x_pos + MARGIN_LEFT;
    plot_y = y_pos + MARGIN_TOP;
    plot_width = chart_width - MARGIN_LEFT - MARGIN_RIGHT;
    plot_height = chart_height - MARGIN_TOP - MARGIN_BOTTOM;
}

void Chart::setData(const std::vector<DataPoint>& data) {
    data_points = data;
    if (auto_scale) {
        calculateAutoScale();
    }
}

void Chart::clearData() {
    data_points.clear();
}

void Chart::setYRange(float min, float max) {
    y_min = min;
    y_max = max;
    auto_scale = false;
}

void Chart::setAutoScale(bool enable) {
    auto_scale = enable;
    if (auto_scale && !data_points.empty()) {
        calculateAutoScale();
    }
}

void Chart::calculateAutoScale() {
    if (data_points.empty()) {
        y_min = 0.0f;
        y_max = 100.0f;
        return;
    }
    
    float min_val = data_points[0].value;
    float max_val = data_points[0].value;
    
    for (const auto& point : data_points) {
        if (point.value < min_val) min_val = point.value;
        if (point.value > max_val) max_val = point.value;
    }
    
    // Add some padding (10% on each side)
    float range = max_val - min_val;
    if (range < 0.1f) range = 0.1f; // Avoid division by zero
    
    y_min = min_val - range * 0.1f;
    y_max = max_val + range * 0.1f;
}

int Chart::valueToY(float value) const {
    if (y_max == y_min) return plot_y + plot_height / 2;
    
    // Invert Y (screen coordinates increase downward)
    float normalized = (value - y_min) / (y_max - y_min);
    return plot_y + plot_height - (int)(normalized * plot_height);
}

int Chart::indexToX(int index) const {
    if (data_points.size() <= 1) return plot_x;
    
    return plot_x + (index * plot_width) / (data_points.size() - 1);
}

void Chart::drawAxes() {
    // Draw Y-axis (left)
    epd_draw_vline(plot_x, plot_y, plot_height, 0, fb);
    
    // Draw X-axis (bottom)
    epd_draw_hline(plot_x, plot_y + plot_height, plot_width, 0, fb);
}

void Chart::drawGrid() {
    // Draw horizontal grid lines
    for (int i = 0; i <= GRID_DIVISIONS_Y; i++) {
        int y = plot_y + (i * plot_height) / GRID_DIVISIONS_Y;
        
        // Draw dashed line
        for (int x = plot_x; x < plot_x + plot_width; x += 8) {
            epd_draw_hline(x, y, 4, 200, fb); // Light gray dashed line
        }
    }
    
    // Draw vertical grid lines
    for (int i = 0; i <= GRID_DIVISIONS_X; i++) {
        int x = plot_x + (i * plot_width) / GRID_DIVISIONS_X;
        
        // Draw dashed line
        for (int y = plot_y; y < plot_y + plot_height; y += 8) {
            epd_draw_vline(x, y, 4, 200, fb); // Light gray dashed line
        }
    }
}

void Chart::drawDayBackgrounds() {
    if (data_points.empty()) return;
    
    // Find day boundaries in the data
    std::vector<int> day_boundaries;
    day_boundaries.push_back(0); // Start with first point
    
    time_t last_day = -1;
    for (size_t i = 0; i < data_points.size(); i++) {
        time_t time_val = (time_t)data_points[i].timestamp;
        struct tm* timeinfo = localtime(&time_val);
        
        if (timeinfo) {
            int current_day = timeinfo->tm_yday; // Day of year
            int current_year = timeinfo->tm_year;
            
            // Check if we crossed into a new day
            if (last_day != -1) {
                struct tm last_tm;
                time_t last_time = (time_t)data_points[i-1].timestamp;
                localtime_r(&last_time, &last_tm);
                
                if (current_day != last_tm.tm_yday || current_year != last_tm.tm_year) {
                    day_boundaries.push_back(i);
                }
            }
            last_day = current_day;
        }
    }
    day_boundaries.push_back(data_points.size() - 1); // End with last point
    
    // Draw alternating backgrounds for each day
    for (size_t i = 0; i < day_boundaries.size() - 1; i++) {
        int start_idx = day_boundaries[i];
        int end_idx = day_boundaries[i + 1];
        
        // Calculate X positions
        int x1 = indexToX(start_idx);
        int x2 = indexToX(end_idx);
        
        // Alternate between white (255) and very light gray (245)
        uint8_t color = (i % 2 == 0) ? 0xff : 0xEE;
        
        // Fill the background rectangle
        epd_fill_rect(x1, plot_y, x2 - x1, plot_height, color, fb);
    }
}

void Chart::drawData() {
    if (data_points.size() < 2) return;
    
    // Draw line connecting data points
    for (size_t i = 0; i < data_points.size() - 1; i++) {
        int x1 = indexToX(i);
        int y1 = valueToY(data_points[i].value);
        int x2 = indexToX(i + 1);
        int y2 = valueToY(data_points[i + 1].value);
        
        // Draw line
        epd_draw_line(x1, y1, x2, y2, 0, fb);
        
        // Draw small circles at data points
        epd_fill_circle(x1, y1, 2, 0, fb);
    }
    
    // Draw last point
    if (!data_points.empty()) {
        int x = indexToX(data_points.size() - 1);
        int y = valueToY(data_points.back().value);
        epd_fill_circle(x, y, 2, 0, fb);
    }
}

void Chart::drawYLabels() {
    char label[16];
    
    for (int i = 0; i <= GRID_DIVISIONS_Y; i++) {
        float value = y_max - (i * (y_max - y_min)) / GRID_DIVISIONS_Y;
        int y = plot_y + (i * plot_height) / GRID_DIVISIONS_Y;
        
        snprintf(label, sizeof(label), "%.1f", value);
        
        // Draw label to the left of the Y-axis
        int32_t label_x = plot_x - 35;
        int32_t label_y = y + 4; // Center text vertically
        
        write_string(font, label, &label_x, &label_y, fb);
    }
}

void Chart::drawXLabels() {
    // Override in derived classes to show time labels
    // Default implementation shows point indices
    for (int i = 0; i <= GRID_DIVISIONS_X; i++) {
        int32_t x = plot_x + (i * plot_width) / GRID_DIVISIONS_X;
        int32_t y = plot_y + plot_height + 15;
        
        char label[16];
        int point_index = (i * (data_points.size() - 1)) / GRID_DIVISIONS_X;
        snprintf(label, sizeof(label), "%d", point_index);
        
        write_string(font, label, &x, &y, fb);
    }
}

void Chart::drawTitle() {
    // Override in derived classes to show custom title
}

void Chart::draw() {
    // Draw day backgrounds first (behind everything)
    drawDayBackgrounds();
    
    // Draw grid (above backgrounds, behind data)
    drawGrid();
    
    // Draw axes
    drawAxes();
    
    // Draw data
    drawData();
    
    // Draw labels
    drawYLabels();
    drawXLabels();
    
    // Draw title
    drawTitle();
}

void Chart::update(const std::vector<DataPoint>& data) {
    setData(data);
    draw();
}
