# Configuration Setup Guide

This project uses a two-file configuration system to keep sensitive data secure.

## Quick Setup

1. **Copy the secrets template:**
   ```bash
   cp main/secrets.h.example main/secrets.h
   ```

2. **Edit `main/secrets.h` with your credentials:**
   ```bash
   nano main/secrets.h  # or your preferred editor
   ```

3. **Configure your settings in `main/config.h` (optional):**
   - Display dimensions
   - Update intervals
   - Other non-sensitive settings

## Configuration Files

### `main/secrets.h` ⚠️ (Not in git)
Contains sensitive credentials:
- WiFi SSID and password
- Web server URL and API keys

**Important:** This file is gitignored and never committed to version control.

### `main/secrets.h.example` ✅ (In git)
Template file showing required configuration format. Safe to commit.

### `main/config.h` ✅ (In git)
Non-sensitive project configuration:
- Display settings
- Update intervals
- HTTP timeouts
- Data retention settings

## Configuration Reference

### WiFi Settings (secrets.h)

```c
#define WIFI_SSID               "YourNetworkName"
#define WIFI_PASSWORD           "YourNetworkPassword"
```

### Web Server Settings (secrets.h + config.h)

**Secrets (secrets.h):**
```c
#define WEBSERVER_URL           "http://192.168.1.100:5000/api/history"
#define WEBSERVER_API_KEY       "your_api_key_if_needed"
```

**Config (config.h):**
```c
#define HTTP_TIMEOUT_MS         10000
#define HTTP_BUFFER_SIZE        8192
```

### Display Settings (config.h)

```c
// Update interval
#define DISPLAY_UPDATE_INTERVAL_MINUTES     5

// Display dimensions (adjust for your Lillygo T5 model)
#define DISPLAY_WIDTH   960
#define DISPLAY_HEIGHT  540
```

Common display sizes:
- **2.13"**: 250x122 or 212x104
- **2.9"**: 296x128  
- **4.2"**: 400x300
- **7.5"**: 960x540 or 800x480

## Security Best Practices

### ✅ Do:
- Keep `secrets.h` out of version control
- Use environment-specific secrets files for different deployments
- Limit file permissions: `chmod 600 main/secrets.h`

### ❌ Don't:
- Commit `secrets.h` to git
- Share your secrets file
- Use plain text passwords in production without encryption
- Hardcode credentials in other source files

## Multiple Environments

For different deployment environments (dev/staging/production):

```bash
# Development
cp main/secrets.dev.h main/secrets.h

# Production
cp main/secrets.prod.h main/secrets.h
```

Keep environment-specific templates outside the repository or in a secure vault.

## Verification

After configuration, verify your settings:

```bash
# Check that secrets.h exists
ls -la main/secrets.h

# Verify it's gitignored
git status main/secrets.h  # Should not appear in git status

# Build the project
idf.py build
```

## Troubleshooting

**Problem:** Build fails with "secrets.h: No such file or directory"
- **Solution:** Copy `secrets.h.example` to `secrets.h`

**Problem:** Git wants to commit secrets.h
- **Solution:** Ensure `.gitignore` includes `main/secrets.h`

**Problem:** WiFi connection fails
- **Solution:** Double-check SSID and password in `secrets.h`

## Example Configurations

### Home Development Setup
```c
// secrets.h
#define WIFI_SSID               "HomeWiFi"
#define WIFI_PASSWORD           "MyPassword123"
#define WEBSERVER_URL           "http://192.168.1.100:5000/api/history"
```

### Production Setup with Security
```c
// secrets.h
#define WIFI_SSID               "ProductionNetwork"
#define WIFI_PASSWORD           "SecurePassword456!"
#define WEBSERVER_URL           "https://api.example.com/weather/history"
#define WEBSERVER_API_KEY       "Bearer eyJ0eXAiOiJKV1QiLCJhbGc..."
```

## Migration from Kconfig

If you're migrating from the old Kconfig system:

1. Run `idf.py menuconfig` and note your current settings
2. Copy values to `secrets.h` and `config.h`
3. Build the project - it now uses the new configuration system
4. Optional: Remove `Kconfig.projbuild` if you prefer pure C configuration

## Additional Resources

- [ESP-IDF Configuration System](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/kconfig.html)
- [Git Security Best Practices](https://git-scm.com/book/en/v2/Git-Tools-Credential-Storage)
