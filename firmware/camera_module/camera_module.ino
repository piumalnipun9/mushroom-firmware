/*
  camera_module.ino

  ESP32-CAM sketch that starts Wi-Fi and serves a camera stream.
  Use an AI-Thinker style ESP32-CAM module or adapt the pinout below to your board.

  Endpoints:
  - /         -> HTML page with embedded stream
  - /capture  -> returns a single JPEG image (Content-Type: image/jpeg)
  - /stream   -> MJPEG stream (port 81)

  When IP changes, the stream URL is automatically pushed to Firebase:
  camera/cameraUrl -> "http://<IP>:81/stream"

  Configure `WIFI_SSID` and `WIFI_PASSWORD` before uploading.

  Notes: this sketch is meant to be uploaded to the dedicated camera ESP32 module.
*/

#include <Arduino.h>
#include <WiFi.h>
#include "esp_camera.h"
#include <WebServer.h>
#include <HTTPClient.h>
#include "esp_http_server.h"
#include <ArduinoJson.h>

// WiFi credentials
const char *WIFI_SSID = "Pixel 6a";
const char *WIFI_PASSWORD = "12345678";

// Firebase configuration
const char *FIREBASE_HOST = "https://project-mushroom-2f8a9-default-rtdb.asia-southeast1.firebasedatabase.app/";
const char *FIREBASE_SECRET = "ZT0lSGdPU92LVTESOaXzYd3LOFgWKyVGf3Hm1zXH";

// AI-Thinker ESP32-CAM pins
#define PWDN_GPIO_NUM 32
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM 0
#define SIOD_GPIO_NUM 26
#define SIOC_GPIO_NUM 27

#define Y9_GPIO_NUM 35
#define Y8_GPIO_NUM 34
#define Y7_GPIO_NUM 39
#define Y6_GPIO_NUM 36
#define Y5_GPIO_NUM 21
#define Y4_GPIO_NUM 19
#define Y3_GPIO_NUM 18
#define Y2_GPIO_NUM 5
#define VSYNC_GPIO_NUM 25
#define HREF_GPIO_NUM 23
#define PCLK_GPIO_NUM 22

// On-board flash LED (AI-Thinker module)
#define CAM_LED_PIN 4
#define CAM_LED_CHANNEL 7
#define CAM_LED_FREQ 5000
#define CAM_LED_RESOLUTION 8

WebServer server(80);
httpd_handle_t stream_httpd = NULL;

// Track last published IP to detect changes
String lastPublishedIP = "";
unsigned long lastFrameUploadMs = 0;
float frameUploadFps = 2.0f; // Change to control Base64 update rate (frames per second)
unsigned long lastLightPollMs = 0;
const unsigned long lightPollIntervalMs = 1000; // ms between DB polls for light control
uint8_t lastLedDuty = 0;

unsigned long getFrameUploadIntervalMs()
{
    float fps = frameUploadFps;
    if (fps < 0.1f)
    {
        fps = 0.1f;
    }
    unsigned long interval = (unsigned long)(1000.0f / fps);
    return interval == 0 ? 1 : interval;
}

// ============================================
// Firebase HTTP Helper Functions
// ============================================
int firebasePut(const String &path, const String &jsonPayload)
{
    if (WiFi.status() != WL_CONNECTED)
        return -1;

    HTTPClient http;
    String url = String(FIREBASE_HOST);
    if (!url.endsWith("/"))
        url += "/";
    url += path;
    if (!url.endsWith(".json"))
        url += ".json";
    url += "?auth=" + String(FIREBASE_SECRET);

    http.begin(url);
    http.addHeader("Content-Type", "application/json");
    int code = http.PUT(jsonPayload);

    if (code > 0)
    {
        Serial.printf("[Firebase] PUT %s -> HTTP %d\n", path.c_str(), code);
    }
    else
    {
        Serial.printf("[Firebase] PUT %s -> FAILED (error: %d)\n", path.c_str(), code);
    }

    http.end();
    return code;
}

