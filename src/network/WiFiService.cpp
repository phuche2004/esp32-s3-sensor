#include "WiFiService.h"
#include "Config.h"
#include <ESPmDNS.h>
#include <time.h>

WiFiService::WiFiService(StorageManager &storage)
    : storage(storage), state(WIFI_IDLE), connectResult(RESULT_IDLE), lastError(""),
      connectStartTime(0), lastReconnectAttempt(0), delayedStopApTime(0), currentIsEnterprise(false) {}

void WiFiService::init() {
    String dummyBackend;
    if (this->storage.hasCredentials()) {
        this->storage.loadCredentials(this->currentSSID, this->currentPass, this->currentUser, this->currentIsEnterprise, dummyBackend);
        Serial.printf("[WiFi] Tim thay WiFi da luu: %s (Che do: %s)\n", 
                      this->currentSSID.c_str(), 
                      this->currentIsEnterprise ? "WPA2-Enterprise (Truong hoc)" : "WPA2-Personal (Ca nhan)");
        this->connectStation(this->currentSSID, this->currentPass, this->currentUser, this->currentIsEnterprise, dummyBackend);
    } else {
        Serial.println("[WiFi] Chua co cau hinh WiFi nao -> Kich hoat che do phat Access Point.");
        this->startAP();
    }
}

void WiFiService::connectStation(const String &ssid, const String &password, const String &username, bool isEnterprise, const String &backendUrl) {
    this->currentSSID = ssid;
    this->currentPass = password;
    this->currentUser = username;
    this->currentIsEnterprise = isEnterprise;
    if (backendUrl.length() > 0) {
        this->currentBackendUrl = backendUrl;
    }

    this->connectResult = RESULT_CONNECTING;
    this->lastError = "";
    this->delayedStopApTime = 0;

    // Ngat ket noi STA cu ma khong tat AP neu dang mo
    WiFi.disconnect(false);
    delay(50);

    // Neu dang phat AP thi giu nguyen AP_STA de nguoi dung khong bi mat ket noi trang web
    if (this->state == WIFI_AP_PORTAL || (WiFi.getMode() & WIFI_AP)) {
        WiFi.mode(WIFI_AP_STA);
    } else {
        WiFi.mode(WIFI_STA);
    }

    if (isEnterprise) {
        Serial.printf("[WiFi] Dang thu ket noi WPA2-Enterprise: %s (User: %s)...\n", ssid.c_str(), username.c_str());
        esp_wifi_sta_wpa2_ent_clear_identity();
        esp_wifi_sta_wpa2_ent_clear_username();
        esp_wifi_sta_wpa2_ent_clear_password();

        esp_wifi_sta_wpa2_ent_set_identity((const unsigned char *)username.c_str(), username.length());
        esp_wifi_sta_wpa2_ent_set_username((const unsigned char *)username.c_str(), username.length());
        esp_wifi_sta_wpa2_ent_set_password((const unsigned char *)password.c_str(), password.length());
        esp_wifi_sta_wpa2_ent_enable();
        WiFi.begin(ssid.c_str());
    } else {
        Serial.printf("[WiFi] Dang thu ket noi WPA2-Personal: %s...\n", ssid.c_str());
        esp_wifi_sta_wpa2_ent_disable();
        if (password.length() > 0) {
            WiFi.begin(ssid.c_str(), password.c_str());
        } else {
            WiFi.begin(ssid.c_str());
        }
    }

    this->connectStartTime = millis();
    this->state = WIFI_CONNECTING;
}

void WiFiService::startAP() {
    this->state = WIFI_AP_PORTAL;
    WiFi.mode(WIFI_AP);
    
    // Phat WiFi Access Point
    if (strlen(AP_PASSWORD_DEFAULT) > 0) {
        WiFi.softAP(AP_SSID_DEFAULT, AP_PASSWORD_DEFAULT);
    } else {
        WiFi.softAP(AP_SSID_DEFAULT);
    }

    IPAddress apIP = WiFi.softAPIP();
    Serial.println("==================================================");
    Serial.printf("[AP] Da phat WiFi: %s\n", AP_SSID_DEFAULT);
    Serial.printf("[AP] Dia chi IP Web UI: http://%s\n", apIP.toString().c_str());
    Serial.println("[AP] Hay dung dien thoai ket noi vao WiFi nay de cai dat.");
    Serial.println("==================================================");
}

void WiFiService::stopAP() {
    if (WiFi.getMode() & WIFI_AP) {
        Serial.println("[AP] Da tat phat Access Point.");
        WiFi.softAPdisconnect(true);
        WiFi.mode(WIFI_STA);
    }
}

