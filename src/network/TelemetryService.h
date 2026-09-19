#pragma once
#include <Arduino.h>
#include <HTTPClient.h>
#include <FS.h>
#include <LittleFS.h>
#include "WiFiService.h"
#include "StorageManager.h"

// Cau truc ban ghi nhi phan luu truc tiep tren Flash LittleFS (toi uu kich thuoc 48 bytes)
struct TelemetryRecord {
    uint32_t seq;
    uint32_t timestamp;
    float temperature;
    float humidity;
    float dewPoint;
    float vpd;
    float chipTemp;
    float cpuLoad;
    float cpu0;
    float cpu1;
    uint32_t freeHeap;
    uint32_t uptimeSec;
    int16_t wifiRssi;
    bool isAlert;
    bool sensorValid;
};

struct TelemetryPayload {
    uint32_t seq = 0;
    uint32_t timestamp = 0;
    float temperature = 0.0f;
    float humidity = 0.0f;
    float dewPoint = 0.0f;
    float vpd = 0.0f;
    float chipTemp = 0.0f;
    float cpuLoad = 0.0f;
    float cpu0 = 0.0f;
    float cpu1 = 0.0f;
    uint32_t freeHeap = 0;
    uint32_t uptimeSec = 0;
    int wifiRssi = 0;
    bool isAlert = false;
    bool sensorValid = true;
};

class TelemetryService {
public:
    static constexpr const char* OFFLINE_FILE = "/telemetry_offline.bin";
    static constexpr size_t MAX_OFFLINE_RECORDS = 3600; // Lưu toi da 3600 mau (1 gio mat mang @ 1s) tren Flash
    static constexpr size_t BATCH_CHUNK_SIZE = 50;      // Moi dot Batch Ingestion gui 50 ban ghi de tiet kiem RAM

    TelemetryService(WiFiService &wifiService, StorageManager &storage);
    ~TelemetryService();

    // Gui du lieu telemetry chuan IIoT (ho tro Batch Ingestion & Flash LittleFS)
    void sendData(TelemetryPayload data);

    // Tuong thich nguoc cho ban tin rut gon
    void sendData(float temperature, float humidity);

    // Lay Device ID duy nhat tu MAC Hardware
    String getDeviceId();

    // Lay so thu tu goi tin hien tai
    uint32_t getSequenceNumber() const;

    // So luong ban ghi dang luu ben vung tren Flash
    size_t getFlashBacklogCount();

private:
    WiFiService &wifiService;
    StorageManager &storage;
    uint32_t packetSeq;
    String cachedDeviceId;
    bool littleFsMounted;

    bool appendRecordToFlash(const TelemetryRecord &rec);
    void pruneOldRecordsFromFlash(size_t recordsToRemove);
    bool flushBatchFromFlash(const String &backendUrl);
    bool sendHttpPayload(const String &payload, const String &backendUrl, bool isBatch, size_t count, uint32_t seq);
};
