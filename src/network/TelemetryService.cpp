#include "TelemetryService.h"
#include <WiFi.h>
#include <time.h>

TelemetryService::TelemetryService(WiFiService &wifiService, StorageManager &storage)
    : wifiService(wifiService), storage(storage), packetSeq(0), cachedDeviceId(""), littleFsMounted(false) {}

TelemetryService::~TelemetryService() {}

void TelemetryService::init() {
    this->littleFsMounted = LittleFS.begin(false);
    if (this->littleFsMounted) {
        size_t count = this->getFlashBacklogCount();
        if (count > 0) {
            Serial.printf("[Telemetry-Flash] Tim thay %u ban ghi offline ton dong tren Flash tu truoc!\n", (unsigned int)count);
        } else {
            Serial.println("[Telemetry-Flash] LittleFS san sang (Chua co ban ghi offline nao).");
        }
    } else {
        Serial.println("[Telemetry-Flash] LOI: LittleFS chua duoc mount!");
    }
}

String TelemetryService::getDeviceId() {
    if (this->cachedDeviceId.length() == 0) {
        uint8_t mac[6];
        WiFi.macAddress(mac);
        char id[24];
        snprintf(id, sizeof(id), "ESP32S3_%02X%02X%02X%02X%02X%02X",
                 mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        this->cachedDeviceId = String(id);
    }
    return this->cachedDeviceId;
}

uint32_t TelemetryService::getSequenceNumber() const {
    return this->packetSeq;
}

size_t TelemetryService::getFlashBacklogCount() {
    if (!this->littleFsMounted || !LittleFS.exists(OFFLINE_FILE)) {
        return 0;
    }
    File f = LittleFS.open(OFFLINE_FILE, "rb");
    if (!f) return 0;
    size_t count = f.size() / sizeof(TelemetryRecord);
    f.close();
    return count;
}

void TelemetryService::pruneOldRecordsFromFlash(size_t recordsToRemove) {
    if (!this->littleFsMounted || !LittleFS.exists(OFFLINE_FILE)) return;

    File f = LittleFS.open(OFFLINE_FILE, "rb");
    if (!f) return;

    size_t totalBytes = f.size();
    size_t totalRecords = totalBytes / sizeof(TelemetryRecord);
    if (totalRecords <= recordsToRemove) {
        f.close();
        LittleFS.remove(OFFLINE_FILE);
        return;
    }

    // Bo qua recordsToRemove ban ghi cu nhat o dau file theo dung nguyen ly FIFO
    f.seek(recordsToRemove * sizeof(TelemetryRecord), SeekSet);

    const char* TEMP_FILE = "/telemetry_fifo.bin";
    File temp = LittleFS.open(TEMP_FILE, "wb");
    if (temp) {
        uint8_t buf[256];
        while (f.available()) {
            size_t n = f.read(buf, sizeof(buf));
            temp.write(buf, n);
        }
        temp.close();
        f.close();
        LittleFS.remove(OFFLINE_FILE);
        LittleFS.rename(TEMP_FILE, OFFLINE_FILE);
        Serial.printf("[Telemetry-FIFO] Da cat bo %u ban ghi cu nhat (Duy tri dung luong Flash luon an toan, khong bao gio tran)!\n",
                      (unsigned int)recordsToRemove);
    } else {
        f.close();
    }
}

bool TelemetryService::appendRecordToFlash(const TelemetryRecord &rec) {
    if (!this->littleFsMounted) return false;

    // Co che FIFO: Kiem tra neu vuot nguong MAX_OFFLINE_RECORDS (3600 mau = 1 gio @ 1s)
    // hoac dung luong Flash con trong duoi 300KB thi cat bo 100 ban ghi cu nhat o dau file
    size_t currentCount = this->getFlashBacklogCount();
    size_t freeBytes = LittleFS.totalBytes() - LittleFS.usedBytes();

    if (currentCount >= MAX_OFFLINE_RECORDS || freeBytes < 300000) {
        this->pruneOldRecordsFromFlash(100);
    }

    File f = LittleFS.open(OFFLINE_FILE, "ab");
    if (!f) {
        Serial.println("[Telemetry-Flash] LOI: Khong the mo file de ghi!");
        return false;
    }
    size_t written = f.write((const uint8_t *)&rec, sizeof(TelemetryRecord));
    f.close();
    return (written == sizeof(TelemetryRecord));
}

bool TelemetryService::sendHttpPayload(const String &payload, const String &backendUrl, bool isBatch, size_t count, uint32_t seq) {
    String deviceId = this->getDeviceId();

    // Timeout 1200ms cho batch de backend co du thoi gian parse mang JSON
    HTTPClient http;
    http.setTimeout(isBatch ? 1500 : 800);
    http.setReuse(true);

    bool success = false;
    if (http.begin(backendUrl)) {
        http.addHeader("Content-Type", "application/json");
        http.addHeader("User-Agent", "ESP32-S3-Sensor/2.0");
        http.addHeader("X-Device-ID", deviceId);
        http.addHeader("X-Batch-Mode", isBatch ? "true" : "false");
        if (isBatch) {
            http.addHeader("X-Batch-Count", String(count));
        } else {
            http.addHeader("X-Packet-Seq", String(seq));
        }

        int httpCode = http.POST(payload);
        if (httpCode >= 200 && httpCode < 300) {
            success = true;
        } else {
            Serial.printf("[Telemetry] Gui goi %s that bai -> HTTP %d (%s)\n",
                          isBatch ? "BATCH" : ("#" + String(seq)).c_str(),
                          httpCode, http.errorToString(httpCode).c_str());
        }
        http.end();
    } else {
        Serial.println("[Telemetry] Khong the khoi tao ket noi toi Backend URL!");
    }
    return success;
}

bool TelemetryService::flushBatchFromFlash(const String &backendUrl) {
    if (!this->littleFsMounted || !LittleFS.exists(OFFLINE_FILE)) {
        return true;
    }

    File f = LittleFS.open(OFFLINE_FILE, "rb");
    if (!f) return false;

    size_t totalBytes = f.size();
    size_t totalRecords = totalBytes / sizeof(TelemetryRecord);
    if (totalRecords == 0) {
        f.close();
        LittleFS.remove(OFFLINE_FILE);
        return true;
    }

    // Doc toi da BATCH_CHUNK_SIZE ban ghi moi lan gui
    size_t recordsToRead = (totalRecords > BATCH_CHUNK_SIZE) ? BATCH_CHUNK_SIZE : totalRecords;
    TelemetryRecord chunk[BATCH_CHUNK_SIZE];
    size_t actualRead = f.read((uint8_t *)chunk, recordsToRead * sizeof(TelemetryRecord)) / sizeof(TelemetryRecord);

    if (actualRead == 0) {
        f.close();
        return false;
    }

    // Dong goi mang JSON Batch Ingestion
    String json;
    json.reserve(actualRead * 280 + 120);
    json = "{";
    json += "\"device_id\":\"" + this->getDeviceId() + "\",";
    json += "\"batch\":true,";
    json += "\"count\":" + String(actualRead) + ",";
    json += "\"records\":[";

    for (size_t i = 0; i < actualRead; i++) {
        if (i > 0) json += ",";
        json += "{";
        json += "\"timestamp\":" + String(chunk[i].timestamp) + ",";
        json += "\"seq\":" + String(chunk[i].seq) + ",";
        json += "\"metrics\":{";
        json += "\"temperature\":" + String(chunk[i].temperature, 2) + ",";
        json += "\"humidity\":" + String(chunk[i].humidity, 2) + ",";
        json += "\"dew_point\":" + String(chunk[i].dewPoint, 2) + ",";
        json += "\"vpd\":" + String(chunk[i].vpd, 2);
        json += "},";
        json += "\"diagnostics\":{";
        json += "\"chip_temp\":" + String(chunk[i].chipTemp, 1) + ",";
        json += "\"cpu_load\":" + String(chunk[i].cpuLoad, 1) + ",";
        json += "\"cpu0\":" + String(chunk[i].cpu0, 1) + ",";
        json += "\"cpu1\":" + String(chunk[i].cpu1, 1) + ",";
        json += "\"free_heap\":" + String(chunk[i].freeHeap) + ",";
        json += "\"uptime_sec\":" + String(chunk[i].uptimeSec) + ",";
        json += "\"wifi_rssi\":" + String(chunk[i].wifiRssi);
        json += "},";
        json += "\"status\":{";
        json += "\"alert\":" + String(chunk[i].isAlert ? "true" : "false") + ",";
        json += "\"sensor_valid\":" + String(chunk[i].sensorValid ? "true" : "false");
        json += "}";
        json += "}";
    }
    json += "]}";

    // Gui Batch Ingestion len Backend
    bool ok = this->sendHttpPayload(json, backendUrl, true, actualRead, 0);
    if (!ok) {
        f.close();
        return false;
    }

    Serial.printf("[Batch-Ingestion] Da gui bu thanh cong dot %u ban ghi Flash len Backend!\n", (unsigned int)actualRead);

    // Xu ly phan con lai trong file
    size_t remainingRecords = totalRecords - actualRead;
    if (remainingRecords == 0) {
        f.close();
        LittleFS.remove(OFFLINE_FILE);
        Serial.println("[Batch-Ingestion] DA XA SACH TOAN BO DU LIEU OFFLINE TREN FLASH!");
    } else {
        // Doc phan con lai va ghi vao file tam
        const char* TEMP_FILE = "/telemetry_temp.bin";
        File tempFile = LittleFS.open(TEMP_FILE, "wb");
        if (tempFile) {
            uint8_t buffer[256];
            while (f.available()) {
                size_t n = f.read(buffer, sizeof(buffer));
                tempFile.write(buffer, n);
            }
            tempFile.close();
            f.close();
            LittleFS.remove(OFFLINE_FILE);
            LittleFS.rename(TEMP_FILE, OFFLINE_FILE);
            Serial.printf("[Batch-Ingestion] Con lai %u ban ghi tren Flash, se tiep tuc gui dot tiep theo.\n", (unsigned int)remainingRecords);
        } else {
            f.close();
        }
    }
    return true;
}

void TelemetryService::sendData(TelemetryPayload data) {
    // 1. Gan Sequence Number va Timestamp thoi gian thuc ngay tai thoi diem do nay
    this->packetSeq++;
    data.seq = this->packetSeq;

    time_t nowEpoch = time(nullptr);
    data.timestamp = (nowEpoch > 100000) ? (uint32_t)nowEpoch : (millis() / 1000);

    String dummySSID, dummyPass, dummyUser, backendUrl;
    bool dummyIsEnt = false;
    this->storage.loadCredentials(dummySSID, dummyPass, dummyUser, dummyIsEnt, backendUrl);

    if (backendUrl.length() == 0) {
        return;
    }

    // Chuan bi san ban ghi TelemetryRecord de phong truong hop can luu Flash
    TelemetryRecord rec;
    rec.seq = data.seq;
    rec.timestamp = data.timestamp;
    rec.temperature = data.temperature;
    rec.humidity = data.humidity;
    rec.dewPoint = data.dewPoint;
    rec.vpd = data.vpd;
    rec.chipTemp = data.chipTemp;
    rec.cpuLoad = data.cpuLoad;
    rec.cpu0 = data.cpu0;
    rec.cpu1 = data.cpu1;
    rec.freeHeap = data.freeHeap;
    rec.uptimeSec = data.uptimeSec;
    rec.wifiRssi = (int16_t)data.wifiRssi;
    rec.isAlert = data.isAlert;
    rec.sensorValid = data.sensorValid;

    // 2. Kiem tra tinh trang ket noi Wi-Fi
    if (!this->wifiService.isConnected()) {
        // Mat Wi-Fi -> Ghi ben vung vao Flash LittleFS ngay lap tuc
        this->appendRecordToFlash(rec);
        Serial.printf("[Telemetry] Mat Wi-Fi -> Da luu goi #%u vao Flash LittleFS (Dang luu: %u ban ghi)\n",
                      data.seq, (unsigned int)this->getFlashBacklogCount());
        return;
    }

    // 3. Neu Wi-Fi dang ket noi va co ban ghi offline tren Flash: Gui Batch Ingestion de xa sach truoc
    if (this->getFlashBacklogCount() > 0) {
        this->flushBatchFromFlash(backendUrl);
    }

    // 4. Dong goi ban tin don le thoi gian thuc hien tai
    String payload;
    payload.reserve(380);
    payload = "{";
    payload += "\"device_id\":\"" + this->getDeviceId() + "\",";
    payload += "\"batch\":false,";
    payload += "\"count\":1,";
    payload += "\"timestamp\":" + String(data.timestamp) + ",";
    payload += "\"seq\":" + String(data.seq) + ",";
    payload += "\"metrics\":{";
    payload += "\"temperature\":" + String(data.temperature, 2) + ",";
    payload += "\"humidity\":" + String(data.humidity, 2) + ",";
    payload += "\"dew_point\":" + String(data.dewPoint, 2) + ",";
    payload += "\"vpd\":" + String(data.vpd, 2);
    payload += "},";
    payload += "\"diagnostics\":{";
    payload += "\"chip_temp\":" + String(data.chipTemp, 1) + ",";
    payload += "\"cpu_load\":" + String(data.cpuLoad, 1) + ",";
    payload += "\"cpu0\":" + String(data.cpu0, 1) + ",";
    payload += "\"cpu1\":" + String(data.cpu1, 1) + ",";
    payload += "\"free_heap\":" + String(data.freeHeap) + ",";
    payload += "\"uptime_sec\":" + String(data.uptimeSec) + ",";
    payload += "\"wifi_rssi\":" + String(data.wifiRssi);
    payload += "},";
    payload += "\"status\":{";
    payload += "\"alert\":" + String(data.isAlert ? "true" : "false") + ",";
    payload += "\"sensor_valid\":" + String(data.sensorValid ? "true" : "false");
    payload += "}";
    payload += "}";

    // Gui goi tin thoi gian thuc
    bool ok = this->sendHttpPayload(payload, backendUrl, false, 1, data.seq);
    if (ok) {
        Serial.printf("[Telemetry] #%u | T: %.2f*C | H: %.2f%% | RSSI: %d dBm -> HTTP 200 (Flash Backlog: %u)\n",
                      data.seq, data.temperature, data.humidity, data.wifiRssi, (unsigned int)this->getFlashBacklogCount());
    } else {
        // Neu server loi hoac mang chap chon, ghi luon goi tin vao Flash de khong bao gio bi mat!
        this->appendRecordToFlash(rec);
        Serial.printf("[Telemetry] Gui that bai -> Da luu goi #%u vao Flash LittleFS de gui lai sau (Flash: %u)\n",
                      data.seq, (unsigned int)this->getFlashBacklogCount());
    }
}

void TelemetryService::sendData(float temperature, float humidity) {
    TelemetryPayload payload;
    payload.temperature = temperature;
    payload.humidity = humidity;
    payload.sensorValid = true;
    payload.uptimeSec = millis() / 1000;
    payload.freeHeap = ESP.getFreeHeap();
    payload.wifiRssi = WiFi.RSSI();
    this->sendData(payload);
}
