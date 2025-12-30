
// Simplified Actuators implementation using analogWrite for PWM
#include "Actuators.h"
#include "Config.h"
#include <Arduino.h>

namespace Actuators
{

    void begin()
    {
        // Use simple analogWrite-based PWM. Ensure pin is output.
        pinMode(PIN_LIGHT, OUTPUT);
        setLightIntensity(0);
        Serial.printf("Actuators: PWM (analogWrite) on pin %d\n", PIN_LIGHT);
    }

    void setLightIntensity(int percent)
    {
        if (percent < 0)
            percent = 0;
        if (percent > 100)
            percent = 100;

        // Map percent to 0-255 duty for analogWrite
        int duty = map(percent, 0, 100, 0, 255);
        analogWrite(PIN_LIGHT, duty);
        Serial.printf("Actuator: set light intensity to %d%% (duty=%d)\n", percent, duty);
    }

    void setHumidifier(bool on)
    {
        // Keep minimal stub for compatibility
        Serial.printf("Actuator: humidifier %s\n", on ? "ON" : "OFF");
    }

    void setExhaustFan(bool on)
    {
        Serial.printf("Actuator: exhaust fan %s\n", on ? "ON" : "OFF");
    }

} // namespace Actuators
