#!/usr/bin/env php
<?php
/**
 * Simple test that invokes the API functions directly
 */

require_once 'config.php';
require_once 'index.php';

echo "Testing Weather API Functions\n";
echo "==============================\n\n";

// Test getCurrentData
echo "1. Testing getCurrentData()...\n";
$current = getCurrentData();

if (isset($current['error'])) {
    echo "   ✗ Error: " . $current['error'] . "\n";
} else {
    echo "   ✓ Success!\n";
    echo "     Temperature: " . ($current['temperature'] ?? 'N/A') . "°C\n";
    echo "     Humidity: " . ($current['humidity'] ?? 'N/A') . "%\n";
    echo "     Pressure: " . ($current['pressure'] ?? 'N/A') . " hPa\n";
    echo "     Wind Speed: " . ($current['wind_speed'] ?? 'N/A') . " m/s\n";
    echo "     Wind Direction: " . ($current['wind_direction'] ?? 'N/A') . "°\n";
    echo "     Rainfall: " . ($current['rainfall'] ?? 'N/A') . " mm\n";
}

// Test getHistoricalData
echo "\n2. Testing getHistoricalData(1 day)...\n";
$history = getHistoricalData(1);

if (isset($history['error'])) {
    echo "   ✗ Error: " . $history['error'] . "\n";
} else if (is_array($history) && count($history) > 0) {
    echo "   ✓ Success! Retrieved " . count($history) . " records\n";
    $first = $history[0];
    echo "     First record timestamp: " . date('Y-m-d H:i:s', $first['timestamp'] ?? 0) . "\n";
    echo "     - Temperature: " . ($first['temperature'] ?? 'N/A') . "°C";
    if (isset($first['temperature_min'])) echo " (min: {$first['temperature_min']}°C, max: {$first['temperature_max']}°C)";
    echo "\n";
    echo "     - Humidity: " . ($first['humidity'] ?? 'N/A') . "%\n";
    echo "     - Wind Speed: " . ($first['wind_speed'] ?? 'N/A') . " m/s\n";
    
    $last = $history[count($history) - 1];
    echo "     Last record timestamp: " . date('Y-m-d H:i:s', $last['timestamp'] ?? 0) . "\n";
} else {
    echo "   ✗ No data returned (or empty array)\n";
}

echo "\n3. Testing getHistoricalData(7 days)...\n";
$history7 = getHistoricalData(7);

if (isset($history7['error'])) {
    echo "   ✗ Error: " . $history7['error'] . "\n";
} else if (is_array($history7)) {
    echo "   ✓ Success! Retrieved " . count($history7) . " records for 7 days\n";
} else {
    echo "   ✗ Unexpected response\n";
}

echo "\n✓ Function tests complete!\n";
?>
