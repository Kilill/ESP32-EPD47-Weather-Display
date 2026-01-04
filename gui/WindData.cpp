/**
 * @file WindData.cpp
 * @brief Wind data display widget implementation
 * 
 * Converts wind direction to 16-point cardinal directions and formats
 * wind speed and gust data as text. Uses degree-to-cardinal conversion
 * algorithm with 22.5° sectors for accurate direction naming.
 */

#include "WindData.h"
#include "WeatherUI.h"
#include "epd_driver.h"
#include <cmath>
#include <cstdio>
#include "esp_log.h"

static const char* TAG = "WindData";

// 16-point cardinal directions
static const char* CARDINAL_DIRECTIONS[16] = {
    "N", "NNE", "NE", "ENE",
    "E", "ESE", "SE", "SSE",
    "S", "SSW", "SW", "WSW",
    "W", "WNW", "NW", "NNW"
};

WindData::WindData(int x, int y, const GFXfont* font)
    : x_(x)
    , y_(y)
    , width_(0)
    , height_(0)
    , direction_(0.0f)
    , speed_(0.0f)
    , gust_(0.0f)
    , font_(font) 
    , font_props ( { .fg_color = 0, .bg_color = 0x0F, .fallback_glyph = 0, .flags = 0 } ) {
    
    // Calculate dimensions based on glyph sizes
    
    width_ = 400;
    
    height_ = (font->ascender + font->descender + 30) * 3 ; // 3 lines + spacing
    
    //ESP_LOGI(TAG, "WindData initialized at (%d, %d) with calculated width %d, height %d", 
             //x_, y_, width_, height_);
}

const char* WindData::degreesToCardinal(float degrees) const {
    // Normalize to 0-359
    while (degrees < 0) degrees += 360.0f;
    while (degrees >= 360.0f) degrees -= 360.0f;
    
    // Each cardinal direction covers 22.5 degrees (360/16)
    // Offset by 11.25 to center each direction in its range
    int index = static_cast<int>((degrees + 11.25f) / 22.5f) % 16;
    
    return CARDINAL_DIRECTIONS[index];
}

void WindData::draw(float direction, float speed, float gust) {
    uint8_t* framebuffer = WeatherUI::getInstance().getFramebuffer();
    if (framebuffer == nullptr) return;
    
    direction_ = direction;
    speed_ = speed;
    gust_ = gust;
    
    // Normalize direction to 0-359 range
    while (direction_ < 0) direction_ += 360.0f;
    while (direction_ >= 360.0f) direction_ -= 360.0f;

    // Draw components
    drawDirectionLine();
    drawSpeedLine();
    
    //ESP_LOGI(TAG, "WindData drawn: Dir=%.0f° (%s), Speed=%.1f m/s, Gust=%.1f m/s",
    //         direction_, degreesToCardinal(direction_), speed_, gust_);
}


 void WindData::drawDirectionLine() {
    uint8_t* framebuffer = WeatherUI::getInstance().getFramebuffer();
    // Format: "Direction: 123° (SE)"
    char dir_str[32];
    snprintf(dir_str, sizeof(dir_str), "%.0f° (%s)", 
             direction_, degreesToCardinal(direction_));
    int32_t x=x_+50;
    int32_t y=y_+font_->ascender;    
    write_mode(font_, dir_str, &x, &y, framebuffer, BLACK_ON_WHITE, &font_props);
}

void WindData::drawSpeedLine() {
    uint8_t* framebuffer = WeatherUI::getInstance().getFramebuffer();
    // Format: "5.2 m/s (8.1 m/s)"
    char speed_str[20];
    const int h = (font_->ascender + font_->descender + 70) ; // line + spacing
    int32_t x = x_;
    int32_t y = y_ + h;
    snprintf(speed_str, sizeof(speed_str), "  %.1f m/s", speed_);
    write_mode(font_, speed_str, &x, &y, framebuffer, BLACK_ON_WHITE, &font_props);

    x = x_;
    y += 50;
    snprintf(speed_str, sizeof(speed_str), " (%.1f m/s)", gust_);
    write_mode(font_, speed_str, &x, &y, framebuffer, BLACK_ON_WHITE, &font_props);
}

void WindData::update(float direction, float speed, float gust) {
    draw(direction, speed, gust);
}
