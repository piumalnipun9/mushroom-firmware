#include "Actuators.h"
#include <Arduino.h>

namespace Actuators
{

    void begin()
    {
        // Initialize PWM pins, relays, etc.
    }

    void setLightIntensity(int percent)
    {
        Serial.printf("Actuator: set light intensity to %d%%\n", percent);
    }

    void setHumidifier(bool on)
    {
        Serial.printf("Actuator: humidifier %s\n", on ? "ON" : "OFF");
    }

    void setExhaustFan(bool on)
    {
        Serial.printf("Actuator: exhaust fan %s\n", on ? "ON" : "OFF");
    }

} // namespace Actuators
