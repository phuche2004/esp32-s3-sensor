#include "CaptivePortal.h"
#include "Config.h"
#include "web/WebPage.h"
#include "../drivers/StressTester.h"
#include "../drivers/DataLogger.h"
#include <WiFi.h>

CaptivePortal::CaptivePortal(WiFiService &wifiService, StorageManager &storage, RgbController &rgbController, SensorSettings &sensorSettings, PowerManager &powerManager)
    : wifiService(wifiService), storage(storage), rgbController(rgbController), sensorSettings(sensorSettings), powerManager(powerManager),
      server(80), active(false), currentTemp(0.0f), currentHum(0.0f), sensorValid(false), countdownTriggered(false) {}

void CaptivePortal::init() {
    this->setupRoutes();
    this->dnsServer.start(53, "*", WiFi.softAPIP());
    this->server.begin();
    this->active = true;
    Serial.println("[Portal] Captive Portal Web Server da khoi dong tren cong 80.");
}

void CaptivePortal::setupRoutes() {
    this->server.on("/", HTTP_GET, [this]() { this->handleRoot(); });
    this->server.on("/save", HTTP_POST, [this]() { this->handleSave(); });
    this->server.on("/scan", HTTP_GET, [this]() { this->handleScan(); });
    this->server.on("/status", HTTP_GET, [this]() { this->handleStatus(); });
    this->server.on("/sensor", HTTP_GET, [this]() { this->handleSensor(); });
    this->server.on("/api/sensor-cfg", HTTP_GET, [this]() { this->handleSensorConfigGet(); });
    this->server.on("/api/sensor-cfg", HTTP_POST, [this]() { this->handleSensorConfigSet(); });
    this->server.on("/api/led", HTTP_GET, [this]() { this->handleLedGet(); });
    this->server.on("/api/led", HTTP_POST, [this]() { this->handleLedSet(); });
    this->server.on("/api/led/reset", HTTP_POST, [this]() { this->handleLedReset(); });
    this->server.on("/api/power", HTTP_GET, [this]() { this->handlePowerGet(); });
    this->server.on("/api/power", HTTP_POST, [this]() { this->handlePowerSet(); });
    this->server.on("/api/stress-test", HTTP_POST, [this]() { this->handleStressTest(); });
    this->server.on("/api/stress-test", HTTP_GET, [this]() { this->handleStressTest(); });
    this->server.on("/api/history", HTTP_GET, [this]() { this->handleHistoryGet(); });
    this->server.on("/api/history/csv", HTTP_GET, [this]() { this->handleHistoryCsv(); });
    this->server.on("/api/sync-time", HTTP_POST, [this]() { this->handleSyncTime(); });
    this->server.on("/shutdown-ap", HTTP_POST, [this]() { this->handleShutdownAP(); });
    this->server.on("/shutdown-ap", HTTP_GET, [this]() { this->handleShutdownAP(); });
    this->server.on("/api/reset-wifi", HTTP_POST, [this]() { this->handleResetWifi(); });
    this->server.on("/api/reset-wifi", HTTP_GET, [this]() { this->handleResetWifi(); });

    // 1. Tranh vong lap vo tan cho favicon
    this->server.on("/favicon.ico", HTTP_GET, [this]() {
        this->server.send(204);
    });

    // 2. Ham chuyen huong tieu chuan tuong thich cao voi Samsung One UI, Apple CNA, Android
    auto sendCaptiveRedirect = [this]() {
        this->server.sendHeader("Location", "http://192.168.4.1/", true);
        this->server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
        this->server.sendHeader("Pragma", "no-cache");
        this->server.sendHeader("Expires", "0");
        this->server.send(302, "text/html", F("<!DOCTYPE html><html><head><meta charset='utf-8'>"
            "<meta http-equiv='refresh' content='0;url=http://192.168.4.1/'>"
            "<script>window.location.replace('http://192.168.4.1/');</script>"
            "<title>Redirecting...</title></head>"
            "<body><p>Dang chuyen huong...</p>"
            "<p><a href='http://192.168.4.1/'>Nhan vao day neu khong tu dong chuyen</a></p></body></html>"));
    };

    // 3. Danh sach day du cac connectivity probe endpoints
    // Android / Google / Samsung One UI
    this->server.on("/generate_204", HTTP_GET, sendCaptiveRedirect);
    this->server.on("/gen_204", HTTP_GET, sendCaptiveRedirect);
    this->server.on("/ncsi.txt", HTTP_GET, sendCaptiveRedirect);
    this->server.on("/mobile/status.go", HTTP_GET, sendCaptiveRedirect);
    this->server.on("/check_network_status.txt", HTTP_GET, sendCaptiveRedirect);

    // Apple iOS / macOS CNA
    this->server.on("/hotspot-detect.html", HTTP_GET, sendCaptiveRedirect);
    this->server.on("/library/test/success.html", HTTP_GET, sendCaptiveRedirect);
    this->server.on("/success.txt", HTTP_GET, sendCaptiveRedirect);
    this->server.on("/bag", HTTP_GET, sendCaptiveRedirect);

    // Microsoft Windows
    this->server.on("/connecttest.txt", HTTP_GET, sendCaptiveRedirect);
    this->server.on("/redirect", HTTP_GET, sendCaptiveRedirect);
    this->server.on("/canonical.html", HTTP_GET, sendCaptiveRedirect);

    // 4. Bat moi request chua xu ly (Catch-all)
    this->server.onNotFound([this, sendCaptiveRedirect]() {
        if (this->server.uri().indexOf("favicon") >= 0) {
            this->server.send(204);
            return;
        }
        sendCaptiveRedirect();
    });
}

