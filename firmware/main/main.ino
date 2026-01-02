#include <Arduino.h>
#include <ArduinoJson.h>

#include "WiFiManager.h"
#include "FirebaseHTTP.h"
#include "Sensors.h"
#include "Actuators.h"
#include "RobotArm.h"

// ---- Configuration: set these before upload ----
const char *WIFI_SSID = "Pixel_9483";
const char *WIFI_PASSWORD = "12345678";

// Example Firebase from repository; replace with your DB host and secret
const char *FIREBASE_HOST = "https://project-mushroom-2f8a9-default-rtdb.asia-southeast1.firebasedatabase.app/";
const char *FIREBASE_SECRET = "ZT0lSGdPU92LVTESOaXzYd3LOFgWKyVGf3Hm1zXH"; // if using database secret, put it here

// Timing
unsigned long lastSendMs = 0;
unsigned long lastHistoryMs = 0;
unsigned long lastControlCheckMs = 0;

const unsigned long sendIntervalMs = 5000;       // Current values every 5 sec
const unsigned long historyIntervalMs = 60000;   // History every 60 sec (reduce Firebase load)
const unsigned long controlCheckIntervalMs = 10000; // Check controls every 10 sec

void setup()
{
    Serial.begin(9600);
    delay(100);

    WiFiManagerMod::begin(WIFI_SSID, WIFI_PASSWORD);
    FirebaseHTTP::begin(FIREBASE_HOST, FIREBASE_SECRET);

    Sensors::begin();
    Actuators::begin();
    RobotArm::begin();
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
            String htemp; serializeJson(hist, htemp);
            FirebaseHTTP::post("sensors/temperature/history", htemp);

            hist["value"] = hum;
            String hhum; serializeJson(hist, hhum);
            FirebaseHTTP::post("sensors/humidity/history", hhum);
            
            hist["value"] = co2;
            String hco2; serializeJson(hist, hco2);
            FirebaseHTTP::post("sensors/co2/history", hco2);
            
            hist["value"] = moist;
            String hmo; serializeJson(hist, hmo);
            FirebaseHTTP::post("sensors/moisture/history", hmo);
            
            hist["value"] = ph;
            String hph; serializeJson(hist, hph);
            FirebaseHTTP::post("sensors/ph/history", hph);
            
            Serial.println("History posted.");
        }
    }
    
    // ===== Check controls less frequently (every 10 seconds) =====
    if (now - lastControlCheckMs >= controlCheckIntervalMs)
    {
        lastControlCheckMs = now;
        
        // Check robotArm status
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
                    int target = tResp.toInt();
                    RobotArm::moveToPlot(target);
                    FirebaseHTTP::put("robotArm/status", String("\"idle\""));
                    FirebaseHTTP::put("robotArm/currentPlot", String("" + String(target)));
                }
            }
        }

        // Check lightControl
        String lresp;
        if (FirebaseHTTP::get("lightControl/status", &lresp) > 0)
        {
            lresp.trim();
            if (lresp.indexOf('"') == 0)
                lresp = lresp.substring(1, lresp.length() - 1);
            int intensity = 75;
            String iresp;
            if (FirebaseHTTP::get("lightControl/intensity", &iresp) > 0)
                intensity = iresp.toInt();
            Actuators::setLightIntensity(lresp == "on" ? intensity : 0);
        }
    }

    delay(10);
}
