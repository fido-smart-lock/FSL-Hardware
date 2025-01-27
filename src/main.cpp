#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include <FS.h>
#include <SPIFFS.h>
#include "config.h"

#define RELAY 21

int state = 0; // Start with door locked
bool doorUnlocking = false;
unsigned long unlockStartTime = 0; // Track when the door was unlocked
const unsigned long unlockDuration = 10000; // Unlock duration in milliseconds

Config config;
AsyncWebServer server(80);

void notFound(AsyncWebServerRequest *request) {
    request->send(404, "text/plain", "Not found");
}

void setup() {
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); // Disable Brownout detector
    Serial.begin(115200);
    pinMode(RELAY, OUTPUT);
    digitalWrite(RELAY, LOW); // Start with door locked

    // Initialize SPIFFS
    if (!SPIFFS.begin(true)) {  // Mount SPIFFS, formatting if failed
        Serial.println("SPIFFS Mount Failed");
        return;
    }

    // Read config.json
    const char *filename = "/config.json"; 
    readConfigFromSPIFFS(filename, config);

    // Setting WiFi
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

    // Endpoint for checking the state of the door
    server.on("/get-state", HTTP_GET, [](AsyncWebServerRequest *request) {
        String message = "Current state: " + String(state);
        request->send(200, "text/plain", message);
    });

    // Endpoint for unlocking the door
    server.on("/set-state", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (!doorUnlocking) {
            Serial.println("Unlocking door...");
            doorUnlocking = true;
            unlockStartTime = millis();
            digitalWrite(RELAY, HIGH); // Unlock door
            request->send(200, "text/plain", "Door unlocked");
        } else {
            request->send(200, "text/plain", "Door already unlocked");
        }
    });

    server.onNotFound(notFound);
    server.begin();
}

void loop() {
    // Handle door unlock timer
    if (doorUnlocking) {
        if (millis() - unlockStartTime >= unlockDuration) {
            Serial.println("Locking door...");
            digitalWrite(RELAY, LOW); // Lock door
            doorUnlocking = false;
        }
    }
}
