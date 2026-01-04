/**
 * @file Windrose.cpp
 * @brief Compass/windrose widget implementation
 * 
 * Renders a circular compass with cardinal direction labels and
 * a directional arrow pointing to current wind direction. Uses
 * trigonometry to calculate label positions and arrow orientation.
 */

#include "Windrose.h"
#include "WeatherUI.h"
#include "epd_driver.h"
#include <cmath>
#include "esp_log.h"
#include "Fonts/atkinson_hyperlegible_mono_20_bold.h"
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifdef Windrose_DEBUG
const char* Windrose::TAG = "Windrose";
#endif


Windrose::Windrose(int x_, int y_ ,int size_, const EpdFont* font_)
    : font(font_)
    , center_x(x_ + (size_ / 2))
    , center_y(y_ + (size_ / 2))
    , x(x_)
    , y(y_) 
    , size(size_)
    , radius(0)  // Will be calculated below
    , direction(0) {

    // Get actual label dimensions using get_glyph
    font_props = {
        .fg_color = 0,
        .bg_color = 15,
        .fallback_glyph = 0,
        .flags = 0
    };

    GFXglyph* glyph_ptr = nullptr;
    get_glyph(&AtkinsonHyperlegibleMono_20_Bold, 'W', &glyph_ptr);
    label_height = glyph_ptr->height;  // Actual height of this specific character
    label_width = glyph_ptr->width;
    
    // Calculate max radius considering both constraints
    radius = (size/2 - (label_height>label_width ? label_height : label_width))-5 ; 
    

    //ESP_LOGI(TAG, "Windrose initialized at (%d, %d) with size %d, radius %d, Label H/W %d/%d", x, y, size, radius, label_height, label_width);
}

void Windrose::draw(int direction_) {
    uint8_t* framebuffer = WeatherUI::getInstance().getFramebuffer();
    if (framebuffer == nullptr) return ;

    direction = direction_;

    // Clear the widget area first (entire square)
    epd_fill_rect(x, y, size, size, 0xFF, framebuffer); // White

   // Draw outer circle 
      for (int i = 0; i <= 16; i++) {
        epd_draw_circle(center_x, center_y, radius - i, i<<4, framebuffer);
    }
    
    // Draw cardinal point tick marks and labels
    int cardinal_angles[] = {0, 45, 90, 135, 180, 225, 270, 315};  // Degrees for N, NE, E, SE, S, SW, W, NW
    
    for (int i = 0; i < 8; i++) {
        double angle_rad = (cardinal_angles[i] - 90) * M_PI / 180.0;  // -90 to make 0° point up
        
        // Calculate position for tick mark (outer edge)
        int tick_outer_x = center_x + static_cast<int>((radius +5) * std::cos(angle_rad));
        int tick_outer_y = center_y + static_cast<int>((radius+5) * std::sin(angle_rad));
        
        // Calculate position for tick mark (inner point)
        int tick_inner_x = center_x + static_cast<int>((radius - 25) * std::cos(angle_rad));
        int tick_inner_y = center_y + static_cast<int>((radius - 25) * std::sin(angle_rad));
        
        // Draw tick mark - make it thicker by drawing 3 parallel lines
        for (int j = -2; j <= 2; j++) {
            double perp = angle_rad + M_PI / 2.0;
            int perp_x = static_cast<int>(j * std::cos(perp));
            int perp_y = static_cast<int>(j * std::sin(perp));
            epd_draw_line(tick_outer_x + perp_x, tick_outer_y + perp_y, 
                         tick_inner_x + perp_x, tick_inner_y + perp_y, 
                         0x00, framebuffer);
        }
    }
        
    // Draw center dot - make it bigger
    epd_fill_circle(center_x, center_y, 8, 0x00, framebuffer);
    int32_t label_x, label_y;

    FontProperties font_props_local = {
        .fg_color = 0,
        .bg_color = 15,
        .fallback_glyph = 0,
        .flags = 0
    };
    
    // N
    label_x = center_x-label_width/2+5;
    label_y = y + GAP + label_height;

    write_mode(font, "N", &label_x, &label_y, framebuffer, BLACK_ON_WHITE, &font_props_local);

    // S
    label_x = center_x-label_width/2+5;
    label_y = y + size ;
    write_mode(font, "S", &label_x, &label_y, framebuffer, BLACK_ON_WHITE, &font_props_local);
    
    // W
    label_x = x;
    label_y = center_y + label_height / 2;
    write_mode(font, "W", &label_x, &label_y, framebuffer, BLACK_ON_WHITE, &font_props_local);
   
    // E: 5 pixels from left edge + half label width, centered vertically
    label_x = x + size - label_width+5;
    label_y = center_y + label_height/2;  // Add half height for baseline centering
    write_mode(font, "E", &label_x, &label_y, framebuffer, BLACK_ON_WHITE, &font_props_local);
   
    
    // Draw the needle
    drawNeedle(direction_);
}

