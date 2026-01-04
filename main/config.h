/**
 * @file config.h
 * @brief Main configuration file for ESP32 weather station
 * 
 * Defines all configuration constants for display, network, MQTT, HTTP, and
 * weather data storage. Includes secrets.h for sensitive credentials.
 * 
 * @note Edit secrets.h (not in git) for WiFi/MQTT credentials
 */

#ifndef CONFIG_H
#define CONFIG_H

// Include the secrets file (not in git)
#include "secrets.h"

// =============================================================================
// DISPLAY CONFIGURATION
// =============================================================================

// Display update interval in minutes
#define DISPLAY_UPDATE_INTERVAL_MINUTES     5

// Display dimensions - adjust for your Lillygo T5 model
// Common sizes:
//   2.13": 250x122 or 212x104
//   2.9":  296x128
//   4.2":  400x300
//   7.5":  960x540 or 800x480
#define DISPLAY_WIDTH   960
#define DISPLAY_HEIGHT  540

// =============================================================================
// NETWORK CONFIGURATION
// =============================================================================

// WiFi retry settings
#define WIFI_MAX_RETRY_COUNT        5
#define WIFI_RECONNECT_DELAY_MS     5000

// =============================================================================
// MQTT CONFIGURATION
// =============================================================================

// MQTT topics for weather data (multiple topics)
#define MQTT_TOPIC_TEMP         "weather/temp"
#define MQTT_TOPIC_HUMIDITY     "weather/humidity"
#define MQTT_TOPIC_PRESSURE     "weather/pressure"
#define MQTT_TOPIC_WIND         "weather/wind"
#define MQTT_TOPIC_RAIN         "weather/rain"       // Optional if available

// MQTT QoS level (0, 1, or 2)
#define MQTT_QOS_LEVEL              0

// MQTT keepalive interval in seconds
#define MQTT_KEEPALIVE_SEC          120

// =============================================================================
// HTTP CONFIGURATION
// =============================================================================

// HTTP API endpoint for historical data
// Using hourly resolution for 7-day history (168 data points)
#define HTTP_HISTORY_ENDPOINT       "/weatherAPI/history?days=7&resolution=1h"

// HTTP API endpoint for current weather data
#define HTTP_CURRENT_ENDPOINT       "/weatherAPI/current"

// HTTP request timeout in milliseconds
#define HTTP_TIMEOUT_MS             30000

// HTTP buffer size for responses (64KB for 7 days of hourly data)
#define HTTP_BUFFER_SIZE            65536

// =============================================================================
// WEATHER DATA CONFIGURATION
// =============================================================================

// Number of days of historical data to store
#define WEATHER_HISTORY_DAYS        7

// Data points per day (24 = hourly)
#define WEATHER_POINTS_PER_DAY      24

// =============================================================================
// POWER MANAGEMENT CONFIGURATION
// =============================================================================

// Light sleep duration in seconds between display updates
#define SLEEP_DURATION_SECONDS      60

// =============================================================================
// TIME CONFIGURATION
// =============================================================================

// Timezone string (POSIX format)
// Examples:
//   CET (Central European):     "CET-1CEST,M3.5.0,M10.5.0/3"
//   EST (US Eastern):           "EST5EDT,M3.2.0,M11.1.0"
//   PST (US Pacific):           "PST8PDT,M3.2.0,M11.1.0"
//   GMT (Greenwich):            "GMT0"
//   JST (Japan):                "JST-9"
#define TIMEZONE_STRING             "CET-1CEST,M3.5.0,M10.5.0/3"

// SNTP server for time synchronization
#define SNTP_SERVER                 "pool.ntp.org"

// =============================================================================
// LOGGING CONFIGURATION
// =============================================================================

// Enable/disable debug logging (0 = disabled, 1 = enabled)
#define DEBUG_LOGGING               1

#endif // CONFIG_H
