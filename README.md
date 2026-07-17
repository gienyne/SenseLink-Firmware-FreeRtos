# SenseLink Firmware

> A FreeRTOS-based environmental monitoring system for STM32 featuring deterministic multitasking, custom embedded drivers and a complete IoT telemetry pipeline.

![Platform](https://img.shields.io/badge/Platform-STM32-blue)
![RTOS](https://img.shields.io/badge/RTOS-FreeRTOS-green)
![Language](https://img.shields.io/badge/Language-C-orange)
![Dashboard](https://img.shields.io/badge/Frontend-React-61DAFB)

---

## Overview

SenseLink is a real-time embedded application developed for the **STM32 Nucleo-F030R8** using **FreeRTOS**.

The project demonstrates how to build a deterministic multitasking firmware on a resource-constrained microcontroller (**48 MHz Cortex-M0, 8 KB SRAM**) while extending it with a complete IoT monitoring pipeline.

The firmware periodically reads environmental data from a Bosch BME280 sensor, distributes measurements through FreeRTOS queues, updates a local LCD, manages an alarm state machine and streams telemetry over UART.

A lightweight Python gateway forwards telemetry to an MQTT broker, allowing a React dashboard to visualize the system in real time and remotely acknowledge alarms.

---

## Features

- FreeRTOS multitasking architecture
- Thread-safe communication using queues and mutexes
- Custom HD44780 LCD driver over PCF8574 (I²C)
- Bosch BME280 environmental monitoring
- UART telemetry with runtime CPU statistics
- Python UART ↔ MQTT bridge
- React monitoring dashboard
- Remote alarm reset
- Optimized memory usage for an STM32 with only **8 KB SRAM**

---

## Hardware

| Component | Description |
|------------|-------------|
| MCU | STM32 Nucleo-F030R8 (Cortex-M0, 48 MHz, 8 KB SRAM) |
| Sensor | Bosch BME280 |
| Display | HD44780 LCD + PCF8574 I²C expander |
| LEDs | Green, Yellow and Red status indicators |
| Communication | USART2 @ 38400 baud |

---

## System Overview

The firmware is built around four independent FreeRTOS tasks communicating through typed queues while shared peripherals are protected using mutexes.

The complete embedded architecture, task design and communication flow are documented in:

- 📄 **docs/freertos.md**
- 📄 **docs/architecture.md**

### End-to-End Pipeline

```mermaid
graph TD
    classDef HW fill:#f4f4f4,stroke:#333,stroke-width:2px;
    classDef OS fill:#e3f2fd,stroke:#0d47a1,stroke-width:2px;
    classDef SW fill:#fff3e0,stroke:#e65100,stroke-width:2px;
    classDef NET fill:#ede7f6,stroke:#4a148c,stroke-width:2px;

    BME["BME280 Sensor"]:::HW
    TS["TaskSensor<br/>(FreeRTOS)"]:::OS
    
    Q_Alarm[("AlarmQueueHandle")]:::OS
    Q_Lcd[("LcdQueueHandle")]:::OS
    Q_Uart[("UartQueueHandle")]:::OS
    
    TA["TaskAlarm"]:::OS
    TL["TaskLCD"]:::OS
    TU["TaskUART"]:::OS
    
    LEDs["Physical LEDs<br/>(PA8, PA9, PB5)"]:::HW
    LCD["16x2 LCD Display"]:::HW
    UART2["USART2<br/>(38400 baud)"]:::HW

    BME -->|I2C Mutex Protected| TS
    TS --> Q_Alarm --> TA --> LEDs
    TS --> Q_Lcd --> TL --> LCD
    TS --> Q_Uart --> TU --> UART2

    PY["bridge.py (Python)<br/>pyserial + paho-mqtt"]:::SW
    Broker["Mosquitto Broker<br/>(localhost:1883/9001)"]:::NET

    UART2 <-->|USB / Virtual COM| PY
    PY -->|"Publish: senselink/data & senselink/cpu"| Broker

    Dashboard["React Dashboard (WebSockets)<br/>Gauges · Chart · LED panel · CPU"]:::SW
    Broker <-->|WebSockets| Dashboard

    %% --- PIPELINE DE RETOUR (RESET COMMAND) ---
    ISR["STM32 UART ISR<br/>HAL_UART_RxCpltCallback"]:::OS
    
    Dashboard -.->|"Click Reset Alarm Button (senselink/cmd)"| Broker
    Broker -.-> PY
    PY -.->|"ser.write(b'R')"| ISR
    ISR -.->|reset_request = 1| TA

---

## Memory Optimization

Running FreeRTOS on a microcontroller with only **8 KB of SRAM** required careful tuning of:

- task stack sizes
- queue depths
- heap allocation
- runtime memory usage

A complete analysis of the memory budget and optimisation strategy is available in:

📄 **docs/memory.md**

---

## Project Structure

```text
SenseLink_Firmware/
│
├── Core/
├── docs/
├── SenseLink_Bridge/
├── senselink-dashboard/
└── README.md
```

---

## Getting Started

### Requirements

- STM32CubeIDE
- Python 3
- Node.js
- Mosquitto MQTT Broker

### Flash the firmware

Compile and flash the firmware using STM32CubeIDE.

### Start the bridge

```bash
cd SenseLink_Bridge
python bridge.py
```

### Start the dashboard

```bash
cd senselink-dashboard
npm install
npm run dev
```

---

## Dashboard

### Nominal

![Nominal](Screenshots/S_Offline.png)

### Warning

![Warning](Screenshots/S_Übersicht1.png)

### Critical

![Critical](Screenshots/S_Übersicht2.png)

### UART Debug Output

![UART](Screenshots/Putty.png)

---

## Engineering Challenges

| Challenge | Solution |
|------------|----------|
| Shared I²C bus contention | FreeRTOS mutex |
| Recursive mutex deadlock | Driver-level locking |
| SRAM limitations | Stack tuning and centralized formatting |
| LCD update latency | Queue sizing optimisation |
| Runtime profiling | FreeRTOS runtime statistics |

---

## Author

**Dimitry Ntofeu Nyatcha**

Embedded Systems & IoT Engineer

📧 ntofeunyatchadimitry@gmail.com
