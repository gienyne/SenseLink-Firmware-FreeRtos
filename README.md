# SenseLink - Real-Time IoT Environmental Monitoring Station

A full-stack embedded IoT project built on an **STM32 Nucleo-F030R8** running **FreeRTOS**, with a Python UART-to-MQTT bridge and a React real-time dashboard.

---

## Table of Contents

- [Overview](#overview)
- [Hardware](#hardware)
- [System Architecture](#system-architecture)
- [Project Structure](#project-structure)
- [FreeRTOS Task Architecture](#freertos-task-architecture)
- [Getting Started](#getting-started)
  - [1. Flash the Firmware](#1-flash-the-firmware)
  - [2. Configure and Start Mosquitto](#2-configure-and-start-mosquitto)
  - [3. Run the Python Bridge](#3-run-the-python-bridge)
  - [4. Launch the React Dashboard](#4-launch-the-react-dashboard)
- [UART Protocol](#uart-protocol)
- [MQTT Topics](#mqtt-topics)
- [Dashboard Screenshots](#dashboard-screenshots)
- [Known Limitations](#known-limitations)

---

## Overview

SenseLink is a real-time environmental monitoring station that reads temperature, humidity and atmospheric pressure from a BME280 sensor and delivers the data across a full IoT stack:

```
STM32 (FreeRTOS) --> UART --> Python Bridge --> MQTT --> React Dashboard
                                          <-- MQTT <-- Reset Alarm button
```

The alarm system uses a **latching three-state state machine** (Nominal / Warning / Critical) with physical RGB LEDs on the Nucleo board and virtual LED indicators on the dashboard. A hardware reset command can be sent from the dashboard to clear the alarm.

---

## Hardware

| Component | Details |
|---|---|
| Microcontroller | STM32 Nucleo-F030R8 (Cortex-M0, 48 MHz, 8 KB RAM) |
| Sensor | Bosch BME280 (temperature, humidity, pressure) via I2C |
| Display | 16x2 HD44780 LCD via PCF8574 I2C expander |
| Green LED | PA9 — Nominal alarm state |
| Yellow LED | PA8 — Warning alarm state |
| Red LED | PB5 — Critical alarm state (blinking) |
| UART | USART2 at 38400 baud via USB-UART (ST-Link) |

### Wiring

| Signal | STM32 Pin |
|---|---|
| I2C SCL (BME280 + LCD shared) | PB6 |
| I2C SDA (BME280 + LCD shared) | PB7 |
| LCD VCC | 5V |
| BME280 VCC | 3.3V |
| BME280 CS | 3.3V (forces I2C mode) |

---

## System Architecture

```
┌─────────────────────────────────────┐
│         STM32 Nucleo-F030R8         │
│                                     │
│  ┌──────────┐    ┌───────────────┐  │
│  │ BME280   │    │  HD44780 LCD  │  │
│  │ (I2C)    │    │  (I2C/PCF)   │  │
│  └────┬─────┘    └───────┬───────┘  │
│       │    I2C Mutex     │          │
│  ┌────▼─────────────────▼───────┐   │
│  │        FreeRTOS Kernel       │   │
│  │  TaskSensor  TaskLCD         │   │
│  │  TaskUART    TaskAlarm       │   │
│  └──────────────┬───────────────┘   │
│                 │ USART2 38400 baud  │
└─────────────────┼───────────────────┘
                  │ USB
          ┌───────▼────────┐
          │  bridge.py     │
          │  pyserial      │
          │  paho-mqtt     │
          └───────┬────────┘
                  │ MQTT (localhost:1883)
          ┌───────▼────────┐
          │   Mosquitto    │
          │   Broker       │
          │   port 1883    │
          │   port 9001 WS │
          └───────┬────────┘
                  │ WebSockets (localhost:9001)
          ┌───────▼────────┐
          │ React Dashboard│
          │  Vite + mqtt.js│
          │  + Recharts    │
          └────────────────┘
```

---

## Project Structure

```
SenseLink_Firmware/
├── Core/
│   ├── Inc/
│   │   ├── alarm_task.h       # LED pin definitions, alarm task declaration
│   │   ├── bme280_task.h      # Sensor task declaration
│   │   ├── lcd_i2c.h          # LCD driver API
│   │   ├── lcd_task.h         # LCD task declaration
│   │   ├── queues.h           # Inter-task queue handle declarations
│   │   ├── sensor_data.h      # Shared data structures and alarm thresholds
│   │   ├── uart_task.h        # UART task declaration
│   │   ├── FreeRTOSConfig.h   # FreeRTOS configuration
│   │   └── main.h
│   └── Src/
│       ├── alarm_task.c       # Latching alarm state machine + LED control
│       ├── bme280_task.c      # Sensor acquisition + data formatting
│       ├── lcd_i2c.c          # HD44780 driver via PCF8574
│       ├── lcd_task.c         # LCD display consumer task
│       ├── queues.c           # Queue handle definitions
│       ├── uart_task.c        # UART telemetry + CPU stats reporter
│       └── main.c             # Peripheral init, RTOS setup, ISR callbacks
│
├── SenseLink_Bridge/
│   ├── bridge.py              # Bidirectional UART <-> MQTT bridge
│   ├── .gitignore             # Excludes venv/
│   └── venv/                  # Python virtual environment (not committed)
│
└── senselink-dashboard/
    ├── src/
    │   ├── App.jsx            # Root component, MQTT connection, state management
    │   ├── App.css            # Global styles and CSS variables
    │   ├── constants/
    │   │   └── config.js      # Thresholds, MQTT URLs, alarm config
    │   ├── hooks/
    │   │   └── useUptime.js   # Uptime counter custom hook
    │   └── components/
    │       ├── Sidebar/       # Connection status, alarm chip, event log
    │       ├── Gauges/        # Semi-circular SVG gauges + pressure card
    │       ├── Chart/         # Recharts area chart (temp + humidity)
    │       └── BottomRow/     # Hardware LEDs, system info, CPU monitor
    └── package.json
```

---

## FreeRTOS Task Architecture

| Task | Stack (words) | Priority | Role |
|---|---|---|---|
| TaskSensor | 256 | Normal | BME280 acquisition, snprintf formatting, queue dispatch |
| TaskLCD | 128 | Normal | LCD consumer, display formatted strings |
| TaskUART | 192 | Normal | UART telemetry + CPU statistics reporter |
| TaskAlarm | 64 | Normal | Latching alarm state machine, LED control |
| IDLE | 128 | Idle | FreeRTOS idle task (~98% CPU) |

**Heap configuration:** `configTOTAL_HEAP_SIZE = 3972 bytes`

**Queue depths:**

| Queue | Type | Depth |
|---|---|---|
| UartQueueHandle | FormattedData_t | 2 |
| AlarmQueueHandle | SensorData_t | 1 |
| LcdQueueHandle | FormattedData_t | 3 |

The LCD queue depth is 3 (not 2) to absorb I2C timing jitter between the sensor cycle and the display refresh.

---

## Getting Started

### Prerequisites

- STM32CubeIDE (firmware build and flash)
- Python 3.x with pip
- Node.js 18+ and npm
- Mosquitto MQTT broker (Windows: mosquitto.org/download)

---

### 1. Flash the Firmware

Open `SenseLink_Firmware` in STM32CubeIDE, build the project and flash it to the Nucleo board. The green LED should light up after a few seconds indicating the system is running normally.

---

### 2. Configure and Start Mosquitto

Open `C:\Program Files\mosquitto\mosquitto.conf` as Administrator and ensure the following lines are present at the end of the file:

```
listener 1883
allow_anonymous true

listener 9001
protocol websockets
```

Restart the Mosquitto service:

```powershell
Restart-Service mosquitto
```

Verify both ports are listening:

```powershell
netstat -an | findstr "1883 9001"
```

Expected output:

```
TCP    0.0.0.0:1883    0.0.0.0:0    LISTENING
TCP    0.0.0.0:9001    0.0.0.0:0    LISTENING
```

---

### 3. Run the Python Bridge

Open a terminal in `SenseLink_Bridge/` and activate the virtual environment:

```bash
# Git Bash / WSL
source venv/Scripts/activate

# PowerShell
.\venv\Scripts\Activate.ps1
```

Close any PuTTY session that may be holding COM9, then start the bridge:

```bash
python bridge.py
```

Expected output:

```
== SenseLink Bridge UART <-> MQTT ==
Listening on COM9...
Connected to Mosquitto Broker
Data -> {"temperature": 27.9, "humidity": 57.9, "pressure": 1001.3, "alarmState": 1, ...}
CPU [IDLE]: 98%
CPU [TaskSensor]: 0.5%
```

---

### 4. Launch the React Dashboard

```bash
cd senselink-dashboard
npm install        # first time only
npm run dev
```

Open your browser at `http://localhost:5173`.

The dashboard will display **Connected** in the sidebar once MQTT data starts flowing. The gauges, chart and CPU monitor update in real time.

---

## UART Protocol

### Telemetry line (every 2 seconds)

```
T:27.9 H:57.9 P:1001.3 A:1\r\n
```

| Field | Description |
|---|---|
| T | Temperature in degrees Celsius |
| H | Relative humidity in percent |
| P | Atmospheric pressure in hPa |
| A | Alarm state: 1=Nominal, 2=Warning, 3=Critical |

### CPU statistics block (every 5 seconds)

```
--- CPU USE ---
TaskSensor   : <1%
TaskUART     : <1%
TaskLCD      : 0%
TaskAlarm    : <1%
IDLE         : 98%
---------------
```

### Downlink command

The Python bridge forwards a single byte `R` over UART when the dashboard publishes `RESET` on `senselink/cmd`. The STM32 ISR detects this byte and clears the alarm latch.

---

## MQTT Topics

| Topic | Direction | Payload | Description |
|---|---|---|---|
| `senselink/data` | Bridge -> Dashboard | JSON | Sensor telemetry + alarm state |
| `senselink/cpu` | Bridge -> Dashboard | JSON | Per-task CPU usage |
| `senselink/cmd` | Dashboard -> Bridge | String | Downlink commands (`RESET`) |

Example `senselink/data` payload:

```json
{
  "temperature": 27.9,
  "humidity": 57.9,
  "pressure": 1001.3,
  "alarmState": 1,
  "timestamp": 1782950000
}
```

Example `senselink/cpu` payload:

```json
{
  "taskName": "IDLE",
  "cpuUsage": 98,
  "timestamp": 1782950000
}
```

---

## Dashboard Screenshots

### Live telemetry — Nominal state
![Dashboard Nominal](docs/screenshots/dashboard_nominal.png)

### Live telemetry — Warning state
![Dashboard Warning](https://github.com/gienyne/SenseLink-Firmware-FreeRtos/blob/main/Screenshots/S_%C3%9Cbersicht1.png)

### Live telemetry — Critical state
![Dashboard Critical](https://github.com/gienyne/SenseLink-Firmware-FreeRtos/blob/main/Screenshots/S_%C3%9Cbersicht2.png)

### Python bridge terminal output
![Bridge terminal](https://github.com/gienyne/SenseLink-Firmware-FreeRtos/blob/main/Screenshots/T_Python%C3%9Cbersicht.png)

### PuTTY UART debug output
![PuTTY UART](https://github.com/gienyne/SenseLink-Firmware-FreeRtos/blob/main/Screenshots/Putty.png)

### Demo

https://github.com/user-attachments/assets/c6a56bb9-8c16-4eff-bb1e-8c196a48b11d


---

## Design Considerations & System Behavior

- The IDLE task consumes ~98% of CPU time, which is the expected and normaly correct behaviour for a well-designed RTOS application where tasks spend most of their time blocked.
- CPU statistics use `xTaskGetTickCount()` as the time base (1 ms resolution). Tasks that execute in under 1 ms per cycle will report 0% or `<1%`.
- The alarm reset only takes effect when sensor readings have returned to within thresholds. If the environment is still above the threshold when RESET is pressed, the alarm will re-trigger on the next measurement cycle.
- The LCD queue depth must remain at 3 or higher to prevent display corruption caused by timing jitter on the shared I2C bus.
