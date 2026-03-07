
// Humidifier control — simple digital output
// OFF = HIGH, ON = LOW
#include "Actuators.h"
#include "Config.h"
#include <Arduino.h>

namespace Actuators
{
    static bool humidifierOn = false;

    void begin()
    {
        pinMode(PIN_HUMIDIFIER, OUTPUT);
        digitalWrite(PIN_HUMIDIFIER, HIGH); // OFF by default
        humidifierOn = false;
        Serial.printf("[Actuators] Humidifier on pin %d — HIGH=OFF, LOW=ON\n", PIN_HUMIDIFIER);
    }

    void turnOn()
    {
        if (humidifierOn) return;
        humidifierOn = true;
        digitalWrite(PIN_HUMIDIFIER, LOW);
        Serial.println("[Actuators] Humidifier ON (LOW)");
    }

    void turnOff()
    {
        if (!humidifierOn) return;
        humidifierOn = false;
        digitalWrite(PIN_HUMIDIFIER, HIGH);
        Serial.println("[Actuators] Humidifier OFF (HIGH)");
    }

    bool isBusy() { return false; }
    bool isOn()   { return humidifierOn; }
    void update() { } // nothing to do

    void setExhaustFan(bool on)
    {
        Serial.printf("[Actuators] Exhaust fan %s\n", on ? "ON" : "OFF");
    }

} // namespace Actuators


