# ESP32 Weather Station Display

Had a lillygo EPD47 (esp32 epaper module) laying around for years and never realy got around doing anything with it.
I also have a DIY weather station that reports via MQTT to a Timescale database (baiscally souped up Postgress)
and i whanted to play around with these newfangled AI tools to see what the hype was all about so i wondered if
it would be possible to do something usefull with it. This is the result.

The journey here has been interesting. AI is definitly usefull but the experience so far is that it needs so be monitored closely
and given very specific tasks to do, which means carefull wording of the prompts. 
Also, carfull selection of which AI to use.
I started out with the free trial version of Copilot that has access to Claude as that seemed the best choice and was pretty impressed with what it came up with
but the the "Premium request" pool ran out and i had to revert to GPT, that was a catastrophe, GPT completely magled the code and i had to back it out manualy. I decided to go for a pro sub on the Copilot
and i got Claude 4.5 back.

Most of the code here is AI generated,  with some minor tweeks here and there, and it shows, it is not the best code possible but it works.
So for quick'ish project that does not need the ultimate in efficency, code cleanliness etc AI is ok.
All of the documentation is AI generated including this README beyond this introduction

I could never get the LVGL grphical library to work correctly the same for the EPDIY library so ended up with the original Lilygo-epd47 implementation with some patches (mostly for drawing/clearing arcs)
the first atempts to only clear the portions that where redrawn ended i failure, meaning weird artifacts left behind etc etc. But since the display only refreshes every minute (could probably be set to 5 min)
so i ended up clearing and repainting the hole display at each update.

I also had the Claude generate an PHP API to retreive the data, you find that in the WeatherAPI directory


# !!!! WARNING AI generated stuff follows !!!!

# A comprehensive weather station display system for the LilyGo EPD47 E-paper display board (ESP32-based), featuring real-time weather data visualization, historical charts, battery monitoring, and low-power operation.

## 🎯 Features

- **Large E-Paper Display**: 960×540 resolution for clear data visualization
- **Real-Time Weather Data**: Fetches current weather from HTTP REST API
- **Historical Charts**: 7-day historical data with temperature, humidity, wind speed
- **Visual Widgets**:
  - Analog thermometer with mercury column
  - 16-point wind rose with directional indicators
  - Wind speed and gust display
  - Multi-chart historical data visualization
- **Battery Monitoring**: ADC-based battery level display with percentage
- **Low Power Operation**: Light sleep mode with WiFi preservation (60-second wake cycles)
- **SNTP Time Sync**: Automatic time synchronization with CET/CEST timezone support

## 📱 Display Components

### Status Bar
- **Battery Monitor**: ADC-based battery percentage with visual fill indicator
- **WiFi Status**: Connection indicator
- **Clock**: Current time with SNTP synchronization (CET/CEST timezone)

### Weather Widgets
1. **Analog Thermometer**: Classic mercury-style visualization with bulb and rising column
2. **Wind Rose**: 16-point compass rose showing wind direction with cardinal labels (N, NNE, NE, ENE, etc.)
3. **Wind Data**: Current direction (degrees + cardinal), wind speed, and gust speed in m/s
4. **Historical Charts**: Multi-day trends for temperature, humidity, wind speed, and rainfall

## 🖥️ Hardware Requirements

- **LilyGo EPD47 E-Paper Display Board**
  - ESP32-WROVER-B microcontroller
  - 960×540 pixel 4.7" E-paper display
  - 4MB PSRAM
  - Built-in WiFi
- **Battery** (optional): 3.7V Li-Po for portable operation
- **USB-C**: For power and programming

## 🚀 Quick Start

-  [API Integration](docs/API_INTEGRATION.md) - Web API documentation
- ⚙️ [Configuration Guide](CONFIGURATION.md) - Setup instructions

## 📋 Prerequisites

### Required Software

1. **ESP-IDF v5.5+**: ESP32 development framework
   ```bash
   git clone -b v5.5.1 --recursive https://github.com/espressif/esp-idf.git ~/esp/v5.5.1/esp-idf
   cd ~/esp/v5.5.1/esp-idf
   ./install.sh
   . ./export.sh
   ```

