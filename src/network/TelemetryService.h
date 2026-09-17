#pragma once
#include <Arduino.h>
#include <HTTPClient.h>
#include "WiFiService.h"
#include "StorageManager.h"

class TelemetryService {
public:
    TelemetryService(WiFiService &wifiService, StorageManager &storage);

    // Gui du lieu nhiet do & do am len backend (chi gui khi WiFi da ket noi)
    void sendData(float temperature, float humidity);

private:
    WiFiService &wifiService;
    StorageManager &storage;
};
