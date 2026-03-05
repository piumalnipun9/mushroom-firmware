#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <HTTPClient.h>

#include "WiFiManager.h"
#include "FirebaseHTTP.h"
#include "Sensors.h"
#include "Actuators.h"
#include "RobotArm.h"

// ---- Configuration: set these before upload ----
const char *WIFI_SSID = "Pixel 6a";
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
const unsigned long controlCheckIntervalMs = 3000;   // Check controls every 3 sec
const unsigned long humidifierPollIntervalMs = 1000; // Poll humidifier mode every 1 sec

void setup()
{
    Serial.begin(9600);
    delay(100);

    WiFiManagerMod::begin(WIFI_SSID, WIFI_PASSWORD);
    FirebaseHTTP::begin(FIREBASE_HOST, FIREBASE_SECRET);

    Sensors::begin();
    Actuators::begin();

    // Write homing status BEFORE the blocking homing sequence so UI updates immediately
    FirebaseHTTP::put("robotArm/status", String("\"homing\""));
    FirebaseHTTP::put("robotArm/currentPlot", String(0));
    FirebaseHTTP::put("robotArm/lastAction", String("\"Homing in progress...\""));

    RobotArm::begin(); // blocking — stepper homes here

    // Homing complete
    FirebaseHTTP::put("robotArm/status", String("\"idle\""));
    FirebaseHTTP::put("robotArm/lastAction", String("\"System homed and ready\""));

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

// Poll humidifier mode from Firebase and sync with Actuators
void pollHumidifierMode()
{
    unsigned long now = millis();
    if (now - lastHumidifierPollMs < humidifierPollIntervalMs)
        return;

    lastHumidifierPollMs = now;

    String resp;
    int code = firebaseGet("humidifier", &resp);
    if (code == 200 && resp.length() > 0)
    {
        StaticJsonDocument<128> doc;
        DeserializationError err = deserializeJson(doc, resp);
        if (!err && doc["mode"].is<const char *>())
        {
            String mode = doc["mode"].as<String>();
            if (mode == "OFF")
                Actuators::setHumidifierMode(Actuators::MODE_OFF);
            else if (mode == "SLOW")
                Actuators::setHumidifierMode(Actuators::MODE_SLOW);
            else if (mode == "FAST")
                Actuators::setHumidifierMode(Actuators::MODE_FAST);
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

    // ===== Read sensors and update current values every 5 seconds =====
    if (now - lastSendMs >= sendIntervalMs)
    {
        lastSendMs = now;

        float temp = Sensors::readTemperature();
        float hum = Sensors::readHumidity();
        float co2 = Sensors::readCO2();
        float moist = Sensors::readMoisture();
        float ph = Sensors::readPH();

        // Print sensor readings
        Serial.printf("Temp: %.1fC, Hum: %.1f%%, CO2: %.0f, Moist: %.0f%%, pH: %.1f\n",
                      temp, hum, co2, moist, ph);

        // Update current sensor values (single PUT - fast)
        StaticJsonDocument<256> doc;
        doc["temperature"] = temp;
        doc["humidity"] = hum;
        doc["co2"] = co2;
        doc["moisture"] = moist;
        doc["ph"] = ph;

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

            hist["value"] = moist;
            String hmo;
            serializeJson(hist, hmo);
            FirebaseHTTP::post("sensors/moisture/history", hmo);

            hist["value"] = ph;
            String hph;
            serializeJson(hist, hph);
            FirebaseHTTP::post("sensors/ph/history", hph);

            Serial.println("History posted.");
        }
    }

    // ===== Robot Arm: tick the non-blocking state machine =====
    RobotArm::update();

    // ===== Sync Firebase when arm finishes a move =====
    if (RobotArm::checkAndClearArrival())
    {
        int loc = RobotArm::getCurrentLocation();
        FirebaseHTTP::put("robotArm/currentPlot", String(loc));
        FirebaseHTTP::put("robotArm/status", String("\"idle\""));
        String action = "\"Arrived at Plot " + String(loc) + "\"";
        FirebaseHTTP::put("robotArm/lastAction", action);
        Serial.printf("[Main] Robot arrived at plot %d, Firebase updated.\n", loc);
    }

    // ===== Check controls less frequently (every 10 seconds) =====
    if (now - lastControlCheckMs >= controlCheckIntervalMs)
    {
        lastControlCheckMs = now;

        // Only accept a new command when the arm is idle
        if (RobotArm::isIdle())
        {
            String resp;
            int gcode = FirebaseHTTP::get("robotArm/status", &resp);
            if (gcode > 0 && resp.length() > 0)
            {
                resp.trim();
                if (resp.indexOf("\"") == 0)
                    resp = resp.substring(1, resp.length() - 1);

                if (resp == "moving")
                {
                    String tResp;
                    if (FirebaseHTTP::get("robotArm/targetPlot", &tResp) > 0)
                    {
                        tResp.trim();
                        int target = tResp.toInt();
                        Serial.printf("[Main] Command received: move to plot %d (current=%d)\n",
                                      target, RobotArm::getCurrentLocation());

                        bool dispatched = RobotArm::setTarget(target);

                        if (dispatched)
                        {
                            // Arm is now moving — acknowledge in Firebase immediately
                            FirebaseHTTP::put("robotArm/status", String("\"moving\""));
                            FirebaseHTTP::put("robotArm/commandTimestamp",
                                              String((unsigned long)millis()));
                            Serial.printf("[Main] Arm dispatched to plot %d\n", target);
                        }
                        else
                        {
                            // Already there or invalid — reset status so UI unlocks
                            FirebaseHTTP::put("robotArm/status", String("\"idle\""));
                            Serial.printf("[Main] Already at plot %d or invalid target — resetting status\n", target);
                        }
                    }
                }
            }
        }
    }

    // ===== Poll humidifier mode from Firebase (every 1 second) =====
    pollHumidifierMode();

    delay(10);
}
