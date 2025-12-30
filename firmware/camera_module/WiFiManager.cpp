#include "WiFiManager.h"
#include <WiFi.h>

namespace WiFiManagerMod
{

    void begin(const char *ssid, const char *password, unsigned long timeout_ms)
    {
        Serial.print("Connecting to WiFi");
        WiFi.begin(ssid, password);
        unsigned long start = millis();
        while (WiFi.status() != WL_CONNECTED && (millis() - start) < timeout_ms)
        {
            delay(250);
            Serial.print('.');
        }
        if (WiFi.status() == WL_CONNECTED)
        {
            Serial.println();
            Serial.print("WiFi connected, IP: ");
            Serial.println(WiFi.localIP());
        }
        else
        {
            Serial.println();
            Serial.println("WiFi connect timeout");
        }
    }

    bool connected()
    {
        return WiFi.status() == WL_CONNECTED;
    }

} // namespace WiFiManagerMod
