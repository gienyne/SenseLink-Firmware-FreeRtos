# Setup Guide

This guide explains how to build the firmware and run the complete SenseLink IoT system.

---

# Requirements

## Hardware

* STM32 Nucleo-F030R8
* Bosch BME280 sensor
* 16×2 HD44780 LCD with PCF8574 I2C backpack
* USB cable

---

## Software

* STM32CubeIDE
* Python 3.x
* Node.js 18+
* Mosquitto MQTT Broker

---

# 1. Flash the Firmware

Open the firmware project in STM32CubeIDE.

Build the project and flash it to the STM32 board.

After boot, the firmware waits approximately three seconds before accessing the I2C bus, allowing the LCD controller to complete its power-on initialization.

---

# 2. Start Mosquitto

Enable both the MQTT and WebSocket listeners.

Example configuration:

```text
listener 1883
allow_anonymous true

listener 9001
protocol websockets
```

Restart the broker after modifying the configuration.

---

# 3. Start the Python Bridge

Open a terminal:

```bash
cd SenseLink_Bridge
python bridge.py
```

The bridge connects to the STM32 through the serial port and forwards telemetry to MQTT.

---

# 4. Start the Dashboard

Open another terminal:

```bash
cd senselink-dashboard
npm install
npm run dev
```

The dashboard automatically connects to the MQTT broker using WebSockets.

---

# Expected Behaviour

Once every component is running:

* Environmental measurements appear on the dashboard.
* LCD values update every acquisition cycle.
* CPU usage statistics are refreshed periodically.
* Alarm LEDs reflect the current alarm state.
* Pressing **Reset Alarm** sends a command back to the STM32 firmware.

---

# Troubleshooting

## No sensor values

Verify:

* BME280 wiring
* I2C address
* Sensor power supply

---

## LCD remains blank

Verify:

* PCF8574 address
* LCD contrast adjustment
* I2C connections

---

## Dashboard does not receive data

Verify:

* Mosquitto is running.
* The Python bridge is connected to the correct COM port.
* MQTT topics are correctly configured.

---

## Reset button has no effect

Verify:

* The Python bridge forwards the `R` command.
* UART communication is operational.
* `HAL_UART_RxCpltCallback()` is receiving incoming bytes.
