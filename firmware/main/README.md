ESP32 Modular Firmware for Mushroom Monitoring

Overview

This sketch demonstrates a modular ESP32 firmware structure that:

- Connects to Wi-Fi
- Sends sensor data to Firebase Realtime Database using standard HTTPS GET/PUT/POST
- Provides modular stubs for sensors, actuators, camera, and robot arm

Files

- `main.ino` — main sketch tying modules together
- `WiFiManager.h/.cpp` — Wi-Fi connect helper
- `FirebaseHTTP.h/.cpp` — simple REST wrapper for Firebase using `HTTPClient`
- `Sensors.h/.cpp` — sensor read stubs (temperature, humidity, CO2, moisture, pH)
- `Actuators.h/.cpp` — actuator control stubs (lights, humidifier, fan)
- `RobotArm.h/.cpp` — robot arm control stubs
- `Camera.h/.cpp` — camera control stub
- `Camera.h/.cpp` — camera control stub (kept for local integration testing)
- `camera_module.ino` — dedicated ESP32-CAM sketch; run on a separate ESP32 module

Configuration

Edit `main.ino` to set your `WIFI_SSID`, `WIFI_PASSWORD`, `FIREBASE_HOST`, and `FIREBASE_SECRET`.

Usage

1. Open this folder as an Arduino sketch in the Arduino IDE or PlatformIO.
2. Install dependencies: `ArduinoJson` (used for JSON construction).
3. Customize sensor/actuator implementations in their modules.
4. Upload to ESP32.

Notes

- This is a starting point. Replace stubs with real sensor drivers and camera code (e.g., `esp32-camera`).
- Firebase REST uses `.json?auth=YOUR_SECRET` for requests; for production prefer secure token-based auth.
