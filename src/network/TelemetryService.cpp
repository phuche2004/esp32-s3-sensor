#include "TelemetryService.h"
#include <WiFi.h>
#include <time.h>
#include <sys/time.h>
#include <sys/stat.h>

// Ham phu tro tach URL dang http:// hoac https:// thanh host, port, path, isHttps
static bool parseBackendUrl(const String &url, String &host, uint16_t &port, String &path, bool &isHttps) {
    String rest;
    if (url.startsWith("https://")) {
        isHttps = true;
        rest = url.substring(8);
        port = 443;
    } else if (url.startsWith("http://")) {
        isHttps = false;
        rest = url.substring(7);
        port = 80;
    } else {
        return false;
    }

    int slashIdx = rest.indexOf('/');
    String hostPort = (slashIdx >= 0) ? rest.substring(0, slashIdx) : rest;
    path = (slashIdx >= 0) ? rest.substring(slashIdx) : "/";

    int colonIdx = hostPort.indexOf(':');
    if (colonIdx >= 0) {
        host = hostPort.substring(0, colonIdx);
        port = (uint16_t)hostPort.substring(colonIdx + 1).toInt();
    } else {
        host = hostPort;
    }
    return (host.length() > 0 && port > 0);
}

TelemetryService::TelemetryService(WiFiService &wifiService, StorageManager &storage)
    : wifiService(wifiService), storage(storage), packetSeq(0), cachedDeviceId(""),
      littleFsMounted(false), activeClient(nullptr), cachedBackendUrl(""), lastConfigCheckMs(0),
      telemetryQueue(nullptr), telemetryTaskHandle(nullptr),
      psramBuffer(nullptr), psramCount(0), psramAvailable(false) {}

TelemetryService::~TelemetryService() {
    this->closeStream();
    if (this->telemetryTaskHandle != nullptr) {
        vTaskDelete(this->telemetryTaskHandle);
        this->telemetryTaskHandle = nullptr;
    }
    if (this->telemetryQueue != nullptr) {
        vQueueDelete(this->telemetryQueue);
        this->telemetryQueue = nullptr;
    }
    if (this->psramBuffer != nullptr) {
        free(this->psramBuffer);
        this->psramBuffer = nullptr;
    }
}

void TelemetryService::init() {
    // 1. Khoi tao LittleFS Flash
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

    // 2. Doc truoc Backend URL luu vao RAM Cache
    String dummySSID, dummyPass, dummyUser;
    bool dummyIsEnt = false;
    this->storage.loadCredentials(dummySSID, dummyPass, dummyUser, dummyIsEnt, this->cachedBackendUrl);
    this->lastConfigCheckMs = millis();

    // 3. Cap phat bo dem PSRAM/RAM de chong mon Flash LittleFS
#if defined(BOARD_HAS_PSRAM)
    if (psramFound()) {
        this->psramBuffer = (TelemetryRecord *)ps_malloc(sizeof(TelemetryRecord) * PSRAM_BUFFER_CAPACITY);
        if (this->psramBuffer) {
            this->psramAvailable = true;
            Serial.printf("[Telemetry-PSRAM] Da cap phat Circular Buffer %u mau (%u bytes) tren PSRAM chong mon Flash!\n",
                          (unsigned int)PSRAM_BUFFER_CAPACITY,
                          (unsigned int)(sizeof(TelemetryRecord) * PSRAM_BUFFER_CAPACITY));
        }
    }
#endif
    if (!this->psramBuffer) {
        this->psramBuffer = (TelemetryRecord *)malloc(sizeof(TelemetryRecord) * PSRAM_BUFFER_CAPACITY);
        if (this->psramBuffer) {
            Serial.printf("[Telemetry-RAM] Da cap phat bo dem SRAM %u mau (%u bytes) cho Telemetry.\n",
                          (unsigned int)PSRAM_BUFFER_CAPACITY,
                          (unsigned int)(sizeof(TelemetryRecord) * PSRAM_BUFFER_CAPACITY));
        } else {
            Serial.println("[Telemetry] CANH BAO: Khong the cap phat bo dem RAM/PSRAM!");
        }
    }
    this->psramCount = 0;

    // 4. Khoi tao FreeRTOS Queue va Task doc lap tren Core 0
    this->telemetryQueue = xQueueCreate(16, sizeof(TelemetryPayload));
    if (this->telemetryQueue) {
        BaseType_t taskCreated = xTaskCreatePinnedToCore(
            TelemetryService::telemetryTaskStatic,
            "TelemetryTask",
            8192,
            this,
            1,
            &this->telemetryTaskHandle,
            0 // Ghim doc lap tren Core 0 cung voi Wi-Fi stack
        );
        if (taskCreated == pdPASS) {
            Serial.println("[Telemetry-RTOS] Task Telemetry da khoi tao va ghim vao Core 0 (Stack: 8192B, Priority: 1).");
        } else {
            Serial.println("[Telemetry-RTOS] LOI: Khong the tao TelemetryTask!");
        }
    } else {
        Serial.println("[Telemetry-RTOS] LOI: Khong the tao Telemetry Queue!");
    }
}

