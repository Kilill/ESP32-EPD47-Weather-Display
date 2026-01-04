/**
 * @file Status.cpp
 * @brief System status widget implementation
 * 
 * Renders system indicators using geometric shapes:
 * - WiFi: Concentric arcs with dot at base
 * - Battery: Rectangular battery icon with fill level
 * - MQTT: Pub/sub broker topology (central square with 3 connected nodes)
 * - Error: Triangle with exclamation mark
 * - Time/Date: Formatted time and date display
 */

#include "Status.h"
#include "epd_driver.h"
#include "Fonts/mplus_rounded_1c_medium_12.h"
#include <cstring>
#include <cmath>
#include <ctime>
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

extern "C" {
    #include "wifi_manager.h"
    #include "mqtt_handler.h"
    #include "esp_wifi.h"
}

Status::Status(int x, int y, int w, int h, uint8_t* framebuffer)
    : x_pos(x), y_pos(y), width(w), height(h), fb(framebuffer),
      wifi_status(WiFiStatus::DISCONNECTED),
      battery_level(BatteryLevel::MEDIUM),
      mqtt_status(MQTTStatus::DISCONNECTED),
      error_status(ErrorStatus::NO_ERROR),
      power_mode(PowerMode::BATTERY),
      battery_percentage(50) {
}

void Status::updateWiFi(WiFiStatus status) {
    wifi_status = status;
}

void Status::updateBattery(BatteryLevel level) {
    battery_level = level;
}

void Status::updateMQTT(MQTTStatus status) {
    mqtt_status = status;
}

void Status::updateError(ErrorStatus status) {
    error_status = status;
}

void Status::draw() {
    drawTimeDate();
    drawWiFi();
    drawBattery();
    drawMQTT();
    drawError();
}

void Status::drawWiFi() {
    // Draw WiFi symbol with proper circular arcs pointing upward - scaled to 80% (51x51)
    // Position adjusted to be below time/date display
    int x = x_pos;
    int y = y_pos + 52;  // Offset to be below time/date
    const int symbol_size = 51;  // 80% of 64
    const int center_x = x + 25;
    const int base_y = y + symbol_size - 1;  // Bottom of symbol
    
    if (wifi_status == WiFiStatus::DISCONNECTED) {
        // Draw X through the symbol
        epd_draw_line(x, y, x + symbol_size - 1, y + symbol_size - 1, 0, fb);
        epd_draw_line(x + symbol_size - 1, y, x, y + symbol_size - 1, 0, fb);
    } else {
        // Draw WiFi base (small circle/dot at bottom)
        epd_fill_circle(center_x, base_y - 4, 3, 0, fb);
        
        // Draw arcs based on signal strength using proper circular arcs
        // Arcs are drawn as partial circles, only the top portion
        if (wifi_status >= WiFiStatus::WEAK) {
            // First arc (smallest, closest to base)
            int radius1 = 10;
            for (int angle = 180; angle <= 360; angle += 2) {
                float rad = angle * 3.14159f / 180.0f;
                int arc_x = center_x + (int)(radius1 * cosf(rad));
                int arc_y = base_y - 8 + (int)(radius1 * sinf(rad));
                epd_draw_pixel(arc_x, arc_y, 0, fb);
                epd_draw_pixel(arc_x, arc_y - 1, 0, fb);  // Thicker line
            }
        }
        
        if (wifi_status >= WiFiStatus::MEDIUM) {
            // Second arc (medium)
            int radius2 = 18;
            for (int angle = 180; angle <= 360; angle += 2) {
                float rad = angle * 3.14159f / 180.0f;
                int arc_x = center_x + (int)(radius2 * cosf(rad));
                int arc_y = base_y - 8 + (int)(radius2 * sinf(rad));
                epd_draw_pixel(arc_x, arc_y, 0, fb);
                epd_draw_pixel(arc_x, arc_y - 1, 0, fb);  // Thicker line
            }
        }
        
        if (wifi_status >= WiFiStatus::STRONG) {
            // Third arc (largest, outermost)
            int radius3 = 26;
            for (int angle = 180; angle <= 360; angle += 2) {
                float rad = angle * 3.14159f / 180.0f;
                int arc_x = center_x + (int)(radius3 * cosf(rad));
                int arc_y = base_y - 8 + (int)(radius3 * sinf(rad));
                epd_draw_pixel(arc_x, arc_y, 0, fb);
                epd_draw_pixel(arc_x, arc_y - 1, 0, fb);  // Thicker line
            }
        }
    }
}

