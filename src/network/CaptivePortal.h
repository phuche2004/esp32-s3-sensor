#pragma once
#include <Arduino.h>
#include <WebServer.h>
#include <DNSServer.h>
#include "Config.h"
#include "WiFiService.h"
#include "StorageManager.h"
#include "../drivers/RgbController.h"
#include "../drivers/PowerManager.h"

class CaptivePortal {
public:
    CaptivePortal(WiFiService &wifiService, StorageManager &storage, RgbController &rgbController, SensorSettings &sensorSettings, PowerManager &powerManager);

    void init();
    void loop();
    void updateSensorData(float temp, float hum, bool valid);

private:
    WiFiService &wifiService;
    StorageManager &storage;
    RgbController &rgbController;
    SensorSettings &sensorSettings;
    PowerManager &powerManager;
    WebServer server;
    DNSServer dnsServer;
    bool active;

    float currentTemp = 0.0f;
    float currentHum = 0.0f;
    bool sensorValid = false;
    bool countdownTriggered = false;

    void setupRoutes();
    void handleRoot();
    void handleSave();
    void handleScan();
    void handleStatus();
    void handleSensor();
    void handleLedGet();
    void handleLedSet();
    void handleLedReset();
    void handlePowerGet();
    void handlePowerSet();
    void handleSensorConfigGet();
    void handleSensorConfigSet();
    void handleShutdownAP();
    void handleResetWifi();
    void handleStressTest();
    void handleHistoryGet();
    void handleHistoryCsv();
    void handleSyncTime();
};
