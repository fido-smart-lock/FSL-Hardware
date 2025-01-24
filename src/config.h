#ifndef SRC_CONFIG_H
#define SRC_CONFIG_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include "SPIFFS.h"

struct Config {
  String ssid;
  String password;
  IPAddress local_IP;
  IPAddress gateway;
  IPAddress subnet;
};

void readConfigFromSPIFFS(const char *filename, Config &config);

#endif