void WiFiService::triggerShutdownCountdown(uint32_t delayMs) {
    if (WiFi.getMode() & WIFI_AP) {
        this->delayedStopApTime = millis() + delayMs;
        Serial.printf("[AP] Web da nhan ket qua -> Bat dau dem nguoc dung %u ms de tat Access Point!\n", delayMs);
    }
}

void WiFiService::loop() {
    // Xu ly hen gio tat AP sau khi nguoi dung da xem thong bao thanh cong tren web
    if (this->delayedStopApTime > 0 && millis() >= this->delayedStopApTime) {
        this->delayedStopApTime = 0;
        this->stopAP();
    }

    if (this->state == WIFI_CONNECTING) {
        wl_status_t status = WiFi.status();
        if (status == WL_CONNECTED) {
            this->state = WIFI_CONNECTED;
            this->connectResult = RESULT_SUCCESS;
            this->lastError = "";
            Serial.println();
            Serial.println("==================================================");
            Serial.printf("[WiFi] KET NOI THANH CONG! IP: %s\n", WiFi.localIP().toString().c_str());
            Serial.println("==================================================");

            // Khoi tao ten mien mDNS de truy cap qua http://esp32-sensor.local
            MDNS.end();
            if (MDNS.begin(MDNS_HOSTNAME)) {
                MDNS.addService("http", "tcp", 80);
                Serial.printf("[mDNS] Da kich hoat ten mien: http://%s.local\n", MDNS_HOSTNAME);
            } else {
                Serial.println("[mDNS] Loi khoi tao mDNS!");
            }

            // Dong bo thoi gian thuc UTC+7 qua Internet (NTP)
            configTime(7 * 3600, 0, "pool.ntp.org", "time.google.com");
            Serial.println("[NTP] Da khoi tao dong bo thoi gian tu pool.ntp.org");
            
            // Chi luu vao bo nho Flash NVS khi da chac chan ket noi duoc!
            this->storage.saveCredentials(this->currentSSID, this->currentPass, this->currentUser, this->currentIsEnterprise, this->currentBackendUrl);

            // Hen gio du phong (failsafe) tu tat AP sau 8s neu trinh duyet khong chu dong bao
            if (WiFi.getMode() & WIFI_AP) {
                this->delayedStopApTime = millis() + 8000;
            }
        } else if (status == WL_CONNECT_FAILED) {
            Serial.println();
            Serial.println("[WiFi] Ket noi that bai: Sai mat khau hoac xac thuc that bai!");
            this->connectResult = RESULT_FAILED;
            this->lastError = "Sai mat khau hoac xac thuc that bai! Vui long kiem tra lai.";
            this->state = WIFI_AP_PORTAL;
            if (!(WiFi.getMode() & WIFI_AP)) {
                this->startAP();
            }
        } else if (status == WL_NO_SSID_AVAIL) {
            Serial.println();
            Serial.println("[WiFi] Ket noi that bai: Khong tim thay mang WiFi chi dinh!");
            this->connectResult = RESULT_FAILED;
            this->lastError = "Khong tim thay mang WiFi chi dinh! Vui long kiem tra ten SSID.";
            this->state = WIFI_AP_PORTAL;
            if (!(WiFi.getMode() & WIFI_AP)) {
                this->startAP();
            }
        } else if (millis() - this->connectStartTime > WIFI_CONNECT_TIMEOUT_MS) {
            Serial.println();
            Serial.println("[WiFi] Ket noi that bai (Timeout)! Mang khong phan hoi.");
            this->connectResult = RESULT_FAILED;
            this->lastError = "Het thoi gian cho (Timeout)! Vui long kiem tra lai mat khau.";
            this->state = WIFI_AP_PORTAL;
            if (!(WiFi.getMode() & WIFI_AP)) {
                this->startAP();
            }
        }
    } else if (this->state == WIFI_CONNECTED) {
        // Tu dong ket noi lai neu rot mang
        if (WiFi.status() != WL_CONNECTED) {
            if (millis() - this->lastReconnectAttempt > 10000) {
                this->lastReconnectAttempt = millis();
                Serial.println("[WiFi] Bi mat ket noi, dang thu ket noi lai...");
                WiFi.reconnect();
            }
        }
    }
}

bool WiFiService::isConnected() const {
    return this->state == WIFI_CONNECTED && WiFi.status() == WL_CONNECTED;
}

bool WiFiService::isAPMode() const {
    return this->state == WIFI_AP_PORTAL;
}

String WiFiService::getIP() const {
    if (this->isConnected()) {
        return WiFi.localIP().toString();
    } else if (this->isAPMode()) {
        return WiFi.softAPIP().toString();
    }
    return "0.0.0.0";
}

String WiFiService::getSSID() const {
    return this->currentSSID;
}

WiFiConnectResult WiFiService::getConnectResult() const {
    return this->connectResult;
}

String WiFiService::getLastError() const {
    return this->lastError;
}
