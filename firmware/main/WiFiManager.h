#pragma once
#include <Arduino.h>

namespace WiFiManagerMod {
  void begin(const char* ssid, const char* password, unsigned long timeout_ms = 20000);
  bool connected();
}