void CaptivePortal::loop() {
    if (this->active) {
        // Xu ly nhanh nhieu goi tin DNS trong hang doi UDP de tranh timeout khi thiet bi gui song song
        for (uint8_t i = 0; i < 4; i++) {
            this->dnsServer.processNextRequest();
        }
        this->server.handleClient();
    }
}

void CaptivePortal::updateSensorData(float temp, float hum, bool valid) {
    this->currentTemp = temp;
    this->currentHum = hum;
    this->sensorValid = valid;
}

void CaptivePortal::handleSensor() {
    float t = this->currentTemp;
    float h = this->currentHum;

    // Tinh diem suong (Dew Point) theo Magnus formula
    float a = 17.27f;
    float b = 237.7f;
    float safeH = (h > 1.0f) ? h : 1.0f;
    float alpha = ((a * t) / (b + t)) + logf(safeH / 100.0f);
    float dewPoint = (b * alpha) / (a - alpha);

    // Tinh do thieu hut ap suat hoi (Vapor Pressure Deficit - VPD kPa)
    float vpSat = 0.61078f * expf((17.27f * t) / (t + 237.3f));
    float vpd = vpSat * (1.0f - (h / 100.0f));
    if (vpd < 0.0f) vpd = 0.0f;

    uint32_t totalHeap = ESP.getHeapSize();
    uint32_t freeHeap = ESP.getFreeHeap();
    uint32_t minFreeHeap = ESP.getMinFreeHeap();
    uint32_t uptimeSec = millis() / 1000;
    time_t nowEpoch = time(nullptr);
    uint32_t cpuFreq = ESP.getCpuFreqMHz();
    uint16_t logCount = DataLogger::getInstance().getCount();

    String json = "{";
    json += "\"temp\":" + String(this->currentTemp, 2) + ",";
    json += "\"hum\":" + String(this->currentHum, 2) + ",";
    json += "\"dewPoint\":" + String(dewPoint, 1) + ",";
    json += "\"vpd\":" + String(vpd, 2) + ",";
    json += "\"valid\":" + String(this->sensorValid ? "true" : "false") + ",";
    bool isAlert = (this->currentTemp > this->sensorSettings.tempAlert || this->currentHum > this->sensorSettings.humAlert);
    json += "\"alert\":" + String(isAlert ? "true" : "false") + ",";
    json += "\"chipTemp\":" + String(StressTester::getInstance().getChipTemperature(), 1) + ",";
    json += "\"cpu\":" + String(StressTester::getInstance().getCpuLoad(), 1) + ",";
    json += "\"cpu0\":" + String(StressTester::getInstance().getCpuLoadCore0(), 1) + ",";
    json += "\"cpu1\":" + String(StressTester::getInstance().getCpuLoadCore1(), 1) + ",";
    json += "\"cpuFreq\":" + String(cpuFreq) + ",";
    json += "\"totalHeap\":" + String(totalHeap) + ",";
    json += "\"freeHeap\":" + String(freeHeap) + ",";
    json += "\"minFreeHeap\":" + String(minFreeHeap) + ",";
    json += "\"uptimeSec\":" + String(uptimeSec) + ",";
    json += "\"epoch\":" + String((uint32_t)nowEpoch) + ",";
    json += "\"logCount\":" + String(logCount) + ",";
    json += "\"stressActive\":" + String(StressTester::getInstance().isRunning() ? "true" : "false") + ",";
    json += "\"stressSec\":" + String(StressTester::getInstance().getElapsedSec());
    json += "}";
    this->server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    this->server.send(200, "application/json", json);
}

