#pragma once
#include <Arduino.h>

namespace Sensors {
  void begin();
  float readTemperature();
  float readHumidity();
  float readCO2();
  float readMoisture();
  float readPH();
}
