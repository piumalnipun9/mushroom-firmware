#pragma once
#include <Arduino.h>

namespace RobotArm {

  // ─── User-configurable positioning constants ─────────────────────────────
  // Change only these; everything else derives automatically.

  // Servo centre position (µs) — 1500 = true centre
  constexpr int  SERVO_CENTER_US     = 1500;

  // Per-servo offset from centre (µs)
  //   Servo 1: A-side = center + 100 = 1600 µs  |  B-side = center - 100 = 1400 µs
  constexpr int  SERVO1_OFFSET_US    = 100;
  //   Servo 2: A-side = center + 500 = 2000 µs  |  B-side = center - 500 = 1000 µs
  constexpr int  SERVO2_OFFSET_US    = 500;

  // Derived µs values — do not edit
  constexpr int  SERVO1_US_A         = SERVO_CENTER_US + SERVO1_OFFSET_US;  // 1600 µs — Locations 1 & 3
  constexpr int  SERVO1_US_B         = SERVO_CENTER_US - SERVO1_OFFSET_US;  // 1400 µs — Locations 2 & 4
  constexpr int  SERVO2_US_A         = SERVO_CENTER_US + SERVO2_OFFSET_US;  // 2000 µs — Locations 1 & 3
  constexpr int  SERVO2_US_B         = SERVO_CENTER_US - SERVO2_OFFSET_US;  // 1000 µs — Locations 2 & 4

  // Stepper target positions (steps from home)
  constexpr long STEPPER_POS_NEAR    = 10000;  // Locations 1 & 2
  constexpr long STEPPER_POS_FAR     = 50000;  // Locations 3 & 4

  // Servo smooth-move duration per phase (ms) — servo 2 moves first, then servo 1
  constexpr int  SERVO_MOVE_MS       = 800;

  // Stepper pulse interval (µs between pulses — lower = faster)
  constexpr unsigned int STEP_INTERVAL_US = 800;
  // ─────────────────────────────────────────────────────────────────────────

  enum State {
    STATE_IDLE,
    STATE_MOVING_TO_IDLE_ARM,
    STATE_MOVING_STEPPER,
    STATE_MOVING_TO_TARGET_ARM
  };

  void  begin();                 // Home stepper at startup (blocking)
  bool  setTarget(int location); // Returns true=dispatched, false=ignored (busy/already there)
  void  update();                // Call every loop tick — runs the state machine
  State getState();
  int   getCurrentLocation();
  bool  isIdle();
  bool  checkAndClearArrival();  // Returns true once after each completed move

} // namespace RobotArm
