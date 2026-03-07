#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <HTTPClient.h>

#include "WiFiManager.h"
#include "FirebaseHTTP.h"
#include "Sensors.h"
#include "Actuators.h"
#include "RobotArm.h"
#include "SDLogger.h"

// ---- Configuration: set these before upload ----
const char *WIFI_SSID = "iPhone";
const char *WIFI_PASSWORD = "12345678";  

// Example Firebase from repository; replace with your DB host and secret
const char *FIREBASE_HOST = "https://project-mushroom-2f8a9-default-rtdb.asia-southeast1.firebasedatabase.app/";
const char *FIREBASE_SECRET = "ZT0lSGdPU92LVTESOaXzYd3LOFgWKyVGf3Hm1zXH"; // if using database secret, put it here

// Timing
unsigned long lastSendMs = 0;
unsigned long lastHistoryMs = 0;
unsigned long lastControlCheckMs = 0;
unsigned long lastHumidifierPollMs = 0;

const unsigned long sendIntervalMs = 5000;           // Current values every 5 sec
const unsigned long historyIntervalMs = 60000;       // History every 60 sec (reduce Firebase load)
const unsigned long controlCheckIntervalMs = 1000;   // Check controls every 1 sec
const unsigned long humidifierPollIntervalMs = 1000; // Poll humidifier mode every 1 sec


void setup()
{
    Serial.begin(9600);



    WiFiManagerMod::begin(WIFI_SSID, WIFI_PASSWORD);
    FirebaseHTTP::begin(FIREBASE_HOST, FIREBASE_SECRET);

    Sensors::begin();
    Actuators::begin();
    SDLogger::begin();  // Mount SD card — safe to continue even if card is absent

    // Write homing status BEFORE the blocking homing sequence so UI updates immediately
    FirebaseHTTP::put("robotArm/status/state", String("\"homing\""));
    FirebaseHTTP::put("robotArm/status/currentPlot", String(0));
    FirebaseHTTP::put("robotArm/status/lastAction", String("\"Homing in progress...\""));
    // Initialise command field so firmware always has a valid value to read
    FirebaseHTTP::put("robotArm/command/action", String("\"none\""));

    RobotArm::begin(); // blocking — stepper homes here

    // Homing complete
    FirebaseHTTP::put("robotArm/status/state", String("\"idle\""));
    FirebaseHTTP::put("robotArm/status/lastAction", String("\"System homed and ready\""));

    Serial.println("[Main] System initialized");
}

// Firebase GET helper for humidifier control
int firebaseGet(const String &path, String *response)
{
    if (WiFi.status() != WL_CONNECTED)
        return -1;

    HTTPClient http;
    String url = String(FIREBASE_HOST);
    if (!url.endsWith("/"))
        url += "/";
    url += path;
    if (!url.endsWith(".json"))
        url += ".json";
    url += "?auth=" + String(FIREBASE_SECRET);

    http.begin(url);
    int code = http.GET();
    if (code > 0 && response != nullptr)
    {
        *response = http.getString();
    }
    http.end();
    return code;
}

// Poll humidifier mode from Firebase and drive the module via pulses
void pollHumidifierMode()
{
    unsigned long now = millis();
    if (now - lastHumidifierPollMs < humidifierPollIntervalMs)
        return;

    lastHumidifierPollMs = now;

    if (Actuators::isBusy())
        return; // pulses in progress — ignore new commands

    String resp;
    int code = firebaseGet("humidifier", &resp);
    if (code == 200 && resp.length() > 0)
    {
        StaticJsonDocument<128> doc;
        DeserializationError err = deserializeJson(doc, resp);
        if (!err && doc["mode"].is<const char *>())
        {
            String mode = doc["mode"].as<String>();
            if (mode == "ON")
                Actuators::turnOn();
            else if (mode == "OFF")
                Actuators::turnOff();
        }
    }
}

