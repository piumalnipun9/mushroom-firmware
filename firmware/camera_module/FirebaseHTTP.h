#pragma once
#include <Arduino.h>

namespace FirebaseHTTP {
  void begin(const char* host, const char* secret);
  int put(const String& path, const String& jsonPayload);
  int post(const String& path, const String& jsonPayload, String* response = nullptr);
  int get(const String& path, String* response);
}
