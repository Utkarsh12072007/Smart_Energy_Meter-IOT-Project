# IoT Smart Energy Monitoring System (ESP32-S3)

An IoT-enabled real-time AC energy monitoring and overload alerting system built using the **ESP32-S3 (YD-ESP32-S3-N16R8)**, **ACS712 (5A) Hall-Effect Current Sensor**, **ZMPT101B AC Voltage Sensor**, **Blynk IoT 2.0**, and **Telegram Bot API**.

## Features
- **True RMS AC Measurement:** High-speed waveform sampling (~10 AC cycles per window) with automatic DC midpoint offset calibration.
- **Real-Time Telemetry:** Computes RMS Voltage ($V$), RMS Current ($I$), Active Power ($W$), Cumulative Energy ($\text{kWh}$), and Estimated Electricity Bill ($\text{₹ INR}$).
- **Cloud Dashboard (Blynk 2.0):** Live mobile & web dashboard gauges across Virtual Pins `V0`–`V4`.
- **Automated Telegram Alerts:** Instant overload notifications when power exceeds the safety threshold, plus periodic energy consumption summaries.

## Hardware Components
- **Microcontroller:** YD-ESP32-S3-N16R8 (Dual-Core Xtensa LX7, 12-bit ADC1)
- **Current Sensor:** ACS712ELCTR-05B (5A Hall-Effect Sensor, $185\text{ mV/A}$ sensitivity at $5\text{V}$)
- **Voltage Sensor:** ZMPT101B Single-Phase AC Voltage Sensor Module ($3.3\text{V}$ supply)
- **Test Load:** $100\text{W}$ Incandescent Bulb ($230\text{V}$ AC, $50\text{Hz}$)

## Pinout Configuration

| Component Pin | ESP32-S3 / Power Rail | Notes |
| :--- | :--- | :--- |
| **ACS712 `VCC`** | External `+5V` Supply | Ensures stable $\sim 2.5\text{V}$ DC midpoint bias |
| **ACS712 `GND`** | Common `GND` | Tied to ESP32-S3 `GND` |
| **ACS712 `OUT`** | `GPIO 5` (`ADC1_CH4`) | Analog current waveform input |
| **ZMPT101B `VCC`** | `3V3` | Regulated $3.3\text{V}$ supply |
| **ZMPT101B `GND`** | Common `GND` | Tied to ESP32-S3 `GND` |
| **ZMPT101B `OUT`** | `GPIO 4` (`ADC1_CH3`) | Analog voltage waveform input |

## Setup & Usage
1. Install `Blynk`, `UniversalTelegramBot`, and `ArduinoJson` in Arduino IDE.
2. Add your Wi-Fi, Blynk Template/Auth Token, and Telegram Bot Token/Chat ID in `src/smart_energy_meter.ino`.
3. Select **ESP32S3 Dev Module** (`USB CDC On Boot: Disabled`) and flash at `115200` baud.
