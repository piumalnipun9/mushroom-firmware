/*
 * ESP32 Firebase Integration Reference
 * Mushroom Farm Monitoring System
 * 
 * This file shows the Firebase database structure and example code
 * for your ESP32 to write sensor readings and read commands.
 * 
 * Firebase Database URL: https://project-mushroom-2f8a9-default-rtdb.asia-southeast1.firebasedatabase.app
 */

// ============================================
// FIREBASE DATABASE STRUCTURE
// ============================================

/*
Firebase Realtime Database Structure:

/sensors
  /current
    temperature: 24.5      (number - °C)
    humidity: 85.2         (number - %)
    co2: 820               (number - ppm)
    moisture: 72.8         (number - %)
    ph: 6.5                (number - pH)
  
  /temperature/history
    /reading_0
      timestamp: 1732617600000   (number - Unix timestamp in ms)
      value: 24.3                (number)
    /reading_1
      ...
  
  /humidity/history
    ...same structure...
  
  /co2/history
    ...same structure...
  
  /moisture/history
    ...same structure...
  
  /ph/history
    ...same structure...

/robotArm
  currentPlot: 1           (number - 1-6)
  targetPlot: 1            (number - 1-6)
  status: "idle"           (string - "idle", "moving", "operating")
  lastAction: "..."        (string)
  commandTimestamp: ...    (number - Unix timestamp)

/lightControl
  intensity: 75            (number - 0-100%)
  isAuto: true             (boolean)
  status: "on"             (string - "on" or "off")

/commands
  /sensors
    /[push_key]
      sensorType: "temperature"  (string)
      action: "read"             (string - "read" or "calibrate")
      timestamp: ...             (number)
  
  /robotArm
    lastCommand: "none"          (string)
    timestamp: ...               (number)

/mlModel
  name: "..."
  version: "..."
  accuracy: 94.5
  status: "active"
  ...

/plots
  /plot_1
    id: 1
    name: "Plot 1"
    status: "active"
    lastVisited: "2025-11-26T..."
  /plot_2
    ...

/alerts
  /[alert_id]
    type: "warning"
    message: "..."
    timestamp: ...
    acknowledged: false
*/

// ============================================
// ESP32 ARDUINO CODE EXAMPLE
// ============================================

/*
Required Libraries:
- Firebase ESP Client by Mobizt
- WiFi (built-in)
- DHT sensor library (for temperature/humidity)

Install via Arduino Library Manager:
1. Firebase ESP Client
2. ArduinoJson

*/

#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

// WiFi Credentials
#define WIFI_SSID "YOUR_WIFI_SSID"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"

// Firebase Credentials
#define API_KEY "AIzaSyDwwpLAITscXtUaGdD0px0iUOxi3qD3GSo"
#define DATABASE_URL "https://project-mushroom-2f8a9-default-rtdb.asia-southeast1.firebasedatabase.app"

// Firebase objects
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// Timing
unsigned long sendDataPrevMillis = 0;
const long sendDataInterval = 5000; // Send every 5 seconds

// Sensor pins (adjust based on your setup)
#define DHT_PIN 4
#define MOISTURE_PIN 34
#define PH_PIN 35
#define CO2_PIN 32

void setup() {
  Serial.begin(115200);
  
  // Connect to WiFi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
  
  // Firebase setup
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  
  // Anonymous authentication (or use email/password)
  if (Firebase.signUp(&config, &auth, "", "")) {
    Serial.println("Firebase authenticated");
  } else {
    Serial.printf("Firebase auth failed: %s\n", config.signer.signupError.message.c_str());
  }
  
  config.token_status_callback = tokenStatusCallback;
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
}

void loop() {
  if (Firebase.ready() && (millis() - sendDataPrevMillis > sendDataInterval || sendDataPrevMillis == 0)) {
    sendDataPrevMillis = millis();
    
    // Read sensors (replace with your actual sensor reading code)
    float temperature = readTemperature();
    float humidity = readHumidity();
    float co2 = readCO2();
    float moisture = readMoisture();
    float ph = readPH();
    
    // Update current values
    updateCurrentSensorValues(temperature, humidity, co2, moisture, ph);
    
    // Add to history
    addToHistory("temperature", temperature);
    addToHistory("humidity", humidity);
    addToHistory("co2", co2);
    addToHistory("moisture", moisture);
    addToHistory("ph", ph);
    
    // Check for commands
    checkCommands();
  }
}

