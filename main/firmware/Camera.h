#pragma once
#include <Arduino.h>

namespace CameraMod {
  void begin();
  String takeSnapshotBase64(); // stub: returns an empty string; replace with actual camera code
}
