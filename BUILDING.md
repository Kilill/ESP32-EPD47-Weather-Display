# Building the ESP32 Weather Station Display

This guide walks you through the complete process of building and flashing the weather station display firmware, from obtaining the code to running it on your hardware.

## Table of Contents

- [Prerequisites](#prerequisites)
- [Getting the Code](#getting-the-code)
- [Setting Up ESP-IDF](#setting-up-esp-idf)
- [Configuration](#configuration)
- [Building the Project](#building-the-project)
- [Flashing to Device](#flashing-to-device)
- [First Run](#first-run)
- [Troubleshooting Build Issues](#troubleshooting-build-issues)

## Prerequisites

### Hardware Requirements

- **LilyGo EPD47 E-Paper Display Board**
  - ESP32-WROVER-B with 4MB PSRAM
  - 960×540 pixel 4.7" E-paper display
- **USB-C Cable** for programming and power
- **Computer** running Linux, macOS, or Windows (with WSL2)
- **Optional**: 3.7V Li-Po battery for portable operation

### Software Requirements

- **Git** (2.0 or later)
- **Python** (3.8 or later)
- **CMake** (3.16 or later)
- **Ninja build system**
- **C/C++ compiler** (GCC/Clang)
- **Approximately 2GB** of free disk space for ESP-IDF and toolchain

### Operating System Specific

**Linux (Ubuntu/Debian):**
```bash
sudo apt-get update
sudo apt-get install git wget flex bison gperf python3 python3-pip python3-venv \
    cmake ninja-build ccache libffi-dev libssl-dev dfu-util libusb-1.0-0
```

**macOS:**
```bash
brew install cmake ninja dfu-util
```

**Windows:**
- Install [WSL2](https://learn.microsoft.com/en-us/windows/wsl/install) with Ubuntu
- Follow Linux instructions inside WSL2

## Getting the Code

### 1. Clone the Repository

Choose a location for your project and clone the repository:

```bash
# Create a workspace directory (optional)
mkdir -p ~/projects
cd ~/projects

# Clone the repository with submodules
git clone --recursive https://github.com/YOUR_USERNAME/weather-display.git
cd weather-display
```

**Important**: The `--recursive` flag downloads submodules including the LilyGo EPD47 display driver component.

**Note**: Replace `YOUR_USERNAME/weather-display` with the actual repository path.

**If you already cloned without --recursive:**
```bash
cd weather-display
git submodule update --init --recursive
```

### 2. Verify Repository Contents

Check that you have the expected directory structure:

```bash
ls -la
```

You should see:
- `main/` - Main application code
- `gui/` - Display and UI components
- `components/` - External components (lilygo-epd47)
- `scripts/` - Build and utility scripts
- `weatherAPI/` - PHP weather API server
- `CMakeLists.txt` - Root build configuration
- `README.md` - Project documentation

## Setting Up ESP-IDF

The project requires **ESP-IDF v5.5.1** or later.

### 1. Download ESP-IDF

```bash
# Create ESP-IDF directory
mkdir -p ~/esp
cd ~/esp

# Clone ESP-IDF v5.5.1
git clone -b v5.5.1 --recursive https://github.com/espressif/esp-idf.git v5.5.1/esp-idf

# Navigate to ESP-IDF directory
cd v5.5.1/esp-idf
```

**Note**: The `--recursive` flag is important as it downloads all submodules.

### 2. Install ESP-IDF Tools

```bash
# Run the installation script
./install.sh esp32

# This will download and install:
# - Xtensa ESP32 toolchain
# - CMake and build tools
# - Python packages
# - OpenOCD debugger
```

The installation may take 10-20 minutes depending on your internet connection.

### 3. Set Up Environment Variables

**For the current terminal session:**
```bash
. ~/esp/v5.5.1/esp-idf/export.sh
```

**To make it permanent, add to your shell profile:**

For bash (`~/.bashrc` or `~/.bash_profile`):
```bash
echo 'alias get_idf=". ~/esp/v5.5.1/esp-idf/export.sh"' >> ~/.bashrc
source ~/.bashrc
```

For zsh (`~/.zshrc`):
```bash
echo 'alias get_idf=". ~/esp/v5.5.1/esp-idf/export.sh"' >> ~/.zshrc
source ~/.zshrc
```

Now you can activate ESP-IDF with:
```bash
get_idf
```

### 4. Verify ESP-IDF Installation

```bash
idf.py --version
```

Expected output:
```
ESP-IDF v5.5.1
```

## Configuration

### 1. Navigate to Project Directory

```bash
cd ~/projects/weather-display
```

### 2. Create Secrets File

The project uses a two-file configuration system:
- `main/config.h` - Non-sensitive settings (in git)
- `main/secrets.h` - Credentials (NOT in git)

Copy the secrets template:
```bash
cp main/secrets.h.example main/secrets.h
```

### 3. Edit Secrets File

Edit `main/secrets.h` with your actual credentials:

```bash
nano main/secrets.h
# or use your preferred editor: vim, code, etc.
```

**Required settings:**
```c
#define WIFI_SSID               "YourNetworkName"
#define WIFI_PASSWORD           "YourWiFiPassword"
#define WEBSERVER_URL           "http://192.168.1.100/weatherAPI"
```

### 4. Review Optional Configuration

Edit `main/config.h` to adjust:

```c
// Sleep duration between updates (seconds)
#define SLEEP_DURATION_SECONDS      60

// Timezone (POSIX format)
#define TIMEZONE_STRING             "CET-1CEST,M3.5.0,M10.5.0/3"

// SNTP server for time sync
#define SNTP_SERVER                 "pool.ntp.org"

// HTTP timeouts
#define HTTP_TIMEOUT_MS             30000
```

See [CONFIGURATION.md](CONFIGURATION.md) for all available options.

### 5. Patch LilyGo EPD47 Component

The project includes patches for the LilyGo EPD47 display driver to add circle drawing functions and ESP-IDF 5.x compatibility.

```bash
# Apply the patch to the lilygo-epd47 component
cd components/lilygo-epd47
git apply ../../scripts/lilygo-epd47.patch
cd ../..
```

**What the patch adds:**
- `epd_push_pixels_circle()` - Draw circular areas
- `epd_clear_circle()` - Clear circular regions
- `LCD_CLK_SRC_PLL160M` clock source for ESP-IDF 5.x

**Note**: If the patch fails (already applied), you'll see:
```
error: patch failed: ...
error: ... already exists in working directory
```
This is normal if the component already has the patches applied.

### 6. Set ESP32 Target

```bash
idf.py set-target esp32
```

This configures the build system for the ESP32 chip (not ESP32-S2, ESP32-C3, etc.).

## Building the Project

### 1. Clean Build (First Time)

For the first build, perform a clean build:

```bash
idf.py fullclean
idf.py build
```

**Build process steps:**
1. CMake generates build files
2. Ninja compiles source files
3. Links firmware binary
4. Generates flash images

**Expected build time**: 2-5 minutes (first build), 30-60 seconds (incremental)

### 2. Monitor Build Progress

The build output shows:
```
[0/1] Re-running CMake...
[1/1000] Building C object...
[500/1000] Linking CXX executable...
[1000/1000] Generating binary image...
```

### 3. Successful Build Output

When build completes successfully, you'll see:

```
Project build complete. To flash, run this command:
 python -m esptool --chip esp32 -b 460800 --before default_reset --after hard_reset write_flash --flash_mode dio --flash_size detect --flash_freq 80m 0x1000 build/bootloader/bootloader.bin 0x8000 build/partition_table/partition-table.bin 0x10000 build/weather_station.bin
or from the "/path/to/weather-display/build" directory
 python -m esptool --chip esp32 -b 460800 --before default_reset --after hard_reset write_flash "@flash_args"
```

**Generated files location:**
- Firmware: `build/weather_station.bin`
- Bootloader: `build/bootloader/bootloader.bin`
- Partition table: `build/partition_table/partition-table.bin`

### 4. Check Binary Size

```bash
idf.py size
```

This shows memory usage:
```
Total sizes:
 DRAM .data size:   xxxxx bytes
 DRAM .bss  size:   xxxxx bytes
Used static DRAM:   xxxxx bytes ( xxxxx available, xx.x% used)
Used static IRAM:   xxxxx bytes ( xxxxx available, xx.x% used)
      Flash code:  xxxxxx bytes
    Flash rodata:   xxxxx bytes
Total image size:~ xxxxxx bytes (.bin may be padded larger)
```

## Flashing to Device

### 1. Connect Hardware

1. Connect LilyGo EPD47 board to computer via USB-C cable
2. The device should appear as a serial port

### 2. Identify Serial Port

**Linux:**
```bash
ls /dev/ttyUSB*
# or
ls /dev/ttyACM*
```

**macOS:**
```bash
ls /dev/cu.usbserial-*
# or
ls /dev/cu.SLAB_USBtoUART
```

**WSL2 (Windows):**
```bash
ls /dev/ttyS*
```

**Note**: Common ports are `/dev/ttyUSB0` (Linux), `/dev/cu.usbserial-0001` (macOS)

### 3. Set USB Permissions (Linux Only)

If you get permission errors:

```bash
# Add your user to dialout group
sudo usermod -a -G dialout $USER

# You must log out and back in for this to take effect
# Or use sudo for the flash command
```

### 4. Flash Firmware

**Method 1: Flash and Monitor (Recommended)**
```bash
idf.py -p /dev/ttyUSB0 flash monitor
```

**Method 2: Flash Only**
```bash
idf.py -p /dev/ttyUSB0 flash
```

**Method 3: Erase Flash First (for troubleshooting)**
```bash
idf.py -p /dev/ttyUSB0 erase-flash
idf.py -p /dev/ttyUSB0 flash
```

**Flashing process:**
1. Connecting to device
2. Detecting chip type
3. Writing bootloader
4. Writing partition table
5. Writing application firmware
6. Verifying flash
7. Hard resetting device

**Expected flash time**: 30-60 seconds

### 5. Successful Flash Output

```
Connecting.....
Chip is ESP32-D0WDQ6 (revision 1)
Features: WiFi, BT, Dual Core, 240MHz, VRef calibration in efuse, Coding Scheme None
Crystal is 40MHz
MAC: xx:xx:xx:xx:xx:xx
Uploading stub...
Running stub...
Stub running...
Changing baud rate to 460800
Changed.
Configuring flash size...
Flash will be erased from 0x00001000 to 0x00007fff...
Flash will be erased from 0x00008000 to 0x00008fff...
Flash will be erased from 0x00010000 to 0x0017ffff...
Compressed 24544 bytes to 15542...
Wrote 24544 bytes (15542 compressed) at 0x00001000 in 0.4 seconds...
Hash of data verified.
Leaving...
Hard resetting via RTS pin...
Done
```

## First Run

### 1. Monitor Serial Output

```bash
idf.py -p /dev/ttyUSB0 monitor
```

**Exit monitor**: Press `Ctrl+]`

### 2. Expected Boot Sequence

```
I (xxx) cpu_start: Starting scheduler on APP CPU.
I (xxx) main: Initializing NVS...
I (xxx) main: Weather UI instance retrieved. Initializing...
I (xxx) WeatherUI: Initializing EPD driver...
I (xxx) WeatherUI: EPD driver initialized
I (xxx) wifi: WiFi init...
I (xxx) wifi: Connecting WiFi...
I (xxx) wifi: WiFi connected, IP address obtained
I (xxx) main: SNTP initializing...
I (xxx) main: Time synchronized: Thu Jan  2 12:34:56 2026
I (xxx) main: HTTP Client initializing
I (xxx) WeatherData: Fetching current weather
I (xxx) http_client: HTTP GET Status = 200, content_length = 234
I (xxx) WeatherData: Current weather updated: Temp=23.5°C
I (xxx) WeatherUI: Starting display update
I (xxx) WeatherUI: Display update complete
```

### 3. Verify Display Update

The E-paper display should show:
- **Status Bar**: Battery level, WiFi icon, current time
- **Thermometer**: Current temperature visualization
- **Wind Rose**: Wind direction indicator
- **Wind Data**: Speed and gust informationa
- [Troubleshooting Build Issues](#troubleshooting-build-issuesa

### 4. Common First-Run Issues

**WiFi not connecting:**
- Check SSID and password in `secrets.h`
- Ensure 2.4GHz network (ESP32 doesn't support 5GHz)
- Check serial monitor for error messages

**Display shows nothing:**
- Check ribbon cable connection
- Verify power supply is adequate
- Look for initialization errors in serial monitor

**No weather data:**
- Verify API server is accessible
- Check `WEBSERVER_URL` in `secrets.h`
- Test API endpoints with curl:
  ```bash
  curl http://your-server/weatherAPI/current
  ```

## Troubleshooting Build Issues

### IRAM Overflow Error

```
ld: region `iram0_0_seg' overflowed by XXXX bytes
```

**Solution**: Already configured in sdkconfig:
- WiFi IRAM optimization disabled
- Compiler size optimization (`-Os`)
- FreeRTOS functions in flash

If issue persists, check `sdkconfig`:
```bash
# Verify these settings
grep "CONFIG_COMPILER_OPTIMIZATION_SIZE" sdkconfig
grep "CONFIG_FREERTOS_PLACE_FUNCTIONS_INTO_FLASH" sdkconfig
grep "CONFIG_ESP32_WIFI_IRAM_OPT" sdkconfig
```

### Missing Dependencies

```
fatal error: cJSON.h: No such file or directory
```

**Solution**: Reconfigure to install managed components:
```bash
idf.py reconfigure
idf.py build
```

### Python Package Errors

```
ModuleNotFoundError: No module named 'click'
```

**Solution**: Reinstall ESP-IDF tools:
```bash
cd ~/esp/v5.5.1/esp-idf
./install.sh esp32 --reinstall
```

### Build Cache Issues

```
CMake Error: ...
```

**Solution**: Clean and rebuild:
```bash
idf.py fullclean
rm -rf build
idf.py build
```

### Toolchain Issues

```
xtensa-esp32-elf-gcc: command not found
```

**Solution**: Re-export ESP-IDF environment:
```bash
. ~/esp/v5.5.1/esp-idf/export.sh
```

### Serial Port Permission Denied

```
Could not open /dev/ttyUSB0, the port is busy or doesn't exist
```

**Solution (Linux)**:
```bash
# Add user to dialout group
sudo usermod -a -G dialout $USER
# Log out and back in, or use:
sudo chmod 666 /dev/ttyUSB0
```

**Solution (macOS)**:
```bash
# Identify correct port
ls -la /dev/cu.*
# Use the correct device name
idf.py -p /dev/cu.usbserial-0001 flash monitor
```

### Flash Write Errors

```
A fatal error occurred: Failed to write to target RAM
```

**Solution**: Reset board and try again:
1. Press RESET button on board
2. Run flash command immediately
3. If persistent, try lower baud rate:
   ```bash
   idf.py -p /dev/ttyUSB0 -b 115200 flash
   ```

## Incremental Builds

After the initial build, subsequent builds are much faster:

```bash
# Only rebuild changed files
idf.py build

# Flash and monitor in one command
idf.py -p /dev/ttyUSB0 flash monitor
```

**Build time**: Typically 30-60 seconds for incremental builds.

## Advanced Build Options

### Build with Verbose Output

```bash
idf.py -v build
```

### Build Specific Components

```bash
idf.py build --only gui
idf.py build --only main
```

### Size Optimization Report

```bash
idf.py size-components
idf.py size-files
```

### Generate Compilation Database

For IDE integration:
```bash
idf.py build
# Creates build/compile_commands.json
```

## Next Steps

After successful build and flash:

1. **Configure Weather API**: Set up the included PHP weather API (see [weatherAPI/README.md](weatherAPI/README.md))
2. **Customize Display**: Adjust widgets and layout (see [README.md](README.md#customization))
3. **Optimize Power**: Tune sleep intervals and update frequency
4. **Monitor Performance**: Use serial monitor to track memory usage and timing

## Additional Resources

- **[README.md](README.md)** - Project overview and features
- **[CONFIGURATION.md](CONFIGURATION.md)** - Detailed configuration guide
- **[docs/API_INTEGRATION.md](docs/API_INTEGRATION.md)** - Weather API integration
- **[ESP-IDF Documentation](https://docs.espressif.com/projects/esp-idf/en/v5.5.1/)** - Official ESP-IDF docs
- **[LilyGo EPD47](https://github.com/Xinyuan-LilyGO/LilyGo-EPD47)** - Hardware documentation

## Getting Help

If you encounter issues not covered here:

1. **Check Serial Monitor**: Most errors are visible in `idf.py monitor`
2. **Review Logs**: Build logs are in `build/log/`
3. **Search Issues**: Check project issues on GitHub
4. **ESP-IDF Issues**: https://github.com/espressif/esp-idf/issues
5. **Open New Issue**: Provide full build output and error messages

---

**Happy Building! 🔨**
