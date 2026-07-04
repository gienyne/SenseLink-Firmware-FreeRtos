"""
bridge.py

SenseLink UART <-> MQTT Bidirectional Bridge

This script forms the central communication layer between the STM32 firmware
and the React dashboard. It runs on a host PC connected to the STM32 Nucleo
board via USB (virtual COM port).

Responsibilities:
  Upstream   (STM32 -> dashboard): Reads UART lines from the firmware, parses
             sensor telemetry and FreeRTOS CPU statistics, and publishes them
             as JSON payloads to the local Mosquitto MQTT broker.

  Downstream (dashboard -> STM32): Subscribes to the command topic. When a
             RESET command is received from the React dashboard, transmits the
             single byte 'R' over UART to trigger the hardware alarm reset on
             the STM32.

MQTT topics:
  senselink/data  (publish)   Sensor telemetry JSON
  senselink/cpu   (publish)   Per-task CPU usage JSON
  senselink/cmd   (subscribe) Downlink command strings (e.g. "RESET")

UART format expected from firmware:
  Telemetry : "T:25.9 H:54.3 P:1001.2 A:1\\r\\n"
  CPU block : "--- CPU USE ---\\r\\n"
              "TaskSensor   : <1%\\r\\n"
              ...
              "---------------\\r\\n"

Dependencies: pyserial, paho-mqtt
"""

import serial
import re
import json
import paho.mqtt.client as mqtt
import time

# ---------------------------------------------------------------------------
# Configuration
# ---------------------------------------------------------------------------

SERIAL_PORT     = 'COM9'           # Virtual COM port assigned to the STM32
BAUD_RATE       = 38400            # Must match MX_USART2_UART_Init in firmware
MQTT_BROKER     = 'localhost'      # Mosquitto broker host
MQTT_PORT       = 1883             # Standard MQTT port
MQTT_TOPIC_DATA = 'senselink/data' # Sensor telemetry upstream topic
MQTT_TOPIC_CPU  = 'senselink/cpu'  # FreeRTOS CPU stats upstream topic
MQTT_TOPIC_CMD  = 'senselink/cmd'  # Downlink command topic

print('== SenseLink Bridge UART <-> MQTT ==')

# ---------------------------------------------------------------------------
# Serial port initialisation
# ---------------------------------------------------------------------------

# Open the serial port before connecting to MQTT so that any port error
# causes an early exit before the broker connection is established.
try:
    ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
    print(f"Listening on {SERIAL_PORT}...")
except Exception as e:
    print(f"Unable to open port {SERIAL_PORT}: {e}")
    exit(1)

# ---------------------------------------------------------------------------
# MQTT downlink callback
# ---------------------------------------------------------------------------

def on_message(client, userdata, msg):
    """
    Called by the paho-mqtt background thread when a message arrives on any
    subscribed topic (currently only senselink/cmd).

    On reception of "RESET", writes the single byte b'R' to the UART.
    The STM32 HAL_UART_RxCpltCallback then detects this byte and sets the
    reset_request flag, which the alarm task uses to clear the alarm latch
    and return the system to the Nominal state.

    All other command strings are silently ignored to allow future extension
    without modifying this handler's signature.
    """
    try:
        command = msg.payload.decode('utf-8').strip()
        if command == "RESET":
            ser.write(b'R')
            print("Transmitted 'R' (RESET) to STM32")
    except Exception as e:
        print(f"Error handling MQTT message: {e}")

# ---------------------------------------------------------------------------
# MQTT client initialisation
# ---------------------------------------------------------------------------

# Use the paho-mqtt v2 callback API to avoid deprecation warnings.
mqtt_client = mqtt.Client(callback_api_version=mqtt.CallbackAPIVersion.VERSION2)
mqtt_client.on_message = on_message