void CaptivePortal::handleSensorConfigGet() {
    String json = "{";
    json += "\"tempAlert\":" + String(this->sensorSettings.tempAlert, 1) + ",";
    json += "\"humAlert\":" + String(this->sensorSettings.humAlert, 1) + ",";
    json += "\"readInterval\":" + String(this->sensorSettings.readIntervalSec) + ",";
    json += "\"sendInterval\":" + String(this->sensorSettings.sendIntervalSec);
    json += "}";
    this->server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    this->server.send(200, "application/json", json);
}

void CaptivePortal::handleSensorConfigSet() {
    if (this->server.hasArg("reset") && this->server.arg("reset") == "1") {
        this->sensorSettings.tempAlert = TEMP_ALERT_THRESHOLD;
        this->sensorSettings.humAlert = HUM_ALERT_THRESHOLD;
        this->sensorSettings.readIntervalSec = 1;
        this->sensorSettings.sendIntervalSec = 1;
        this->storage.saveSensorSettings(this->sensorSettings.tempAlert, this->sensorSettings.humAlert,
                                         this->sensorSettings.readIntervalSec, this->sensorSettings.sendIntervalSec);
        Serial.println("[Sensor] Da khoi phuc cau hinh cam bien ve mac dinh!");
        this->handleSensorConfigGet();
        return;
    }

    if (this->server.hasArg("temp_alert")) {
        this->sensorSettings.tempAlert = this->server.arg("temp_alert").toFloat();
    }
    if (this->server.hasArg("hum_alert")) {
        this->sensorSettings.humAlert = this->server.arg("hum_alert").toFloat();
    }
    if (this->server.hasArg("read_interval")) {
        uint32_t r = this->server.arg("read_interval").toInt();
        if (r < 1) r = 1;
        if (r > 60) r = 60;
        this->sensorSettings.readIntervalSec = r;
    }
    if (this->server.hasArg("send_interval")) {
        uint32_t s = this->server.arg("send_interval").toInt();
        if (s < 1) s = 1;
        if (s > 300) s = 300;
        this->sensorSettings.sendIntervalSec = s;
    }

    this->storage.saveSensorSettings(this->sensorSettings.tempAlert, this->sensorSettings.humAlert,
                                     this->sensorSettings.readIntervalSec, this->sensorSettings.sendIntervalSec);
    Serial.printf("[Sensor] Da cap nhat cau hinh: TempAlert=%.1f *C, HumAlert=%.1f %%, Read=%ds, Send=%ds\n",
                  this->sensorSettings.tempAlert, this->sensorSettings.humAlert,
                  this->sensorSettings.readIntervalSec, this->sensorSettings.sendIntervalSec);

    this->handleSensorConfigGet();
}