void TelemetryService::telemetryTaskStatic(void *pvParameters) {
    TelemetryService *service = static_cast<TelemetryService *>(pvParameters);
    service->telemetryTaskLoop();
}

void TelemetryService::telemetryTaskLoop() {
    TelemetryPayload payload;
    while (true) {
        if (xQueueReceive(this->telemetryQueue, &payload, pdMS_TO_TICKS(100)) == pdTRUE) {
            this->processPayloadInternal(payload);
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

bool TelemetryService::pushPayload(const TelemetryPayload &data) {
    if (!this->telemetryQueue) {
        return false;
    }
    BaseType_t res = xQueueSend(this->telemetryQueue, &data, 0);
    if (res != pdTRUE) {
        Serial.println("[Telemetry-Queue] Canh bao: Queue telemetry bi day, bo qua goi tin!");
        return false;
    }
    return true;
}

void TelemetryService::sendData(TelemetryPayload data) {
    this->pushPayload(data);
}

void TelemetryService::sendData(float temperature, float humidity) {
    TelemetryPayload payload;
    payload.temperature = temperature;
    payload.humidity = humidity;
    payload.sensorValid = true;
    payload.uptimeSec = millis() / 1000;
    payload.freeHeap = ESP.getFreeHeap();
    payload.wifiRssi = WiFi.RSSI();
    this->pushPayload(payload);
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
    if (!this->littleFsMounted) return 0;
    struct stat st;
    if (stat("/littlefs/telemetry_offline.bin", &st) == 0 && st.st_size >= sizeof(TelemetryRecord)) {
        return st.st_size / sizeof(TelemetryRecord);
    }
    return 0;
}

size_t TelemetryService::getPsramBufferCount() const {
    return this->psramCount;
}

void TelemetryService::pruneOldRecordsFromFlash(size_t recordsToRemove) {
    if (!this->littleFsMounted) return;
    struct stat st;
    if (stat("/littlefs/telemetry_offline.bin", &st) != 0 || st.st_size == 0) return;

    File f = LittleFS.open(OFFLINE_FILE, "rb");
    if (!f) return;

    size_t totalBytes = f.size();
    size_t totalRecords = totalBytes / sizeof(TelemetryRecord);
    if (totalRecords <= recordsToRemove) {
        f.close();
        LittleFS.remove(OFFLINE_FILE);
        return;
    }

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
        Serial.printf("[Telemetry-FIFO] Da cat bo %u ban ghi cu nhat (Duy tri dung luong Flash luon an toan)!\n",
                      (unsigned int)recordsToRemove);
    } else {
        f.close();
    }
}

bool TelemetryService::appendRecordToFlash(const TelemetryRecord &rec) {
    if (!this->littleFsMounted) return false;

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

bool TelemetryService::flushPsramToFlash() {
    if (!this->littleFsMounted || !this->psramBuffer || this->psramCount == 0) {
        return false;
    }

    size_t currentCount = this->getFlashBacklogCount();
    size_t freeBytes = LittleFS.totalBytes() - LittleFS.usedBytes();
    if (currentCount + this->psramCount > MAX_OFFLINE_RECORDS || freeBytes < 300000) {
        this->pruneOldRecordsFromFlash(this->psramCount + 50);
    }

    File f = LittleFS.open(OFFLINE_FILE, "ab");
    if (!f) {
        Serial.println("[Telemetry-Flash] LOI: Khong the mo file Flash de flush buffer!");
        return false;
    }

    size_t bytesToWrite = this->psramCount * sizeof(TelemetryRecord);
    size_t written = f.write((const uint8_t *)this->psramBuffer, bytesToWrite);
    f.close();

    bool ok = (written == bytesToWrite);
    if (ok) {
        Serial.printf("[Telemetry-PSRAM] Da flush %u mau tu PSRAM xuong Flash LittleFS (1 lan ghi duy nhat)!\n",
                      (unsigned int)this->psramCount);
        this->psramCount = 0;
    } else {
        Serial.println("[Telemetry-Flash] LOI: Ghi PSRAM buffer vao Flash khong tron ven!");
    }
    return ok;
}

bool TelemetryService::appendRecordToBuffer(const TelemetryRecord &rec) {
    if (!this->psramBuffer) {
        return this->appendRecordToFlash(rec);
    }

    if (this->psramCount < PSRAM_BUFFER_CAPACITY) {
        this->psramBuffer[this->psramCount++] = rec;
    }

    if (this->psramCount >= PSRAM_BUFFER_CAPACITY) {
        return this->flushPsramToFlash();
    }
    return true;
}

void TelemetryService::syncTimeFromServer(const String &responseBody) {
    if (responseBody.length() == 0) return;
    int idx = responseBody.indexOf("\"server_time\"");
    if (idx == -1) return;
    int colon = responseBody.indexOf(':', idx);
    if (colon == -1) return;

    const char *p = responseBody.c_str() + colon + 1;
    while (*p == ' ' || *p == '\t') p++;
    time_t sTime = (time_t)atoll(p);

    if (sTime > 1700000000) {
        time_t now = time(nullptr);
        if (now < 1700000000 || labs((long)(sTime - now)) > 5) {
            struct timeval tv = { .tv_sec = sTime, .tv_usec = 0 };
            settimeofday(&tv, NULL);
            Serial.printf("[Telemetry-RTC] Da dong bo RTC he thong tu server_time: %lld (Lech cu: %ld s)\n",
                          (long long)sTime, (long)(sTime - now));
        }
    }
}

void TelemetryService::closeStream() {
    if (this->activeClient != nullptr) {
        if (this->activeClient->connected()) {
            this->activeClient->stop();
        }
        this->activeClient = nullptr;
    }
}

bool TelemetryService::ensureStreamConnected(const String &backendUrl) {
    if (this->activeClient != nullptr && this->activeClient->connected()) {
        return true; // Da mo san duong ong WebSocket -> Dung tiep, 0 tao request moi!
    }

    String host, path;
    uint16_t port = 80;
    bool isHttps = false;
    if (!parseBackendUrl(backendUrl, host, port, path, isHttps)) {
        Serial.println("[Telemetry-WS] LOI: Dinh dang Backend URL khong hop le!");
        return false;
    }

    if (isHttps) {
        this->secureClient.setInsecure();
        this->activeClient = &this->secureClient;
    } else {
        this->activeClient = &this->streamClient;
    }

    if (!this->activeClient->connect(host.c_str(), port, 2000)) {
        this->activeClient = nullptr;
        return false;
    }

    this->activeClient->setNoDelay(true);

    // GUI DUNG 1 HTTP UPGRADE REQUEST DUY NHAT XUYEN QUA CLOUDFLARE & NGINX
    String hostHeader = (port == 80 || port == 443) ? host : (host + ":" + String(port));
    this->activeClient->printf(
        "GET %s HTTP/1.1\r\n"
        "Host: %s\r\n"
        "User-Agent: ESP32-S3-Sensor/3.0-WS\r\n"
        "Upgrade: websocket\r\n"
        "Connection: Upgrade\r\n"
        "Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n"
        "Sec-WebSocket-Version: 13\r\n"
        "X-Device-ID: %s\r\n\r\n",
        path.c_str(), hostHeader.c_str(), this->getDeviceId().c_str()
    );

    uint32_t startWait = millis();
    while (!this->activeClient->available()) {
        if (!this->activeClient->connected() || (millis() - startWait > 4000)) {
            this->closeStream();
            return false;
        }
        vTaskDelay(pdMS_TO_TICKS(5));
    }

    String statusLine = this->activeClient->readStringUntil('\n');
    statusLine.trim();

    // Doc bo qua HTTP Response Headers ban dau
    while (this->activeClient->connected()) {
        String headerLine = this->activeClient->readStringUntil('\n');
        if (headerLine == "\r" || headerLine.length() == 0) {
            break;
        }
    }

    // Kiem tra ma 101 Switching Protocols (hoac 200 neu test local cu)
    if (statusLine.indexOf("101") == -1 && statusLine.indexOf("200") == -1) {
        Serial.printf("[Telemetry-WS] Server tu choi nang cap WebSocket: %s\n", statusLine.c_str());
        this->closeStream();
        return false;
    }

    Serial.println("[Telemetry-WS] DA THIET LAP DUONG ONG WEBSOCKET 1-REQUEST DUY NHAT! Bat dau bom data 1Hz...");
    return true;
}

HttpSendResult TelemetryService::sendHttpPayload(const String &payload, const String &backendUrl, bool isBatch, size_t count, uint32_t seq) {
    if (!this->ensureStreamConnected(backendUrl)) {
        return HTTP_SEND_RETRYABLE_ERROR;
    }

    // Don sach byte thua trong buffer doc (neu co)
    while (this->activeClient->available()) {
        this->activeClient->read();
    }

    // Dong goi WebSocket Text Frame (RFC 6455, FIN=1, Opcode=0x01, Masked=1)
    size_t len = payload.length();
    size_t headerLen = (len < 126) ? 6 : 8;
    size_t totalFrameLen = headerLen + len;

    uint8_t *frame = (uint8_t *)malloc(totalFrameLen);
    if (!frame) {
        return HTTP_SEND_RETRYABLE_ERROR;
    }

    const uint8_t mask[4] = { 0x37, 0xFA, 0x21, 0x3D };
    frame[0] = 0x81; // FIN + Text Frame
    size_t maskOffset = 2;

    if (len < 126) {
        frame[1] = 0x80 | (uint8_t)len;
        maskOffset = 2;
    } else {
        frame[1] = 0x80 | 126;
        frame[2] = (uint8_t)((len >> 8) & 0xFF);
        frame[3] = (uint8_t)(len & 0xFF);
        maskOffset = 4;
    }

    memcpy(frame + maskOffset, mask, 4);
    const char *raw = payload.c_str();
    uint8_t *dst = frame + headerLen;
    for (size_t i = 0; i < len; i++) {
        dst[i] = ((uint8_t)raw[i]) ^ mask[i & 3];
    }

    size_t written = this->activeClient->write(frame, totalFrameLen);
    free(frame);

    if (written < totalFrameLen) {
        this->closeStream();
        return HTTP_SEND_RETRYABLE_ERROR;
    }

    // Doi ACK Frame tu Server
    uint32_t timeoutMs = isBatch ? 4000 : 2000;
    uint32_t startWait = millis();
    String resp = "";

    while (millis() - startWait < timeoutMs) {
        if (this->activeClient->available() >= 2) {
            uint8_t b0 = this->activeClient->read();
            uint8_t b1 = this->activeClient->read();
            uint8_t opcode = b0 & 0x0F;
            size_t rLen = b1 & 0x7F;

            if (rLen == 126) {
                uint32_t t0 = millis();
                while (this->activeClient->available() < 2 && (millis() - t0 < 500)) {
                    vTaskDelay(pdMS_TO_TICKS(1));
                }
                uint8_t hi = this->activeClient->read();
                uint8_t lo = this->activeClient->read();
                rLen = ((size_t)hi << 8) | lo;
            }

            if (opcode == 0x08) { // Server dong WS
                this->closeStream();
                return HTTP_SEND_RETRYABLE_ERROR;
            }

            // Doc noi dung payload cua Frame Server tra ve
            String frameBody;
            frameBody.reserve(rLen + 1);
            uint32_t tRead = millis();
            while (frameBody.length() < rLen && (millis() - tRead < 1000)) {
                if (this->activeClient->available()) {
                    frameBody += (char)this->activeClient->read();
                } else {
                    vTaskDelay(pdMS_TO_TICKS(1));
                }
            }

            if (opcode == 0x01 && frameBody.indexOf("\"status\"") != -1) {
                resp = frameBody;
                break;
            }
        } else {
            if (!this->activeClient->connected()) {
                this->closeStream();
                return HTTP_SEND_RETRYABLE_ERROR;
            }
            vTaskDelay(pdMS_TO_TICKS(2));
        }
    }

    if (resp.length() == 0) {
        Serial.printf("[Telemetry-WS] Timeout cho ACK goi %s -> Reset WS!\n",
                      isBatch ? "BATCH" : ("#" + String(seq)).c_str());
        this->closeStream();
        return HTTP_SEND_RETRYABLE_ERROR;
    }

    if (resp.indexOf("\"status\":\"ok\"") != -1 || resp.indexOf("\"status\": \"ok\"") != -1) {
        this->syncTimeFromServer(resp);
        return HTTP_SEND_SUCCESS;
    } else if (resp.indexOf("\"status\":\"client_error\"") != -1 || resp.indexOf("\"status\": \"client_error\"") != -1) {
        return HTTP_SEND_CLIENT_ERROR;
    }

    this->closeStream();
    return HTTP_SEND_RETRYABLE_ERROR;
}

bool TelemetryService::flushBatchFromFlash(const String &backendUrl) {
    if (!this->littleFsMounted) return true;
    struct stat st;
    if (stat("/littlefs/telemetry_offline.bin", &st) != 0 || st.st_size == 0) return true;

    File f = LittleFS.open(OFFLINE_FILE, "rb");
    if (!f) return false;

    size_t totalBytes = f.size();
    size_t totalRecords = totalBytes / sizeof(TelemetryRecord);
    if (totalRecords == 0) {
        f.close();
        LittleFS.remove(OFFLINE_FILE);
        return true;
    }

    size_t recordsToRead = (totalRecords > BATCH_CHUNK_SIZE) ? BATCH_CHUNK_SIZE : totalRecords;
    TelemetryRecord chunk[BATCH_CHUNK_SIZE];
    size_t actualRead = f.read((uint8_t *)chunk, recordsToRead * sizeof(TelemetryRecord)) / sizeof(TelemetryRecord);

    if (actualRead == 0) {
        f.close();
        return false;
    }

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

    HttpSendResult res = this->sendHttpPayload(json, backendUrl, true, actualRead, 0);
    if (res == HTTP_SEND_RETRYABLE_ERROR) {
        f.close();
        return false;
    }

    if (res == HTTP_SEND_CLIENT_ERROR) {
        Serial.printf("[Telemetry-Flash] Backend tu choi batch %u ban ghi (Client Error) -> Bo qua de tranh deadlock!\n",
                      (unsigned int)actualRead);
    } else {
        Serial.printf("[Batch-Ingestion] Da gui bu thanh cong dot %u ban ghi Flash qua Stream!\n",
                      (unsigned int)actualRead);
    }

    size_t remainingRecords = totalRecords - actualRead;
    if (remainingRecords == 0) {
        f.close();
        LittleFS.remove(OFFLINE_FILE);
        Serial.println("[Batch-Ingestion] DA XA SACH TOAN BO DU LIEU OFFLINE TREN FLASH!");
    } else {
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

void TelemetryService::processPayloadInternal(TelemetryPayload data) {
    this->packetSeq++;
    data.seq = this->packetSeq;

    time_t nowEpoch = time(nullptr);
    if (nowEpoch > 1700000000) {
        data.timestamp = (uint32_t)nowEpoch;
    } else {
        data.timestamp = 0;
    }

    // Chi kiem tra cap nhat Backend URL tu NVS moi 15 giay (tranh doc NVS Flash moi giay)
    uint32_t nowMs = millis();
    if (this->cachedBackendUrl.length() == 0 || (nowMs - this->lastConfigCheckMs >= 15000)) {
        this->lastConfigCheckMs = nowMs;
        String dummySSID, dummyPass, dummyUser, latestUrl;
        bool dummyIsEnt = false;
        this->storage.loadCredentials(dummySSID, dummyPass, dummyUser, dummyIsEnt, latestUrl);
        if (latestUrl != this->cachedBackendUrl) {
            this->cachedBackendUrl = latestUrl;
            this->closeStream(); // Neu nguoi dung doi URL tren Web UI thi ngat Stream cu
        }
    }

    if (this->cachedBackendUrl.length() == 0) {
        return;
    }

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

    // Kiem tra ket noi Wi-Fi
    if (!this->wifiService.isConnected()) {
        this->closeStream();
        this->appendRecordToBuffer(rec);
        Serial.printf("[Telemetry] Mat Wi-Fi -> Da luu goi #%u vao PSRAM Buffer (Dem: %u/%u, Flash: %u)\n",
                      data.seq, (unsigned int)this->psramCount, (unsigned int)PSRAM_BUFFER_CAPACITY,
                      (unsigned int)this->getFlashBacklogCount());
        return;
    }

    // Co Wi-Fi: Neu co ban ghi tam trong PSRAM, flush xuong Flash truoc de dam bao thu tu thoi gian
    if (this->psramCount > 0) {
        this->flushPsramToFlash();
    }

    // Kiem tra ket noi WebSocket toi Server truoc khi xa Flash hoac gui goi moi (Chong treo Task Queue)
    if (!this->ensureStreamConnected(this->cachedBackendUrl)) {
        this->appendRecordToBuffer(rec);
        Serial.printf("[Telemetry] Khong ket noi duoc toi %s -> Luu goi #%u vao Buffer (Dem: %u/%u, Flash: %u)\n",
                      this->cachedBackendUrl.c_str(), data.seq, (unsigned int)this->psramCount, (unsigned int)PSRAM_BUFFER_CAPACITY,
                      (unsigned int)this->getFlashBacklogCount());
        return;
    }

    // Xa sach cac batch offline tren Flash qua Stream
    while (this->getFlashBacklogCount() > 0) {
        bool ok = this->flushBatchFromFlash(this->cachedBackendUrl);
        if (!ok) {
            break;
        }
    }

    // Dong goi va gui ban tin thoi gian thuc tren 1 dong NDJSON
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

    HttpSendResult sendRes = this->sendHttpPayload(payload, this->cachedBackendUrl, false, 1, data.seq);
    if (sendRes == HTTP_SEND_SUCCESS) {
        Serial.printf("[Telemetry-Stream] #%u | T: %.2f*C | H: %.2f%% | RSSI: %d dBm -> STREAM ACK (Backlog: %u)\n",
                      data.seq, data.temperature, data.humidity, data.wifiRssi, (unsigned int)this->getFlashBacklogCount());
    } else if (sendRes == HTTP_SEND_CLIENT_ERROR) {
        Serial.printf("[Telemetry-Stream] #%u | Backend tu choi (Client Error) -> Bo qua de tranh deadlock!\n", data.seq);
    } else {
        this->appendRecordToBuffer(rec);
        Serial.printf("[Telemetry] Gui Stream that bai -> Da luu goi #%u vao PSRAM Buffer (Dem: %u/%u, Flash: %u)\n",
                      data.seq, (unsigned int)this->psramCount, (unsigned int)PSRAM_BUFFER_CAPACITY,
                      (unsigned int)this->getFlashBacklogCount());
    }
}
