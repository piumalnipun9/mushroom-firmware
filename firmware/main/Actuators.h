#pragma once
#include <Arduino.h>

namespace Actuators {
  // Initialize actuators (configure PWM pin)
  void begin();

  // Set light intensity (0-100%) — drives PWM to motor-driver/LED-strip input
  void setLightIntensity(int percent);

  // Simple stubs (kept minimal)
  void setHumidifier(bool on);
  void setExhaustFan(bool on);
}