void CaptivePortal::handleLedGet() {
    String json = "{";
    json += "\"mode\":" + String((uint8_t)this->rgbController.getMode()) + ",";
    json += "\"r\":" + String(this->rgbController.getR()) + ",";
    json += "\"g\":" + String(this->rgbController.getG()) + ",";
    json += "\"b\":" + String(this->rgbController.getB()) + ",";
    json += "\"brightness\":" + String(this->rgbController.getBrightness()) + ",";
    json += "\"speed\":" + String(this->rgbController.getSpeed());
    json += "}";
    this->server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    this->server.send(200, "application/json", json);
}

void CaptivePortal::handleLedSet() {
    uint8_t mode = this->server.hasArg("mode") ? this->server.arg("mode").toInt() : (uint8_t)this->rgbController.getMode();
    uint8_t r = this->server.hasArg("r") ? this->server.arg("r").toInt() : this->rgbController.getR();
    uint8_t g = this->server.hasArg("g") ? this->server.arg("g").toInt() : this->rgbController.getG();
    uint8_t b = this->server.hasArg("b") ? this->server.arg("b").toInt() : this->rgbController.getB();
    uint8_t brightness = this->server.hasArg("brightness") ? this->server.arg("brightness").toInt() : this->rgbController.getBrightness();
    uint8_t speed = this->server.hasArg("speed") ? this->server.arg("speed").toInt() : this->rgbController.getSpeed();

    this->rgbController.applyConfig(mode, r, g, b, brightness, speed);

    if (this->server.hasArg("save") && this->server.arg("save") == "1") {
        this->storage.saveLedConfig(mode, r, g, b, brightness, speed);
        Serial.printf("[LED] Da luu cau hinh vao Flash (NVS): Mode=%d, R=%d, G=%d, B=%d, Bright=%d, Speed=%d\n",
                      mode, r, g, b, brightness, speed);
        this->handleLedGet();
        return;
    }

    this->server.sendHeader("Connection", "keep-alive");
    this->server.send(204, "text/plain", "");
}

void CaptivePortal::handleLedReset() {
    this->rgbController.setPurpleBreathing();
    this->storage.saveLedConfig(RGB_MODE_BREATHING, 168, 85, 247, 220, 50);
    Serial.println("[LED] Da khoi phuc LED ve mac dinh: Tim Breathing.");
    this->handleLedGet();
}

void CaptivePortal::handleShutdownAP() {
    Serial.println("[Portal] Nhan lenh tu Web UI sau 3s dem nguoc -> Tat Access Point ngay lap tuc.");
    this->server.send(200, "application/json", "{\"success\":true}");
    delay(100);
    this->wifiService.stopAP();
}

void CaptivePortal::handleResetWifi() {
    Serial.println("[Portal] Nhan lenh Reset WiFi: Xoa credentials NVS va khoi dong lai -> AP mode.");
    this->server.send(200, "application/json", "{\"success\":true}");
    delay(200);
    // Xoa toan bo credentials da luu, ESP.restart() se vao AP mode vi hasCredentials() = false
    this->storage.clear();
    delay(100);
    ESP.restart();
}

void CaptivePortal::handleScan() {
    int n = WiFi.scanNetworks();
    String json = "[";
    for (int i = 0; i < n; ++i) {
        if (i > 0) json += ",";
        json += "{\"ssid\":\"" + WiFi.SSID(i) + "\",\"rssi\":" + String(WiFi.RSSI(i)) + "}";
    }
    json += "]";
    this->server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    this->server.send(200, "application/json", json);
}

