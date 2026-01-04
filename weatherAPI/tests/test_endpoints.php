#!/usr/bin/env php
<?php
/**
 * Test the API endpoints directly
 */

echo "Testing Weather API Endpoints\n";
echo "==============================\n\n";

// Test getCurrentData
echo "1. Testing /current endpoint...\n";
$_SERVER['REQUEST_URI'] = '/api/current';
ob_start();
include 'index.php';
$output = ob_get_clean();
$data = json_decode($output, true);

if (isset($data['error'])) {
    echo "   ✗ Error: " . $data['error'] . "\n";
} else {
    echo "   ✓ Success! Got current data:\n";
    echo "     - Temperature: " . ($data['temperature'] ?? 'N/A') . "°C\n";
    echo "     - Humidity: " . ($data['humidity'] ?? 'N/A') . "%\n";
    echo "     - Pressure: " . ($data['pressure'] ?? 'N/A') . " hPa\n";
    echo "     - Wind Speed: " . ($data['wind_speed'] ?? 'N/A') . " m/s\n";
    echo "     - Wind Direction: " . ($data['wind_direction'] ?? 'N/A') . "°\n";
    echo "     - Rainfall: " . ($data['rainfall'] ?? 'N/A') . " mm\n";
}

echo "\n2. Testing /history endpoint (1 day)...\n";
$_SERVER['REQUEST_URI'] = '/api/history?days=1';
$_GET['days'] = 1;
ob_start();
include 'index.php';
$output = ob_get_clean();
$data = json_decode($output, true);

if (isset($data['error'])) {
    echo "   ✗ Error: " . $data['error'] . "\n";
} else if (is_array($data) && count($data) > 0) {
    echo "   ✓ Success! Got " . count($data) . " historical records\n";
    echo "     First record:\n";
    $first = $data[0];
    echo "       - Timestamp: " . date('Y-m-d H:i:s', $first['timestamp'] ?? 0) . "\n";
    echo "       - Temperature: " . ($first['temperature'] ?? 'N/A') . "°C\n";
    echo "       - Humidity: " . ($first['humidity'] ?? 'N/A') . "%\n";
    echo "       - Wind Speed: " . ($first['wind_speed'] ?? 'N/A') . " m/s\n";
} else {
    echo "   ✗ No data returned\n";
}

echo "\n✓ Tests complete!\n";
?>
