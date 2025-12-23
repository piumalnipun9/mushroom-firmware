#include "Sensors.h"
#include <Arduino.h>

namespace Sensors
{

    void begin()
    {
        randomSeed(analogRead(0));
    }

    float readTemperature()
    {
        return 24.0f + (random(-20, 20) / 10.0f);
    }

    float readHumidity()
    {
        return 80.0f + (random(-50, 50) / 10.0f);
    }

    float readCO2()
    {
        return 800.0f + random(-100, 100);
    }

    float readMoisture()
    {
        return 70.0f + (random(-50, 50) / 10.0f);
    }

    float readPH()
    {
        return 6.5f + (random(-10, 10) / 10.0f);
    }

} // namespace Sensors
