# Weather API Integration Guide

## Overview

The ESP32 weather station retrieves current and historical weather data from a REST API server. The system uses HTTP/HTTPS to fetch:
- **Current Weather**: Real-time sensor readings from `/current` endpoint
- **Historical Data**: Time-series data from `/history` endpoint

## Configuration

### Secrets (secrets.h)
```c
#define WEBSERVER_URL "http://your-server.example.com/weatherAPI"
```

**Examples:**
- `http://192.168.1.100/weatherAPI` - Local server
- `https://api.example.com/weather` - HTTPS with custom domain
- `http://weather.local:8080/api` - Custom port

### Public Configuration (config.h)
```c
#define HTTP_CURRENT_ENDPOINT "/current"
#define HTTP_HISTORY_ENDPOINT "/history"
#define HTTP_TIMEOUT_MS       10000
```

## API Endpoints

### 1. Current Weather Endpoint

**Request:**
```
GET /current
```

**Response Format:**
```json
{
  "temperature": 23.5,
  "humidity": 65.2,
  "pressure": 1013.2,
  "wind_speed": 5.3,
  "wind_direction": 245,
  "wind_gust": 8.1,
  "rainfall": 2.5,
  "timestamp": 1704312000
}
```

**Required Fields:**
- `temperature` - Temperature in Celsius (float)
- `humidity` - Relative humidity 0-100% (float)
- `pressure` - Air pressure in millibars/hPa (float)
- `wind_speed` - Wind speed in m/s (float)
- `wind_direction` - Wind direction in degrees 0-359 (integer)
- `wind_gust` - Wind gust speed in m/s (float)
- `rainfall` - Rainfall in millimeters (float)
- `timestamp` - Unix timestamp in seconds (integer)

### 2. Historical Data Endpoint

**Request:**
```
GET /history
```

**Query Parameters (optional):**
- `days=7` - Number of days of history (default: 7)
- `resolution=1h` - Data granularity (default: 1h)

**Query Parameters (optional):**
- `days=7` - Number of days of history (default: 7)
- `resolution=1h` - Data granularity (default: 1h)

**Example Requests:**
```
GET /history
GET /history?days=7&resolution=1h
GET /history?days=3&resolution=5m
```

**Response Format:**
```json
[
  {
    "timestamp": 1704225600,
    "temperature": 22.0,
    "humidity": 60.5,
    "pressure": 1012.5,
    "wind_speed": 4.2,
    "wind_direction": 180,
    "rainfall": 0.5
  },
  {
    "timestamp": 1704229200,
    "temperature": 23.1,
    "humidity": 58.5,
    "pressure": 1013.1,
    "wind_speed": 5.1,
    "wind_direction": 200,
    "rainfall": 1.2
  }
]
```

**Required Fields per Data Point:**
- `timestamp` - Unix timestamp in seconds (integer)
- `temperature` - Temperature in Celsius (float)
- `humidity` - Relative humidity 0-100% (float)
- `wind_speed` - Wind speed in m/s (float)
- `rainfall` - Rainfall in millimeters (float)

**Optional Fields:**
- `pressure` - Air pressure in millibars (float)
- `wind_direction` - Wind direction in degrees (integer)
- `*_min`, `*_max` - Min/max values for range display

### Resolution Options

| Resolution | Description | Data Points (7 days) | Use Case |
|-----------|-------------|---------------------|----------|
| `raw` | Raw sensor readings | ~100,000+ | Real-time display (minutes) |
| `5m` | 5-minute aggregates | 2,016 | Recent detailed history (hours) |
| `1h` | Hourly aggregates | 168 | **Default** - Weekly charts |
| `1d` | Daily aggregates | 7 | Long-term trends (months) |

**Current Setting:** `1h` (168 data points for 7 days)

### Resolution Options

The `resolution` parameter controls data point granularity:

| Resolution | Description | Data Points (7 days) | Use Case |
|-----------|-------------|---------------------|----------|
| `raw` | Raw sensor readings | ~100,000+ | Real-time display (seconds/minutes) |
| `5m` | 5-minute aggregates | 2,016 | Recent detailed history (hours/day) |
| `1h` | Hourly aggregates | 168 | **Default** - Weekly charts |
| `1d` | Daily aggregates | 7 | Long-term trends (months/years) |

**Current Setting:** `1h` (168 data points for 7 days)

This provides optimal balance for:
- **Memory Usage**: ESP32 has limited RAM (~300KB available)
- **Display Resolution**: 960×540 E-paper display
- **Update Frequency**: 60-second wake cycles with light sleep

## Extended Response Format (Optional)

For advanced implementations, the API may return min/max ranges:

```json
[
  {
    "timestamp": 1704225600,
    "temperature": 22.0,
    "temperature_min": 21.5,
    "temperature_max": 22.8,
    "humidity": 60.5,
    "humidity_min": 59.8,
    "humidity_max": 61.2,
    "pressure": 1012.5,
    "pressure_min": 1012.3,
    "pressure_max": 1012.7,
    "wind_speed": 4.2,
    "wind_speed_min": 2.1,
    "wind_speed_max": 7.5,
    "wind_direction": 180,
    "rainfall": 0.5,
    "rainfall_min": 0.0,
    "rainfall_max": 1.2
  }
]
```

