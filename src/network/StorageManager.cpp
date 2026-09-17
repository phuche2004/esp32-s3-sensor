#include "StorageManager.h"
#include "Config.h"

StorageManager::StorageManager() {}

bool StorageManager::init() {
    return this->prefs.begin(this->NAMESPACE, false);
}

void StorageManager::loadCredentials(String &ssid, String &password, String &username, bool &isEnterprise, String &backendUrl) {
    this->prefs.begin(this->NAMESPACE, true);
    ssid = this->prefs.isKey("ssid") ? this->prefs.getString("ssid", "") : "";
    password = this->prefs.isKey("password") ? this->prefs.getString("password", "") : "";
    username = this->prefs.isKey("username") ? this->prefs.getString("username", "") : "";
    isEnterprise = this->prefs.isKey("is_ent") ? this->prefs.getBool("is_ent", false) : false;
    backendUrl = this->prefs.isKey("backend") ? this->prefs.getString("backend", DEFAULT_BACKEND_URL) : DEFAULT_BACKEND_URL;
    this->prefs.end();
}

void StorageManager::saveCredentials(const String &ssid, const String &password, const String &username, bool isEnterprise, const String &backendUrl) {
    this->prefs.begin(this->NAMESPACE, false);
    this->prefs.putString("ssid", ssid);
    this->prefs.putString("password", password);
    this->prefs.putString("username", username);
    this->prefs.putBool("is_ent", isEnterprise);
    this->prefs.putString("backend", backendUrl);
    this->prefs.end();
}

void StorageManager::loadLedConfig(uint8_t &mode, uint8_t &r, uint8_t &g, uint8_t &b, uint8_t &brightness, uint8_t &speed) {
    this->prefs.begin(this->NAMESPACE, true);
    mode = this->prefs.isKey("led_mode") ? this->prefs.getUChar("led_mode", 2) : 2; // Default: 2 (Breathing)
    r = this->prefs.isKey("led_r") ? this->prefs.getUChar("led_r", 168) : 168;       // Default: Tim
    g = this->prefs.isKey("led_g") ? this->prefs.getUChar("led_g", 85) : 85;
    b = this->prefs.isKey("led_b") ? this->prefs.getUChar("led_b", 247) : 247;
    brightness = this->prefs.isKey("led_bright") ? this->prefs.getUChar("led_bright", 220) : 220;
    speed = this->prefs.isKey("led_speed") ? this->prefs.getUChar("led_speed", 50) : 50;
    this->prefs.end();
}

void StorageManager::saveLedConfig(uint8_t mode, uint8_t r, uint8_t g, uint8_t b, uint8_t brightness, uint8_t speed) {
    this->prefs.begin(this->NAMESPACE, false);
    this->prefs.putUChar("led_mode", mode);
    this->prefs.putUChar("led_r", r);
    this->prefs.putUChar("led_g", g);
    this->prefs.putUChar("led_b", b);
    this->prefs.putUChar("led_bright", brightness);
    this->prefs.putUChar("led_speed", speed);
    this->prefs.end();
}

void StorageManager::loadSensorSettings(float &tempAlert, float &humAlert, uint32_t &readIntervalSec, uint32_t &sendIntervalSec) {
    this->prefs.begin(this->NAMESPACE, true);
    tempAlert = this->prefs.isKey("cfg_t_alert") ? this->prefs.getFloat("cfg_t_alert", TEMP_ALERT_THRESHOLD) : TEMP_ALERT_THRESHOLD;
    humAlert = this->prefs.isKey("cfg_h_alert") ? this->prefs.getFloat("cfg_h_alert", HUM_ALERT_THRESHOLD) : HUM_ALERT_THRESHOLD;
    readIntervalSec = this->prefs.isKey("cfg_r_sec") ? this->prefs.getUInt("cfg_r_sec", 1) : 1;
    sendIntervalSec = this->prefs.isKey("cfg_s_sec") ? this->prefs.getUInt("cfg_s_sec", 5) : 5;
    this->prefs.end();
}

void StorageManager::saveSensorSettings(float tempAlert, float humAlert, uint32_t readIntervalSec, uint32_t sendIntervalSec) {
    this->prefs.begin(this->NAMESPACE, false);
    this->prefs.putFloat("cfg_t_alert", tempAlert);
    this->prefs.putFloat("cfg_h_alert", humAlert);
    this->prefs.putUInt("cfg_r_sec", readIntervalSec);
    this->prefs.putUInt("cfg_s_sec", sendIntervalSec);
    this->prefs.end();
}

bool StorageManager::hasCredentials() {
    this->prefs.begin(this->NAMESPACE, true);
    bool has = this->prefs.isKey("ssid") && (this->prefs.getString("ssid", "").length() > 0);
    this->prefs.end();
    return has;
}

void StorageManager::clear() {
    this->prefs.begin(this->NAMESPACE, false);
    this->prefs.clear();
    this->prefs.end();
}
