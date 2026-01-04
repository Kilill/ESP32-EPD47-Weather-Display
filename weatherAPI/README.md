# Weather Station Historical Data API

PHP REST API that provides historical weather data from a TimescaleDB database.

## Database Schema

The database contains the following base tables:
- `temp` - Temperature readings (°C)
- `humidity` - Humidity readings (%)
- `pressure` - Air pressure readings (hPa)
- `wind` - Wind speed (m/s) and direction (degrees)
- `rain` - Rainfall (mm)

And aggregate views for different time intervals:
- `*_5m` - 5-minute aggregates
- `*_1h` - Hourly aggregates
- `*_1d` - Daily aggregates

Each aggregate view includes the average value plus min/max columns (e.g., `temperature`, `mintemperature`, `maxtemperature`).

## Data Resolution

All historical data endpoints support a `resolution` parameter to control data granularity:

| Resolution | Description | Use Case | Includes Min/Max |
|-----------|-------------|----------|------------------|
| `raw` | Raw sensor data | Real-time display, last few minutes | No |
| `5m` | 5-minute aggregates | Recent detailed history (hours) | Yes |
| `1h` | Hourly aggregates | Weekly charts (default) | Yes |
| `1d` | Daily aggregates | Monthly/yearly trends | Yes |

**Aliases:** `realtime` → `raw`, `hourly` → `1h`, `daily` → `1d`

Default resolution is `1h` if not specified.

## API Endpoints

### Get Current Weather
```
GET /current
or
GET /index.php?current
```

Returns the most recent weather reading from all sensors.

**Example:**
```bash
curl "https://weather.example.com:4433/weatherAPI/current"
```

**Response:**
```json
{
  "timestamp": 1733313982,
  "temperature": 7.0,
  "humidity": 92.2,
  "pressure": 1002.7,
  "wind_speed": 0.0,
  "wind_speed_max": 0.0,
  "wind_speed_min": 0.0,
  "wind_direction": 275,
  "rainfall": 0.0
}
```

### Get Historical Data (All Metrics)
```
GET /history?days=7
or
GET /index.php?days=7
GET /history?days=7&resolution=1h
```

Returns aggregated weather data for all metrics combined.

**Example:**
```bash
# Get 7 days of hourly data (default)
curl "https://weather.example.com:4433/weatherAPI/history?days=7"

# Get 1 day of 5-minute resolution data
curl "https://weather.example.com:4433/weatherAPI/history?days=1&resolution=5m"

# Get 30 days of daily aggregates
curl "https://weather.example.com:4433/weatherAPI/history?days=30&resolution=1d"
```

**Parameters:**
- `days` (optional, default: 7) - Number of days of historical data (1-30)
- `resolution` (optional, default: 1h) - Data granularity: `raw`, `5m`, `1h`, `1d`

**Response:**
```json
[
  {
    "timestamp": 1732518000,
    "temperature": -0.6,
    "temperature_min": -1.0,
    "temperature_max": -0.5,
    "humidity": 80.6,
    "humidity_min": 80.5,
    "humidity_max": 80.7,
    "pressure": 1003.2,
    "pressure_min": 1003.0,
    "pressure_max": 1003.4,
    "wind_direction": 20,
    "wind_speed": 2.4,
    "wind_speed_min": 0.7,
    "wind_speed_max": 6.1,
    "rainfall": 0.0,
    "rainfall_min": 0.0,
    "rainfall_max": 0.0
  }
]
```

### Get Individual Metric History

Retrieve historical data for a specific metric only. More efficient when you only need one data type.

All individual metric endpoints support the `resolution` parameter.

#### Temperature
```
GET /temperature?days=7
GET /temperature?days=1&resolution=raw
GET /temperature?days=30&resolution=1d
```

**Response (aggregated data):**
```json
[
  {
    "timestamp": 1732518000,
    "value": -0.6,
    "min": -1.0,
    "max": -0.5
  }
]
```

**Response (raw data):**
```json
[
  {
    "timestamp": 1732518000,
    "value": -0.6
  }
]
```

Note: Raw resolution (`resolution=raw`) returns only the `value` field without `min`/`max`.

#### Humidity
```
GET /humidity?days=7&resolution=1h
```

**Response:**
```json
[
  {
    "timestamp": 1732518000,
    "value": 80.6,
    "min": 80.5,
    "max": 80.7
  }
]
```

#### Pressure
```
GET /pressure?days=7
```

**Response:**
```json
[
  {
    "timestamp": 1732518000,
    "value": 1003.2,
    "min": 1003.0,
    "max": 1003.4
  }
]
```

#### Wind
```
GET /wind?days=7
```

**Response:**
```json
[
  {
    "timestamp": 1732518000,
    "speed": 2.4,
    "speed_min": 0.7,
    "speed_max": 6.1,
    "direction": 20
  }
]
```

#### Rainfall
```
GET /rain?days=7
```

**Response:**
```json
[
  {
    "timestamp": 1732518000,
    "value": 0.0,
    "min": 0.0,
    "max": 0.0
  }
]
```

