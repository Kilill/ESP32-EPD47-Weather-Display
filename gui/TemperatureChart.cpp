/**
 * @file TemperatureChart.cpp
 * @brief Temperature chart widget implementation
 * 
 * Renders temperature history as line chart with:
 * - Auto-scaling Y-axis (minimum 5°C range)
 * - °C unit label above Y-axis
 * - HH:MM time labels on X-axis
 */

#include "TemperatureChart.h"
#include <cstdio>
#include <ctime>

#include "Fonts/mplus_rounded_1c_regular_8.h"
#define smallFont MPLUSRounded1c_Regular_8

TemperatureChart::TemperatureChart(int x, int y, int width, int height, uint8_t* framebuffer, const GFXfont* font)
    : Chart(x, y, width, height, framebuffer, font) {
    
    // Set typical temperature range (-10 to 40°C)
    // Can be auto-scaled based on actual data
    setYRange(-10.0f, 40.0f);
    setAutoScale(true);
}

void TemperatureChart::updateTempData(const std::vector<float>& temperatures, const std::vector<uint32_t>& timestamps) {
    std::vector<DataPoint> points;
    
    size_t count = std::min(temperatures.size(), timestamps.size());
    for (size_t i = 0; i < count; i++) {
        DataPoint point;
        point.value = temperatures[i];
        point.timestamp = timestamps[i];
        points.push_back(point);
    }
    
    setData(points);
    draw();
}

void TemperatureChart::drawTitle() {
    const char* title = "Temperature";
    
    int32_t title_x = plot_x + plot_width / 2 - 50; // Center approximately
    int32_t title_y = plot_y - 10;
    
    write_string(font, title, &title_x, &title_y, fb);
}

void TemperatureChart::drawYLabels() {
    char label[16];
    
    // Draw unit label above the top tick
    int32_t unit_x = plot_x - 35;
    int32_t unit_y = plot_y - 15;
    write_string(&smallFont, "°C", &unit_x, &unit_y, fb);
    
    for (int i = 0; i <= GRID_DIVISIONS_Y; i++) {
        float value = y_max - (i * (y_max - y_min)) / GRID_DIVISIONS_Y;
        int y = plot_y + (i * plot_height) / GRID_DIVISIONS_Y;
        
        snprintf(label, sizeof(label), "%.0f", value);
        
        int32_t label_x = plot_x - 35;
        int32_t label_y = y + 4;
        
        write_string(&smallFont, label, &label_x, &label_y, fb);
    }
}

void TemperatureChart::drawXLabels() {
    if (data_points.empty()) return;
    
    // Show date/time labels across the data range
    for (int i = 0; i <= GRID_DIVISIONS_X; i++) {
        int32_t x = plot_x + (i * plot_width) / GRID_DIVISIONS_X;
        int32_t y = plot_y + plot_height + 18;
        
        size_t point_index = (i * (data_points.size() - 1)) / GRID_DIVISIONS_X;
        if (point_index >= data_points.size()) point_index = data_points.size() - 1;
        
        uint32_t timestamp = data_points[point_index].timestamp;
        time_t time_val = (time_t)timestamp;
        struct tm* timeinfo = localtime(&time_val);
        
        char label[16];
        if (timeinfo) {
            // Format as DD/MM for multi-day data
            snprintf(label, sizeof(label), "%02d/%02d", timeinfo->tm_mday, timeinfo->tm_mon + 1);
        } else {
            snprintf(label, sizeof(label), "%d", (int)point_index);
        }
        
        write_string(&smallFont, label, &x, &y, fb);
    }
}

void TemperatureChart::calculateAutoScale() {
    if (data_points.empty()) {
        y_min = -10.0f;
        y_max = 40.0f;  // Default range
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
    
    // Ensure minimum range of 5°C
    float current_range = y_max - y_min;
    if (current_range < 5.0f) {
        float center = (y_max + y_min) / 2.0f;
        y_min = center - 2.5f;
        y_max = center + 2.5f;
    }
}
