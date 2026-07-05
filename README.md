# SenseLink — Real-Time IoT Environmental Monitoring Station

> A FreeRTOS multitasking firmware project on STM32, extended with a full IoT
> pipeline: Python UART-to-MQTT bridge and a React real-time dashboard.

---

## Table of Contents

- [Motivation](#motivation)
- [What This Project Demonstrates](#what-this-project-demonstrates)
- [Hardware](#hardware)
- [FreeRTOS Architecture — The Core of the Project](#freertos-architecture--the-core-of-the-project)
  - [Task Design](#task-design)
  - [Inter-Task Communication with Queues](#inter-task-communication-with-queues)
  - [Shared Resource Protection with Mutexes](#shared-resource-protection-with-mutexes)
  - [Alarm State Machine](#alarm-state-machine)
  - [CPU Usage Statistics](#cpu-usage-statistics)
  - [Memory Budget](#memory-budget)
- [Full IoT Pipeline](#full-io-t-pipeline)
- [Project Structure](#project-structure)
- [Getting Started](#getting-started)
  - [1. Flash the Firmware](#1-flash-the-firmware)
  - [2. Configure and Start Mosquitto](#2-configure-and-start-mosquitto)
  - [3. Run the Python Bridge](#3-run-the-python-bridge)
  - [4. Launch the React Dashboard](#4-launch-the-react-dashboard)
- [UART Protocol](#uart-protocol)
- [MQTT Topics](#mqtt-topics)
- [Dashboard Screenshots](#dashboard-screenshots)
- [Key Engineering Challenges](#key-engineering-challenges)

---

## Motivation

This project was built to demonstrate practical competency in **real-time
embedded software engineering** on a resource-constrained microcontroller.
The primary goal was to architect a correct, stable FreeRTOS multitasking
application on a device with only **8 KB of RAM**, then extend it with a
full IoT stack to visualise and interact with the running system in real time.

Every design decision in the firmware — task stack sizes, queue depths,
mutex placement, ISR design — was driven by the constraints of FreeRTOS
running on the STM32F030R8.

---

## What This Project Demonstrates

### FreeRTOS Skills

- Designing a **concurrent multitasking architecture** with four independent
  tasks running on a Cortex-M0 at 48 MHz.
- **Inter-task communication** via typed FreeRTOS message queues with
  carefully chosen depths to balance latency and memory usage.
- **Mutual exclusion** with a FreeRTOS mutex protecting a shared I2C bus
  between two tasks.
- **ISR-to-task signalling** using `HAL_UART_RxCpltCallback` with direct
  flag-based notification to handle downlink commands without blocking.
- **Runtime CPU statistics** using `uxTaskGetSystemState()` and
  `xTaskGetTickCount()` to measure and report per-task CPU usage at runtime.
- **Heap budget management** on an 8 KB device: tuning
  `configTOTAL_HEAP_SIZE`, stack sizes and queue depths to keep the system
  stable with zero heap fragmentation.
- **Latching alarm state machine** implemented as a FreeRTOS task with
  deterministic state transitions and hardware LED output.

### Embedded Systems Skills

- I2C multi-device bus sharing (BME280 sensor + HD44780 LCD via PCF8574).
- HD44780 LCD driver in 4-bit nibble mode with proper initialisation timing.
- UART interrupt-driven reception with error recovery.
- STM32 HAL peripheral configuration (I2C, USART, GPIO, TIM).

### IoT and Software Skills

- Bidirectional UART-to-MQTT bridge in Python.
- React dashboard with real-time WebSocket MQTT subscription.
- Componentised React architecture with custom hooks and isolated CSS.

---

## Hardware

| Component | Details |
|---|---|
| Microcontroller | STM32 Nucleo-F030R8 (Cortex-M0, 48 MHz, **8 KB RAM**) |
| Sensor | Bosch BME280 — temperature, humidity, pressure via I2C |
| Display | 16x2 HD44780 LCD via PCF8574 I2C expander (0x27) |
| Green LED | PA9 — Nominal alarm state |
| Yellow LED | PA8 — Warning alarm state |
| Red LED | PB5 — Critical alarm state (blinking) |
| UART | USART2 at 38400 baud via ST-Link USB |

**Wiring note:** The BME280 and the PCF8574 LCD expander share I2C1 (PB6/PB7).
Bus contention between tasks is prevented by a FreeRTOS mutex.

---

## FreeRTOS Architecture — The Core of the Project

### Task Design

The firmware runs four concurrent FreeRTOS tasks. Each task has a single,
well-defined responsibility following the separation of concerns principle.

```
mermaid
graph TD
    %% Nodes
    TaskSensor["TaskSensor (256 words)<br/>• Reads BME280 via I2C<br/>• Centralized snprintf"]
    TaskAlarm["TaskAlarm (64 words)<br/>• State Machine<br/>• Drives LEDs"]
    TaskLCD["TaskLCD (128 words)<br/>• Writes LCD via I2C"]
    TaskUART["TaskUART (192 words)<br/>• Sends Telemetry<br/>• Reports CPU %"]

    %% Queues
    TaskSensor -->|AlarmQueue<br/>1 x struct| TaskAlarm
    TaskSensor -->|LcdQueue<br/>3 x str| TaskLCD
    TaskSensor -->|UartQueue<br/>2 x str| TaskUART

    %% Styling
    style TaskSensor fill:#f4f4f4,stroke:#333,stroke-width:2px
    style TaskAlarm fill:#e6f2ff,stroke:#333,stroke-width:1px
    style TaskLCD fill:#e6f2ff,stroke:#333,stroke-width:1px
    style TaskUART fill:#e6f2ff,stroke:#333,stroke-width:1px
```

| Task | Stack | Role |
|---|---|---|
| **TaskSensor** | 256 words | Reads BME280 every 2 s. Owns all `snprintf` calls. Dispatches to 3 queues. |
| **TaskLCD** | 128 words | Receives pre-formatted strings and writes them to the LCD display. |
| **TaskUART** | 192 words | Forwards telemetry to UART. Every 5 s, queries FreeRTOS and prints per-task CPU usage. |
| **TaskAlarm** | 64 words | Evaluates thresholds, manages the latching alarm state machine, drives LEDs. |
| **IDLE** | 128 words | FreeRTOS idle task. Consumes ~98% of CPU — the expected result of a correctly designed non-blocking RTOS application. |

### Inter-Task Communication with Queues

**No global variables are used to pass sensor data between tasks.**
All data flows through typed FreeRTOS queues, ensuring thread safety and
decoupling producer from consumers.

```c
/* In main.c — queues created before the scheduler starts */
UartQueueHandle  = xQueueCreate(2, sizeof(FormattedData_t));
AlarmQueueHandle = xQueueCreate(1, sizeof(SensorData_t));
LcdQueueHandle   = xQueueCreate(3, sizeof(FormattedData_t));
```

Two payload types are used:

- `SensorData_t` — raw `float` values sent to TaskAlarm for threshold comparison.
- `FormattedData_t` — pre-formatted strings sent to TaskLCD and TaskUART.
  By centralising all `snprintf` calls in TaskSensor, consumer tasks need
  no floating-point library support and their stack requirements are minimised.

**Queue depths are tuned by design:**

| Queue | Depth | Reasoning |
|---|---|---|
| AlarmQueueHandle | 1 | Only the latest reading matters for alarm evaluation. |
| UartQueueHandle | 2 | Absorbs one cycle of jitter between sensor and UART task. |
| LcdQueueHandle | 3 | Extra depth required to absorb I2C timing jitter caused by mutex contention with TaskSensor on the shared bus. |

### Shared Resource Protection with Mutexes

The BME280 sensor (TaskSensor) and the HD44780 LCD (TaskLCD) both use
**I2C1** (PB6/PB7). Without synchronisation, concurrent I2C transactions
from two tasks would corrupt both devices.

A single FreeRTOS mutex (`i2cMutexHandle`) protects all I2C bus access:

```c
/* In TaskSensor — acquiring the bus before reading the sensor */
osMutexWait(i2cMutexHandle, osWaitForever);
bme280_sensor_read(&sensorData);
osMutexRelease(i2cMutexHandle);

/* In lcd_i2c.c — acquiring the bus before each LCD command */
static void LCD_SendCommand(uint8_t cmd) {
    osMutexWait(i2cMutexHandle, osWaitForever);
    LCD_Send(cmd, 0);
    osMutexRelease(i2cMutexHandle);
}
```

The mutex is placed at the `LCD_SendCommand` / `LCD_SendData` level (not
inside `LCD_WritePointer`) to avoid re-entrant acquisition, which would
cause a deadlock on a non-recursive mutex.

Additionally, TaskSensor delays its first I2C access by 3 seconds to allow
the LCD to complete its HD44780 power-on reset sequence before the BME280
driver claims the bus.

### Alarm State Machine

TaskAlarm implements a **latching three-state alarm state machine**. Once
the alarm escalates, it does not automatically recover — an explicit Reset
command from the dashboard is required.

```
stateDiagram-v2
    %% Configuration des états principaux
    state "NOMINAL <br/> (Green LED On)" as NOMINAL
    state "WARNING <br/> (Yellow LED On)" as WARNING
    state "CRITICAL <br/> (Red LED Blinking - LATCHED)" as CRITICAL
    state "Real-Time Re-evaluation" as EVAL <<choice>>

    %% Transitions initiales et standards
    [*] --> NOMINAL
    
    NOMINAL --> WARNING : T > 30°C OR H > 55%
    WARNING --> NOMINAL : T <= 30°C AND H <= 55%
    
    NOMINAL --> CRITICAL : T > 30°C AND H > 55%
    WARNING --> CRITICAL : T > 30°C AND H > 55%

    %% Logique de RESET et choix conditionnel
    CRITICAL --> EVAL : RESET Command Received (UART 'R')
    
    EVAL --> NOMINAL : Both Thresholds Cleared\n(T <= 30°C AND H <= 55%)
    EVAL --> WARNING : Only One Threshold Exceeded\n(Fallback State)
    EVAL --> CRITICAL : Thresholds Still Exceeded\n(Reset Rejected / Re-trigger)
```

The `current_alarm_state` variable is written by TaskAlarm and read by
TaskSensor, which embeds it in the UART telemetry string (`A:1/2/3`).
The Python bridge parses this field and the dashboard mirrors the physical
LED state in real time.

### CPU Usage Statistics

Every 5 seconds, TaskUART queries FreeRTOS using `uxTaskGetSystemState()`
and transmits a CPU usage report over UART. The Python bridge parses this
report and publishes per-task JSON payloads to `senselink/cpu` for display
on the dashboard.

```
--- CPU USE ---
TaskUART     : <1%
TaskLCD      : 0%
IDLE         : 98%
TaskAlarm    : <1%
TaskSensor   : <1%
---------------
```

**Important design decision:** `xTaskGetTickCount()` is used as the time
base (1 ms resolution via SysTick), not a hardware timer. This avoids
TIM3 overflow issues that caused erroneous 100% readings in early iterations.
Tasks executing in under 1 ms per cycle are reported as `<1%` to distinguish
them from tasks that have never been scheduled.

The array size is determined at runtime using `uxTaskGetNumberOfTasks()` to
prevent buffer overflow if the task count changes:

```c
UBaseType_t uxNbTasks = uxTaskGetNumberOfTasks();
if (uxNbTasks > 10) uxNbTasks = 10;
UBaseType_t uxArraySize = uxTaskGetSystemState(
    pxTaskStatusArray, uxNbTasks, &TotalRunTime);
```

### Memory Budget

The STM32F030R8 has 8 KB of total SRAM. The FreeRTOS heap must coexist
with the HAL layer, global variables, the system stack and all task stacks.

```
configTOTAL_HEAP_SIZE = 3972 bytes

Task stacks (dynamic, from heap):
  TaskSensor  256 words = 1024 bytes
  TaskLCD     128 words =  512 bytes
  TaskUART    192 words =  768 bytes
  TaskAlarm    64 words =  256 bytes
  IDLE        128 words =  512 bytes
                        ----------
  Total stacks          = 3072 bytes

Remaining heap for queues, TCBs, mutex: ~900 bytes
```

Every byte was accounted for. Exceeding the heap caused `osThreadCreate()`
to return `NULL` silently — diagnosed by toggling LD2 on a null handle check.

---

## Full IoT Pipeline

```
BME280 Sensor
     │
     │ I2C (mutex protected)
     ▼
TaskSensor (FreeRTOS)
     │
     ├──► AlarmQueueHandle ──► TaskAlarm ──► Physical LEDs (PA8, PA9, PB5)
     │
     ├──► LcdQueueHandle   ──► TaskLCD   ──► 16x2 LCD Display
     │
     └──► UartQueueHandle  ──► TaskUART  ──► USART2 (38400 baud)
                                                  │
                                           USB / Virtual COM
                                                  │
                                          bridge.py (Python)
                                          pyserial + paho-mqtt
                                                  │
                                         ┌────────┴────────┐
                                         │                 │
                                  senselink/data     senselink/cpu
                                         │                 │
                                   Mosquitto Broker (localhost:1883/9001)
                                         │
                                  React Dashboard (WebSockets)
                                  Gauges · Chart · LED panel · CPU monitor
                                         │
                                  senselink/cmd  ◄── Reset Alarm button
                                         │
                                  bridge.py ──► ser.write(b'R') ──► STM32 ISR
                                         │
                                  HAL_UART_RxCpltCallback
                                  reset_request = 1
                                         │
                                  TaskAlarm clears latch ──► Green LED
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
│   │   ├── queues.h           # FreeRTOS queue handle declarations
│   │   ├── sensor_data.h      # Shared structs (SensorData_t, FormattedData_t)
│   │   ├── uart_task.h        # UART task declaration
│   │   └── FreeRTOSConfig.h   # Heap, tick rate, runtime stats config
│   └── Src/
│       ├── alarm_task.c       # Latching alarm state machine + LED control
│       ├── bme280_task.c      # Sensor acquisition + centralised formatting
│       ├── lcd_i2c.c          # HD44780 4-bit driver via PCF8574
│       ├── lcd_task.c         # LCD display consumer task
│       ├── queues.c           # Queue handle definitions
│       ├── uart_task.c        # UART telemetry + CPU stats reporter
│       └── main.c             # Peripheral init, queues, tasks, ISR callbacks
│
├── SenseLink_Bridge/
│   ├── bridge.py              # Bidirectional UART <-> MQTT bridge
│   └── .gitignore             # Excludes venv/
│
└── senselink-dashboard/
    ├── src/
    │   ├── App.jsx            # Root: MQTT client, state, component composition
    │   ├── App.css            # Global CSS variables and shell layout
    │   ├── constants/config.jsx # Thresholds, MQTT URLs, alarm config
    │   ├── hooks/useUptime.jsx  # Live uptime counter custom hook
    │   └── components/
    │       ├── Sidebar/        # Connection, alarm badge, event log, reset button
    │       ├── Gauges/         # SVG semi-circular gauges, pressure bar
    │       ├── Chart/          # Recharts area chart (last 20 readings)
    │       └── BottomRow/      # Hardware LEDs, system info, CPU task monitor
    └── package.json
```

---

## Getting Started

### Prerequisites

- STM32CubeIDE (firmware build and flash)
- Python 3.x with pip
- Node.js 18+ and npm
- Mosquitto MQTT broker

### 1. Flash the Firmware

Open `SenseLink_Firmware` in STM32CubeIDE, build and flash to the Nucleo
board. The green LED lights up after ~3 seconds (the FreeRTOS startup delay
that allows the LCD to initialise before the sensor task claims the I2C bus).

### 2. Configure and Start Mosquitto

Edit `C:\Program Files\mosquitto\mosquitto.conf` as Administrator:

```
listener 1883
allow_anonymous true

listener 9001
protocol websockets
```

Restart the service and verify:

```powershell
Restart-Service mosquitto
netstat -an | findstr "1883 9001"
```

Both ports should show `LISTENING`.

### 3. Run the Python Bridge

```bash
# Activate the virtual environment (Git Bash)
cd SenseLink_Bridge
source venv/Scripts/activate

# Close PuTTY first — it locks COM9
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

### 4. Launch the React Dashboard

```bash
cd senselink-dashboard
npm install        # first time only
npm run dev
```

Open `http://localhost:5173`. The sidebar shows **Connected** once MQTT
data arrives.

---

## UART Protocol

### Telemetry line (every 2 seconds, produced by TaskSensor via TaskUART)

```
T:27.9 H:57.9 P:1001.3 A:1\r\n
```

| Field | Description |
|---|---|
| T | Temperature (°C) |
| H | Relative humidity (%) |
| P | Pressure (hPa) |
| A | Alarm state: 1=Nominal, 2=Warning, 3=Critical |

### CPU statistics block (every 5 seconds, produced by TaskUART)

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

The bridge writes `b'R'` when the dashboard publishes `RESET`. The STM32
`HAL_UART_RxCpltCallback` sets `reset_request = 1` and `current_alarm_state = 1`.
TaskAlarm clears the latch on the next sensor reading if values are within range.

---

## MQTT Topics

| Topic | Direction | Payload |
|---|---|---|
| `senselink/data` | Bridge -> Dashboard | `{"temperature": 29.7, "humidity": 54.2, "pressure": 1001.3, "alarmState": 1, "timestamp": ...}` |
| `senselink/cpu` | Bridge -> Dashboard | `{"taskName": "IDLE", "cpuUsage": 98, "timestamp": ...}` |
| `senselink/cmd` | Dashboard -> Bridge | `"RESET"` |

---

## Dashboard Screenshots

<!-- Add your screenshots here -->

### Live telemetry — Nominal state
![Dashboard Nominal](docs/screenshots/dashboard_nominal.png)

### Live telemetry — Warning state
![Dashboard Warning](docs/screenshots/dashboard_warning.png)

### Python bridge terminal output
![Bridge terminal](docs/screenshots/bridge_terminal.png)

### PuTTY UART debug output
![PuTTY UART](docs/screenshots/putty_uart.png)

---

## Key Engineering Challenges

| Challenge | Solution |
|---|---|
| I2C bus conflict between LCD and BME280 | FreeRTOS mutex + 3 s startup delay in TaskSensor |
| Mutex deadlock (re-entrant acquisition) | Moved mutex to `LCD_SendCommand` level only |
| Heap exhaustion on 8 KB device | Tuned stack sizes, centralised snprintf, heap set to 3972 bytes |
| LCD display corruption | Increased LCD queue depth from 2 to 3 |
| CPU stats buffer overflow after removing a task | Runtime `uxTaskGetNumberOfTasks()` guard |
| CPU stats showing 100% on all tasks | Replaced TIM3 (overflowed every 65 ms) with `xTaskGetTickCount()` |
| UART downlink command lost in queue noise | ISR filters for `'R'` only before acting |
