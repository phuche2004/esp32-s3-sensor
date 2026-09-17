#include "DataLogger.h"

static const char* SNAPSHOT_FILE = "/history.bin";

DataLogger::DataLogger()
    : buffer(nullptr), head(0), count(0),
      lastSampleTime(0), lastSnapshotTime(0), initialized(false) {}

void DataLogger::init() {
    if (this->initialized) return;
    this->initialized = true;

    // Cap phat bo nho dem 1440 diem (11.5 KB), uu tien PSRAM neu co
#if defined(BOARD_HAS_PSRAM)
    if (psramFound()) {
        this->buffer = (HistoryPoint*)ps_malloc(sizeof(HistoryPoint) * MAX_POINTS);
        if (this->buffer) {
            Serial.println("[DataLogger] Da cap phat Circular Buffer 1440 diem tren PSRAM (11.5 KB).");
        }
    }
#endif
    if (!this->buffer) {
        this->buffer = (HistoryPoint*)malloc(sizeof(HistoryPoint) * MAX_POINTS);
        if (this->buffer) {
            Serial.println("[DataLogger] Da cap phat Circular Buffer 1440 diem tren SRAM (11.5 KB).");
        } else {
            Serial.println("[LOI] Khong the cap phat bo nho cho DataLogger!");
            return;
        }
    }

    memset(this->buffer, 0, sizeof(HistoryPoint) * MAX_POINTS);

    // Khoi tao LittleFS Flash de luu snapshot
    if (LittleFS.begin(true)) {
        Serial.println("[DataLogger] LittleFS da san sang.");
        this->loadSnapshot();
    } else {
        Serial.println("[LOI] Khong the khoi tao LittleFS!");
    }

    this->lastSnapshotTime = millis();
}

void DataLogger::addPoint(float temp, float hum, uint32_t timestamp) {
    if (!this->buffer) return;

    if (timestamp == 0) {
        time_t now = time(nullptr);
        if (now > 1000000000) {
            timestamp = (uint32_t)now;
        } else {
            timestamp = millis() / 1000;
        }
    }

    HistoryPoint pt;
    pt.timestamp = timestamp;
    pt.temp = (int16_t)roundf(temp * 100.0f);
    pt.hum  = (int16_t)roundf(hum * 100.0f);

    this->buffer[this->head] = pt;
    this->head = (this->head + 1) % MAX_POINTS;
    if (this->count < MAX_POINTS) {
        this->count++;
    }

    // Luu Flash ngay sau moi diem do de rut cap/mat dien khong bi mat du lieu
    this->saveSnapshot();
}

void DataLogger::syncEpoch(uint32_t currentEpoch) {
    if (currentEpoch < 1000000000 || !this->buffer || this->count == 0) return;
    uint32_t currentUptime = millis() / 1000;
    uint32_t bootEpoch = (currentEpoch >= currentUptime) ? (currentEpoch - currentUptime) : 0;

    for (uint16_t i = 0; i < this->count; i++) {
        if (this->buffer[i].timestamp < 1000000000 && this->buffer[i].timestamp > 0) {
            this->buffer[i].timestamp = bootEpoch + this->buffer[i].timestamp;
        }
    }

    // Luu lai Flash sau khi da dong bo gio thuc thanh cong
    this->saveSnapshot();
}

void DataLogger::loop() {
    // Du lieu da duoc luu dong bo moi khi co diem do moi trong addPoint()
}

String DataLogger::getHistoryJson(uint8_t filterHours) {
    if (!this->buffer || this->count == 0) {
        return "{\"count\":0,\"data\":[]}";
    }

    uint16_t pointsToTake = filterHours * 60;
    if (pointsToTake > this->count || pointsToTake == 0) {
        pointsToTake = this->count;
    }

    // Tinh chi so diem cu nhat can lay trong ring buffer
    // head la vi tri se ghi tiep theo, nen diem moi nhat la (head - 1 + MAX_POINTS) % MAX_POINTS
    uint16_t startIndex;
    if (this->count < MAX_POINTS) {
        startIndex = (this->count >= pointsToTake) ? (this->count - pointsToTake) : 0;
    } else {
        startIndex = (this->head + MAX_POINTS - pointsToTake) % MAX_POINTS;
    }

    String json;
    json.reserve(pointsToTake * 22 + 64);
    json = "{\"count\":";
    json += String(pointsToTake);
    json += ",\"points\":[";

    for (uint16_t i = 0; i < pointsToTake; i++) {
        uint16_t idx = (startIndex + i) % MAX_POINTS;
        HistoryPoint &pt = this->buffer[idx];

        if (i > 0) json += ",";
        json += "[";
        json += String(pt.timestamp);
        json += ",";
        json += String(pt.temp / 100.0f, 2);
        json += ",";
        json += String(pt.hum / 100.0f, 2);
        json += "]";
    }
    json += "]}";
    return json;
}

String DataLogger::getCsvString() {
    if (!this->buffer || this->count == 0) {
        return "Timestamp(s),Temperature(C),Humidity(%)\r\n";
    }

    String csv;
    csv.reserve(this->count * 28 + 64);
    csv = "Timestamp(s),Temperature(C),Humidity(%)\r\n";

    uint16_t startIndex;
    if (this->count < MAX_POINTS) {
        startIndex = 0;
    } else {
        startIndex = this->head; // Diem cu nhat nam ngay tai head
    }

    for (uint16_t i = 0; i < this->count; i++) {
        uint16_t idx = (startIndex + i) % MAX_POINTS;
        HistoryPoint &pt = this->buffer[idx];

        csv += String(pt.timestamp);
        csv += ",";
        csv += String(pt.temp / 100.0f, 2);
        csv += ",";
        csv += String(pt.hum / 100.0f, 2);
        csv += "\r\n";
    }
    return csv;
}

bool DataLogger::saveSnapshot() {
    if (!this->buffer || this->count == 0) return false;

    File f = LittleFS.open(SNAPSHOT_FILE, "w");
    if (!f) {
        Serial.println("[DataLogger] Loi mo file ghi snapshot Flash!");
        return false;
    }

    f.write((uint8_t*)&this->head, sizeof(this->head));
    f.write((uint8_t*)&this->count, sizeof(this->count));
    f.write((uint8_t*)this->buffer, sizeof(HistoryPoint) * MAX_POINTS);
    f.close();

    Serial.printf("[DataLogger] Da luu Snapshot %u diem vao Flash LittleFS.\n", this->count);
    return true;
}

bool DataLogger::loadSnapshot() {
    if (!this->buffer) return false;
    if (!LittleFS.exists(SNAPSHOT_FILE)) {
        Serial.println("[DataLogger] Chua co file snapshot Flash cu.");
        return false;
    }

    File f = LittleFS.open(SNAPSHOT_FILE, "r");
    if (!f) return false;

    if (f.size() != (sizeof(this->head) + sizeof(this->count) + sizeof(HistoryPoint) * MAX_POINTS)) {
        Serial.println("[DataLogger] Kich thuoc snapshot khong khop, bo qua.");
        f.close();
        return false;
    }

    f.read((uint8_t*)&this->head, sizeof(this->head));
    f.read((uint8_t*)&this->count, sizeof(this->count));
    f.read((uint8_t*)this->buffer, sizeof(HistoryPoint) * MAX_POINTS);
    f.close();

    if (this->head >= MAX_POINTS || this->count > MAX_POINTS) {
        this->head = 0;
        this->count = 0;
        return false;
    }

    Serial.printf("[DataLogger] Da khoi phuc thanh cong %u diem lich su tu Flash.\n", this->count);
    return true;
}
