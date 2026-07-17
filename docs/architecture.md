# System Architecture

This document describes the complete SenseLink system architecture, from sensor acquisition on the STM32 to real-time visualisation in the web dashboard.

---

# Architecture Overview

SenseLink is composed of four independent layers:

1. Embedded Firmware (STM32 + FreeRTOS)
2. UART Communication
3. MQTT Gateway
4. React Dashboard

Each layer has a well-defined responsibility and communicates only through clearly defined interfaces.

---

# Embedded Layer

The embedded firmware runs on an STM32F030R8 microcontroller using FreeRTOS.

The firmware is responsible for:

* Reading environmental data from the BME280 sensor.
* Evaluating alarm conditions.
* Updating the LCD display.
* Streaming telemetry over UART.
* Receiving remote commands.

Communication between tasks is entirely handled through FreeRTOS queues.

---

# UART Layer

`TaskUART` periodically transmits telemetry over USART2.

Typical messages include:

* Temperature
* Humidity
* Pressure
* Alarm state
* CPU statistics

Incoming UART data is handled through interrupts using `HAL_UART_RxCpltCallback()`.

The only command currently implemented is:

```text
R
```

which resets the latched alarm state.

---

# Python Bridge

The Python bridge acts as a gateway between the embedded firmware and the MQTT broker.

Its responsibilities are:

* Read UART telemetry.
* Parse incoming messages.
* Publish JSON payloads to MQTT.
* Receive MQTT commands.
* Forward commands back to the STM32 through UART.

This component completely decouples the firmware from the networking stack.

---

# MQTT Layer

Mosquitto is used as the MQTT broker.

Example topics:

| Topic            | Description                 |
| ---------------- | --------------------------- |
| `senselink/data` | Environmental telemetry     |
| `senselink/cpu`  | FreeRTOS runtime statistics |
| `senselink/cmd`  | Dashboard commands          |

The dashboard communicates with the broker through WebSockets.

---

# React Dashboard

The dashboard subscribes to MQTT topics and displays:

* Live sensor values
* Alarm status
* Hardware LEDs
* Runtime CPU usage
* Historical measurements

It also allows the user to remotely reset the alarm.

---

# End-to-End System Architecture

The following diagram illustrates the complete SenseLink architecture,
from sensor acquisition on the STM32 to real-time visualization on the
dashboard, including the return path used to remotely reset the alarm.

```mermaid
graph TD
    %% Configuration des styles pour différencier les couches du projet
    classDef HW fill:#f4f4f4,stroke:#333,stroke-width:2px;
    classDef OS fill:#e3f2fd,stroke:#0d47a1,stroke-width:2px;
    classDef SW fill:#fff3e0,stroke:#e65100,stroke-width:2px;
    classDef NET fill:#ede7f6,stroke:#4a148c,stroke-width:2px;

    %% --- COUCHE EMBARQUÉE (STM32 & FreeRTOS) ---
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

    %% Liens Couche Embarquée
    BME -->|I2C Mutex Protected| TS
    TS --> Q_Alarm --> TA --> LEDs
    TS --> Q_Lcd --> TL --> LCD
    TS --> Q_Uart --> TU --> UART2

    %% --- COUCHE PASSERELLE & CRYPTE MQTT ---
    PY["bridge.py (Python)<br/>pyserial + paho-mqtt"]:::SW
    Broker["Mosquitto Broker<br/>(localhost:1883/9001)"]:::NET

    UART2 <-->|USB / Virtual COM| PY
    PY -->|"Publish: senselink/data & senselink/cpu"| Broker

    %% --- COUCHE IHM (React Dashboard) ---
    Dashboard["React Dashboard (WebSockets)<br/>Gauges · Chart · LED panel · CPU"]:::SW
    Broker <-->|WebSockets| Dashboard

    %% --- PIPELINE DE RETOUR (RESET COMMAND) ---
    ISR["STM32 UART ISR<br/>HAL_UART_RxCpltCallback"]:::OS
    
    Dashboard -.->|"Click Reset Alarm Button (senselink/cmd)"| Broker
    Broker -.-> PY
    PY -.->|"ser.write(b'R')"| ISR
    ISR -.->|reset_request = 1| TA

```


# Design Principles

The architecture was designed around the following principles:

* Separation of concerns
* Loose coupling between software layers
* Thread-safe communication
* Modular design
* Extensibility

Because each subsystem has a single responsibility, new sensors, dashboard features or communication protocols can be added with minimal impact on the rest of the system.
