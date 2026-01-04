/**
 * @file Windrose.h
 * @brief Compass/windrose widget interface
 * 
 * Displays wind direction as a compass with:
 * - Circular compass rose with cardinal direction labels (N, S, E, W)
 * - Directional needle pointing to wind direction
 * - 360-degree coverage (0=North, 90=East, 180=South, 270=West)
 */

#ifndef WINDROSE_HPP
#define WINDROSE_HPP

#include <cstdint>
#include "epd_driver.h"

/**
 * @brief Windrose (compass) widget class
 * 
 * This class manages a windrose widget that displays wind direction
 * with a compass needle. The widget can be updated to show different
 * directions without redrawing the entire compass.
 */
class Windrose {
public:
    /**
     * @brief Construct a new Windrose widget
     * 
     * @param x X coordinate of the windrose square's upper left corner
     * @param y Y coordinate of the windrose square's upper left corner
     * @param size Side length of the square containing the entire windrose (including cardinal labels)
     * @param font Pointer to the font for cardinal labels
     */
    Windrose(int x, int y, int size, const GFXfont* font);

    /**
     * @brief Draw the windrose widget
     * 
     * @param direction Wind direction in degrees (0=North, 90=East, 180=South, 270=West)
     */
    void draw(int direction);

    /**
     * @brief Update the wind direction and redraw the widget
     * 
     * @param direction New wind direction in degrees
     */
    void update(int direction);

    /**
     * @brief Get the current direction
     * 
     * @return Current wind direction in degrees
     */
    int getDirection() const { return direction; }

    /**
     * @brief Get the center X coordinate
     */
    int getCenterX() const { return center_x; }

    /**
     * @brief Get the center Y coordinate
     */
    int getCenterY() const { return center_y; }

    /**
     * @brief Get the radius
     */
    int getRadius() const { return radius; }

    /**
     * @brief Get the square size
     */
    int getSize() const { return size; }

private:
    const GFXfont* font;        // Font for cardinal labels
    int center_x;
    int center_y;
    int x;
    int y;
    int size;
    int radius;
    int direction;
    int label_width;
    int label_height;
    const int GAP=5;
    FontProperties font_props;

    void drawCompassRing();
    void drawNeedle(int direction);
    void drawNeedleWithColor(int direction, uint8_t color);

    #ifdef Windrose_DEBUG
    static const char* TAG;
    #endif

};

#endif // WINDROSE_HPP
