#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include "esp_wpa2.h"
#include "StorageManager.h"

enum WiFiState {
    WIFI_IDLE,
    WIFI_CONNECTING,
    WIFI_CONNECTED,
    WIFI_AP_PORTAL
};

enum WiFiConnectResult {
    RESULT_IDLE,
    RESULT_CONNECTING,
    RESULT_SUCCESS,
    RESULT_FAILED
};

class WiFiService {
public:
    WiFiService(StorageManager &storage);

    void init();
    void loop();

    // Thu ket noi vao mang Station (ca Personal hoac Enterprise)
    void connectStation(const String &ssid, const String &password, const String &username, bool isEnterprise, const String &backendUrl = "");

    // Kich hoat che do phat Access Point de nguoi dung cau hinh qua Web
    void startAP();

    // Tat Access Point sau khi da ket noi duoc WiFi
    void stopAP();

    // Kich hoat dem nguoc tat AP tu thoi diem Web nhan ket qua success
    void triggerShutdownCountdown(uint32_t delayMs = 3000);

    bool isConnected() const;
    bool isAPMode() const;
    String getIP() const;
    String getSSID() const;

    WiFiConnectResult getConnectResult() const;
    String getLastError() const;

private:
    StorageManager &storage;
    WiFiState state;
    WiFiConnectResult connectResult;
    String lastError;
    uint32_t connectStartTime;
    uint32_t lastReconnectAttempt;
    uint32_t delayedStopApTime;
    
    String currentSSID;
    String currentPass;
    String currentUser;
    bool currentIsEnterprise;
    String currentBackendUrl;
};
