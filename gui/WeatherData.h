/**
 * @file WeatherData.h
 * @brief Historical weather data management interface (C++ wrapper)
 * 
 * Provides C++ interface for fetching, parsing, and caching historical weather data.
 * Wraps C-based HTTP client and JSON parsing with std::vector-based data structures.
 * Implements singleton pattern with PSRAM buffer allocation for large responses.
 */

#ifndef WEATHER_DATA_HPP
#define WEATHER_DATA_HPP

#include <vector>
#include <cstdint>
#include <ctime>
#include "esp_err.h"

extern "C" {
#include "http_client.h"
}

/**
 * @brief Current weather data structure
 */
struct CurrentWeather {
    float temperature;
    float humidity;
    float pressure;
    float wind_speed;
    float wind_speed_max;
    float wind_speed_min;
    uint16_t wind_direction;
    float rainfall;
    time_t timestamp;
    bool valid;
    
    CurrentWeather() : temperature(0), humidity(0), pressure(0), 
                      wind_speed(0), wind_speed_max(0), wind_speed_min(0),
                      wind_direction(0), rainfall(0), timestamp(0), valid(false) {}
};

/**
 * @brief WeatherData class - handles all historical weather data operations
 * 
 * This class consolidates historical data fetching, parsing, and management.
 * It provides a clean C++ interface for retrieving historical weather data
 * from the HTTP API and converting it to chart-ready format.
 */
class WeatherData {
public:
    /**
     * @brief Structure to hold historical data for charts
     */
    struct HistoryData {
        std::vector<float> wind_speeds;
        std::vector<float> temperatures;
        std::vector<float> pressures;
        std::vector<uint32_t> timestamps;
        
        bool isEmpty() const { return timestamps.empty(); }
        size_t size() const { return timestamps.size(); }
    };

    /**
     * @brief Get the singleton instance of WeatherData
     * @return Reference to the WeatherData instance
     */
    static WeatherData& getInstance();

    /**
     * @brief Fetch historical data from HTTP API
     * 
     * This method fetches historical weather data from the configured HTTP endpoint,
     * parses the JSON response, and returns structured data ready for chart display.
     * If the fetch fails, returns the cached data if available, or empty data.
     * 
     * @return HistoryData structure containing historical data points
     */
    HistoryData fetchHistoricalData();
    
    /**
     * @brief Fetch current weather data from HTTP API
     * 
     * Fetches the latest weather reading from the API and updates internal storage.
     * 
     * @return true if fetch and parse successful, false otherwise
     */
    bool fetchCurrentWeather();
    
    /**
     * @brief Get the current weather data
     * 
     * @return Reference to current weather data
     */
    const CurrentWeather& getCurrentWeather() const { return current_weather; }
    
    /**
     * @brief Get the last successfully fetched historical data
     * 
     * @return Reference to cached historical data
     */
    const HistoryData& getCachedData() const { return cached_data; }

    /**
     * @brief Generate sample historical data for testing/fallback
     * 
     * @param points Number of data points to generate
     * @return HistoryData structure with sample data
     */
    HistoryData generateSampleData(int points = 24);

    // Delete copy constructor and assignment operator
    WeatherData(const WeatherData&) = delete;
    WeatherData& operator=(const WeatherData&) = delete;

private:
    WeatherData() = default;
    ~WeatherData() = default;

    /**
     * @brief Parse JSON historical data
     * 
     * @param json_data JSON string containing historical data array
     * @param data Output HistoryData structure to populate
     */
    void parseHistoryJson(const char* json_data, HistoryData& data);
    
    /**
     * @brief Parse JSON current weather data
     * 
     * @param json_data JSON string containing current weather object
     * @return true if parsing successful
     */
    bool parseCurrentJson(const char* json_data);

    /**
     * @brief Static callback for HTTP response data
     * 
     * @param data HTTP response data
     * @param len Length of response data
     */
    static void httpResponseCallback(const char *data, int len);

    // Static storage for HTTP response (used by callback)
    static char* http_response_buffer;
    static int http_response_length;
    static const int max_response_size = 65536; // 64KB buffer for 7 days

    // Cached historical data (persists between fetches)
    HistoryData cached_data;
    
    // Current weather data
    CurrentWeather current_weather;

    static const char* TAG;
};

#endif // WEATHER_DATA_HPP