void CaptivePortal::handleStatus() {
    WiFiConnectResult res = this->wifiService.getConnectResult();
    String stateStr = "idle";
    if (res == RESULT_CONNECTING) {
        stateStr = "connecting";
    } else if (res == RESULT_SUCCESS) {
        stateStr = "success";
        if (!this->countdownTriggered) {
            this->countdownTriggered = true;
            this->wifiService.triggerShutdownCountdown(3000);
        }
    } else if (res == RESULT_FAILED) {
        stateStr = "failed";
    }

    String json = "{";
    json += "\"state\":\"" + stateStr + "\",";
    json += "\"ip\":\"" + this->wifiService.getIP() + "\",";
    json += "\"error\":\"" + this->wifiService.getLastError() + "\"";
    json += "}";

    this->server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    this->server.sendHeader("Pragma", "no-cache");
    this->server.sendHeader("Expires", "0");
    this->server.send(200, "application/json", json);
}

void CaptivePortal::handleSave() {
    String ssid = this->server.arg("ssid");
    String pass = this->server.arg("password");
    String isEntStr = this->server.arg("is_ent");
    String user = this->server.arg("username");
    String backend = this->server.arg("backend");

    bool isEnt = (isEntStr == "1" || isEntStr == "true" || isEntStr == "on");

    if (ssid.length() == 0) {
        this->server.send(400, "application/json", "{\"success\":false,\"error\":\"Ten WiFi khong duoc de trong!\"}");
        return;
    }

    this->countdownTriggered = false;
    this->wifiService.connectStation(ssid, pass, user, isEnt, backend);
    this->server.send(200, "application/json", "{\"success\":true,\"message\":\"Dang ket noi...\"}");
}

void CaptivePortal::handleRoot() {
    this->server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    this->server.sendHeader("Pragma", "no-cache");
    this->server.sendHeader("Expires", "0");
    this->server.setContentLength(CONTENT_LENGTH_UNKNOWN);
    this->server.send(200, "text/html", "");

    String savedSSID, savedPass, savedUser, savedBackend;
    bool savedIsEnt = false;
    this->storage.loadCredentials(savedSSID, savedPass, savedUser, savedIsEnt, savedBackend);

    streamCaptivePortalHTML(this->server, savedSSID, savedPass, savedUser, savedIsEnt, savedBackend);
    this->server.sendContent("");
}

void CaptivePortal::handleStressTest() {
    if (this->server.hasArg("action") && this->server.arg("action") == "stop") {
        StressTester::getInstance().stop();
    } else if (this->server.hasArg("action") && this->server.arg("action") == "start") {
        StressTester::getInstance().start();
    }
    String json = "{";
    json += "\"running\":" + String(StressTester::getInstance().isRunning() ? "true" : "false") + ",";
    json += "\"elapsedSec\":" + String(StressTester::getInstance().getElapsedSec()) + ",";
    json += "\"chipTemp\":" + String(StressTester::getInstance().getChipTemperature(), 1) + ",";
    json += "\"cpu\":" + String(StressTester::getInstance().getCpuLoad(), 1) + ",";
    json += "\"cpu0\":" + String(StressTester::getInstance().getCpuLoadCore0(), 1) + ",";
    json += "\"cpu1\":" + String(StressTester::getInstance().getCpuLoadCore1(), 1);
    json += "}";
    this->server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    this->server.send(200, "application/json", json);
}