int firebaseGet(const String &path, String *response)
{
    if (WiFi.status() != WL_CONNECTED)
        return -1;

    HTTPClient http;
    String url = String(FIREBASE_HOST);
    if (!url.endsWith("/"))
        url += "/";
    url += path;
    if (!url.endsWith(".json"))
        url += ".json";
    url += "?auth=" + String(FIREBASE_SECRET);

    http.begin(url);
    int code = http.GET();
    if (code > 0 && response != nullptr)
    {
        *response = http.getString();
    }
    else if (code <= 0)
    {
        Serial.printf("[Firebase] GET %s -> FAILED (error: %d)\n", path.c_str(), code);
    }
    http.end();
    return code;
}
static const char base64Alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

String base64Encode(const uint8_t *data, size_t length)
{
    if (!data || !length)
        return "";

    String encoded;
    encoded.reserve(((length + 2) / 3) * 4);

    for (size_t i = 0; i < length; i += 3)
    {
        uint32_t octet_a = data[i];
        uint32_t octet_b = (i + 1 < length) ? data[i + 1] : 0;
        uint32_t octet_c = (i + 2 < length) ? data[i + 2] : 0;
        uint32_t triple = (octet_a << 16) | (octet_b << 8) | octet_c;

        encoded += base64Alphabet[(triple >> 18) & 0x3F];
        encoded += base64Alphabet[(triple >> 12) & 0x3F];
        encoded += (i + 1 < length) ? base64Alphabet[(triple >> 6) & 0x3F] : '=';
        encoded += (i + 2 < length) ? base64Alphabet[triple & 0x3F] : '=';
    }
    return encoded;
}

void setCameraLedDuty(uint8_t duty)
{
    if (duty > ((1 << CAM_LED_RESOLUTION) - 1))
        duty = (1 << CAM_LED_RESOLUTION) - 1;
    if (duty == lastLedDuty)
        return;
    lastLedDuty = duty;
    ledcWrite(CAM_LED_PIN, duty);
}

void setCameraLedPercent(int percent)
{
    if (percent < 0)
        percent = 0;
    if (percent > 100)
        percent = 100;
    uint8_t duty = (uint8_t)((percent * ((1 << CAM_LED_RESOLUTION) - 1)) / 100);
    setCameraLedDuty(duty);
}

void applyLightControlJson(const String &json)
{
    DynamicJsonDocument doc(256);
    DeserializationError err = deserializeJson(doc, json);
    if (err)
    {
        Serial.println("[Camera] Failed to parse lightControl JSON");
        return;
    }

    String status = doc["status"].as<String>();
    int intensity = doc["intensity"].isNull() ? 0 : doc["intensity"].as<int>();

    if (!status.equalsIgnoreCase("on"))
    {
        intensity = 0;
    }

    setCameraLedPercent(intensity);
}

void pollLightControl()
{
    unsigned long now = millis();
    if (now - lastLightPollMs < lightPollIntervalMs)
        return;
    if (WiFi.status() != WL_CONNECTED)
        return;

    lastLightPollMs = now;

    String resp;
    int code = firebaseGet("lightControl", &resp);
    if (code == 200 && resp.length() > 0)
    {
        applyLightControlJson(resp);
    }
    else if (code > 0)
    {
        Serial.printf("[Camera] lightControl GET failed: %d\n", code);
    }
}

void maybeUploadFrameToRealtimeDB()
{
    unsigned long intervalMs = getFrameUploadIntervalMs();
    if (millis() - lastFrameUploadMs < intervalMs)
        return;
    lastFrameUploadMs = millis();

    if (WiFi.status() != WL_CONNECTED)
        return;

    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb)
    {
        Serial.println("[Camera] Frame grab failed");
        return;
    }

    size_t rawLen = fb->len;
    String encoded = base64Encode(fb->buf, fb->len);
    esp_camera_fb_return(fb);

    if (!encoded.length())
    {
        Serial.println("[Camera] Base64 encoding failed");
        return;
    }

    String payload = "\"" + encoded + "\"";
    int code = firebasePut("camera/frame", payload);
    if (code == 200)
    {
        Serial.printf("[Camera] Uploaded Base64 frame (%u bytes raw)\n", (unsigned)rawLen);
    }
    else
    {
        Serial.printf("[Camera] Base64 frame upload failed: %d\n", code);
    }
}

