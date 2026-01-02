# ESP32 Main Controller - Pinout & Wiring

## Overview

This document lists all ESP32 pin connections for the mushroom monitoring system main controller. This ESP32 handles sensors, actuators, and Firebase communication.

---

## Quick Reference Table

| GPIO | Function | Component | Notes |
|------|----------|-----------|-------|
| **4** | DHT Data | DHT11 Sensor | Temperature & Humidity |
| **15** | PWM | Grow Lights | LEDC Channel 0 |
| **21** | I2C SDA | ENS160 (CO2) | Shared I2C bus |
| **22** | I2C SCL | ENS160 (CO2) | Shared I2C bus |
| **34** | ADC Input | Soil Moisture | Analog reading (input-only) |
| **23** | Digital Input | Soil Moisture | Digital threshold output |
| **16** | Serial2 RX | pH Sensor RS485 | MAX485 RO |
| **17** | Serial2 TX | pH Sensor RS485 | MAX485 DI |
| **33** | Digital Output | pH Sensor RS485 | MAX485 RE (Receiver Enable) |
| **25** | Digital Output | pH Sensor RS485 | MAX485 DE (Driver Enable) |

---

## Detailed Wiring

### 1. DHT11 Temperature & Humidity Sensor

```
DHT11              ESP32
═════              ═════
VCC ─────────────► 3.3V
GND ─────────────► GND
DATA ────────────► GPIO4 (with 4.7K pull-up to 3.3V)
```

**Config:** `PIN_DHT = 4`, `DHT_TYPE = DHT11`

---

### 2. ENS160 Air Quality Sensor (I2C) - CO2 Readings

```
ENS160             ESP32
══════             ═════
VCC ─────────────► 3.3V (or 5V if module has regulator)
GND ─────────────► GND
SDA ─────────────► GPIO21
SCL ─────────────► GPIO22
```

**Config:** 
- `PIN_I2C_SDA = 21`
- `PIN_I2C_SCL = 22`
- `ENS160_I2C_ADDR = 0x53`

**Notes:**
- Provides: eCO2 (400-65000 ppm), TVOC, AQI
- Warm-up time: ~3 minutes for initial readings, ~1 hour for accuracy

---

### 3. Soil Moisture Sensor (Capacitive)

```
Soil Sensor        ESP32
═══════════        ═════
VCC ─────────────► 3.3V-5V
GND ─────────────► GND
AO (Analog) ─────► GPIO34 (ADC1_CH6, input-only)
DO (Digital) ────► GPIO23
```

**Config:**
- `PIN_SOIL_ANALOG = 34`
- `PIN_SOIL_DIGITAL = 23`

**Notes:**
- Analog: 0-4095 (12-bit ADC), inversely proportional to moisture
- Digital: LOW = wet, HIGH = dry
- GPIO34 is input-only, perfect for ADC

---

### 4. CWT-Soil-THCPH-S pH Sensor (RS485 via MAX485)

```
ESP32              MAX485              CWT pH Sensor
═════              ══════              ═════════════
3.3V ────────────► VCC
GND  ────────────► GND ◄─────────────► Black (GND)

GPIO16 ◄─────────  RO (Receiver Out)
GPIO17 ──────────► DI (Driver In)
GPIO33 ──────────► RE (Receiver Enable)
GPIO25 ──────────► DE (Driver Enable)

                   A ◄───────────────► Yellow (A+)
                   B ◄───────────────► Blue (B-)

5V-30V DC ────────────────────────────► Brown (VCC)
```

**Config:**
- `PIN_SOIL_RS485_RX = 16`
- `PIN_SOIL_RS485_TX = 17`
- `PIN_SOIL_RS485_RE = 33`
- `PIN_SOIL_RS485_DE = 25`
- `SOIL_RS485_BAUDRATE = 4800`

**Modbus Settings:**
- Slave Address: 0x01
- Register 0x0000: pH value (divide by 10)
- Uses `readInputRegisters()` function

---

### 5. Grow Light Control (PWM)

```
ESP32              MOSFET/Relay       LED Strip
═════              ════════════       ═════════
GPIO15 ──────────► Gate/Control ────► LED Strip +
GND ─────────────► Source ───────────► Common GND
                   Drain ◄───────────── LED Strip -
                                        12V/24V Supply
```

**Config:**
- `PIN_LIGHT = 15`
- `PWM_FREQ = 5000`
- `PWM_CHANNEL = 0`
- `PWM_RESOLUTION = 8` (0-255 duty cycle)

---

## Pin Usage Summary

### Used Pins
- GPIO4: DHT11 (Temperature/Humidity)
- GPIO15: Grow Light PWM
- GPIO16: RS485 RX (pH sensor)
- GPIO17: RS485 TX (pH sensor)
- GPIO21: I2C SDA (ENS160)
- GPIO22: I2C SCL (ENS160)
- GPIO23: Soil Moisture Digital
- GPIO25: RS485 DE (pH sensor)
- GPIO33: RS485 RE (pH sensor)
- GPIO34: Soil Moisture Analog

### Available Pins
- GPIO2, GPIO5, GPIO12, GPIO13, GPIO14
- GPIO18, GPIO19, GPIO26, GPIO27
- GPIO32, GPIO35, GPIO36, GPIO39

### Pins to Avoid
- GPIO6-11: Connected to flash memory
- GPIO34-39: Input-only (no PWM output)

---

## Power Requirements

| Component | Voltage | Current | Notes |
|-----------|---------|---------|-------|
| ESP32 | 3.3V/5V | ~240mA | Via USB or VIN |
| DHT11 | 3.3V | ~2.5mA | |
| ENS160 | 3.3V-5V | ~30mA | |
| Soil Moisture | 3.3V-5V | ~35mA | |
| MAX485 | 3.3V | ~0.5mA | |
| CWT pH Sensor | 5V-30V | ~50mA | External power recommended |
| Grow Lights | 12V/24V | Variable | Use separate power supply |

---

## Troubleshooting

| Issue | Check |
|-------|-------|
| No sensor readings | Verify wiring and power |
| pH shows 0 or fallback | Check MAX485 connections, GND must be shared |
| ADC readings stuck at 0 or 4095 | Check if sensor is connected |
| I2C devices not found | Verify SDA/SCL connections, check I2C addresses |
