/**
 * @file Thermometer.cpp
 * @brief Vertical thermometer gauge widget implementation
 * 
 * Renders a classic thermometer visualization with bulb at bottom,
 * vertical bar with mercury fill, scale markings, and temperature label.
 * Uses geometric shapes to create realistic thermometer appearance.
 */

#include "Thermometer.h"
#include "WeatherUI.h"
#include "epd_driver.h"
#include <cmath>
#include <cstdio>
#include "esp_log.h"

// #include "Fonts/atkinson_hyperlegible_32_bold.h"
#include "Fonts/mplus_rounded_1c_medium_32.h"
#define theFont MPLUSRounded1c_Medium_32

Thermometer::Thermometer(int x, int y, int width, int height, const GFXfont* font)
    : x_(x)
    , y_(y)
    , width_(width)
    , height_(height)
    , temperature_(0.0f)
    , font_(font) {
    
    // Calculate bar height from total widget height
    // Account for: top label space (30px), bulb (BULB_RADIUS * 2), and bottom label space (50px)
    const int TOP_LABEL_SPACE = 30;
    const int BOTTOM_SPACE = BULB_RADIUS * 2 + 70;  // Bulb + temperature label
    bar_height_ = height_ - TOP_LABEL_SPACE - BOTTOM_SPACE;
    
    #ifdef THERMOMETER_DEBUG
    //ESP_LOGI(TAG, "Thermometer initialized at (%d, %d) with width %d, total height %d, bar height %d", 
    //         x_, y_, width_, height_, bar_height_);
    #endif
}

void Thermometer::draw(float temperature) {
    uint8_t* framebuffer = WeatherUI::getInstance().getFramebuffer();
    if (framebuffer == nullptr) return;
    
    temperature_ = temperature;
    
    // Clamp temperature to valid range
    if (temperature_ < MIN_TEMP) temperature_ = MIN_TEMP;
    if (temperature_ > MAX_TEMP) temperature_ = MAX_TEMP;
    
    // Draw components
    drawScale();
    drawBar(temperature_);
    drawBulb();
    drawTemperatureLabel(temperature_);
}

void Thermometer::drawScale() {
    uint8_t* framebuffer = WeatherUI::getInstance().getFramebuffer();
    if (framebuffer == nullptr) return;
    
    const int TOP_LABEL_SPACE = 30;
    
    // Draw outer tube
    int tube_left = x_ + width_ / 4;
    int tube_right = x_ + (width_ * 3) / 4;
    int tube_top = y_ + TOP_LABEL_SPACE;
    int tube_bottom = tube_top + bar_height_;
    
    // Draw tube outline (2 pixels thick)
    for (int i = 0; i < 4; i++) {
        epd_draw_vline(tube_left + i, tube_top, tube_bottom - tube_top, 0x00, framebuffer);
        epd_draw_vline(tube_right - i, tube_top, tube_bottom - tube_top, 0x00, framebuffer);
    }
    
    // Draw tick marks and labels at 10 degree intervals
    // Tick marks at: -10, 0, 10, 20, 30, 40
    int tick_temps[] = {-10, 0, 10, 20, 30, 40};
    int num_ticks = sizeof(tick_temps) / sizeof(tick_temps[0]);
    
    FontProperties font_props = {
        .fg_color = 0,
        .bg_color = 15,
        .fallback_glyph = 0,
        .flags = 0
    };
    
    for (int i = 0; i < num_ticks; i++) {
        int temp = tick_temps[i];
        int tick_y = temperatureToY(static_cast<float>(temp));
        
        // Determine tick mark length (longer for 0 and major intervals)
        int tick_length = (temp == 0) ? 20 : 15;
        int tick_thickness = (temp == 0) ? 5 : 3;
        
        // Draw tick mark extending right from tube
        for (int t = 0; t < tick_thickness; t++) {
            epd_draw_hline(tube_right, tick_y + t, tick_length, 0x00, framebuffer);
        }
        
        // Draw temperature label
        char label[8];
        snprintf(label, sizeof(label), "%d", temp);
        
        int32_t label_x = tube_right + tick_length + 5;
        int32_t label_y = tick_y + 5;  // Approximate vertical centering
        
        write_mode(font_, label, &label_x, &label_y, framebuffer, BLACK_ON_WHITE, &font_props);
    }
    
    // Draw degree symbol "°C" label at the top
    int32_t label_x = tube_left ;
    int32_t label_y = y_ ;
    write_mode(font_, "°C", &label_x, &label_y, framebuffer, BLACK_ON_WHITE, &font_props);
}

