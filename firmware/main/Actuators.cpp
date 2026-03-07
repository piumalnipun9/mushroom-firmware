
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
        // Write the state BEFORE setting pinMode to OUTPUT to prevent
        // the pin from briefly floating LOW and triggering the relay.
        digitalWrite(PIN_HUMIDIFIER, HIGH); // OFF by default
        pinMode(PIN_HUMIDIFIER, OUTPUT);
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
        // Always write HIGH just in case the physical pin state drifted
        digitalWrite(PIN_HUMIDIFIER, HIGH);
        
        if (!humidifierOn) return; // avoid duplicate serial prints
        humidifierOn = false;
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


