#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include <FS.h>
#include <SPIFFS.h>
#include "config.h"

#define RELAY 21

int state = 0; // สถานะเริ่มต้นของประตู (0 = ล็อก, 1 = ปลดล็อก)

Config config;
AsyncWebServer server(80);

void notFound(AsyncWebServerRequest *request) {
    request->send(404, "text/plain", "Not found");
}

void setup() {
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); // Disable Brownout detector
    Serial.begin(115200);
    // pinMode(REED, INPUT_PULLUP);
    pinMode(RELAY, OUTPUT);
    digitalWrite(RELAY, LOW); // เริ่มต้นล็อกประตู

    // Initialize SPIFFS
    if (!SPIFFS.begin(true)) {  // Mount SPIFFS, formatting if failed
        Serial.println("SPIFFS Mount Failed");
        return;
    }

    // อ่านค่าคอนฟิกจากไฟล์ config.json
    const char *filename = "/config.json"; 
    readConfigFromSPIFFS(filename, config);

    // ตั้งค่า WiFi
    if (!WiFi.config(config.local_IP, config.gateway, config.subnet)) {
        Serial.println("STA Failed to configure");
    }
    WiFi.begin(config.ssid.c_str(), config.password.c_str());
    WiFi.mode(WIFI_STA);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("Connecting....");
    }
    Serial.println("Connected to WiFi");
    Serial.println(WiFi.localIP());

    // Endpoint สำหรับตรวจสอบสถานะ state
    server.on("/get-state", HTTP_GET, [](AsyncWebServerRequest *request) {
        String message = "Current state: " + String(state);
        request->send(200, "text/plain", message);
    });

    // Endpoint สำหรับตั้งค่าการล็อกหรือปลดล็อก
    server.on("/set-state", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (request->hasParam("state")) {
            String stateParam = request->getParam("state")->value();
            int newState = stateParam.toInt();

            if (newState == 1) { // ปลดล็อก
                Serial.println("Unlocking door...");
                digitalWrite(RELAY, HIGH); // ปลดล็อก
                state = 1;
                request->send(200, "text/plain", "Door unlocked");
            } else if (newState == 0) { // ล็อก
                Serial.println("Locking door...");
                digitalWrite(RELAY, LOW); // ล็อก
                state = 0;
                request->send(200, "text/plain", "Door locked");
            } else {
                request->send(400, "text/plain", "Invalid state value (0 or 1 expected)");
            }
        } else {
            request->send(400, "text/plain", "Missing parameter 'state'");
        }
    });

    server.onNotFound(notFound);
    server.begin();
}

void loop() {
    // ไม่มีอะไรใน loop เพราะการควบคุมผ่าน HTTP Request
}
