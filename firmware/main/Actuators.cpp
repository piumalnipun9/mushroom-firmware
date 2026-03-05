
// Humidifier state machine implementation
#include "Actuators.h"
#include "Config.h"
#include <Arduino.h>

namespace Actuators
{
    // Current humidifier mode state
    static HumidifierMode currentMode = MODE_OFF;

    // ISR for detecting falling edge on humidifier pin
    static void IRAM_ATTR humidifierISR()
    {
        // Cycle through modes: OFF -> SLOW -> FAST -> OFF
        currentMode = (HumidifierMode)((currentMode + 1) % 3);
        Serial.printf("[Actuators] Humidifier mode changed to: ");
        switch (currentMode)
        {
        case MODE_OFF:
            Serial.println("OFF");
            break;
        case MODE_SLOW:
            Serial.println("SLOW");
            break;
        case MODE_FAST:
            Serial.println("FAST");
            break;
        }
    }

    void begin()
    {
        // Configure humidifier pin for input with internal pull-up
        // Detect falling edges (LOW transitions)
        pinMode(PIN_HUMIDIFIER, INPUT_PULLUP);
        attachInterrupt(digitalPinToInterrupt(PIN_HUMIDIFIER), humidifierISR, FALLING);

        currentMode = MODE_OFF;
        Serial.printf("[Actuators] Humidifier initialized on pin %d (state machine)\n", PIN_HUMIDIFIER);
    }

    HumidifierMode getHumidifierMode()
    {
        return currentMode;
    }

    void setHumidifierMode(HumidifierMode mode)
    {
        if (mode >= 0 && mode < 3)
        {
            currentMode = mode;
            Serial.printf("[Actuators] Humidifier mode set to: %d\n", mode);
        }
    }

    void setExhaustFan(bool on)
    {
        Serial.printf("[Actuators] Exhaust fan %s\n", on ? "ON" : "OFF");
    }

} // namespace Actuators