void Windrose::drawNeedle(int direction) {
    uint8_t* framebuffer = WeatherUI::getInstance().getFramebuffer();
    if (framebuffer == nullptr) return;
    
    // Clear the inner circle area (but not the compass ring/labels)
    // The needle extends from center-50 to center+(radius-20)
    // Clear a circle large enough to cover the old needle
    
    // Normalize direction to 0-359
    direction = direction % 360;
    if (direction < 0) direction += 360;
    
    // Draw the new needle in black
    drawNeedleWithColor(direction, 0x00); // Black
    
    // Update the stored direction
    this->direction = direction;
    
    // Redraw center dot to cover needle base
    epd_fill_circle(center_x, center_y, 8, 0x00, framebuffer);
}

void Windrose::drawNeedleWithColor(int direction, uint8_t color) {
    uint8_t* framebuffer = WeatherUI::getInstance().getFramebuffer();
    if (framebuffer == nullptr) return;
    
    // Normalize direction to 0-359
    direction = direction % 360;
    if (direction < 0) direction += 360;
    
    // Convert direction to radians (0° = North = up, clockwise)
    double needle_angle_rad = (direction - 90) * M_PI / 180.0;  // -90 to make 0° point up
    
    // Calculate needle front endpoint (pointing to wind direction)
    int needle_front_length = radius - 20;
    int needle_front_x = center_x + static_cast<int>(needle_front_length * std::cos(needle_angle_rad));
    int needle_front_y = center_y + static_cast<int>(needle_front_length * std::sin(needle_angle_rad));
    
    // Calculate needle back endpoint (50 pixels behind center)
    int needle_back_x = center_x - static_cast<int>(50 * std::cos(needle_angle_rad));
    int needle_back_y = center_y - static_cast<int>(50 * std::sin(needle_angle_rad));
    
    // Draw needle as a very thick line from back to front
    for (int offset = -5; offset <= 5; offset++) {
        // Calculate perpendicular offset
        double perp_angle = needle_angle_rad + M_PI / 2.0;
        int offset_x = static_cast<int>(offset * std::cos(perp_angle));
        int offset_y = static_cast<int>(offset * std::sin(perp_angle));
        
        epd_draw_line(needle_back_x + offset_x, needle_back_y + offset_y, 
                     needle_front_x + offset_x, needle_front_y + offset_y, 
                     color, framebuffer);
    }
    
    // Fill gaps by drawing circles along the needle path - critical for diagonal angles
    int total_needle_length = needle_front_length + 50;
    int num_circles = total_needle_length / 3;  // Draw a circle every 3 pixels
    for (int i = 0; i <= num_circles; i++) {
        float t = static_cast<float>(i) / static_cast<float>(num_circles);
        int x = needle_back_x + static_cast<int>(t * (needle_front_x - needle_back_x));
        int y = needle_back_y + static_cast<int>(t * (needle_front_y - needle_back_y));
        epd_fill_circle(x, y, 5, color, framebuffer);
    }
    
    // Draw arrowhead at the front
    int arrow_length = 20;  // Length of arrowhead sides
    double arrow_angle = 25 * M_PI / 180.0;  // 25 degree angle for arrowhead
    
    // Calculate the two points of the arrowhead
    int arrow_left_x = needle_front_x - static_cast<int>(arrow_length * std::cos(needle_angle_rad - arrow_angle));
    int arrow_left_y = needle_front_y - static_cast<int>(arrow_length * std::sin(needle_angle_rad - arrow_angle));
    
    int arrow_right_x = needle_front_x - static_cast<int>(arrow_length * std::cos(needle_angle_rad + arrow_angle));
    int arrow_right_y = needle_front_y - static_cast<int>(arrow_length * std::sin(needle_angle_rad + arrow_angle));
    
    // Draw arrowhead lines - make them thick
    for (int i = -1; i <= 1; i++) {
        double perp_angle = needle_angle_rad + M_PI / 2.0;
        int offset_x = static_cast<int>(i * std::cos(perp_angle));
        int offset_y = static_cast<int>(i * std::sin(perp_angle));
        
        epd_draw_line(needle_front_x + offset_x, needle_front_y + offset_y, 
                     arrow_left_x + offset_x, arrow_left_y + offset_y, 
                     color, framebuffer);
        epd_draw_line(needle_front_x + offset_x, needle_front_y + offset_y, 
                     arrow_right_x + offset_x, arrow_right_y + offset_y, 
                     color, framebuffer);
    }
    
    // Fill arrowhead with circles for solid appearance
    for (int i = 0; i <= 10; i++) {
        float t = static_cast<float>(i) / 10.0f;
        int x1 = needle_front_x + static_cast<int>(t * (arrow_left_x - needle_front_x));
        int y1 = needle_front_y + static_cast<int>(t * (arrow_left_y - needle_front_y));
        int x2 = needle_front_x + static_cast<int>(t * (arrow_right_x - needle_front_x));
        int y2 = needle_front_y + static_cast<int>(t * (arrow_right_y - needle_front_y));
        
        epd_fill_circle(x1, y1, 3, color, framebuffer);
        epd_fill_circle(x2, y2, 3, color, framebuffer);
    }
}

void Windrose::update(int direction) {
    draw(direction);
}

