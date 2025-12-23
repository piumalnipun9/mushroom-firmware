/*
  camera_module.ino

  ESP32-CAM sketch that starts Wi-Fi and serves a single capture endpoint.
  Use an AI-Thinker style ESP32-CAM module or adapt the pinout below to your board.

  Endpoints:
  - /capture  -> returns a single JPEG image (Content-Type: image/jpeg)

  Configure `WIFI_SSID` and `WIFI_PASSWORD` before uploading.

  Notes: this sketch is meant to be uploaded to the dedicated camera ESP32 module.
*/

#include <WiFi.h>
#include "esp_camera.h"
#include <WebServer.h>

// Replace with your network credentials
const char *WIFI_SSID = "YOUR_WIFI_SSID";
const char *WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";

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

WebServer server(80);

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
    String page = "<html><body><h3>ESP32-CAM</h3><img src=\"/capture\" style=\"max-width:100%\"></body></html>";
    server.send(200, "text/html", page);
}

void startCameraServer()
{
    server.on("/", HTTP_GET, handleRoot);
    server.on("/capture", HTTP_GET, handleCapture);
    server.begin();
}

void setup()
{
    Serial.begin(115200);
    Serial.println();

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

    // init with high specs to capture decent images; adjust if needed
    if (psramFound())
    {
        config.frame_size = FRAMESIZE_UXGA;
        config.jpeg_quality = 10;
        config.fb_count = 2;
    }
    else
    {
        config.frame_size = FRAMESIZE_SVGA;
        config.jpeg_quality = 12;
        config.fb_count = 1;
    }

    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK)
    {
        Serial.printf("Camera init failed with error 0x%x\n", err);
        while (true)
            delay(1000);
    }

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.printf("Connecting to WiFi %s", WIFI_SSID);
    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < 20000)
    {
        delay(250);
        Serial.print('.');
    }
    Serial.println();
    if (WiFi.status() == WL_CONNECTED)
    {
        Serial.print("Connected. IP: ");
        Serial.println(WiFi.localIP());
    }
    else
    {
        Serial.println("WiFi connect timeout");
    }

    startCameraServer();
    Serial.println("Camera server started. Visit http://<ip>/capture");
}

void loop()
{
    server.handleClient();
    delay(2);
}
