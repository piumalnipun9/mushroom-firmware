#pragma once
#include <Arduino.h>

namespace Actuators {
  // Initialize actuators (configure humidifier interrupt)
  void begin();

  // Humidifier state machine: OFF -> SLOW -> FAST -> OFF (cycles on falling edges)
  enum HumidifierMode {
    MODE_OFF = 0,
    MODE_SLOW = 1,
    MODE_FAST = 2
  };

  // Get current humidifier mode
  HumidifierMode getHumidifierMode();
  
  // Set humidifier mode directly
  void setHumidifierMode(HumidifierMode mode);

  // Simple stubs (kept minimal)
  void setExhaustFan(bool on);
}