void loop()
{
    if (!WiFiManagerMod::connected())
    {
        delay(1000);
        return;
    }

    unsigned long now = millis();

    // ===== Read sensors and update Firebase — skip while arm servos are moving =====
    // HTTP calls can block 100–500 ms and would cause the servo to visibly jump.
    // During STATE_MOVING_STEPPER the stepper has its own yield() cadence, so
    // only the servo phases (IDLE_ARM / TARGET_ARM / STOPPING) need protection.
    bool armInServoPhase = (RobotArm::getState() == RobotArm::STATE_MOVING_TO_IDLE_ARM ||
                            RobotArm::getState() == RobotArm::STATE_MOVING_TO_TARGET_ARM ||
                            RobotArm::getState() == RobotArm::STATE_STOPPING);

    if (!armInServoPhase && now - lastSendMs >= sendIntervalMs)
    {
        lastSendMs = now;

        float temp = Sensors::readTemperature();
        float hum = Sensors::readHumidity();
        float co2 = Sensors::readCO2();
        
        static float lastMoist = 0;
        static float lastPh = 0;
        
        // Only read moisture and pH if the arm is parked in a plot
        // Otherwise, probe is in the air reading invalid data
        bool armAtPlot = RobotArm::isIdle() && RobotArm::getCurrentLocation() > 0;
        if (armAtPlot) {
            lastMoist = Sensors::readMoisture();
            lastPh = Sensors::readPH();
        }

        Serial.printf("Temp: %.1fC, Hum: %.1f%%, CO2: %.0f, Moist: %.0f%%, pH: %.1f %s\n",
                      temp, hum, co2, lastMoist, lastPh, armAtPlot ? "(Updated soil)" : "(Cached)");

        StaticJsonDocument<256> doc;
        doc["temperature"] = temp;
        doc["humidity"] = hum;
        doc["co2"] = co2;
        doc["moisture"] = lastMoist;
        doc["ph"] = lastPh;

        String payload;
        serializeJson(doc, payload);

        int code = FirebaseHTTP::put("sensors/current", payload);
        Serial.printf("PUT /sensors/current -> %d\n", code);

        // ===== Push history less frequently (every 60 seconds) =====
        if (now - lastHistoryMs >= historyIntervalMs)
        {
            lastHistoryMs = now;
            Serial.println("Posting history...");

            StaticJsonDocument<128> hist;
            hist["timestamp"] = (unsigned long)(now + 1700000000000ULL);

            hist["value"] = temp;
            String htemp;
            serializeJson(hist, htemp);
            FirebaseHTTP::post("sensors/temperature/history", htemp);

            hist["value"] = hum;
            String hhum;
            serializeJson(hist, hhum);
            FirebaseHTTP::post("sensors/humidity/history", hhum);

            hist["value"] = co2;
            String hco2;
            serializeJson(hist, hco2);
            FirebaseHTTP::post("sensors/co2/history", hco2);

            if (armAtPlot) {
                hist["value"] = lastMoist;
                String hmo;
                serializeJson(hist, hmo);
                FirebaseHTTP::post("sensors/moisture/history", hmo);

                hist["value"] = lastPh;
                String hph;
                serializeJson(hist, hph);
                FirebaseHTTP::post("sensors/ph/history", hph);
            }

            Serial.println("History posted.");
        }
    }

    // ===== Humidifier: handle SLOW mode timed toggling =====
    Actuators::update();

    // ===== Robot Arm: tick the non-blocking state machine =====
    RobotArm::update();

    // ===== Sync Firebase when arm finishes a move =====
    if (RobotArm::checkAndClearArrival())
    {
        int loc = RobotArm::getCurrentLocation();
        FirebaseHTTP::put("robotArm/status/currentPlot", String(loc));
        FirebaseHTTP::put("robotArm/status/state", String("\"idle\""));
        String action;
        if (loc == 0)
            action = "\"Returned to Home\"";
        else
            action = "\"Arrived at Plot " + String(loc) + "\"";
        FirebaseHTTP::put("robotArm/status/lastAction", action);
        Serial.printf("[Main] Robot at %s, Firebase updated.\n", loc == 0 ? "Home" : ("plot " + String(loc)).c_str());

        // ===== SD card log — only when arm is at a real plot, not home =====
        if (loc > 0)
        {
            // Read all sensors right now (arm is deployed, probe is in soil)
            float temp  = Sensors::readTemperature();
            float hum   = Sensors::readHumidity();
            float co2   = Sensors::readCO2();
            float moist = Sensors::readMoisture();
            float ph    = Sensors::readPH();

            SDLogger::logSensorReading(loc, temp, hum, co2, moist, ph);
        }
    }

    // ===== Check controls less frequently (every 3 seconds) =====
    if (now - lastControlCheckMs >= controlCheckIntervalMs)
    {
        lastControlCheckMs = now;

        // --- Stop command: handle even while arm is busy ---
        String actionResp;
        int gcode = FirebaseHTTP::get("robotArm/command/action", &actionResp);
        if (gcode > 0 && actionResp.length() > 0)
        {
            actionResp.trim();
            // Strip surrounding quotes if present
            if (actionResp.startsWith("\"")) actionResp = actionResp.substring(1, actionResp.length() - 1);

            if (RobotArm::isIdle())
            {
                // Move and Home commands only accepted when idle
                if (actionResp == "move")
                {
                    String tResp;
                    if (FirebaseHTTP::get("robotArm/command/targetPlot", &tResp) > 0)
                    {
                        tResp.trim();
                        int target = tResp.toInt();
                        Serial.printf("[Main] Command: move to plot %d\n", target);

                        bool dispatched = RobotArm::setTarget(target);
                        FirebaseHTTP::put("robotArm/command/action", String("\"none\""));

                        if (dispatched)
                        {
                            FirebaseHTTP::put("robotArm/status/state", String("\"moving\""));
                            FirebaseHTTP::put("robotArm/status/lastAction",
                                             "\"Moving to Plot " + String(target) + "\"");
                            FirebaseHTTP::put("robotArm/command/timestamp", String((unsigned long)millis()));
                            Serial.printf("[Main] Arm dispatched to plot %d\n", target);
                        }
                        else
                        {
                            FirebaseHTTP::put("robotArm/status/state", String("\"idle\""));
                            Serial.printf("[Main] Already at plot %d or invalid — ignoring.\n", target);
                        }
                    }
                }
                else if (actionResp == "home")
                {
                    bool dispatched = RobotArm::goHome();
                    FirebaseHTTP::put("robotArm/command/action", String("\"none\""));

                    if (dispatched)
                    {
                        FirebaseHTTP::put("robotArm/status/state", String("\"homing\""));
                        FirebaseHTTP::put("robotArm/status/lastAction", String("\"Returning Home\""));
                        Serial.println("[Main] Arm dispatched home.");
                    }
                    else
                    {
                        FirebaseHTTP::put("robotArm/status/state", String("\"idle\""));
                        Serial.println("[Main] Already home — ignoring.");
                    }
                }
            }
        }
    }

    // ===== Poll humidifier mode from Firebase — skip during servo phases =====
    if (!armInServoPhase)
        pollHumidifierMode();

    delay(10);
}