void Status::drawBattery() {
    // Position adjusted to be below time/date display
    const int spacing = 50;  // 80% of 96
    int x = x_pos + spacing;
    int y = y_pos + 52 + 13;  // Offset to be below time/date

    // Read battery via ADC (GPIO 36)
    int voltage_mv = readBatteryADC();
    if (voltage_mv > 4500) {
        // USB powered (> 4.5V indicates USB)
        power_mode = PowerMode::USB;
        battery_percentage = 100;
    } else {
        power_mode = PowerMode::BATTERY;
        // Convert voltage to percentage (3.0V = 0%, 4.2V = 100%)
        battery_percentage = ((voltage_mv - 3000) * 100) / (4200 - 3000);
        if (battery_percentage < 0) battery_percentage = 0;
        if (battery_percentage > 100) battery_percentage = 100;
    }
    
    // Update battery level enum based on percentage
    if (battery_percentage < 20) {
        battery_level = BatteryLevel::CRITICAL;
    } else if (battery_percentage < 40) {
        battery_level = BatteryLevel::LOW;
    } else if (battery_percentage < 70) {
        battery_level = BatteryLevel::MEDIUM;
    } else {
        battery_level = BatteryLevel::HIGH;
    }

    // If running on USB power only, display "USB" text instead of battery
    if (power_mode == PowerMode::USB) {
        // Use 12pt font
        const GFXfont* font = &MPLUSRounded1c_Medium_12;
        
        int32_t cursor_x = x;
        int32_t cursor_y = y + 18;  // Vertically center text
        
        write_string(font, "USB", &cursor_x, &cursor_y, fb);
        return;
    }

    // Draw battery outline (38x22 rectangle)
    epd_draw_rect(x, y, 38, 22, 0, fb);
    epd_draw_rect(x + 1, y + 1, 36, 20, 0, fb); // Thicker outline
    
    // Battery terminal (small bump on right)
    epd_fill_rect(x + 38, y + 6, 6, 10, 0, fb);
    
    // Fill battery based on actual percentage from ADC
    int fill_width = 0;
    if (battery_percentage >= 0) {
        // Calculate fill width based on percentage (0-100%)
        fill_width = (battery_percentage * 32) / 100;  // 32 is max fill width
        if (fill_width > 32) fill_width = 32;
    }
    
    if (fill_width > 0) {
        epd_fill_rect(x + 3, y + 3, fill_width, 16, 0, fb);
    }

    // Draw lightning bolt if charging
    if (power_mode == PowerMode::CHARGING) {
        drawLightning(x + 14, y + 4);
    }
}

void Status::drawLightning(int x, int y) {
    // Draw a small lightning bolt symbol (14x14)
    // Lightning shape: zig-zag pattern
    
    // Top part of lightning (left side going down-right)
    epd_draw_line(x + 7, y, x + 4, y + 6, 255, fb);      // Top to middle-left
    epd_draw_line(x + 4, y + 6, x + 7, y + 6, 255, fb);  // Middle horizontal
    epd_draw_line(x + 7, y + 6, x + 3, y + 14, 255, fb); // Middle to bottom
    
    // Fill the lightning (right side going up-left)
    epd_draw_line(x + 3, y + 14, x + 6, y + 8, 255, fb); // Bottom to middle-right
    epd_draw_line(x + 6, y + 8, x + 3, y + 8, 255, fb);  // Middle horizontal
    epd_draw_line(x + 3, y + 8, x + 7, y, 255, fb);      // Middle to top
    
    // Make it more visible with thicker lines
    epd_draw_line(x + 7, y + 1, x + 4, y + 7, 255, fb);
    epd_draw_line(x + 6, y + 7, x + 2, y + 13, 255, fb);
}

