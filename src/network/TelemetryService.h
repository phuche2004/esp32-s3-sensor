#pragma once
#include <Arduino.h>
#include <HTTPClient.h>
#include <FS.h>
#include <LittleFS.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include "WiFiService.h"
#include "StorageManager.h"

// Ket qua gui HTTP phan loai ro rang de xu ly (Chong Deadlock 4xx)
enum HttpSendResult {
    HTTP_SEND_SUCCESS = 0,     // 2xx: Gui thanh cong
    HTTP_SEND_CLIENT_ERROR,    // 4xx: Loi do payload/auth -> Bo qua batch de tranh deadlock
    HTTP_SEND_RETRYABLE_ERROR  // 5xx hoac timeout (<0): Loi server/mang -> Giu lai de thu lai sau
};

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
    static constexpr size_t MAX_OFFLINE_RECORDS = 3600; // Luu toi da 3600 mau (1 gio mat mang @ 1s) tren Flash
    static constexpr size_t BATCH_CHUNK_SIZE = 50;      // Moi dot Batch Ingestion gui 50 ban ghi de tiet kiem RAM
    static constexpr size_t PSRAM_BUFFER_CAPACITY = 60; // Gom 60 mau tren PSRAM truoc khi flush LittleFS de chong mon Flash

    TelemetryService(WiFiService &wifiService, StorageManager &storage);
    ~TelemetryService();

    void init();

    // Day payload vao FreeRTOS Queue phi chan (Non-blocking), goi truc tiep tu Core 1
    bool pushPayload(const TelemetryPayload &data);

    // Tuong thich nguoc: chuyen tiep sang pushPayload
    void sendData(TelemetryPayload data);
    void sendData(float temperature, float humidity);

    // Lay Device ID duy nhat tu MAC Hardware
    String getDeviceId();

    // Lay so thu tu goi tin hien tai
    uint32_t getSequenceNumber() const;

    // So luong ban ghi dang luu ben vung tren Flash
    size_t getFlashBacklogCount();

    // So luong ban ghi dang cho tren bo dem PSRAM
    size_t getPsramBufferCount() const;

private:
    WiFiService &wifiService;
    StorageManager &storage;
    uint32_t packetSeq;
    String cachedDeviceId;
    bool littleFsMounted;

    // FreeRTOS Task & Queue (Chay doc lap tren Core 0)
    QueueHandle_t telemetryQueue;
    TaskHandle_t telemetryTaskHandle;
    static void telemetryTaskStatic(void *pvParameters);
    void telemetryTaskLoop();

    // PSRAM Buffer chong mon Flash
    TelemetryRecord *psramBuffer;
    size_t psramCount;
    bool psramAvailable;

    bool appendRecordToBuffer(const TelemetryRecord &rec);
    bool flushPsramToFlash();
    bool appendRecordToFlash(const TelemetryRecord &rec);
    void pruneOldRecordsFromFlash(size_t recordsToRemove);
    bool flushBatchFromFlash(const String &backendUrl);
    HttpSendResult sendHttpPayload(const String &payload, const String &backendUrl, bool isBatch, size_t count, uint32_t seq);
    void syncTimeFromServer(const String &responseBody);
    void processPayloadInternal(TelemetryPayload data);
};
