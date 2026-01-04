<?php
/**
 * Test script for Weather API
 * 
 * Run from command line:
 * php test_api.php
 */

require_once 'config.php';

echo "Weather API Test\n";
echo "================\n\n";

// Test database connection
echo "Testing database connection...\n";

try {
    $dsn = sprintf(
        "pgsql:host=%s;port=%s;dbname=%s",
        DB_HOST,
        DB_PORT,
        DB_NAME
    );
    
    $pdo = new PDO($dsn, DB_USER, DB_PASSWORD, [
        PDO::ATTR_ERRMODE => PDO::ERRMODE_EXCEPTION
    ]);
    
    echo "✓ Database connection successful\n";
    
    // Test query
    echo "\nTesting query...\n";
    $stmt = $pdo->query("SELECT COUNT(*) as count FROM temp");
    $result = $stmt->fetch();
    echo "✓ Query successful\n";
    echo "  Temperature records: " . $result['count'] . "\n";
    
    // Test most recent record
    echo "\nMost recent records:\n";
    
    $stmt = $pdo->query("SELECT time, temperature FROM temp ORDER BY time DESC LIMIT 1");
    $row = $stmt->fetch();
    if ($row) {
        echo "  Temperature: " . $row['temperature'] . "°C at " . $row['time'] . "\n";
    }
    
    $stmt = $pdo->query("SELECT time, humidity FROM humidity ORDER BY time DESC LIMIT 1");
    $row = $stmt->fetch();
    if ($row) {
        echo "  Humidity: " . $row['humidity'] . "% at " . $row['time'] . "\n";
    }
    
    $stmt = $pdo->query("SELECT time, speed FROM wind ORDER BY time DESC LIMIT 1");
    $row = $stmt->fetch();
    if ($row) {
        echo "  Wind Speed: " . $row['speed'] . " m/s at " . $row['time'] . "\n";
    }
    
    echo "\n✓ All tests passed!\n";
    
} catch (PDOException $e) {
    echo "✗ Error: " . $e->getMessage() . "\n";
    exit(1);
}
?>
