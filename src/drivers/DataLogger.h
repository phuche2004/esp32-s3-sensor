#pragma once
#include <Arduino.h>
#include <FS.h>
#include <LittleFS.h>

// Kich thuoc diem du lieu nhi phan (8 bytes)
#pragma pack(push, 1)
struct HistoryPoint {
    uint32_t timestamp; // Thoi gian Unix epoch hoac giay tu luc boot
    int16_t  temp;      // Nhiet do x 100 (VD: 3425 = 34.25 *C)
    int16_t  hum;       // Do am x 100 (VD: 6540 = 65.40 %)
};
#pragma pack(pop)

class DataLogger {
public:
    static constexpr uint16_t MAX_POINTS = 1440; // 1440 diem = 24 gio (1 diem/phut)

    static DataLogger& getInstance() {
        static DataLogger instance;
        return instance;
    }

    void init();
    void loop();
    void addPoint(float temp, float hum, uint32_t timestamp = 0);
    void syncEpoch(uint32_t currentEpoch);

    // Xuat lich su ra chuoi JSON phuc vu ve bieu do tren Web
    // filterMinutes: 15 (15m), 60 (1h), 180 (3h), 360 (6h), 720 (12h), 1440 (24h), 0 (Toan bo)
    String getHistoryJson(uint16_t filterMinutes = 1440);

    // Xuat ra dinh dang CSV de nguoi dung tai file ve may
    String getCsvString();

    // Luu snapshot vao Flash LittleFS de bao toan du lieu khi mat dien
    bool saveSnapshot();
    bool loadSnapshot();

    uint16_t getCount() const { return this->count; }

private:
    DataLogger();

    HistoryPoint* buffer; // Cap phat tren PSRAM / Heap
    uint16_t head;
    uint16_t count;
    uint32_t lastSampleTime;
    uint32_t lastSnapshotTime;
    bool initialized;
};