void Thermometer::drawBar(float temperature) {
    uint8_t* framebuffer = WeatherUI::getInstance().getFramebuffer();
    if (framebuffer == nullptr) return;
    
    const int TOP_LABEL_SPACE = 30;
    
    // Calculate bar fill level
    int tube_left = x_ + width_ / 4;
    int tube_right = x_ + (width_ * 3) / 4;
    int tube_bottom = y_ + TOP_LABEL_SPACE + bar_height_;
    
    int fill_top = temperatureToY(temperature);
    
    // Fill the bar from bottom to current temperature
    // Use gradient from dark at bottom to lighter at top
    for (int y = fill_top; y < tube_bottom; y++) {
        // Calculate color gradient (darker at bottom/bulb)
        float ratio = static_cast<float>(y - fill_top) / static_cast<float>(tube_bottom - fill_top);
        uint8_t color = static_cast<uint8_t>(0x00 + ratio * 0xF0);
        
        epd_draw_hline(tube_left + 2, y, tube_right - tube_left - 4, color, framebuffer);
    }
}

void Thermometer::drawBulb() {
    uint8_t* framebuffer = WeatherUI::getInstance().getFramebuffer();
    if (framebuffer == nullptr) return;
    
    const int TOP_LABEL_SPACE = 30;
    
    // Draw bulb at bottom of bar
    int bulb_x = x_ + width_ / 2;
    int bulb_y = y_ + TOP_LABEL_SPACE + bar_height_ + BULB_RADIUS;
    
    // Draw filled circle for bulb
    epd_fill_circle(bulb_x, bulb_y, BULB_RADIUS, 0x60, framebuffer);
    
    // Draw outline
    for (int i = 0; i < 2; i++) {
        epd_draw_circle(bulb_x, bulb_y, BULB_RADIUS + i, 0x00, framebuffer);
    }
}

void Thermometer::drawTemperatureLabel(float temperature) {
    uint8_t* framebuffer = WeatherUI::getInstance().getFramebuffer();
    if (framebuffer == nullptr) return;
    
    const int TOP_LABEL_SPACE = 30;
    
    // Format temperature string with one decimal place
    // -10.9°C 
    // the "¯" below is not an error - it is a degree symbol in UTF-8 the font handling seems off by one...
    char temp_str[16];
    snprintf(temp_str, sizeof(temp_str), "%.1f°C", temperature);
    
    // Position label below the bulb
    
    FontProperties font_props = {
        .fg_color = 0,
        .bg_color = 15,
        .fallback_glyph = 0,
        .flags = 0
    };
    
    int32_t label_x = x_-50 ;
    int32_t label_y = y_ + TOP_LABEL_SPACE + bar_height_ + BULB_RADIUS + 60 + font_->ascender;
    
    write_mode(&theFont, temp_str, &label_x, &label_y, framebuffer, BLACK_ON_WHITE, &font_props);
}

int Thermometer::temperatureToY(float temp) const {
    const int TOP_LABEL_SPACE = 30;
    
    // Convert temperature to Y coordinate
    // MIN_TEMP (-15) -> bottom of bar
    // MAX_TEMP (40) -> top of bar
    
    float normalized = (temp - MIN_TEMP) / TEMP_RANGE;  // 0.0 to 1.0
    int y = y_ + TOP_LABEL_SPACE + bar_height_ - static_cast<int>(normalized * bar_height_);
    
    return y;
}

void Thermometer::update(float temperature) {
    draw(temperature);
}
