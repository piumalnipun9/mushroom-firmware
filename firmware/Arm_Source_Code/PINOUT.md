# Robot Arm Controller - Pinout & Wiring

## Overview

This document describes the ESP32 pin connections for the 4-DOF robot arm controller. The arm uses 4 servo motors controlled via PWM microseconds and is accessible via WiFi HTTP API.

---

## Quick Reference Table

| GPIO | Servo | Default Position (μs) | Notes |
|------|-------|----------------------|-------|
| **18** | Servo 1 (Base) | 500 | Base rotation |
| **19** | Servo 2 (Shoulder) | 1400 | Shoulder joint |
| **21** | Servo 3 (Elbow) | 600 | Elbow joint |
| **22** | Servo 4 (Wrist) | 2000 | Wrist/End effector |

---

## Wiring Diagram

```
ESP32                 Servo Motors
═════                 ════════════
GPIO18 ──────────────► Servo 1 Signal (Base)
GPIO19 ──────────────► Servo 2 Signal (Shoulder)
GPIO21 ──────────────► Servo 3 Signal (Elbow)
GPIO22 ──────────────► Servo 4 Signal (Wrist)

5V (External) ───────► All Servos VCC (Red)
GND ─────────────────► All Servos GND (Brown/Black)
```

---

## Power Supply Requirements

⚠️ **IMPORTANT:** Do NOT power servos from ESP32's 5V pin!

```
External 5V-6V Supply (2A+ recommended)
         │
         ├──────────► Servo 1 VCC (Red)
         ├──────────► Servo 2 VCC (Red)
         ├──────────► Servo 3 VCC (Red)
         └──────────► Servo 4 VCC (Red)
         
GND (Common) ────────► All Servo GND + ESP32 GND
```

---

## WiFi Configuration

```cpp
const char* ssid = "Pixel_9483";
const char* password = "12345678";
```

---

## HTTP API

### Endpoint: `/set_positions`

**Method:** GET

**Parameter:** `pos` - Comma-separated microsecond values for 4 servos

**Example Request:**
```
http://<ESP32_IP>/set_positions?pos=500,1400,600,2000
```

**Response:**
- `200 OK` - "Positions updated"
- `400 Bad Request` - "Invalid position data" or "Missing 'pos' parameter"

---

## Servo Position Ranges

| Servo | Min (μs) | Center (μs) | Max (μs) | Notes |
|-------|----------|-------------|----------|-------|
| Servo 1 | 500 | 1500 | 2500 | Base rotation |
| Servo 2 | 500 | 1500 | 2500 | Shoulder |
| Servo 3 | 500 | 1500 | 2500 | Elbow |
| Servo 4 | 500 | 1500 | 2500 | Wrist |

**Note:** Standard servo range is 500-2500 microseconds. Adjust based on your specific servos.

---

## Default Positions

```cpp
int currentPositions[4] = {500, 1400, 600, 2000};
```

---

## Motion Parameters

```cpp
const int duration = 200;        // Total movement duration (ms)
const int updateInterval = 1;    // Update rate (ms per step)
```

The `smoothMove` function interpolates linearly between start and end positions over the specified duration.

---

## Code Configuration

```cpp
const int servoPin1 = 18;  // Base
const int servoPin2 = 19;  // Shoulder
const int servoPin3 = 21;  // Elbow
const int servoPin4 = 22;  // Wrist
```

---

## Required Libraries

Install via Arduino Library Manager:
- **ESP32Servo** by Kevin Harrington
- **WiFi** (built-in with ESP32 core)
- **WebServer** (built-in with ESP32 core)

---

## Usage Example

1. Upload the code to ESP32
2. Open Serial Monitor (115200 baud)
3. Note the IP address printed
4. Send HTTP request:
   ```bash
   curl "http://192.168.1.100/set_positions?pos=1500,1500,1500,1500"
   ```

---

## Pin Conflict Notes

**GPIO 21 and 22** are also the default I2C pins (SDA/SCL). If you need I2C for other sensors on the same ESP32, consider remapping the servo pins to:
- GPIO 13, 14, 27, 26 (alternative PWM-capable pins)

---

## Troubleshooting

| Issue | Solution |
|-------|----------|
| Servos jitter | Use external power supply |
| ESP32 resets | Insufficient current, add capacitor |
| WiFi won't connect | Check SSID/password, verify router |
| Servos don't move | Check wiring, verify microsecond range |
| HTTP timeout | Check IP address, ensure same network |
