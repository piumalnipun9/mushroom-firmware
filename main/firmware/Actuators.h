#pragma once
#include <Arduino.h>

namespace Actuators {
  void begin();
  void setLightIntensity(int percent);
  void setHumidifier(bool on);
  void setExhaustFan(bool on);
}