### Weather Data Source

You need a weather API server that provides:
- **Current Weather Endpoint**: `/current` - Real-time weather data in JSON format
- **Historical Data Endpoint**: `/history` - 7-day historical data

See [weatherAPI/README.md](weatherAPI/README.md) for the included PHP-based weather API implementation.

## ⚙️ Configuration

### Step 1: Copy Secrets Template

```bash
cp main/secrets.h.example main/secrets.h
```

### Step 2: Edit Credentials

Edit `main/secrets.h` with your WiFi and server details:

```c
#define WIFI_SSID               "YourNetworkName"
#define WIFI_PASSWORD           "YourWiFiPassword"
#define WEBSERVER_URL           "http://192.168.1.100/weatherAPI"
```

### Step 3: Review Configuration (Optional)

Edit `main/config.h` to adjust:
- Display update intervals
- HTTP endpoints (`/current`, `/history`)
- Timeout values
- Debug settings

📖 **Detailed Guide**: See [CONFIGURATION.md](CONFIGURATION.md) for complete configuration options.

## 🔨 Building and Flashing

### 0. Get the Code

Clone the repository with submodules:

```bash
git clone --recursive https://github.com/YOUR_USERNAME/ESP32-EPD47-Weather-Display.git
cd weather-display
```

**Important**: The `--recursive` flag downloads the LilyGo EPD47 display driver component as a submodule.

**If already cloned without --recursive:**
```bash
git submodule update --init --recursive
```

### 1. Patch Display Component

Apply patches to the LilyGo EPD47 component (adds circle drawing and ESP-IDF 5.x support):

```bash
cd components/lilygo-epd47
git apply ../../scripts/lilygo-epd47.patch
cd ../..
```

**Note**: If already patched, you'll see "patch already exists" - this is normal.

### 2. Set ESP32 Target

```bash
idf.py set-target esp32
```

### 3. Build the Project

```bash
idf.py build
```

### 4. Flash to Device

```bash
idf.py -p /dev/ttyUSB0 flash
```

### 5. Monitor Output

```bash
idf.py -p /dev/ttyUSB0 monitor
```

**Or combine flash and monitor:**
```bash
idf.py -p /dev/ttyUSB0 flash monitor
```

**Exit monitor**: Press `Ctrl+]`

📖 **Complete Build Instructions**: See [BUILDING.md](BUILDING.md) for detailed setup from scratch, including ESP-IDF installation and troubleshooting.

## 📊 Weather Data API

### Current Weather Endpoint: `/current`

Returns real-time weather data in JSON format:

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

### Historical Data Endpoint: `/history`

Returns array of historical data points (up to 7 days):

```json
[
  {
    "timestamp": 1704225600,
    "temperature": 22.0,
    "humidity": 60.5,
    "wind_speed": 4.2,
    "rainfall": 0.5
  },
  {
    "timestamp": 1704229200,
    "temperature": 23.1,
    "humidity": 58.5,
    "wind_speed": 5.1,
    "rainfall": 1.2
  }
]
```

**Field Descriptions**:
- `temperature`: Temperature in Celsius
- `humidity`: Relative humidity (0-100%)
- `pressure`: Air pressure in millibars (hPa)
- `wind_speed`: Wind speed in m/s
- `wind_direction`: Wind direction in degrees (0-359°)
- `wind_gust`: Wind gust speed in m/s
- `rainfall`: Rainfall in millimeters
- `timestamp`: Unix timestamp (seconds since epoch)

📖 **API Details**: See [docs/API_INTEGRATION.md](docs/API_INTEGRATION.md) and [weatherAPI/README.md](weatherAPI/README.md)

## 📁 Project Structure