void Status::drawMQTT() {
    // Draw MQTT logo - scaled to 80%
    // Position adjusted to be below time/date display
    const int spacing = 50;  // 80% of 96
    int x = x_pos + spacing * 2;
    int y = y_pos + 52 + 13;  // Offset to be below time/date (aligned with battery)
    
    if (mqtt_status == MQTTStatus::CONNECTED) {
        // MQTT logo: central square (broker) with lines to 3 nodes
        // Central square (broker) - 12x12
        int center_x = x + 18;
        int center_y = y + 13;
        epd_fill_rect(center_x - 6, center_y - 6, 12, 12, 0, fb);
        
        // Top node (subscriber)
        epd_fill_circle(center_x, center_y - 20, 3, 0, fb);
        epd_draw_line(center_x, center_y - 6, center_x, center_y - 17, 0, fb);
        epd_draw_line(center_x - 1, center_y - 6, center_x - 1, center_y - 17, 0, fb);
        
        // Bottom-left node (publisher)
        epd_fill_circle(center_x - 14, center_y + 14, 3, 0, fb);
        epd_draw_line(center_x - 6, center_y + 6, center_x - 11, center_y + 11, 0, fb);
        epd_draw_line(center_x - 5, center_y + 5, center_x - 10, center_y + 10, 0, fb);
        
        // Bottom-right node (publisher)
        epd_fill_circle(center_x + 14, center_y + 14, 3, 0, fb);
        epd_draw_line(center_x + 6, center_y + 6, center_x + 11, center_y + 11, 0, fb);
        epd_draw_line(center_x + 5, center_y + 5, center_x + 10, center_y + 10, 0, fb);
    } else {
        // Draw disconnected icon - central square with X through it
        int center_x = x + 18;
        int center_y = y + 13;
        epd_draw_rect(center_x - 6, center_y - 6, 12, 12, 128, fb);
        epd_draw_line(center_x - 6, center_y - 6, center_x + 6, center_y + 6, 128, fb);
        epd_draw_line(center_x + 6, center_y - 6, center_x - 6, center_y + 6, 128, fb);
    }
}

void Status::drawError() {
    // Draw error symbol (exclamation mark in triangle) - scaled to 80% (45x45)
    // Position adjusted to be below time/date display
    const int spacing = 5;  // 80% of 96
    int x = x_pos + spacing * 3 + 77;
    int y = y_pos + 52;  // Offset to be below time/date
    const int symbol_size = 45;  // 80% of 56
    
    if (error_status == ErrorStatus::ERROR) {
        // Draw triangle outline (pointing up, 45 pixels wide, 45 pixels tall)
        epd_draw_line(x + 22, y, x, y + symbol_size, 0, fb);      // Left side
        epd_draw_line(x + 24, y, x + 2, y + symbol_size, 0, fb);  // Left side (thicker)
        epd_draw_line(x + 22, y, x + symbol_size, y + symbol_size, 0, fb); // Right side
        epd_draw_line(x + 24, y, x + symbol_size + 2, y + symbol_size, 0, fb); // Right side (thicker)
        epd_draw_line(x, y + symbol_size, x + symbol_size, y + symbol_size, 0, fb); // Bottom
        epd_draw_line(x, y + symbol_size - 2, x + symbol_size, y + symbol_size - 2, 0, fb); // Bottom (thicker)
        
        // Draw exclamation mark
        // Vertical line (stem of exclamation) - thicker
        epd_draw_vline(x + 22, y + 10, 22, 0, fb);
        epd_draw_vline(x + 24, y + 10, 22, 0, fb);
        
        // Dot at bottom - larger
        epd_fill_circle(x + 22, y + 38, 3, 0, fb);
    }
    // No drawing if no error
}

