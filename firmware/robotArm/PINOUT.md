# ESP32 Robot Arm Controller - Pinout & Wiring

## Overview

This document lists all ESP32 pin connections for the 5-DOF robot arm controller. The arm uses 5 servo motors controlled via PWM.

---

## Quick Reference Table

| GPIO | Function | Servo | Notes |
|------|----------|-------|-------|
| **13** | PWM | Base | Rotation (0-180°) |
| **12** | PWM | Shoulder | Up/Down motion |
| **14** | PWM | Elbow/Arm | Extension |
| **27** | PWM | Wrist | Orientation |
| **26** | PWM | Gripper | Open/Close |

---

## Servo Wiring Diagram

```
ESP32                 Servo
═════                 ═════
GPIO13 ──────────────► Base Servo (Signal/Yellow)
GPIO12 ──────────────► Shoulder Servo (Signal)
GPIO14 ──────────────► Elbow Servo (Signal)
GPIO27 ──────────────► Wrist Servo (Signal)
GPIO26 ──────────────► Gripper Servo (Signal)

5V (External) ───────► All Servos VCC (Red)
GND ─────────────────► All Servos GND (Brown/Black)
```

---

## Detailed Connections

### Power Supply

⚠️ **IMPORTANT:** Do NOT power all 5 servos from ESP32's 5V pin!

```
External 5V-6V Supply (2A+ recommended)
         │
         ├──────────► Servo 1 VCC (Red)
         ├──────────► Servo 2 VCC (Red)
         ├──────────► Servo 3 VCC (Red)
         ├──────────► Servo 4 VCC (Red)
         └──────────► Servo 5 VCC (Red)
         
GND (Common) ────────► All Servo GND + ESP32 GND
```

### Signal Connections

| Servo | Color | ESP32 Pin |
|-------|-------|-----------|
| Base | Yellow/Orange | GPIO13 |
| Shoulder | Yellow/Orange | GPIO12 |
| Elbow | Yellow/Orange | GPIO14 |
| Wrist | Yellow/Orange | GPIO27 |
| Gripper | Yellow/Orange | GPIO26 |

---

## Code Configuration

```cpp
// ESP32 PWM Pins for Servos
const int PIN_BASE = 13;      // GPIO13 - Base servo
const int PIN_SHOULDER = 12;  // GPIO12 - Shoulder servo
const int PIN_ELBOW = 14;     // GPIO14 - Elbow/Arm servo
const int PIN_WRIST = 27;     // GPIO27 - Wrist servo  
const int PIN_GRIPPER = 26;   // GPIO26 - Gripper servo
```

---

## Servo Specifications

| Joint | Range | Default | Purpose |
|-------|-------|---------|---------|
| Base | 0-180° | 90° | Rotate arm left/right |
| Shoulder | 0-180° | 90° | Raise/lower arm |
| Elbow | 0-180° | 90° | Extend/retract |
| Wrist | 0-180° | Auto | Keep gripper level (computed) |
| Gripper | 130°/180° | Open | 130° = Open, 180° = Closed |

---

## Control Commands

Send these via Serial Monitor (9600 baud):

### Joint Angle Control
```
{base,shoulder,arm,grip}

Examples:
{90,60,90,1}      → Center, gripper open
{0,130,100,0}     → Left, gripper closed
{180,90,120,1}    → Right, gripper open
```

### Cartesian Coordinate Control (Inverse Kinematics)
```
[x,y,z]

Examples:
[10,0,15]         → Move to X=10mm, Y=0mm, Z=15mm
[15,5,10]         → Move to X=15mm, Y=5mm, Z=10mm
```

### Predefined Sequence
```
SEQ               → Run pick-and-place demo
```

---

## Link Lengths (for IK calculations)

| Link | Length | Description |
|------|--------|-------------|
| d1 | 2.5 mm | Base height |
| a2 | 10.5 mm | Upper arm length |
| a3 | 14.5 mm | Forearm length |
| d5 | 7.0 mm | Wrist to gripper |

---

## Pin Selection Notes

### Why These Pins?

Pins were chosen to avoid conflicts with the main sensor controller:
- **Avoided:** GPIO4, 15, 16, 17, 21, 22, 23, 25, 33, 34 (used by sensors)
- **Used:** GPIO12, 13, 14, 26, 27 (free PWM-capable pins)

### Available Pins (if expansion needed)
- GPIO2, GPIO5, GPIO18, GPIO19
- GPIO32, GPIO35 (input-only)

---

## Required Library

Install via Arduino Library Manager:
```
ESP32Servo by Kevin Harrington
```

---

## Power Requirements

| Component | Voltage | Current | Notes |
|-----------|---------|---------|-------|
| ESP32 | 3.3V/5V | ~80mA | |
| Servo (each, idle) | 5V-6V | ~10mA | |
| Servo (each, moving) | 5V-6V | ~200-500mA | |
| All 5 servos (peak) | 5V-6V | ~2.5A | Use external power! |

**Recommended Power Supply:** 5V/6V, 3A+ rating

---

## Troubleshooting

| Issue | Solution |
|-------|----------|
| Servos jitter | Use external power supply |
| ESP32 resets when servo moves | Insufficient power, add external supply |
| Servo doesn't respond | Check signal wire and PWM pin |
| "Target unreachable" | Position outside arm's workspace |
| Wrist angle wrong | Check K_CAL value (currently 314.5) |
