/**
 * @file WindSpeedChart.cpp
 * @brief Wind speed chart widget implementation
 * 
 * Renders wind speed history as line chart with:
 * - Auto-scaling Y-axis (minimum 5 m/s range, 5 m/s intervals)
 * - Y-axis always starts at 0 (wind speed can't be negative)
 * - m/s unit label above Y-axis
 * - HH:MM time labels on X-axis
 */

#include "WindSpeedChart.h"
#include <cstdio>
#include <ctime>
#include <cmath>

#include "Fonts/mplus_rounded_1c_regular_8.h"
#define smallFont MPLUSRounded1c_Regular_8

WindSpeedChart::WindSpeedChart(int x, int y, int width, int height, uint8_t* framebuffer, const GFXfont* font)
    : Chart(x, y, width, height, framebuffer, font) {
    
    // Set typical wind speed range (0-20 m/s)
    // Can be auto-scaled based on actual data
    setYRange(0.0f, 20.0f);
    setAutoScale(true);
}

void WindSpeedChart::updateWindData(const std::vector<float>& wind_speeds, const std::vector<uint32_t>& timestamps) {
    std::vector<DataPoint> points;
    
    size_t count = std::min(wind_speeds.size(), timestamps.size());
    for (size_t i = 0; i < count; i++) {
        DataPoint point;
        point.value = wind_speeds[i];
        point.timestamp = timestamps[i];
        points.push_back(point);
    }
    
    setData(points);
    draw();
}

void WindSpeedChart::drawTitle() {
    const char* title = "Wind Speed";
    
    int32_t title_x = plot_x + plot_width / 2 - 40; // Center approximately
    int32_t title_y = plot_y - 10;
    
    write_string(font, title, &title_x, &title_y, fb);
}

void WindSpeedChart::drawYLabels() {
    char label[16];
    
    // Draw unit label above the top tick
    int32_t unit_x = plot_x - 35;
    int32_t unit_y = plot_y - 15;
    write_string(&smallFont, "m/s", &unit_x, &unit_y, fb);
    
    // Determine tick interval based on scale:
    // 0-5: interval 1
    // 0-10: interval 2
    // 0-20: interval 5
    // > 20: interval 10
    int tick_interval;
    if (y_max <= 5.0f) {
        tick_interval = 1;
    } else if (y_max <= 10.0f) {
        tick_interval = 2;
    } else if (y_max <= 20.0f) {
        tick_interval = 5;
    } else {
        tick_interval = 10;
    }
    
    int max_speed = (int)y_max;
    
    // Draw labels from max down to 0 at specified intervals
    for (int speed = max_speed; speed >= 0; speed -= tick_interval) {
        float value = (float)speed;
        
        // Calculate Y position based on value's position in range
        // y_min is always 0 for wind speed
        int y = plot_y + (int)((y_max - value) * plot_height / y_max);
        
        snprintf(label, sizeof(label), "%d", speed);
        
        int32_t label_x = plot_x - 35;
        int32_t label_y = y + 4;
        
        write_string(&smallFont, label, &label_x, &label_y, fb);
    }
}

void WindSpeedChart::drawXLabels() {
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

void WindSpeedChart::calculateAutoScale() {
    // Wind speed always starts at 0
    y_min = 0.0f;
    
    if (data_points.empty()) {
        y_max = 5.0f;  // Default minimum range
        return;
    }
    
    // Find maximum wind speed in the data
    float max_val = 0.0f;
    for (const auto& point : data_points) {
        if (point.value > max_val) max_val = point.value;
    }
    
    // Add 10% padding to max value
    float padded_max = max_val * 1.1f;
    
    // Use breakpoints for clean scaling:
    // <= 5: scale 0-5
    // > 5 and <= 10: scale 0-10
    // > 10 and <= 20: scale 0-20
    // > 20: scale to next multiple of 10
    if (padded_max <= 5.0f) {
        y_max = 5.0f;
    } else if (padded_max <= 10.0f) {
        y_max = 10.0f;
    } else if (padded_max <= 20.0f) {
        y_max = 20.0f;
    } else {
        // For values above 20, round up to next multiple of 10
        y_max = ceilf(padded_max / 10.0f) * 10.0f;
    }
}
