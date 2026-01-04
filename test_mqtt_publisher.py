# Example MQTT Test Publisher
# 
# This is an example Python script to test the weather station
# by publishing sample weather data to the MQTT broker

import paho.mqtt.client as mqtt
import json
import time
import random

# Configuration
MQTT_BROKER = "broker.example.com"  # Replace with your broker
MQTT_PORT = 1883
MQTT_TOPIC = "weather/realtime"
MQTT_USERNAME = ""  # Optional
MQTT_PASSWORD = ""  # Optional

def generate_sample_data():
    """Generate random sample weather data"""
    return {
        "temperature": round(random.uniform(15.0, 30.0), 1),
        "humidity": round(random.uniform(40.0, 80.0), 1),
        "wind_speed": round(random.uniform(0.0, 15.0), 1),
        "air_pressure": round(random.uniform(980.0, 1030.0), 1),
        "rainfall": round(random.uniform(0.0, 10.0), 1),
        "wind_direction": random.randint(0, 359)
    }

def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("Connected to MQTT broker successfully")
    else:
        print(f"Failed to connect, return code {rc}")

def main():
    client = mqtt.Client()
    
    # Set username/password if configured
    if MQTT_USERNAME and MQTT_PASSWORD:
        client.username_pw_set(MQTT_USERNAME, MQTT_PASSWORD)
    
    client.on_connect = on_connect
    
    print(f"Connecting to MQTT broker: {MQTT_BROKER}:{MQTT_PORT}")
    client.connect(MQTT_BROKER, MQTT_PORT, 60)
    client.loop_start()
    
    try:
        while True:
            # Generate and publish sample data
            weather_data = generate_sample_data()
            json_data = json.dumps(weather_data)
            
            print(f"Publishing: {json_data}")
            client.publish(MQTT_TOPIC, json_data)
            
            # Wait 30 seconds before next update
            time.sleep(30)
            
    except KeyboardInterrupt:
        print("\nStopping publisher...")
    finally:
        client.loop_stop()
        client.disconnect()

if __name__ == "__main__":
    main()
