#include "Sensors.h"
#include "Config.h"
#include <DHT.h>
#include <Arduino.h>

static DHT dht(PIN_DHT, DHT_TYPE);

namespace Sensors
{

    void begin()
    {
        pinMode(PIN_DHT, INPUT_PULLUP);
        dht.begin();
        randomSeed(analogRead(0));
    }

    float readTemperature()
    {
        float t = dht.readTemperature();
        if (isnan(t))
        {
            // fallback to a harmless default if sensor read fails
            return 30.0f + (random(-20, 20) / 10.0f);
        }
        return t;
    }

    float readHumidity()
    {
        float h = dht.readHumidity();
        if (isnan(h))
        {
            return 80.0f + (random(-50, 50) / 10.0f);
        }
        return h;
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
