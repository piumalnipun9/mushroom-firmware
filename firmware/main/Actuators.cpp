
// Humidifier control — continuous toggling module
// ON = 5000ms LOW, then 5000ms HIGH, repeating
// OFF = steady HIGH
#include "Actuators.h"
#include "Config.h"
#include <Arduino.h>

namespace Actuators
{
    static const unsigned long PULSE_DURATION_MS = 5000;

    static bool humidifierOn = false;
    static bool phaseIsHigh = true;
    static unsigned long phaseStartMs = 0;

    void begin()
    {
        pinMode(PIN_HUMIDIFIER, OUTPUT);
        digitalWrite(PIN_HUMIDIFIER, HIGH); // OFF state is HIGH
        humidifierOn = false;
        Serial.printf("[Actuators] Humidifier on pin %d — continuous toggle (ON=5s LOW/5s HIGH, OFF=HIGH)\n",
                      PIN_HUMIDIFIER);
    }

    void turnOn()
    {
        if (humidifierOn)
            return; // already on

        humidifierOn = true;
        phaseIsHigh = false; // Start with LOW phase
        digitalWrite(PIN_HUMIDIFIER, LOW);
        phaseStartMs = millis();
        Serial.println("[Actuators] Humidifier turning ON (starting 5s LOW pulse)");
    }

    void turnOff()
    {
        if (!humidifierOn)
            return; // already off

        humidifierOn = false;
        phaseIsHigh = true;
        digitalWrite(PIN_HUMIDIFIER, HIGH); // Steady HIGH when OFF
        Serial.println("[Actuators] Humidifier turning OFF (steady HIGH)");
    }

    bool isBusy() { return false; } // It continuously toggles, never blocks new commands
    bool isOn() { return humidifierOn; }

    void update()
    {
        if (!humidifierOn)
            return; // Nothing to do if OFF

        unsigned long elapsed = millis() - phaseStartMs;

        if (elapsed >= PULSE_DURATION_MS)
        {
            phaseIsHigh = !phaseIsHigh;
            digitalWrite(PIN_HUMIDIFIER, phaseIsHigh ? HIGH : LOW);
            phaseStartMs = millis();
        }
    }

    void setExhaustFan(bool on)
    {
        Serial.printf("[Actuators] Exhaust fan %s\n", on ? "ON" : "OFF");
    }

} // namespace Actuators
