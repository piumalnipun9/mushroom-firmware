#pragma once
#include <Arduino.h>

namespace RobotArm {

  // ─── User-configurable positioning constants ─────────────────────────────
  // Change only these; everything else derives automatically.

  // Idle / Homing positions
  constexpr int  SERVO1_IDLE_US      = 1000;
  constexpr int  SERVO2_IDLE_US      = 700;

  // Final deployed positions for servos (same for all 4 plots)
  constexpr int  SERVO1_DEPLOY_US    = 2100; 
  constexpr int  SERVO2_DEPLOY_US    = 1800;

  // Stepper target positions (steps from home) — 4 distinct positions
  constexpr long STEPPER_POS_PLOT1   = 1000;
  constexpr long STEPPER_POS_PLOT2   = 19000;
  constexpr long STEPPER_POS_PLOT3   = 40000;
  constexpr long STEPPER_POS_PLOT4   = 60000;

  // Servo movement speed (microseconds of pulse width change per second)
  // Higher value = faster. 250 µs/s means a 1000µs move takes 4 seconds.
  constexpr float SERVO_SPEED_US_PER_S = 3000.0;

  // Stepper pulse interval (µs between pulses — lower = faster)
  constexpr unsigned int STEP_INTERVAL_US = 400;
  // ─────────────────────────────────────────────────────────────────────────

  enum State {
    STATE_IDLE,
    STATE_MOVING_TO_IDLE_ARM,
    STATE_MOVING_STEPPER,
    STATE_CAMERA_DELAY,
    STATE_MOVING_TO_TARGET_ARM,
    STATE_STOPPING,          // abort requested — retracting arms then going idle
    STATE_HOMING_STEPPER     // goHome: driving stepper back to position 0
  };

  void  begin();                 // Home stepper at startup (blocking)
  bool  setTarget(int location); // Returns true=dispatched, false=ignored (busy/already there)
  bool  goHome();                // Retract servos, drive stepper to 0, set currentLocation=0
  bool  stop();                  // Abort current move; safe servo retract then idle
  void  update();                // Call every loop tick — runs the state machine
  State getState();
  int   getCurrentLocation();
  bool  isIdle();
  bool  isStopping();
  bool  checkAndClearArrival();  // Returns true once after each completed move
  bool  checkAndClearCameraReady(); // Returns true once when stepper finishes and 5s delay begins

  // Optional callback called every ~200 stepper steps during movement.
  // Register a function that checks Firebase for a stop command and calls
  // RobotArm::stop() if needed. Keeps RobotArm decoupled from Firebase.
  using AbortPollFn = void (*)();
  void  setAbortPollFn(AbortPollFn fn);

} // namespace RobotArm
