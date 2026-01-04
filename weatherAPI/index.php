<?php
/**
 * Weather Station Historical Data API
 * 
 * Returns historical weather data from TimescaleDB
 * 
 * ENDPOINTS:
 * GET /current              - Get most recent weather reading (all metrics)
 * GET /history?days=N       - Get last N days of all metrics (max 30)
 * GET /temperature?days=N   - Get temperature history only
 * GET /humidity?days=N      - Get humidity history only
 * GET /pressure?days=N      - Get pressure history only
 * GET /wind?days=N          - Get wind history only
 * GET /rain?days=N          - Get rainfall history only
 * 
 * RESPONSE FORMAT:
 * Combined (/history):      Array of objects with all metrics
 * Individual metrics:       Array of objects with timestamp, value, min, max
 * Wind metric:              Array with timestamp, speed, speed_min, speed_max, direction
 */

require_once 'config.php';

// Set timezone
date_default_timezone_set(API_TIMEZONE);

// Set security headers
header('Content-Type: application/json');
header('Access-Control-Allow-Origin: *');
header('Access-Control-Allow-Methods: GET');
header('X-Content-Type-Options: nosniff');
header('X-Frame-Options: DENY');
header('X-XSS-Protection: 1; mode=block');
header('Referrer-Policy: no-referrer');
// Recommend HTTPS in production
if (!empty($_SERVER['HTTPS']) && $_SERVER['HTTPS'] !== 'off') {
    header('Strict-Transport-Security: max-age=31536000; includeSubDomains');
}

/**
 * Simple rate limiting (requests per minute per IP)
 */
function checkRateLimit() {
    $rate_limit_file = sys_get_temp_dir() . '/weather_api_ratelimit_' . md5($_SERVER['REMOTE_ADDR']);
    $max_requests = 60; // Max requests per minute
    $time_window = 60; // Time window in seconds
    
    $current_time = time();
    $requests = [];
    
    // Load existing request log
    if (file_exists($rate_limit_file)) {
        $requests = json_decode(file_get_contents($rate_limit_file), true) ?: [];
    }
    
    // Remove old requests outside time window
    $requests = array_filter($requests, function($timestamp) use ($current_time, $time_window) {
        return ($current_time - $timestamp) < $time_window;
    });
    
    // Check if limit exceeded
    if (count($requests) >= $max_requests) {
        http_response_code(429);
        header('Retry-After: 60');
        echo json_encode(['error' => 'Rate limit exceeded. Maximum ' . $max_requests . ' requests per minute.']);
        exit;
    }
    
    // Add current request
    $requests[] = $current_time;
    file_put_contents($rate_limit_file, json_encode($requests));
}

// Apply rate limiting
checkRateLimit();

/**
 * Connect to TimescaleDB
 */
function getDBConnection() {
    try {
        $dsn = sprintf(
            "pgsql:host=%s;port=%s;dbname=%s",
            DB_HOST,
            DB_PORT,
            DB_NAME
        );
        
        $pdo = new PDO($dsn, DB_USER, DB_PASSWORD, [
            PDO::ATTR_ERRMODE => PDO::ERRMODE_EXCEPTION,
            PDO::ATTR_DEFAULT_FETCH_MODE => PDO::FETCH_ASSOC,
            PDO::ATTR_EMULATE_PREPARES => false
        ]);
        
        return $pdo;
    } catch (PDOException $e) {
        // Log full error server-side only
        error_log("Database connection failed: " . $e->getMessage());
        // Don't expose connection details to client
        return null;
    }
}

/**
 * Determine appropriate table suffix based on resolution/interval
 * Returns null if invalid to prevent SQL injection
 */