// ============================================
// Publish Camera URL to Firebase
// ============================================
void publishCameraUrlToFirebase()
{
    if (WiFi.status() != WL_CONNECTED)
    {
        return;
    }

    String currentIP = WiFi.localIP().toString();

    // Only publish if IP changed
    if (currentIP == lastPublishedIP || currentIP == "0.0.0.0")
    {
        return;
    }

    lastPublishedIP = currentIP;

    // Build stream URL - stream runs on port 81
    String streamUrl = "http://" + currentIP + ":81/stream";

    // Firebase expects JSON string value with quotes
    String payload = "\"" + streamUrl + "\"";

    Serial.println("===========================================");
    Serial.println("[Camera] IP changed, updating Firebase...");
    Serial.print("[Camera] New Stream URL: ");
    Serial.println(streamUrl);

    int code = firebasePut("camera/cameraUrl", payload);

    if (code == 200)
    {
        Serial.println("[Camera] Firebase updated successfully!");
    }
    else
    {
        Serial.println("[Camera] Firebase update failed!");
        // Reset so we retry next loop
        lastPublishedIP = "";
    }
    Serial.println("===========================================");
}

// ============================================
// Camera Stream Handler (MJPEG on port 81)
// ============================================
#define PART_BOUNDARY "123456789000000000000987654321"
static const char *_STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char *_STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char *_STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

static esp_err_t stream_handler(httpd_req_t *req)
{
    camera_fb_t *fb = NULL;
    esp_err_t res = ESP_OK;
    char part_buf[64];

    res = httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);
    if (res != ESP_OK)
        return res;

    while (true)
    {
        fb = esp_camera_fb_get();
        if (!fb)
        {
            Serial.println("[Stream] Camera capture failed");
            res = ESP_FAIL;
        }
        else
        {
            size_t hlen = snprintf(part_buf, 64, _STREAM_PART, fb->len);
            res = httpd_resp_send_chunk(req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));
            if (res == ESP_OK)
                res = httpd_resp_send_chunk(req, part_buf, hlen);
            if (res == ESP_OK)
                res = httpd_resp_send_chunk(req, (const char *)fb->buf, fb->len);
            esp_camera_fb_return(fb);
        }

        if (res != ESP_OK)
            break;
    }
    return res;
}

void startStreamServer()
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = 81;

    httpd_uri_t stream_uri = {
        .uri = "/stream",
        .method = HTTP_GET,
        .handler = stream_handler,
        .user_ctx = NULL};

    if (httpd_start(&stream_httpd, &config) == ESP_OK)
    {
        httpd_register_uri_handler(stream_httpd, &stream_uri);
        Serial.println("[Camera] Stream server started on port 81");
    }
}

// ============================================
// Web Server Handlers (port 80)
// ============================================
void handleCapture()
{
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb)
    {
        server.send(500, "text/plain", "Camera capture failed");
        return;
    }

    server.sendHeader("Content-Type", "image/jpeg");
    server.sendHeader("Content-Length", String(fb->len));
    WiFiClient client = server.client();
    client.write(fb->buf, fb->len);
    esp_camera_fb_return(fb);
}

void handleRoot()
{
    String ip = WiFi.localIP().toString();
    String page = "<html><head><title>ESP32-CAM</title>";
    page += "<style>body{font-family:Arial;text-align:center;background:#1a1a2e;color:#fff;padding:20px;}";
    page += "h3{color:#00ff88;}img{max-width:100%;border:3px solid #00ff88;border-radius:10px;}</style></head>";
    page += "<body><h3>ESP32-CAM Stream</h3>";
    page += "<p>Stream URL: <code>http://" + ip + ":81/stream</code></p>";
    page += "<img src=\"http://" + ip + ":81/stream\">";
    page += "<p><a href=\"/capture\" style=\"color:#00ff88;\">Capture Single Image</a></p>";
    page += "</body></html>";
    server.send(200, "text/html", page);
}