void Status::drawTimeDate() {
    // Get current time
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    
    // Format date and time as "DD MMM HH:MM" (e.g., "15 Jan 14:30")
    char datetime_str[32];
    strftime(datetime_str, sizeof(datetime_str), "%d %b %H:%M", &timeinfo);
    
    // Calculate text bounds to center the text
    int32_t x1, y1, w, h;
    int32_t cursor_x = 0;
    int32_t cursor_y = 0;
    get_text_bounds((GFXfont*)&MPLUSRounded1c_Medium_12, datetime_str, 
                    &cursor_x, &cursor_y, &x1, &y1, &w, &h, NULL);
    
    // Center the text within the status widget width
    int32_t datetime_x = x_pos + (width - w) / 2-20;
    int32_t datetime_y = y_pos + 50;
    write_string((GFXfont*)&MPLUSRounded1c_Medium_12, datetime_str, &datetime_x, &datetime_y, fb);
}

void Status::updateFromSystem() {
    // Update WiFi status
    if (wifi_manager_is_connected()) {
        // Get WiFi RSSI to determine signal strength
        wifi_ap_record_t ap_info;
        if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
            int8_t rssi = ap_info.rssi;
            
            // Convert RSSI to WiFi status level
            // Typical RSSI ranges: -30 (excellent) to -90 (poor)
            if (rssi > -50) {
                wifi_status = WiFiStatus::STRONG;
            } else if (rssi > -70) {
                wifi_status = WiFiStatus::MEDIUM;
            } else {
                wifi_status = WiFiStatus::WEAK;
            }
        } else {
            // Connected but can't get RSSI, assume medium
            wifi_status = WiFiStatus::MEDIUM;
        }
    } else {
        wifi_status = WiFiStatus::DISCONNECTED;
    }
    
    // Update MQTT status
    if (mqtt_client_is_connected()) {
        mqtt_status = MQTTStatus::CONNECTED;
    } else {
        mqtt_status = MQTTStatus::DISCONNECTED;
    }
    
    // Draw updated status
    draw();
}

int Status::readBatteryADC() {
    // Battery connected to GPIO 36 (ADC1_CHANNEL_0)
    // Voltage divider: 2x (so 4.2V battery reads as ~2.1V at ADC)
    static adc_oneshot_unit_handle_t adc1_handle = nullptr;
    static adc_cali_handle_t adc1_cali_handle = nullptr;
    static bool adc_initialized = false;
    
    if (!adc_initialized) {
        // Initialize ADC oneshot
        adc_oneshot_unit_init_cfg_t init_config = {};
        init_config.unit_id = ADC_UNIT_1;
        ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc1_handle));
        
        // Configure channel
        adc_oneshot_chan_cfg_t config = {};
        config.bitwidth = ADC_BITWIDTH_DEFAULT;
        config.atten = ADC_ATTEN_DB_12;
        ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL_0, &config));
        
        // Initialize calibration
        adc_cali_line_fitting_config_t cali_config = {};
        cali_config.unit_id = ADC_UNIT_1;
        cali_config.atten = ADC_ATTEN_DB_12;
        cali_config.bitwidth = ADC_BITWIDTH_DEFAULT;
        ESP_ERROR_CHECK(adc_cali_create_scheme_line_fitting(&cali_config, &adc1_cali_handle));
        
        adc_initialized = true;
    }
    
    // Read ADC value multiple times and average
    int sum = 0;
    const int samples = 10;
    for (int i = 0; i < samples; i++) {
        int raw_value;
        adc_oneshot_read(adc1_handle, ADC_CHANNEL_0, &raw_value);
        sum += raw_value;
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    int adc_reading = sum / samples;
    
    // Convert to voltage using calibration
    int voltage_mv;
    adc_cali_raw_to_voltage(adc1_cali_handle, adc_reading, &voltage_mv);
    
    // Account for voltage divider (2:1 ratio)
    voltage_mv *= 2;
    
    return voltage_mv;
}
