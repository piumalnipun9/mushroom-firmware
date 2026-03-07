#pragma once
#include <Arduino.h>

namespace Actuators {

  // Initialise the humidifier pin as OUTPUT (LOW = off)
  void begin();

  // Send 3 rising-edge pulses (1 s apart) to turn the module ON — non-blocking
  void turnOn();

  // Send 1 rising-edge pulse to turn the module OFF — non-blocking
  void turnOff();

  // True while pulses are still being sent (do not accept new commands yet)
  bool isBusy();

  // True once all pulses for an ON command have completed
  bool isOn();

  // Call every loop() — drives the non-blocking pulse state machine
  void update();

  // Simple stub
  void setExhaustFan(bool on);

} // namespace Actuators