void updateCurrentSensorValues(float temp, float hum, float co2, float moist, float ph) {
  Firebase.RTDB.setFloat(&fbdo, "/sensors/current/temperature", temp);
  Firebase.RTDB.setFloat(&fbdo, "/sensors/current/humidity", hum);
  Firebase.RTDB.setFloat(&fbdo, "/sensors/current/co2", co2);
  Firebase.RTDB.setFloat(&fbdo, "/sensors/current/moisture", moist);
  Firebase.RTDB.setFloat(&fbdo, "/sensors/current/ph", ph);
  
  Serial.printf("Updated: T=%.1f H=%.1f CO2=%.0f M=%.1f pH=%.2f\n", temp, hum, co2, moist, ph);
}

void addToHistory(String sensorType, float value) {
  String path = "/sensors/" + sensorType + "/history";
  
  FirebaseJson json;
  json.set("timestamp", (unsigned long)millis() + 1700000000000); // Approximate timestamp
  json.set("value", value);
  
  Firebase.RTDB.pushJSON(&fbdo, path.c_str(), &json);
}

void checkCommands() {
  // Check robot arm commands
  if (Firebase.RTDB.getString(&fbdo, "/robotArm/status")) {
    String status = fbdo.stringData();
    if (status == "moving") {
      int targetPlot = 1;
      if (Firebase.RTDB.getInt(&fbdo, "/robotArm/targetPlot")) {
        targetPlot = fbdo.intData();
      }
      
      // Move robot to target plot
      moveRobotToPlot(targetPlot);
      
      // Update status when done
      Firebase.RTDB.setString(&fbdo, "/robotArm/status", "idle");
      Firebase.RTDB.setInt(&fbdo, "/robotArm/currentPlot", targetPlot);
      Firebase.RTDB.setString(&fbdo, "/robotArm/lastAction", "Arrived at Plot " + String(targetPlot));
    }
  }
  
  // Check light control
  if (Firebase.RTDB.getString(&fbdo, "/lightControl/status")) {
    String lightStatus = fbdo.stringData();
    int intensity = 75;
    if (Firebase.RTDB.getInt(&fbdo, "/lightControl/intensity")) {
      intensity = fbdo.intData();
    }
    
    // Control your lights here
    setLightIntensity(lightStatus == "on" ? intensity : 0);
  }
}

// ============================================
// SENSOR READING FUNCTIONS (implement based on your hardware)
// ============================================

float readTemperature() {
  // Replace with actual DHT/DS18B20 reading
  return 24.0 + random(-20, 20) / 10.0;
}

float readHumidity() {
  // Replace with actual DHT reading
  return 85.0 + random(-50, 50) / 10.0;
}

float readCO2() {
  // Replace with actual MQ-135 or MH-Z19 reading
  return 800 + random(-100, 100);
}

float readMoisture() {
  // Replace with actual soil moisture sensor reading
  // int raw = analogRead(MOISTURE_PIN);
  // return map(raw, 4095, 0, 0, 100);
  return 75.0 + random(-50, 50) / 10.0;
}

float readPH() {
  // Replace with actual pH sensor reading
  // int raw = analogRead(PH_PIN);
  // Calibrate and convert to pH
  return 6.5 + random(-5, 5) / 10.0;
}

void moveRobotToPlot(int plotNumber) {
  Serial.printf("Moving robot to plot %d\n", plotNumber);
  // Implement your robot arm control here
  // Example: Send commands to servo motors, stepper motors, etc.
  delay(2000); // Simulate movement time
}

void setLightIntensity(int intensity) {
  Serial.printf("Setting light intensity to %d%%\n", intensity);
  // Implement PWM control for grow lights
  // Example: analogWrite(LIGHT_PIN, map(intensity, 0, 100, 0, 255));
}
