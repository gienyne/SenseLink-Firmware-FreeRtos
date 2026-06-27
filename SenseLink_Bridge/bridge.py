import serial
import re
import json
import paho.mqtt.client as mqtt
import time

# config
SERIAL_PORT = 'COM9'
BAUD_RATE = 38400;
MQTT_BROKER = 'localhost'
MQTT_PORT = 1883;
MQTT_TOPIC = 'senselink/data'

print('== start of the bridge: UART -> MQTT ==')

# conexion to the MQTT broker
mqtt_client = mqtt.Client(callback_api_version=mqtt.CallbackAPIVersion.VERSION2)

try:
    mqtt_client.connect(MQTT_BROKER, MQTT_PORT, 60)
    mqtt_client.loop_start()
    print("successfully connected to the Mosquitto Broker")
except Exception as e:
    print(f"connexion error to the mqtt broker: {e}")
    exit(1)

# conexion to the Serial Port
try:
    ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
    print(f"Listening on {SERIAL_PORT}...")
except Exception as e:
    print(f"Unable to open port {SERIAL_PORT}: {e}")
    mqtt_client.loop_stop()
    exit(1)


data_pattern = re.compile(r"T:([\d\.]+)\s+H:([\d\.]+)\s+P:([\d\.]+)")

try:
    while True:
        if ser.in_waiting > 0:
            try:
                line = ser.readline().decode('utf-8', errors='ignore').strip()
                
                if line:
                    print(f"Brut reçu: {line}")
                    
                    # Data extraction
                    match = data_pattern.search(line)
                    if match:
                        temperature = float(match.group(1))
                        humidity = float(match.group(2))
                        pressure = float(match.group(3))
                        
                        # custom JSON payload
                        payload = {
                            "temperature": temperature,
                            "humidity": humidity,
                            "pressure": pressure,
                            "timestamp": int(time.time())
                        }
                        
                        # Sending data over the MQTT network
                        json_payload = json.dumps(payload)
                        mqtt_client.publish(MQTT_TOPIC, json_payload)
                        print(f"Published on MQTT [{MQTT_TOPIC}]: {json_payload}")
                        
            except Exception as e:
                print(f" Error whilst processing the line: {e}")
                
        time.sleep(0.1)

except KeyboardInterrupt:
    print("\nBridge closure...")

finally:
    ser.close()
    mqtt_client.loop_stop()
    mqtt_client.disconnect()
    print(" Connections closed properly. See you soon :)")
