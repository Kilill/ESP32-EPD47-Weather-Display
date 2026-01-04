/**
 * @file Status.h
 * @brief System status widget interface
 * 
 * Displays WiFi signal strength, battery level, MQTT connection status,
 * error indicator, and current time/date. Positioned in top-right corner.
 */

#ifndef STATUS_H
#define STATUS_H

#include <stdint.h>
#include "epd_driver.h"

/**
 * @brief Status widget - displays WiFi, Battery, MQTT, and Error indicators
 * 
 * This widget shows system status indicators in the top-right corner
 */
class Status {
public:
    /**
     * @brief WiFi connection status levels
     */
    enum class WiFiStatus {
        DISCONNECTED = 0,
        WEAK = 1,
        MEDIUM = 2,
        STRONG = 3
    };

    /**
     * @brief Battery charge levels
     */
    enum class BatteryLevel {
        CRITICAL = 0,   // < 20%
        LOW = 1,        // 20-40%
        MEDIUM = 2,     // 40-70%
        HIGH = 3        // > 70%
    };

    /**
     * @brief Power mode
     */
    enum class PowerMode {
        BATTERY,         // Running on battery
        USB,            // Running on USB power
        CHARGING        // Battery charging
    };

    /**
     * @brief MQTT connection status
     */
    enum class MQTTStatus {
        DISCONNECTED = 0,
        CONNECTED = 1
    };

    /**
     * @brief Error indicator status
     */
    enum class ErrorStatus {
        NO_ERROR = 0,
        ERROR = 1
    };

    /**
     * @brief Constructor
     * @param x X position of the status widget
     * @param y Y position of the status widget
     * @param width Width of the status widget area
     * @param height Height of the status widget area
     * @param framebuffer Pointer to the framebuffer
     */
    Status(int x, int y, int width, int height, uint8_t* framebuffer);

    /**
     * @brief Update WiFi indicator
     * @param status WiFi connection status
     */
    void updateWiFi(WiFiStatus status);

    /**
     * @brief Update battery indicator
     * @param level Battery charge level
     */
    void updateBattery(BatteryLevel level);

    /**
     * @brief Update MQTT indicator
     * @param status MQTT connection status
     */
    void updateMQTT(MQTTStatus status);

    /**
     * @brief Update error indicator
     * @param status Error status
     */
    void updateError(ErrorStatus status);

    /**
     * @brief Draw all status indicators
     */
    void draw();

    /**
     * @brief Update status from system state and draw
     * 
     * Checks WiFi and MQTT connection status and updates the display
     */
    void updateFromSystem();

private:
    int x_pos;
    int y_pos;
    int width;
    int height;
    uint8_t* fb;
    
    WiFiStatus wifi_status;
    BatteryLevel battery_level;
    MQTTStatus mqtt_status;
    ErrorStatus error_status;
    PowerMode power_mode;
    int battery_percentage;

    /**
     * @brief Draw WiFi symbol
     */
    void drawWiFi();

    /**
     * @brief Draw battery symbol
     */
    void drawBattery();

    /**
     * @brief Draw lightning bolt for charging indicator
     * @param x X position
     * @param y Y position
     */
    void drawLightning(int x, int y);

    /**
     * @brief Draw MQTT label
     */
    void drawMQTT();

    /**
     * @brief Draw error symbol (exclamation mark in triangle)
     */
    void drawError();
    
    /**
     * @brief Draw time and date
     */
    void drawTimeDate();
    
    /**
     * @brief Read battery voltage via ADC (fallback when PMU not available)
     * @return Battery voltage in millivolts
     */
    int readBatteryADC();
};

#endif // STATUS_H