```
Display/
├── CMakeLists.txt              # Root build configuration
├── partitions.csv              # Flash memory partition table
├── sdkconfig                   # ESP-IDF configuration (auto-generated)
├── sdkconfig.defaults          # Default SDK configuration
├── README.md                   # This file
├── CONFIGURATION.md            # Detailed configuration guide
├── CHANGELOG.md                # Version history
├── MQTT_FORMAT.md              # MQTT data format documentation
│
├── main/                       # Main application code
│   ├── CMakeLists.txt          # Main component build config
│   ├── main.cpp                # Application entry point
│   ├── config.h                # Non-sensitive configuration
│   ├── secrets.h               # WiFi/API credentials (gitignored)
│   ├── secrets.h.example       # Template for secrets.h
│   ├── http_client.c/h         # HTTP client for weather API
│   ├── wifi_manager.c/h        # WiFi connection management
│   └── weather_data.c/h        # Weather data structures
│
├── gui/                        # Display and UI components
│   ├── WeatherUI.cpp/h         # Main UI controller
│   ├── WeatherData.cpp/h       # Weather data manager
│   ├── Status.cpp/h            # Status bar (battery, WiFi, time)
│   ├── Thermometer.cpp/h       # Analog thermometer widget
│   ├── Windrose.cpp/h          # Wind rose compass widget
│   ├── WindData.cpp/h          # Wind speed/direction display
│   ├── Chart.cpp/h             # Base chart class
│   └── Fonts/                  # Font definitions
│       ├── mplus_rounded_1c_medium_20.h
│       └── atkinson_hyperlegible_*.h
│
├── components/                 # External components
│   └── lilygo-epd47/           # LilyGo EPD47 display driver
│
├── scripts/                    # Build and utility scripts
│   ├── generate_fonts.sh       # Font generation script
│   ├── fontconvert_extended.py # Font converter with extended charset
│   └── lilygo-epd47.patch      # Component patches
│
├── docs/                       # Additional documentation
│   └── API_INTEGRATION.md      # Weather API integration guide
│
└── weatherAPI/                 # PHP-based weather API server
    ├── index.php               # Main API endpoint
    ├── config.php              # API configuration
    ├── README.md               # API documentation
    └── tests/                  # Test scripts
```

## 🔧 Key Components

## 🔧 Key Components

### Main Application (`main/main.cpp`)
- Application entry point and initialization
- WiFi connection and SNTP time synchronization
- Main loop with light sleep power management
- Watchdog timer management

### Weather Data Manager (`gui/WeatherData.cpp`)
- Fetches current weather from `/current` endpoint
- Fetches historical data from `/history` endpoint
- Manages data storage and caching
- JSON parsing using cJSON library

### Weather UI (`gui/WeatherUI.cpp`)
- Main display controller and layout manager
- Coordinates all widget rendering
- Manages framebuffer and display updates
- Handles display initialization

### Display Widgets

#### Status Bar (`gui/Status.cpp`)
- Battery percentage monitoring via ADC (GPIO 36)
- WiFi connection status
- Current time display

#### Thermometer (`gui/Thermometer.cpp`)
- Classic analog thermometer visualization
- Mercury bulb at bottom
- Rising column based on temperature
- Scale markings and labels

#### Wind Rose (`gui/Windrose.cpp`)
- 16-point compass rose
- Cardinal direction labels (N, NNE, NE, etc.)
- Directional arrow pointing to current wind direction
- Distance rings for visual reference

#### Wind Data (`gui/WindData.cpp`)
- Wind direction in degrees and cardinal direction
- Current wind speed in m/s
- Wind gust speed in m/s
- Formatted text display

#### Chart System (`gui/Chart.cpp`)
- Base class for historical data charts
- Line chart rendering
- Automatic scaling and axis labels
- Supports temperature, humidity, wind, rainfall

### HTTP Client (`main/http_client.c`)
- HTTPS requests with certificate bundle
- JSON response handling
- Connection pooling and timeout management
- Error handling and retry logic

### WiFi Manager (`main/wifi_manager.c`)
- WiFi STA mode initialization
- Connection management and auto-reconnect
- Event handling for connect/disconnect
- Network status monitoring

## ⚡ Power Management

The system uses **light sleep mode** to reduce power consumption while maintaining WiFi connectivity:

- **Active Mode**: Display update, data fetch, rendering (~200mA)
- **Light Sleep**: CPU suspended, WiFi connected (~15-20mA)
- **Wake Interval**: 60 seconds (configurable)
- **E-Paper**: No power consumption when static

### Sleep Behavior
1. Device fetches weather data and updates display
2. Enters light sleep for 60 seconds
3. Wakes up automatically via timer
4. WiFi connection preserved (no reconnection overhead)
5. Repeat cycle

**Note**: Light sleep uses more power than deep sleep but avoids:
- Wake from deep sleep is actually a reboot, and everything gets reinialized.
- WiFi reconnection time (~2-5 seconds)
- SNTP time resync
- Full system reinitialization

## 🔍 Battery Monitoring

Battery level is measured using ESP32's ADC1 (GPIO 36):

- **Voltage Divider**: 2:1 ratio for battery voltage measurement
- **Range**: 3.0V (0%) to 4.2V (100%)
- **Calibration**: Uses `adc_cali_line_fitting` for accurate readings
- **Display**: Percentage with visual fill indicator

## 🛠️ Customization

### Change Update Interval

Edit sleep duration in [main/config.h](main/config.h):
```c
#define SLEEP_DURATION_SECONDS      60  // Light sleep duration between updates
```

### Change Timezone

Edit timezone in [main/config.h](main/config.h):
```c
// POSIX timezone format
#define TIMEZONE_STRING             "CET-1CEST,M3.5.0,M10.5.0/3"
```

**Common Timezones:**
- **US Eastern**: `"EST5EDT,M3.2.0,M11.1.0"`
- **US Pacific**: `"PST8PDT,M3.2.0,M11.1.0"`
- **GMT**: `"GMT0"`
- **Japan**: `"JST-9"`
- **Australia (Sydney)**: `"AEST-10AEDT,M10.1.0,M4.1.0/3"`

### Change SNTP Server

Edit NTP server in [main/config.h](main/config.h):
```c
#define SNTP_SERVER                 "pool.ntp.org"  // Or time.google.com, time.nist.gov
```

### Adjust Display Layout

Modify widget positions in [gui/WeatherUI.cpp](gui/WeatherUI.cpp):
```cpp
Thermometer thermometer(50, 150, &temperatureFont);  // x, y, font
Windrose windrose(500, 150, 180, framebuffer);      // x, y, radius, fb
```

### Add More Data Fields

1. Extend weather data structures in [gui/WeatherData.h](gui/WeatherData.h)
2. Update JSON parsing in [gui/WeatherData.cpp](gui/WeatherData.cpp)
3. Create new widget or modify existing ones in `gui/`

### Change Fonts

Replace font includes in [main/main.cpp](main/main.cpp):
```cpp
#include "../gui/Fonts/your_font_here.h"
#define theFont YourFont
```

### Generate Custom Fonts

The project includes scripts to convert TrueType fonts to C headers compatible with the EPD47 display:

**Quick Start:**
```bash
cd scripts
./generate_fonts.sh
```

This generates font headers with:
- **Basic ASCII** (0x20-0x7E): 95 characters
- **Latin-1 Supplement** (0xA0-0xFF): 96 characters including degree symbol (°)
- **Total**: 191 characters per font
- **Compression**: Optional glyph bitmap compression

**Manual Font Conversion:**
```bash
# Convert single font with specific size
python3 scripts/fontconvert_extended.py \
    --compress \
    FontName \
    20 \
    path/to/font.ttf > gui/Fonts/font_name_20.h
```

**Supported Features:**
- Multiple font sizes (12, 20, 32, etc.)
- Font stacking (fallback fonts)
- Extended character ranges
- Glyph compression for reduced memory usage

See [scripts/generate_fonts.sh](scripts/generate_fonts.sh) for complete font generation examples.

## 🐛 Troubleshooting

### Display Not Updating
- **Check Power**: Ensure adequate power supply (USB-C or battery)
- **Verify Initialization**: Look for "Weather UI initialized" in serial monitor
- **Check Framebuffer**: Ensure framebuffer allocation succeeded
- **EPD47 Connection**: Verify display ribbon cable is securely connected

