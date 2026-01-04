# Example HTTP Server for Historical Weather Data
#
# This is a simple Flask server that provides sample historical weather data
# The weather station will fetch this data via HTTP

from flask import Flask, jsonify
import time
import random

app = Flask(__name__)

def generate_historical_data():
    """Generate 7 days of hourly weather data (168 points)"""
    current_time = int(time.time())
    data = []
    
    # Generate data going back 7 days
    for i in range(168):  # 7 days * 24 hours
        timestamp = current_time - (i * 3600)  # Go back 1 hour per point
        
        # Simulate temperature variation (daily cycle)
        base_temp = 20.0
        temp_variation = 5.0 * random.uniform(0.8, 1.2)
        temperature = base_temp + temp_variation * (1 + 0.3 * (i % 24 - 12) / 12)
        
        # Simulate humidity
        humidity = random.uniform(45.0, 75.0)
        
        # Simulate wind speed
        wind_speed = random.uniform(1.0, 10.0)
        
        # Simulate rainfall
        rainfall = random.uniform(0.0, 5.0)
        
        data.append({
            "timestamp": timestamp,
            "temperature": round(temperature, 1),
            "humidity": round(humidity, 1),
            "wind_speed": round(wind_speed, 1),
            "rainfall": round(rainfall, 1)
        })
    
    # Reverse to have oldest first
    data.reverse()
    return data

@app.route('/api/history', methods=['GET'])
def get_history():
    """Return historical weather data"""
    data = generate_historical_data()
    return jsonify(data)

@app.route('/api/current', methods=['GET'])
def get_current():
    """Return current weather data (alternative endpoint)"""
    current_data = {
        "temperature": round(random.uniform(15.0, 30.0), 1),
        "humidity": round(random.uniform(40.0, 80.0), 1),
        "wind_speed": round(random.uniform(0.0, 15.0), 1),
        "air_pressure": round(random.uniform(980.0, 1030.0), 1),
        "rainfall": round(random.uniform(0.0, 10.0), 1),
        "wind_direction": random.randint(0, 359),
        "timestamp": int(time.time())
    }
    return jsonify(current_data)

@app.route('/')
def index():
    return """
    <h1>Weather Station HTTP Server</h1>
    <p>Available endpoints:</p>
    <ul>
        <li><a href="/api/history">/api/history</a> - Get 7 days of historical data</li>
        <li><a href="/api/current">/api/current</a> - Get current weather data</li>
    </ul>
    """

if __name__ == '__main__':
    print("Starting Weather Station HTTP Server...")
    print("Historical data endpoint: http://localhost:5000/api/history")
    print("Current data endpoint: http://localhost:5000/api/current")
    app.run(host='0.0.0.0', port=5000, debug=True)
