/**
 * @file WindData.h
 * @brief Wind data display widget interface
 * 
 * Displays textual wind information including:
 * - Wind direction (degrees and 16-point cardinal: N, NNE, NE, etc.)
 * - Current wind speed in m/s
 * - Gust speed in m/s
 * 
 * Formatted as compact multi-line text display.
 */

#ifndef WINDDATA_HPP
#define WINDDATA_HPP

#include <cstdint>
#include "epd_driver.h"

/**
 * @brief WindData widget class
 * 
 * This class displays wind information including direction (as degrees and cardinal),
 * wind speed, and gust speed in a compact two-line format.
 */
class WindData {
public:
    /**
     * @brief Construct a new WindData widget
     * 
     * @param x X coordinate of the widget's left edge
     * @param y Y coordinate of the widget's top edge
     * @param font Pointer to the font for text rendering
     * 
     * Width and height are automatically calculated based on font glyph sizes
     */
    WindData(int x, int y, const GFXfont* font);

    /**
     * @brief Draw the wind data widget
     * 
     * @param direction Wind direction in degrees (0-359, 0=North, 90=East, etc.)
     * @param speed Wind speed in m/s
     * @param gust Gust speed in m/s
     */
    void draw(float direction, float speed, float gust);

    /**
     * @brief Update the wind data and redraw the widget
     * 
     * @param direction Wind direction in degrees (0-359)
     * @param speed Wind speed in m/s
     * @param gust Gust speed in m/s
     */
    void update(float direction, float speed, float gust);

    /**
     * @brief Get the current wind direction
     * 
     * @return Current wind direction in degrees
     */
    float getDirection() const { return direction_; }

    /**
     * @brief Get the current wind speed
     * 
     * @return Current wind speed in m/s
     */
    float getSpeed() const { return speed_; }

    /**
     * @brief Get the current gust speed
     * 
     * @return Current gust speed in m/s
     */
    float getGust() const { return gust_; }

    /**
     * @brief Get the width
     */
    int getWidth() const { return width_; }

    /**
     * @brief Get the height
     */
    int getHeight() const { return height_; }

private:
    int x_;
    int y_;
    int width_;
    int height_;
    float direction_;  // Wind direction in degrees
    float speed_;      // Wind speed in m/s
    float gust_;       // Gust speed in m/s
    const GFXfont* font_;
    const FontProperties font_props;

    void drawLine(const char* str, const int x_, const int y_,const int width_);
    /**
     * @brief Convert degrees to 16-point cardinal direction
     * 
     * @param degrees Wind direction in degrees (0-359)
     * @return Cardinal direction string (N, NNE, NE, ENE, E, etc.)
     */
    const char* degreesToCardinal(float degrees) const;

    /**
     * @brief Draw the direction line (degrees and cardinal)
     */
    void drawDirectionLine();

    /**
     * @brief Draw the speed line (wind speed and gust)
     */
    void drawSpeedLine();
};

#endif // WINDDATA_HPP
