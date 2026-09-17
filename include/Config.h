#pragma once
#include <Arduino.h>

// ==========================================
// CAU HINH PHAN CUNG (HARDWARE PINOUT)
// ==========================================
constexpr uint8_t PIN_SDA    = 8;
constexpr uint8_t PIN_SCL    = 9;
constexpr uint8_t PIN_BUZZER = 10;
constexpr uint8_t PIN_LED     = 13;
constexpr uint8_t PIN_RGB_LED = 48;   // LED RGB WS2812 tren bo YD-ESP32-S3
constexpr uint8_t PIN_BOOT    = 0;    // Nut BOOT mac dinh tren bo ESP32-S3
constexpr uint32_t BOOT_HOLD_RESET_MS = 1000; // Nhan giu 1 giay de xoa WiFi

// ==========================================
// CAU HINH CAM BIEN SHT31
// ==========================================
constexpr uint8_t SHT31_I2C_ADDR = 0x44;

// ==========================================
// NGUONG CANH BAO (ALERT THRESHOLDS)
// ==========================================
constexpr float TEMP_ALERT_THRESHOLD = 37.0f; // Nhiet do vuot 37 *C -> canh bao
constexpr float HUM_ALERT_THRESHOLD  = 87.0f; // Do am vuot 87 % -> canh bao

// ==========================================
// CHU KY HOAT DONG (TIMING)
// ==========================================
constexpr uint32_t SENSOR_READ_INTERVAL_MS = 1000; // Doc moi 1 giay (non-blocking)
constexpr uint32_t TELEMETRY_INTERVAL_MS   = 5000; // Ban du lieu len backend moi 5 giay
constexpr uint32_t SERIAL_BAUD_RATE        = 115200;

// ==========================================
// CAU TRUC DU LIEU CAU HINH CAM BIEN DONG
// ==========================================
struct SensorSettings {
    float tempAlert = TEMP_ALERT_THRESHOLD;
    float humAlert = HUM_ALERT_THRESHOLD;
    uint32_t readIntervalSec = 1;
    uint32_t sendIntervalSec = 5;
};

// ==========================================
// CAU HINH ACCESS POINT & CAPTIVE PORTAL
// ==========================================
constexpr const char* AP_SSID_DEFAULT      = "ESP32-S3";
constexpr const char* AP_PASSWORD_DEFAULT  = ""; // Khong dat mat khau cho AP cai dat
constexpr uint32_t WIFI_CONNECT_TIMEOUT_MS = 15000; // Thu ket noi trong 15s, neu fail thi bat AP

// ==========================================
// CAU HINH BACKEND TELEMETRY
// ==========================================
constexpr const char* DEFAULT_BACKEND_URL  = "http://172.16.10.169:3000/api/sensor";

// ==========================================
// CAU HINH TEN MIEN NOI BO (mDNS)
// ==========================================
constexpr const char* MDNS_HOSTNAME        = "esp"; // Truy cap qua http://esp.local