**Common Parameters (all metric endpoints):**
- `days` (optional, default: 7) - Number of days of historical data (1-30)

**Benefits of Individual Endpoints:**
- Smaller response payload (faster for bandwidth-limited devices like ESP32)
- Only retrieve the data you need
- Simpler parsing on client side

## Setup

### 1. Configuration

Copy the example configuration:
```bash
cd weatherAPI
cp config.php.example config.php
```

Edit `config.php` with your database credentials:
```php
define('DB_HOST', 'your-database-host');
define('DB_PORT', '5432');
define('DB_NAME', 'weather');
define('DB_USER', 'your-username');
define('DB_PASSWORD', 'your-password');
```

### 2. Requirements

- PHP 7.0 or higher
- PDO PostgreSQL extension (`php-pgsql`)
- Access to TimescaleDB/PostgreSQL database

Install PHP PostgreSQL extension:
```bash
# Debian/Ubuntu
sudo apt-get install php-pgsql

# RHEL/CentOS
sudo yum install php-pdo php-pgsql
```

### 3. Web Server Setup

#### Apache Configuration

**Step 1: Deploy Files**

Copy the weatherAPI directory to your web server:
```bash
# Copy to Apache document root
sudo cp -r weatherAPI /var/www/html/

# Or to a virtual host directory
sudo cp -r weatherAPI /var/www/yourdomain.com/
```

**Step 2: Enable Required Apache Modules**

```bash
# Enable mod_rewrite for URL rewriting
sudo a2enmod rewrite

# Enable mod_headers for CORS (if needed)
sudo a2enmod headers

# Restart Apache
sudo systemctl restart apache2
```

**Step 3: Configure .htaccess**

The included `.htaccess` file provides URL rewriting to support clean URLs:

```apache
# weatherAPI/.htaccess
RewriteEngine On
RewriteCond %{REQUEST_FILENAME} !-f
RewriteCond %{REQUEST_FILENAME} !-d
RewriteRule ^(.*)$ index.php [QSA,L]
```

This allows URLs like:
- `/weatherAPI/current` → `/weatherAPI/index.php?current`
- `/weatherAPI/history` → `/weatherAPI/index.php?days=7`
- `/weatherAPI/temperature?days=7` → `/weatherAPI/index.php?temperature&days=7`

**Step 4: Enable .htaccess Override**

Apache must be configured to allow `.htaccess` files to override settings.

Edit your Apache configuration file:

**For site-specific (recommended):**
```bash
sudo nano /etc/apache2/sites-available/000-default.conf
# or
sudo nano /etc/apache2/sites-available/yourdomain.conf
```

**For global:**
```bash
sudo nano /etc/apache2/apache2.conf
```

Add or modify the Directory directive for your weatherAPI location:

```apache
<VirtualHost *:80>
    ServerName yourdomain.com
    DocumentRoot /var/www/html

    # Enable .htaccess for weatherAPI directory
    <Directory /var/www/html/weatherAPI>
        Options Indexes FollowSymLinks
        AllowOverride FileInfo
        Require all granted
    </Directory>

    ErrorLog ${APACHE_LOG_DIR}/error.log
    CustomLog ${APACHE_LOG_DIR}/access.log combined
</VirtualHost>
```

**Key setting:** `AllowOverride FileInfo` enables URL rewriting via .htaccess

**Alternative settings:**
- `AllowOverride All` - Allows all .htaccess directives (less secure)
- `AllowOverride None` - Disables .htaccess (must use full URLs with index.php)

**Step 5: Test Configuration**

```bash
# Test Apache configuration syntax
sudo apache2ctl configtest

# If OK, reload Apache
sudo systemctl reload apache2
```

**Step 6: Verify URL Rewriting**

Test that clean URLs work:
```bash
# Should work with URL rewriting enabled
curl http://yourdomain.com/weatherAPI/current

# Should also work (explicit index.php)
curl http://yourdomain.com/weatherAPI/index.php?current
```

If clean URLs return 404, check:
1. `.htaccess` file exists and is readable
2. `AllowOverride FileInfo` is enabled
3. `mod_rewrite` is loaded
4. Apache error logs: `sudo tail -f /var/log/apache2/error.log`

#### Nginx Configuration (Alternative)

If using Nginx instead of Apache:

```nginx
server {
    listen 80;
    server_name yourdomain.com;
    root /var/www/html;

    location /weatherAPI/ {
        try_files $uri $uri/ /weatherAPI/index.php?$args;
        
        location ~ \.php$ {
            fastcgi_pass unix:/var/run/php/php7.4-fpm.sock;
            fastcgi_index index.php;
            fastcgi_param SCRIPT_FILENAME $document_root$fastcgi_script_name;
            include fastcgi_params;
        }
    }
}
```

#### File Permissions