try:
    mqtt_client.connect(MQTT_BROKER, MQTT_PORT, keepalive=60)
    # Subscribe to the command topic to receive downlink instructions
    # from the React dashboard.
    mqtt_client.subscribe(MQTT_TOPIC_CMD)
    # loop_start() runs the MQTT network loop in a background thread,
    # allowing the main loop to focus solely on UART reading.
    mqtt_client.loop_start()
    print("Connected to Mosquitto Broker")
except Exception as e:
    print(f"MQTT connection error: {e}")
    ser.close()
    exit(1)

# ---------------------------------------------------------------------------
# Line parsers (compiled once for efficiency)
# ---------------------------------------------------------------------------

# Matches a sensor telemetry line emitted by StartTaskSensor.
# Groups: (1) temperature, (2) humidity, (3) pressure, (4) alarm state (optional)
# Example: "T:27.3 H:57.5 P:1001.3 A:1"
data_pattern = re.compile(
    r"T:([\d\.]+)\s+H:([\d\.]+)\s+P:([\d\.]+)(?:\s+A:(\d+))?"
)

# Matches a CPU usage line emitted by StartTaskUART.
# Handles both numeric percentages ("98%") and the sub-1% marker ("<1%")
# produced when a task has run but consumed less than 1% of CPU time.
# Groups: (1) task name, (2) percentage value or "<1"
# Example: "TaskSensor   : <1%"
cpu_pattern = re.compile(r"^(\w+)\s*:\s*(<1|\d+)%")

# Flag indicating whether the parser is currently inside a CPU report block.
in_cpu_block = False

# ---------------------------------------------------------------------------
# Main read loop
# ---------------------------------------------------------------------------

try:
    while True:
        if ser.in_waiting > 0:
            try:
                line = ser.readline().decode('utf-8', errors='ignore').strip()

                # Skip empty lines produced by UART noise or blank CRLF pairs.
                if not line:
                    continue

                # Detect the start of a FreeRTOS CPU report block.
                if '--- CPU USE ---' in line:
                    in_cpu_block = True
                    continue

                # Detect the end-of-block delimiter and exit CPU parsing mode.
                if '---------------' in line:
                    in_cpu_block = False
                    continue

                # --- CPU statistics parsing ---
                if in_cpu_block:
                    cpu_match = cpu_pattern.match(line)
                    if cpu_match:
                        task_name = cpu_match.group(1)
                        raw_val   = cpu_match.group(2)

                        # Map the "<1%" marker to 0.5 so the dashboard renders
                        # a small but visible progress bar rather than nothing.
                        cpu_val = 0.5 if raw_val == '<1' else int(raw_val)

                        payload = json.dumps({
                            "taskName":  task_name,
                            "cpuUsage":  cpu_val,
                            "timestamp": int(time.time())
                        })
                        mqtt_client.publish(MQTT_TOPIC_CPU, payload)
                        print(f"CPU [{task_name}]: {cpu_val}%")
                    # Consume the line regardless of whether the regex matched
                    # to prevent CPU lines from being tested against data_pattern.
                    continue

                # --- Sensor telemetry parsing ---
                match = data_pattern.search(line)
                if match:
                    temperature = float(match.group(1))
                    humidity    = float(match.group(2))
                    pressure    = float(match.group(3))
                    # Alarm state is included in the telemetry string by
                    # StartTaskSensor. Default to 1 (Nominal) if absent.
                    alarm_state = int(match.group(4)) if match.group(4) else 1

                    payload = json.dumps({
                        "temperature": temperature,
                        "humidity":    humidity,
                        "pressure":    pressure,
                        "alarmState":  alarm_state,
                        "timestamp":   int(time.time())
                    })
                    mqtt_client.publish(MQTT_TOPIC_DATA, payload)
                    print(f"Data -> {payload}")

            except Exception as e:
                print(f"Error processing line: {e}")

except KeyboardInterrupt:
    print("\nBridge closure...")

finally:
    # Ensure both connections are cleanly closed regardless of how the loop exits.
    ser.close()
    mqtt_client.loop_stop()
    mqtt_client.disconnect()
    print("Connections closed properly. See you Soon :) ")