void startWebServer()
{
    server.on("/", HTTP_GET, handleRoot);
    server.on("/capture", HTTP_GET, handleCapture);
    server.begin();
    Serial.println("[Camera] Web server started on port 80");
}

// ============================================
// Setup
// ============================================
void setup()
{
    Serial.begin(115200);
    Serial.println();
    Serial.println("===========================================");
    Serial.println("       ESP32-CAM Camera Module");
    Serial.println("===========================================");

    // Camera configuration for AI-Thinker module
    camera_config_t config;
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;
    config.pin_d0 = Y2_GPIO_NUM;
    config.pin_d1 = Y3_GPIO_NUM;
    config.pin_d2 = Y4_GPIO_NUM;
    config.pin_d3 = Y5_GPIO_NUM;
    config.pin_d4 = Y6_GPIO_NUM;
    config.pin_d5 = Y7_GPIO_NUM;
    config.pin_d6 = Y8_GPIO_NUM;
    config.pin_d7 = Y9_GPIO_NUM;
    config.pin_xclk = XCLK_GPIO_NUM;
    config.pin_pclk = PCLK_GPIO_NUM;
    config.pin_vsync = VSYNC_GPIO_NUM;
    config.pin_href = HREF_GPIO_NUM;
    config.pin_sccb_sda = SIOD_GPIO_NUM;
    config.pin_sccb_scl = SIOC_GPIO_NUM;
    config.pin_pwdn = PWDN_GPIO_NUM;
    config.pin_reset = RESET_GPIO_NUM;
    config.xclk_freq_hz = 20000000;
    config.pixel_format = PIXFORMAT_JPEG;
    config.grab_mode = CAMERA_GRAB_LATEST;

    // Init with specs based on PSRAM availability
    if (psramFound())
    {
        config.frame_size = FRAMESIZE_VGA; // 640x480 for smooth streaming
        config.jpeg_quality = 12;
        config.fb_count = 2;
        Serial.println("[Camera] PSRAM found - using VGA resolution");
    }
    else
    {
        config.frame_size = FRAMESIZE_QVGA; // 320x240 for low memory
        config.jpeg_quality = 15;
        config.fb_count = 1;
        Serial.println("[Camera] No PSRAM - using QVGA resolution");
    }

    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK)
    {
        Serial.printf("[Camera] Init failed with error 0x%x\n", err);
        while (true)
            delay(1000);
    }
    Serial.println("[Camera] Camera initialized successfully");

    // Configure on-board LED PWM (default off)
    ledcAttach(CAM_LED_PIN, CAM_LED_FREQ, CAM_LED_RESOLUTION);
    setCameraLedDuty(0);

    // Connect to WiFi
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.printf("[WiFi] Connecting to %s", WIFI_SSID);
    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < 20000)
    {
        delay(250);
        Serial.print('.');
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED)
    {
        Serial.print("[WiFi] Connected! IP: ");
        Serial.println(WiFi.localIP());
    }
    else
    {
        Serial.println("[WiFi] Connection timeout - will retry in loop");
    }

    // Start servers
    startWebServer();
    startStreamServer();

    // Publish camera URL to Firebase
    publishCameraUrlToFirebase();

    Serial.println("===========================================");
    Serial.println("Camera ready!");
    Serial.print("  Web UI: http://");
    Serial.println(WiFi.localIP());
    Serial.print("  Stream: http://");
    Serial.print(WiFi.localIP());
    Serial.println(":81/stream");
    Serial.println("===========================================");
}

// ============================================
// Loop
// ============================================
void loop()
{
    // Handle web server requests
    server.handleClient();

    // Check if IP changed and update Firebase
    publishCameraUrlToFirebase();

    // Periodically push Base64 frame to Realtime DB
    maybeUploadFrameToRealtimeDB();

    // Poll lightControl node to drive camera LED based on database state
    pollLightControl();

    delay(10);
}