Ensure proper permissions:
```bash
# Set ownership to web server user
sudo chown -R www-data:www-data /var/www/html/weatherAPI

# Set appropriate permissions
sudo chmod 755 /var/www/html/weatherAPI
sudo chmod 644 /var/www/html/weatherAPI/*.php
sudo chmod 644 /var/www/html/weatherAPI/.htaccess

# Protect config file (optional - make it less accessible)
sudo chmod 600 /var/www/html/weatherAPI/config.php
```

## Testing

Test database connection and queries:
```bash
php test_api.php
```

Test API functions directly:
```bash
php test_functions.php
```

## Configuration Options

Edit [config.php](config.php) to customize:

| Setting | Description | Default |
|---------|-------------|---------|
| `DB_HOST` | Database hostname | localhost |
| `DB_PORT` | PostgreSQL port | 5432 |
| `DB_NAME` | Database name | weather |
| `DB_USER` | Database username | - |
| `DB_PASSWORD` | Database password | - |
| `API_TIMEZONE` | Timezone for date operations | UTC |
| `DEFAULT_DAYS` | Default history period | 7 |
| `MAX_DAYS` | Maximum allowed history period | 30 |
| `DEBUG_MODE` | Enable pretty-printed JSON | false |

## ESP32 Integration

The ESP32 weather station display calls this API to retrieve historical data for chart display.

### Configuration

Configure the API URL in your ESP32 project:

**With URL Rewriting (Clean URLs):**
```c
// In main/secrets.h
#define WEBSERVER_URL "http://your-server/weatherAPI"
```

**Without URL Rewriting:**
```c
// In main/secrets.h
#define WEBSERVER_URL "http://your-server/weatherAPI/index.php"
```

**Endpoints called by ESP32:**
```c
// In main/config.h
#define HTTP_CURRENT_ENDPOINT "/current"           // Real-time data
#define HTTP_HISTORY_ENDPOINT "/history"           // 7-day history
```

### Data Flow

The weather station will:
1. Connect to WiFi
2. Request current weather: `GET WEBSERVER_URL + /current`
3. Request historical data: `GET WEBSERVER_URL + /history`
4. Parse the JSON responses using cJSON
5. Display data on E-paper display
6. Update every 60 seconds (configurable)

### Testing from ESP32 Network

Ensure the API is accessible from your ESP32's network:
```bash
# From a device on the same network as ESP32
curl http://your-server-ip/weatherAPI/current
curl http://your-server-ip/weatherAPI/history?days=7
```

If using a domain name, ensure DNS is configured correctly.

## Technical Notes

### TimescaleDB Continuous Aggregates

The API queries hourly aggregate views (`*_1h` tables) which are TimescaleDB continuous aggregates. These have a quirk where direct ORDER BY on the time column fails. The workaround is using subqueries:

```php
// This fails:
SELECT time, temperature FROM temp_1h ORDER BY time DESC

// This works:
SELECT * FROM (
    SELECT time, temperature FROM temp_1h
) AS sub ORDER BY time DESC
```

### Data Structure

Weather data is split across multiple tables for efficient storage:
- Each metric has its own base table (temp, humidity, etc.)
- Aggregates are computed separately for each metric
- The API combines data from all tables by timestamp
- Missing data is handled gracefully (empty fields if a sensor is offline)

### Performance

- Hourly aggregates contain ~24 records per day
- 7 days = ~168 records per metric
- 30 days = ~720 records per metric
- All queries use indexes on the time column
- Response time typically < 100ms for 7-day queries

## Troubleshooting

### Apache Issues

**"404 Not Found" on Clean URLs**
- Check `.htaccess` file exists: `ls -la weatherAPI/.htaccess`
- Verify `mod_rewrite` is enabled: `apache2ctl -M | grep rewrite`
- Check Apache config has `AllowOverride FileInfo` for the directory
- Test with explicit URL: `curl http://yourserver/weatherAPI/index.php?current`
- Review Apache error log: `sudo tail -f /var/log/apache2/error.log`

**"Internal Server Error" (500)**
- Check `.htaccess` syntax for errors
- Ensure `mod_rewrite` is loaded
- Check file permissions (644 for .htaccess)
- Look for PHP errors in error log

**"403 Forbidden"**
- Check file permissions (755 for directory, 644 for files)
- Verify `Require all granted` in Apache config
- Ensure web server user (www-data) can read files

### Database Issues

**"Database connection failed"**
- Check that PostgreSQL is running
- Verify credentials in config.php
- Ensure PHP PDO extension is installed

**"Query execution failed: ORDER/GROUP BY expression not found"**
- This happens with direct ORDER BY on aggregate views
- The code uses subquery workaround (should not occur)

**Empty result set**
- Check that data exists in the database for the requested time period
- Verify aggregate views are populated: `SELECT COUNT(*) FROM temp_1h`
- Increase `days` parameter to match available data

## Security

- Keep `config.php` out of version control (use .gitignore)
- Restrict database user to SELECT-only permissions
- Use HTTPS in production
- Consider API authentication for public deployments
- The included `.htaccess` provides basic security headers

## License

Part of the ESP32 Weather Station Display project.
