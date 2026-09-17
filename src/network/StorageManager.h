#pragma once
#include <Arduino.h>
#include <Preferences.h>

class StorageManager {
public:
    StorageManager();

    bool init();

    // Doc cau hinh tu NVS
    void loadCredentials(String &ssid, String &password, String &username, bool &isEnterprise, String &backendUrl);

    // Luu cau hinh vao NVS
    void saveCredentials(const String &ssid, const String &password, const String &username, bool isEnterprise, const String &backendUrl);

    // Doc/Ghi cau hinh LED RGB
    void loadLedConfig(uint8_t &mode, uint8_t &r, uint8_t &g, uint8_t &b, uint8_t &brightness, uint8_t &speed);
    void saveLedConfig(uint8_t mode, uint8_t r, uint8_t g, uint8_t b, uint8_t brightness, uint8_t speed);

    // Doc/Ghi cau hinh nguong canh bao va chu ky cam bien
    void loadSensorSettings(float &tempAlert, float &humAlert, uint32_t &readIntervalSec, uint32_t &sendIntervalSec);
    void saveSensorSettings(float tempAlert, float humAlert, uint32_t readIntervalSec, uint32_t sendIntervalSec);

    // Kiem tra da tung luu WiFi chua
    bool hasCredentials();

    // Xoa toan bo cau hinh (Reset ve mac dinh)
    void clear();

private:
    Preferences prefs;
    const char* NAMESPACE = "wifi_cfg";
};
