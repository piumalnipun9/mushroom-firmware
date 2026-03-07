#include "SDLogger.h"
#include "Config.h"
#include <Arduino.h>
#include <SPI.h>
#include <SD.h>

namespace SDLogger
{
    static const char* CSV_FILE = "/sensors.csv";
    static bool sdReady = false;

    bool begin()
    {
        // Use custom SPI pins for the SD card
        SPI.begin(PIN_SD_SCK, PIN_SD_MISO, PIN_SD_MOSI, PIN_SD_CS);

        if (!SD.begin(PIN_SD_CS))
        {
            Serial.println("[SDLogger] SD card init failed — check wiring/card.");
            sdReady = false;
            return false;
        }

        sdReady = true;
        Serial.println("[SDLogger] SD card ready.");

        // Write CSV header if the file doesn't exist yet
        if (!SD.exists(CSV_FILE))
        {
            File f = SD.open(CSV_FILE, FILE_WRITE);
            if (f)
            {
                f.println("unix_timestamp_ms,plot_id,temperature_c,humidity_pct,co2_ppm,soil_moisture_pct,soil_ph");
                f.close();
                Serial.println("[SDLogger] Created sensors.csv with header.");
            }
            else
            {
                Serial.println("[SDLogger] WARNING: Could not create sensors.csv");
            }
        }

        return true;
    }

    void logSensorReading(int plotId, unsigned long unixMs,
                          float temp, float hum, float co2,
                          float moist, float ph)
    {
        if (!sdReady)
        {
            Serial.println("[SDLogger] Skipping log — SD not ready.");
            return;
        }

        File f = SD.open(CSV_FILE, FILE_APPEND);
        if (!f)
        {
            Serial.println("[SDLogger] ERROR: Could not open sensors.csv for appending.");
            return;
        }

        // Format: unix_ms,plot,temp,hum,co2,moist,ph
        f.print(unixMs);
        f.print(',');
        f.print(plotId);
        f.print(',');
        f.print(temp, 2);
        f.print(',');
        f.print(hum, 2);
        f.print(',');
        f.print(co2, 1);
        f.print(',');
        f.print(moist, 2);
        f.print(',');
        f.println(ph, 2);

        f.close();

        Serial.printf("[SDLogger] Logged plot=%d, t=%.1f, h=%.1f, co2=%.0f, moist=%.1f, ph=%.2f\n",
                      plotId, temp, hum, co2, moist, ph);
    }

} // namespace SDLogger
