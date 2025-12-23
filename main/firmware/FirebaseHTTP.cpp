#include "FirebaseHTTP.h"
#include <HTTPClient.h>
#include <WiFi.h>

static String g_host = "";
static String g_secret = "";

namespace FirebaseHTTP
{

    void begin(const char *host, const char *secret)
    {
        g_host = String(host);
        g_secret = String(secret);
    }

    static String makeUrl(const String &path)
    {
        String url = g_host;
        if (!url.endsWith("/"))
            url += "/";
        // path expected like "sensors/current.json" or "sensors/current"
        String p = path;
        if (!p.endsWith(".json"))
            p += ".json";
        url += p;
        if (g_secret.length() > 0)
        {
            url += "?auth=" + g_secret;
        }
        return url;
    }

    int put(const String &path, const String &jsonPayload)
    {
        if (WiFi.status() != WL_CONNECTED)
            return -1;
        HTTPClient http;
        String url = makeUrl(path);
        http.begin(url);
        http.addHeader("Content-Type", "application/json");
        int code = http.PUT(jsonPayload);
        http.end();
        return code;
    }

    int post(const String &path, const String &jsonPayload, String *response)
    {
        if (WiFi.status() != WL_CONNECTED)
            return -1;
        HTTPClient http;
        String url = makeUrl(path);
        http.begin(url);
        http.addHeader("Content-Type", "application/json");
        int code = http.POST(jsonPayload);
        if (response && code > 0)
        {
            *response = http.getString();
        }
        http.end();
        return code;
    }

    int get(const String &path, String *response)
    {
        if (WiFi.status() != WL_CONNECTED)
            return -1;
        HTTPClient http;
        String url = makeUrl(path);
        http.begin(url);
        int code = http.GET();
        if (response && code > 0)
        {
            *response = http.getString();
        }
        http.end();
        return code;
    }

} // namespace FirebaseHTTP