### WiFi Connection Issues
- **SSID/Password**: Double-check credentials in `main/secrets.h`
- **2.4GHz Only**: ESP32 doesn't support 5GHz WiFi
- **Signal Strength**: Move closer to router during testing
- **Serial Output**: Check `idf.py monitor` for WiFi error messages

### Weather Data Not Showing
- **API Server**: Verify weather API server is accessible
- **URL Configuration**: Check `WEBSERVER_URL` in `main/secrets.h`
- **Network Test**: Test API endpoints with `curl` or browser:
  ```bash
  curl http://your-server/weatherAPI/current
  curl http://your-server/weatherAPI/history
  ```
- **JSON Format**: Ensure API returns valid JSON with correct field names
- **HTTP Client**: Check for HTTP errors in serial monitor

### Build Errors

#### IRAM Overflow
```
ld: region `iram0_0_seg' overflowed by XXXX bytes
```
**Solution**: Already configured with:
- WiFi IRAM optimization disabled
- Compiler size optimization (`-Os`)
- FreeRTOS functions in flash

#### Missing Dependencies
```
fatal error: cJSON.h: No such file or directory
```
**Solution**: Managed components should auto-install. If not:
```bash
idf.py reconfigure
```

### Battery Reading Issues
- **No Battery**: Displays 0% or erratic values - this is normal without battery
- **Calibration**: ADC values may need adjustment in [gui/Status.cpp](gui/Status.cpp)

### Time Not Syncing
- **SNTP Server**: Default is `pool.ntp.org` - ensure network allows NTP (port 123)
- **Timezone**: Configured for CET/CEST - adjust in [main/config.h](main/config.h):
  ```c
  #define TIMEZONE_STRING "EST5EDT,M3.2.0,M11.1.0"  // Example: US Eastern
  ```
- **POSIX Format**: Use POSIX TZ format (see config.h for examples)
- **Time Server**: Try alternative servers: `time.google.com`, `time.nist.gov`

## 📝 Development Notes

### Compiler Optimizations
- **Size Optimization** (`-Os`): Enabled to reduce IRAM usage
- **WiFi IRAM**: Disabled to move WiFi functions to flash
- **FreeRTOS**: Functions placed in flash to save IRAM

### Memory Management
- **PSRAM**: Used for framebuffer allocation (960×540 = 518KB)
- **Heap**: Monitor with `esp_get_free_heap_size()`
- **Stack**: Main task uses default stack size

## 🚀 Future Enhancements

- [ ] Touch input support for interactive UI
- [ ] Multiple display pages/screens
- [ ] Moon phase indicator
- [ ] Sunrise/sunset times
- [ ] Weather alerts and warnings
- [ ] Weather forecast display (multi-day predictions)
- [ ] Configurable widget layouts
- [ ] Additional weather data sources
- [ ] Local data caching to reduce API calls
- [ ] Deep sleep mode for ultra-low power operation (requires full reinit on wake)
- [ ] OTA (Over-The-Air) firmware updates

## 📚 Additional Documentation

- **[CONFIGURATION.md](CONFIGURATION.md)** - Detailed configuration options
- **[CHANGELOG.md](CHANGELOG.md)** - Version history and changes
- **[docs/API_INTEGRATION.md](docs/API_INTEGRATION.md)** - Weather API integration guide
- **[weatherAPI/README.md](weatherAPI/README.md)** - PHP weather API documentation

## 🤝 Contributing

Contributions are welcome! Please feel free to submit issues or pull requests.

## 📄 License

This project is provided as-is for educational and development purposes.

## 🙏 Acknowledgments

- **Espressif** - ESP-IDF framework
- **LilyGo** - EPD47 E-paper display board and driver library
- **Community** - Open-source ESP32 and E-paper projects

## 📧 Support

For issues and questions:
- **ESP-IDF**: https://github.com/espressif/esp-idf/issues
- **LilyGo EPD47**: https://github.com/Xinyuan-LilyGO/LilyGo-EPD47
- **Project Issues**: Open an issue in this repository

---

**Made with ❤️ for weather enthusiasts and ESP32 developers**