void CaptivePortal::handleHistoryGet() {
    if (this->server.hasArg("epoch")) {
        uint32_t epoch = (uint32_t)this->server.arg("epoch").toInt();
        if (epoch > 1000000000) {
            struct timeval tv = { (time_t)epoch, 0 };
            settimeofday(&tv, NULL);
            DataLogger::getInstance().syncEpoch(epoch);
        }
    }

    uint16_t filterMins = 1440; // Mac dinh 24h
    if (this->server.hasArg("mins")) {
        int m = this->server.arg("mins").toInt();
        if (m >= 0 && m <= 1440) {
            filterMins = (uint16_t)m;
        }
    } else if (this->server.hasArg("hours")) {
        int h = this->server.arg("hours").toInt();
        if (h > 0 && h <= 24) {
            filterMins = (uint16_t)(h * 60);
        }
    }
    String json = DataLogger::getInstance().getHistoryJson(filterMins);
    this->server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    this->server.send(200, "application/json", json);
}

void CaptivePortal::handleHistoryCsv() {
    String csv = DataLogger::getInstance().getCsvString();
    this->server.sendHeader("Content-Disposition", "attachment; filename=\"sensor_history.csv\"");
    this->server.send(200, "text/csv", csv);
}

void CaptivePortal::handleSyncTime() {
    if (this->server.hasArg("epoch")) {
        uint32_t epoch = (uint32_t)this->server.arg("epoch").toInt();
        if (epoch > 1000000000) {
            struct timeval tv = { (time_t)epoch, 0 };
            settimeofday(&tv, NULL);
            DataLogger::getInstance().syncEpoch(epoch);
            Serial.printf("[Portal] Da dong bo thoi gian thuc tu client: %u (Unix Epoch)\n", epoch);
            this->server.send(200, "application/json", "{\"status\":\"ok\"}");
            return;
        }
    }
    this->server.send(400, "application/json", "{\"status\":\"error\"}");
}


void CaptivePortal::handlePowerGet() {
    String json = "{";
    json += "\"cpuFreq\":" + String((int)this->powerManager.getCurrentCpuFreq()) + ",";
    json += "\"wifiMode\":" + String((int)this->powerManager.getWifiPowerMode()) + ",";
    json += "\"bluetooth\":" + String(this->powerManager.isBluetoothEnabled() ? "true" : "false") + ",";
    json += "\"ledHz\":" + String(this->powerManager.getLedUpdateHz()) + ",";
    json += "\"estimatedCurrent\":" + String(this->powerManager.getEstimatedCurrentDraw(), 1) + ",";
    json += "\"estimatedSavings\":" + String(this->powerManager.getEstimatedPowerSavings(), 1) + ",";
    json += "\"actualCpuFreq\":" + String(getCpuFrequencyMhz());
    json += "}";
    this->server.send(200, "application/json", json);
}

void CaptivePortal::handlePowerSet() {
    bool changed = false;
    
    if (this->server.hasArg("cpu_freq")) {
        int freq = this->server.arg("cpu_freq").toInt();
        if (freq == 80 || freq == 160 || freq == 240) {
            this->powerManager.applyCpuFrequency((CpuFrequency)freq);
            changed = true;
        }
    }
    
    if (this->server.hasArg("wifi_mode")) {
        int mode = this->server.arg("wifi_mode").toInt();
        if (mode >= 0 && mode <= 2) {
            this->powerManager.applyWifiPowerMode((WifiPowerMode)mode);
            changed = true;
        }
    }
    
    if (this->server.hasArg("bluetooth")) {
        bool enabled = (this->server.arg("bluetooth") == "true" || this->server.arg("bluetooth") == "1");
        this->powerManager.setBluetoothEnabled(enabled);
        changed = true;
    }
    
    if (this->server.hasArg("led_hz")) {
        uint8_t hz = this->server.arg("led_hz").toInt();
        if (hz == 30 || hz == 50 || hz == 100) {
            this->powerManager.setLedUpdateHz(hz);
            changed = true;
        }
    }
    
    if (changed) {
        this->server.send(200, "application/json", "{\"success\":true,\"message\":\"Cau hinh tiet kiem nang luong da duoc cap nhat!\"}");
    } else {
        this->server.send(400, "application/json", "{\"success\":false,\"message\":\"Khong co tham so nao hop le!\"}");
    }
}
