#pragma once
#include <Arduino.h>

namespace SDLogger
{
    bool begin();

    void logSensorReading(int plotId, unsigned long unixMs,
                          float temp, float hum, float co2,
                          float moist, float ph);
}