### Current Implementation

The ESP32 currently uses only the average values:
- `timestamp` - Required for time-series plotting
- `temperature` - Average temperature (°C)
- `humidity` - Average humidity (%)
- `wind_speed` - Average wind speed (m/s)
- `rainfall` - Accumulated rainfall (mm)

### Future Enhancements

The `*_min` and `*_max` fields can be used for:
- Error bars on charts showing data variability
- Temperature range displays (e.g., "22-25°C")
- Highlighting extreme values or anomalies
- More detailed weather pattern analysis

## Data Flow

## Data Flow

### Current Weather Update

1. **Wake from Light Sleep** (every 60 seconds)
   - Timer wakeup in main loop

2. **HTTP Request**
   - `weather.fetchCurrentWeather()` calls HTTP client
   - GET request to `WEBSERVER_URL + /current`

3. **Response Handling**
   - HTTP callback receives JSON data
   - cJSON library parses response

4. **Data Storage**
   - Current weather data stored in `WeatherData` singleton
   - Used by UI widgets: thermometer, wind rose, status bar

5. **Display Update**
   - `ui.update()` triggers re-render
   - E-paper display shows updated values

### Historical Data Update

1. **Periodic Fetch** (on display update)
   - `weather.fetchHistoricalData()` in WeatherUI

2. **HTTP Request**
   - GET request to `WEBSERVER_URL + /history`
   - Fetches 7 days of hourly data (168 points)

3. **JSON Parsing**
   - Parses array of historical data points
   - Extracts timestamp, temperature, humidity, wind_speed, rainfall

4. **Data Storage**
   - Stores up to `MAX_HISTORY_POINTS` (168 by default)
   - Historical array in `WeatherData` class

5. **Chart Rendering**
   - `Chart` widgets read historical data
   - Line charts drawn on E-paper display

## Memory Considerations

### Current Configuration (7 days, hourly)
### Current Configuration (7 days, hourly)
- 168 data points × ~32 bytes/point = ~5.4KB
- Plus JSON parsing buffer (~16KB)
- Total: ~21KB RAM usage for historical data

### Alternative Configurations

If memory becomes an issue:

**Option 1: Reduce time range**
```c
// In config.h - modify HTTP_HISTORY_ENDPOINT
#define HTTP_HISTORY_ENDPOINT "/history?days=3"  // 72 points (~2.3KB)
```

**Option 2: Use daily aggregates**
```c
#define HTTP_HISTORY_ENDPOINT "/history?days=30&resolution=1d"  // 30 points (~1KB)
```

**Option 3: Reduce buffer size**
```c
// In http_client.c - reduce MAX_BUFFER_SIZE if responses are predictable
#define MAX_BUFFER_SIZE (8192)  // Instead of 16384
```

## Error Handling

The system handles various failure scenarios:

### 1. Network Errors
- **HTTP timeout** (10 seconds default)
- **Connection failures**
- **DNS resolution errors**

**Behavior:** Logs warning, continues with cached/previous data

### 2. JSON Parse Errors
- **Invalid JSON format**
- **Missing required fields**
- **Type mismatches**

**Behavior:** Logs error, skips invalid entries, uses partial data if available

### 3. Data Validation
- **Checks for array type** in historical data
- **Validates timestamp presence** (required field)
- **Handles missing optional fields** (uses default values)
- **Range checks** for sensor values

## Testing Your API Server

### 1. Test Current Weather Endpoint
```bash
curl "http://your-server.example.com/weatherAPI/current"
```

**Expected Response:**
```json
{
  "temperature": 23.5,
  "humidity": 65.2,
  "pressure": 1013.2,
  "wind_speed": 5.3,
  "wind_direction": 245,
  "wind_gust": 8.1,
  "rainfall": 2.5,
  "timestamp": 1704312000
}
```

### 2. Test Historical Data
```bash
curl "http://your-server.example.com/weatherAPI/history" | jq length
```

**Expected:** Should return number between 1-168 (or your configured days × 24)

### 3. Test with Parameters
```bash
# Test different resolutions
curl "http://your-server.example.com/weatherAPI/history?days=1&resolution=5m"

# Test different time ranges
curl "http://your-server.example.com/weatherAPI/history?days=30&resolution=1d"
```

### 4. Validate JSON Structure
```bash
curl "http://your-server.example.com/weatherAPI/history" | jq '.[0]'
```

**Expected:** Should show first data point with all required fields

## Monitor ESP32 Communication

Enable monitoring to see API communication:

```bash
idf.py monitor
```

**Expected Log Output:**
```
I (12345) WeatherData: Fetching current weather
I (12456) http_client: HTTP GET Status = 200, content_length = 234
I (12567) WeatherData: Current weather updated: Temp=23.5°C
I (45678) WeatherData: Fetching historical data  
I (45789) http_client: HTTP GET Status = 200, content_length = 8543
I (45890) WeatherData: Parsed 168 historical data points
```