function getTableSuffix($resolution) {
    // Normalize input
    $resolution = strtolower(trim($resolution));
    
    // Strict whitelist - only allow exact matches
    $valid_resolutions = ['raw', '5m', '1h', '1d'];
    
    // Map alternative names
    $resolution_map = [
        'raw' => 'raw',
        'realtime' => 'raw',
        'current' => 'raw',
        '5min' => '5m',
        '5m' => '5m',
        '5minute' => '5m',
        'hour' => '1h',
        '1h' => '1h',
        'hourly' => '1h',
        'day' => '1d',
        '1d' => '1d',
        'daily' => '1d'
    ];
    
    if (isset($resolution_map[$resolution])) {
        $suffix = $resolution_map[$resolution];
        // Double-check against strict whitelist
        if (in_array($suffix, $valid_resolutions, true)) {
            return $suffix;
        }
    }
    
    // Default to 1h for invalid input
    return '1h';
}

/**
 * Safely quote table/column identifiers for PostgreSQL
 * Prevents SQL injection in dynamic table names
 */
function quoteIdentifier($identifier) {
    // Remove any existing quotes and escape internal quotes
    $identifier = str_replace('"', '""', $identifier);
    return '"' . $identifier . '"';
}

/**
 * Get historical weather data
 */
function getHistoricalData($days = DEFAULT_DAYS, $resolution = '1h') {
    // Validate and limit days
    $days = max(1, min($days, MAX_DAYS));
    
    // Get appropriate table suffix
    $suffix = getTableSuffix($resolution);
    
    $pdo = getDBConnection();
    if (!$pdo) {
        return ['error' => 'Database connection failed'];
    }
    
    try {
        $cutoff = date('Y-m-d H:i:s', strtotime("-$days days"));
        
        // For raw data, use base tables without aggregation
        if ($suffix === 'raw') {
            // Temperature
            $stmt = $pdo->prepare("SELECT time, temperature FROM temp WHERE time >= ? ORDER BY time ASC");
            $stmt->execute([$cutoff]);
            $temp_data = $stmt->fetchAll();
            
            // Humidity
            $stmt = $pdo->prepare("SELECT time, humidity FROM humidity WHERE time >= ? ORDER BY time ASC");
            $stmt->execute([$cutoff]);
            $humidity_data = $stmt->fetchAll();
            
            // Pressure
            $stmt = $pdo->prepare("SELECT time, pressure FROM pressure WHERE time >= ? ORDER BY time ASC");
            $stmt->execute([$cutoff]);
            $pressure_data = $stmt->fetchAll();
            
            // Wind
            $stmt = $pdo->prepare("SELECT time, direction, speed FROM wind WHERE time >= ? ORDER BY time ASC");
            $stmt->execute([$cutoff]);
            $wind_data = $stmt->fetchAll();
            
            // Rain
            $stmt = $pdo->prepare("SELECT time, rain FROM rain WHERE time >= ? ORDER BY time ASC");
            $stmt->execute([$cutoff]);
            $rain_data = $stmt->fetchAll();
            
            // Format raw data (no min/max values)
            $data = [];
            $max_length = max(
                count($temp_data),
                count($humidity_data),
                count($pressure_data),
                count($wind_data),
                count($rain_data)
            );
            
            for ($i = 0; $i < $max_length; $i++) {
                $entry = ['timestamp' => null];
                
                if (isset($temp_data[$i])) {
                    $entry['timestamp'] = strtotime($temp_data[$i]['time']);
                    $entry['temperature'] = round((float)$temp_data[$i]['temperature'], 1);
                }
                
                if (isset($humidity_data[$i])) {
                    if (!$entry['timestamp']) $entry['timestamp'] = strtotime($humidity_data[$i]['time']);
                    $entry['humidity'] = round((float)$humidity_data[$i]['humidity'], 1);
                }
                
                if (isset($pressure_data[$i])) {
                    if (!$entry['timestamp']) $entry['timestamp'] = strtotime($pressure_data[$i]['time']);
                    $entry['pressure'] = round((float)$pressure_data[$i]['pressure'], 1);
                }
                
                if (isset($wind_data[$i])) {
                    if (!$entry['timestamp']) $entry['timestamp'] = strtotime($wind_data[$i]['time']);
                    $entry['wind_direction'] = (int)$wind_data[$i]['direction'];
                    $entry['wind_speed'] = round((float)$wind_data[$i]['speed'], 1);
                }
                
                if (isset($rain_data[$i])) {
                    if (!$entry['timestamp']) $entry['timestamp'] = strtotime($rain_data[$i]['time']);
                    $entry['rainfall'] = round((float)$rain_data[$i]['rain'], 1);
                }
                
                if ($entry['timestamp']) {
                    $data[] = $entry;
                }
            }
            
            return $data;
        }
        
        // For aggregated data, use aggregate views
        // Query each aggregate table separately
        // Note: Using subqueries because TimescaleDB continuous aggregates don't support direct ORDER BY
        
        // Safely construct table names with proper identifier quoting
        $temp_table = 'temp_' . $suffix;
        $humidity_table = 'humidity_' . $suffix;
        $pressure_table = 'pressure_' . $suffix;
        $wind_table = 'wind_' . $suffix;
        $rain_table = 'rain_' . $suffix;
        
        // Temperature
        $sql = sprintf("
            SELECT * FROM (
                SELECT time, temperature, mintemperature, maxtemperature
                FROM %s
                WHERE time >= ?
            ) AS sub ORDER BY time ASC
        ", quoteIdentifier($temp_table));
        $stmt = $pdo->prepare($sql);
        $stmt->execute([$cutoff]);
        $temp_data = $stmt->fetchAll();
        
        // Humidity
        $sql = sprintf("
            SELECT * FROM (
                SELECT time, humidity, minhumidity, maxhumidity
                FROM %s
                WHERE time >= ?
            ) AS sub ORDER BY time ASC
        ", quoteIdentifier($humidity_table));
        $stmt = $pdo->prepare($sql);
        $stmt->execute([$cutoff]);
        $humidity_data = $stmt->fetchAll();
        
        // Pressure
        $sql = sprintf("
            SELECT * FROM (
                SELECT time, pressure, minpressure, maxpressure
                FROM %s
                WHERE time >= ?
            ) AS sub ORDER BY time ASC
        ", quoteIdentifier($pressure_table));
        $stmt = $pdo->prepare($sql);
        $stmt->execute([$cutoff]);
        $pressure_data = $stmt->fetchAll();
        
        // Wind
        $sql = sprintf("
            SELECT * FROM (
                SELECT time, direction, speed, minspeed, maxspeed
                FROM %s
                WHERE time >= ?
            ) AS sub ORDER BY time ASC
        ", quoteIdentifier($wind_table));
        $stmt = $pdo->prepare($sql);
        $stmt->execute([$cutoff]);
        $wind_data = $stmt->fetchAll();
        
        // Rain
        $sql = sprintf("
            SELECT * FROM (
                SELECT time, rain, minrain, maxrain
                FROM %s
                WHERE time >= ?
            ) AS sub ORDER BY time ASC
        ", quoteIdentifier($rain_table));
        $stmt = $pdo->prepare($sql);
        $stmt->execute([$cutoff]);
        $rain_data = $stmt->fetchAll();
        
        // Format and combine results
        $data = [];
        $max_length = max(
            count($temp_data),
            count($humidity_data),
            count($pressure_data),
            count($wind_data),
            count($rain_data)
        );
        
        for ($i = 0; $i < $max_length; $i++) {
            $entry = ['timestamp' => null];
            
            if (isset($temp_data[$i])) {
                $entry['timestamp'] = strtotime($temp_data[$i]['time']);
                $entry['temperature'] = round((float)$temp_data[$i]['temperature'], 1);
                $entry['temperature_min'] = round((float)$temp_data[$i]['mintemperature'], 1);
                $entry['temperature_max'] = round((float)$temp_data[$i]['maxtemperature'], 1);
            }
            
            if (isset($humidity_data[$i])) {
                if (!$entry['timestamp']) $entry['timestamp'] = strtotime($humidity_data[$i]['time']);
                $entry['humidity'] = round((float)$humidity_data[$i]['humidity'], 1);
                $entry['humidity_min'] = round((float)$humidity_data[$i]['minhumidity'], 1);
                $entry['humidity_max'] = round((float)$humidity_data[$i]['maxhumidity'], 1);
            }
            
            if (isset($pressure_data[$i])) {
                if (!$entry['timestamp']) $entry['timestamp'] = strtotime($pressure_data[$i]['time']);
                $entry['pressure'] = round((float)$pressure_data[$i]['pressure'], 1);
                $entry['pressure_min'] = round((float)$pressure_data[$i]['minpressure'], 1);
                $entry['pressure_max'] = round((float)$pressure_data[$i]['maxpressure'], 1);
            }
            
            if (isset($wind_data[$i])) {
                if (!$entry['timestamp']) $entry['timestamp'] = strtotime($wind_data[$i]['time']);
                $entry['wind_direction'] = round((float)$wind_data[$i]['direction'], 0);
                $entry['wind_speed'] = round((float)$wind_data[$i]['speed'], 1);
                $entry['wind_speed_min'] = round((float)$wind_data[$i]['minspeed'], 1);
                $entry['wind_speed_max'] = round((float)$wind_data[$i]['maxspeed'], 1);
            }
            
            if (isset($rain_data[$i])) {
                if (!$entry['timestamp']) $entry['timestamp'] = strtotime($rain_data[$i]['time']);
                $entry['rainfall'] = round((float)$rain_data[$i]['rain'], 1);
                $entry['rainfall_min'] = round((float)$rain_data[$i]['minrain'], 1);
                $entry['rainfall_max'] = round((float)$rain_data[$i]['maxrain'], 1);
            }
            
            if ($entry['timestamp']) {
                $data[] = $entry;
            }
        }
        
        return $data;
        
    } catch (PDOException $e) {
        // Log full error server-side only
        error_log("Query failed in getHistoricalData: " . $e->getMessage());
        // Generic error message to client
        return ['error' => 'Unable to retrieve historical data'];
    }
}

/**
 * Get current (most recent) weather data
 */
function getCurrentData() {
    $pdo = getDBConnection();
    if (!$pdo) {
        return ['error' => 'Database connection failed'];
    }
    
    try {
        $data = [];
        
        // Get latest temperature
        $stmt = $pdo->query("SELECT time, temperature FROM temp ORDER BY time DESC LIMIT 1");
        $row = $stmt->fetch();
        if ($row) {
            $data['timestamp'] = strtotime($row['time']);
            $data['temperature'] = round((float)$row['temperature'], 1);
        }
        
        // Get latest humidity
        $stmt = $pdo->query("SELECT time, humidity FROM humidity ORDER BY time DESC LIMIT 1");
        $row = $stmt->fetch();
        if ($row) {
            if (!isset($data['timestamp'])) $data['timestamp'] = strtotime($row['time']);
            $data['humidity'] = round((float)$row['humidity'], 1);
        }
        
        // Get latest pressure
        $stmt = $pdo->query("SELECT time, pressure FROM pressure ORDER BY time DESC LIMIT 1");
        $row = $stmt->fetch();
        if ($row) {
            if (!isset($data['timestamp'])) $data['timestamp'] = strtotime($row['time']);
            $data['pressure'] = round((float)$row['pressure'], 1);
        }
        
        // Get latest wind
        $stmt = $pdo->query("SELECT time, direction, speed, speed_max, speed_min FROM wind ORDER BY time DESC LIMIT 1");
        $row = $stmt->fetch();
        if ($row) {
            if (!isset($data['timestamp'])) $data['timestamp'] = strtotime($row['time']);
            $data['wind_direction'] = (int)$row['direction'];
            $data['wind_speed'] = round((float)$row['speed'], 1);
            $data['wind_speed_max'] = round((float)$row['speed_max'], 1);
            $data['wind_speed_min'] = round((float)$row['speed_min'], 1);
        }
        
        // Get latest rain
        $stmt = $pdo->query("SELECT time, rain FROM rain ORDER BY time DESC LIMIT 1");
        $row = $stmt->fetch();
        if ($row) {
            if (!isset($data['timestamp'])) $data['timestamp'] = strtotime($row['time']);
            $data['rainfall'] = round((float)$row['rain'], 1);
        }
        
        if (empty($data)) {
            return ['error' => 'No data available'];
        }
        
        return $data;
        
    } catch (PDOException $e) {
        // Log full error server-side only
        error_log("Query failed in getCurrentData: " . $e->getMessage());
        // Generic error message to client
        return ['error' => 'Unable to retrieve current data'];
    }
}

/**
 * Get historical data for a specific metric
 */
function getMetricHistory($metric, $days = DEFAULT_DAYS, $resolution = '1h') {
    // Validate metric parameter - strict whitelist
    $valid_metrics = ['temperature', 'humidity', 'pressure', 'wind', 'rain'];
    if (!in_array($metric, $valid_metrics, true)) {
        return ['error' => 'Invalid metric. Use: temperature, humidity, pressure, wind, or rain'];
    }
    
    // Validate and limit days
    $days = max(1, min($days, MAX_DAYS));
    
    // Get appropriate table suffix
    $suffix = getTableSuffix($resolution);
    
    // Map metric names to table names
    $table_map = [
        'temperature' => 'temp',
        'humidity' => 'humidity',
        'pressure' => 'pressure',
        'wind' => 'wind',
        'rain' => 'rain'
    ];
    
    if (!isset($table_map[$metric])) {
        return ['error' => 'Invalid metric. Use: temperature, humidity, pressure, wind, or rain'];
    }
    
    $table = $table_map[$metric];
    $pdo = getDBConnection();
    if (!$pdo) {
        return ['error' => 'Database connection failed'];
    }
    
    try {
        $cutoff = date('Y-m-d H:i:s', strtotime("-$days days"));
        
        // Build query based on metric and resolution
        if ($metric === 'temperature') {
            if ($suffix === 'raw') {
                $stmt = $pdo->prepare("SELECT time, temperature FROM temp WHERE time >= ? ORDER BY time ASC");
                $stmt->execute([$cutoff]);
                $data = $stmt->fetchAll();
                
                $result = [];
                foreach ($data as $row) {
                    $result[] = [
                        'timestamp' => strtotime($row['time']),
                        'value' => round((float)$row['temperature'], 1)
                    ];
                }
                return $result;
            } else {
                $table_name = 'temp_' . $suffix;
                $sql = sprintf("
                    SELECT * FROM (
                        SELECT time, temperature, mintemperature, maxtemperature
                        FROM %s
                        WHERE time >= ?
                    ) AS sub ORDER BY time ASC
                ", quoteIdentifier($table_name));
                $stmt = $pdo->prepare($sql);
                $stmt->execute([$cutoff]);
                $data = $stmt->fetchAll();
                
                // Format response
                $result = [];
                foreach ($data as $row) {
                    $result[] = [
                        'timestamp' => strtotime($row['time']),
                        'value' => round((float)$row['temperature'], 1),
                        'min' => round((float)$row['mintemperature'], 1),
                        'max' => round((float)$row['maxtemperature'], 1)
                    ];
                }
                return $result;
            }
            
        } elseif ($metric === 'humidity') {
            if ($suffix === 'raw') {
                $stmt = $pdo->prepare("SELECT time, humidity FROM humidity WHERE time >= ? ORDER BY time ASC");
                $stmt->execute([$cutoff]);
                $data = $stmt->fetchAll();
                
                $result = [];
                foreach ($data as $row) {
                    $result[] = [
                        'timestamp' => strtotime($row['time']),
                        'value' => round((float)$row['humidity'], 1)
                    ];
                }
                return $result;
            } else {
                $table_name = 'humidity_' . $suffix;
                $sql = sprintf("
                    SELECT * FROM (
                        SELECT time, humidity, minhumidity, maxhumidity
                        FROM %s
                        WHERE time >= ?
                    ) AS sub ORDER BY time ASC
                ", quoteIdentifier($table_name));
                $stmt = $pdo->prepare($sql);
                $stmt->execute([$cutoff]);
                $data = $stmt->fetchAll();
                
                $result = [];
                foreach ($data as $row) {
                    $result[] = [
                        'timestamp' => strtotime($row['time']),
                        'value' => round((float)$row['humidity'], 1),
                        'min' => round((float)$row['minhumidity'], 1),
                        'max' => round((float)$row['maxhumidity'], 1)
                    ];
                }
                return $result;
            }
            
        } elseif ($metric === 'pressure') {
            if ($suffix === 'raw') {
                $stmt = $pdo->prepare("SELECT time, pressure FROM pressure WHERE time >= ? ORDER BY time ASC");
                $stmt->execute([$cutoff]);
                $data = $stmt->fetchAll();
                
                $result = [];
                foreach ($data as $row) {
                    $result[] = [
                        'timestamp' => strtotime($row['time']),
                        'value' => round((float)$row['pressure'], 1)
                    ];
                }
                return $result;
            } else {
                $table_name = 'pressure_' . $suffix;
                $sql = sprintf("
                    SELECT * FROM (
                        SELECT time, pressure, minpressure, maxpressure
                        FROM %s
                        WHERE time >= ?
                    ) AS sub ORDER BY time ASC
                ", quoteIdentifier($table_name));
                $stmt = $pdo->prepare($sql);
                $stmt->execute([$cutoff]);
                $data = $stmt->fetchAll();
                
                $result = [];
                foreach ($data as $row) {
                    $result[] = [
                        'timestamp' => strtotime($row['time']),
                        'value' => round((float)$row['pressure'], 1),
                        'min' => round((float)$row['minpressure'], 1),
                        'max' => round((float)$row['maxpressure'], 1)
                    ];
                }
                return $result;
            }
            
        } elseif ($metric === 'wind') {
            if ($suffix === 'raw') {
                $stmt = $pdo->prepare("SELECT time, direction, speed FROM wind WHERE time >= ? ORDER BY time ASC");
                $stmt->execute([$cutoff]);
                $data = $stmt->fetchAll();
                
                $result = [];
                foreach ($data as $row) {
                    $result[] = [
                        'timestamp' => strtotime($row['time']),
                        'speed' => round((float)$row['speed'], 1),
                        'direction' => (int)$row['direction']
                    ];
                }
                return $result;
            } else {
                $table_name = 'wind_' . $suffix;
                $sql = sprintf("
                    SELECT * FROM (
                        SELECT time, direction, speed, minspeed, maxspeed
                        FROM %s
                        WHERE time >= ?
                    ) AS sub ORDER BY time ASC
                ", quoteIdentifier($table_name));
                $stmt = $pdo->prepare($sql);
                $stmt->execute([$cutoff]);
                $data = $stmt->fetchAll();
                
                $result = [];
                foreach ($data as $row) {
                    $result[] = [
                        'timestamp' => strtotime($row['time']),
                        'speed' => round((float)$row['speed'], 1),
                        'speed_min' => round((float)$row['minspeed'], 1),
                        'speed_max' => round((float)$row['maxspeed'], 1),
                        'direction' => round((float)$row['direction'], 0)
                    ];
                }
                return $result;
            }
            
        } elseif ($metric === 'rain') {
            if ($suffix === 'raw') {
                $stmt = $pdo->prepare("SELECT time, rain FROM rain WHERE time >= ? ORDER BY time ASC");
                $stmt->execute([$cutoff]);
                $data = $stmt->fetchAll();
                
                $result = [];
                foreach ($data as $row) {
                    $result[] = [
                        'timestamp' => strtotime($row['time']),
                        'value' => round((float)$row['rain'], 1)
                    ];
                }
                return $result;
            } else {
                $table_name = 'rain_' . $suffix;
                $sql = sprintf("
                    SELECT * FROM (
                        SELECT time, rain, minrain, maxrain
                        FROM %s
                        WHERE time >= ?
                    ) AS sub ORDER BY time ASC
                ", quoteIdentifier($table_name));
                $stmt = $pdo->prepare($sql);
                $stmt->execute([$cutoff]);
                $data = $stmt->fetchAll();
                
                $result = [];
                foreach ($data as $row) {
                    $result[] = [
                        'timestamp' => strtotime($row['time']),
                        'value' => round((float)$row['rain'], 1),
                        'min' => round((float)$row['minrain'], 1),
                        'max' => round((float)$row['maxrain'], 1)
                    ];
                }
                return $result;
            }
        }
        
    } catch (PDOException $e) {
        // Log full error server-side only
        error_log("Query failed in getMetricHistory: " . $e->getMessage());
        // Generic error message to client
        return ['error' => 'Unable to retrieve metric history'];
    }
}

/**
 * Validate request method
 */
if ($_SERVER['REQUEST_METHOD'] !== 'GET') {
    http_response_code(405);
    header('Allow: GET');
    echo json_encode(['error' => 'Method not allowed. Only GET requests are supported.']);
    exit;
}

/**
 * Route the request
 */
$requestUri = $_SERVER['REQUEST_URI'];
$requestPath = parse_url($requestUri, PHP_URL_PATH);

// Validate and sanitize request path
if ($requestPath === false || $requestPath === null) {
    http_response_code(400);
    echo json_encode(['error' => 'Invalid request URI']);
    exit;
}

// Remove trailing slash and any directory traversal attempts
$requestPath = rtrim($requestPath, '/');
$requestPath = str_replace(['../', '..\\'], '', $requestPath);

// Get and validate common parameters
$days = isset($_GET['days']) ? (int)$_GET['days'] : DEFAULT_DAYS;
// Sanitize resolution parameter - remove any potentially dangerous characters
$resolution = isset($_GET['resolution']) ? preg_replace('/[^a-z0-9]/', '', strtolower($_GET['resolution'])) : '1h';

// Extract endpoint
if (strpos($requestPath, 'current') !== false) {
    // Get current data
    $response = getCurrentData();
    
} elseif (strpos($requestPath, 'temperature') !== false) {
    // Get temperature history
    $response = getMetricHistory('temperature', $days, $resolution);
    
} elseif (strpos($requestPath, 'humidity') !== false) {
    // Get humidity history
    $response = getMetricHistory('humidity', $days, $resolution);
    
} elseif (strpos($requestPath, 'pressure') !== false) {
    // Get pressure history
    $response = getMetricHistory('pressure', $days, $resolution);
    
} elseif (strpos($requestPath, 'wind') !== false) {
    // Get wind history
    $response = getMetricHistory('wind', $days, $resolution);
    
} elseif (strpos($requestPath, 'rain') !== false || strpos($requestPath, 'rainfall') !== false) {
    // Get rain history
    $response = getMetricHistory('rain', $days, $resolution);
    
} else {
    // Get all historical data (combined)
    $response = getHistoricalData($days, $resolution);
}

// Output response
if (DEBUG_MODE) {
    echo json_encode($response, JSON_PRETTY_PRINT);
} else {
    echo json_encode($response);
}
?>