## Troubleshooting

### Issue: No data received

**Check:**
1. **WiFi Connection:** Verify ESP32 is connected to WiFi
2. **Server URL:** Check `WEBSERVER_URL` in `secrets.h`
3. **Network Connectivity:** Ping server from ESP32's network
4. **Firewall:** Ensure server allows connections from ESP32's IP
5. **API Server:** Verify server is running and accessible

**Test:**
```bash
# From a device on same network as ESP32
curl "http://your-server-ip/weatherAPI/current"
```

### Issue: JSON parse errors

**Symptoms:**
```
E (12345) WeatherData: Failed to parse JSON response
```

**Solutions:**
1. **Validate JSON:** Use `jq` or online validator
2. **Check Field Names:** Ensure exact match (case-sensitive)
3. **Check Data Types:** Numbers as numbers, strings as strings
4. **Test Response:** `curl ... | jq .`

### Issue: Incomplete historical data

**Symptoms:**
- Charts show gaps
- Fewer than expected data points

**Check:**
1. **Array Length:** API should return ~168 points for 7 days hourly
2. **Timestamp Continuity:** Check for gaps in timestamps
3. **Buffer Size:** Increase `MAX_BUFFER_SIZE` if response is truncated
4. **Memory:** Check for heap allocation failures in logs

**Test:**
```bash
# Count data points
curl "http://your-server/weatherAPI/history" | jq 'length'

# Check timestamp range
curl "http://your-server/weatherAPI/history" | jq '[.[0].timestamp, .[-1].timestamp]'
```

### Issue: Slow updates or timeouts

**Symptoms:**
```
E (45678) http_client: HTTP request timeout
W (45679) WeatherData: Failed to fetch historical data
```

**Solutions:**
1. **Increase Timeout:** Edit `HTTP_TIMEOUT_MS` in `config.h`
2. **Reduce Data:** Use fewer days or higher resolution (1h → 1d)
3. **Check Network:** Measure latency to server
4. **Optimize API:** Add database indexes, caching on server side

### Issue: Memory allocation failures

**Symptoms:**
```
E (12345) http_client: Failed to allocate buffer
E (12346) WeatherData: Out of memory
```

**Solutions:**
1. **Reduce History:** Lower `days` parameter
2. **Increase Heap:** Check available heap with `esp_get_free_heap_size()`
3. **Optimize Parsing:** Parse incrementally instead of loading entire response

## Security Considerations

### HTTPS Setup

For production deployments, use HTTPS:

```c
// In secrets.h
#define WEBSERVER_URL "https://your-server.example.com/weatherAPI"
```

**Requirements:**
- Valid SSL certificate on server
- ESP-IDF certificate bundle enabled (default)
- Server must support TLS 1.2+

### Authentication

If your API requires authentication:

**Option 1: Basic Auth**
```c
// Add to http_client.c
esp_http_client_set_username(client, "username");
esp_http_client_set_password(client, "password");
```

**Option 2: API Key Header**
```c
// Add to http_client.c
esp_http_client_set_header(client, "X-API-Key", "your-api-key");
```

**Option 3: Bearer Token**
```c
esp_http_client_set_header(client, "Authorization", "Bearer your-token");
```

## Sample API Implementation

See the included [weatherAPI/](../weatherAPI/) directory for a sample PHP-based implementation that:
- Provides `/current` and `/history` endpoints
- Supports resolution parameters
- Returns properly formatted JSON
- Includes database schema for TimescaleDB

**Quick Start:**
```bash
cd weatherAPI
cp config.php.example config.php
# Edit config.php with your database credentials
# Deploy to web server with PHP 7.4+
```

## Advanced Features

### Conditional Requests (Future)

Use HTTP `If-Modified-Since` header to reduce bandwidth:

```c
// Only fetch if data has changed since last request
esp_http_client_set_header(client, "If-Modified-Since", last_fetch_time);
```

**Benefits:**
- Reduced bandwidth usage
- Faster responses (304 Not Modified)
- Lower server load

### Compression (Future)

Enable gzip compression for smaller payloads:

```c
esp_http_client_set_header(client, "Accept-Encoding", "gzip");
```

**Benefits:**
- ~70% size reduction for JSON
- Faster data transfer
- Requires ESP-IDF gzip support

### Incremental Updates (Future)

Fetch only new data points since last update:

```
GET /history?since=1704312000
```

**Benefits:**
- Minimal data transfer
- Faster updates
- Lower memory usage

## Reference Implementation

For a complete reference implementation, see:
- **ESP32 Code:** [main/http_client.c](../main/http_client.c)
- **Weather Data Manager:** [gui/WeatherData.cpp](../gui/WeatherData.cpp)
- **Sample API Server:** [weatherAPI/](../weatherAPI/)
- **Configuration:** [main/config.h](../main/config.h) and [main/secrets.h.example](../main/secrets.h.example)
