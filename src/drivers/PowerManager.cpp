#include "PowerManager.h"
#include <WiFi.h>

PowerManager::PowerManager() {
    // Mac dinh: giu nguyen hien tai (240MHz, Full WiFi, BT off)
    this->defaultConfig.cpuFreq = CPU_240MHZ;
    this->defaultConfig.wifiMode = WIFI_POWER_FULL;
    this->defaultConfig.bluetoothEnabled = false;
    this->defaultConfig.psramEnabled = true;  // Detect tu hardware
    this->defaultConfig.ledUpdateHz = 100;
    
    this->config = this->defaultConfig;
}

void PowerManager::init() {
    // Khoi tao voi cau hinh mac dinh (khong doi gi)
    Serial.println("[Power] Khoi tao Power Manager voi cau hinh mac dinh (240MHz, Full WiFi)");
    
    // Tat Bluetooth mac dinh
    this->disableBluetooth();
}

void PowerManager::applyCpuFrequency(CpuFrequency freq) {
    uint32_t oldFreq = getCpuFrequencyMhz();
    bool success = setCpuFrequencyMhz(freq);
    
    if (success) {
        this->config.cpuFreq = freq;
        Serial.printf("[Power] Doi tan so CPU: %dMHz -> %dMHz (AP DUNG NGAY LAP TUC!)\n", 
                      oldFreq, freq);
    } else {
        Serial.printf("[Power] LOI: Khong doi duoc tan so CPU thanh %dMHz\n", freq);
    }
}

void PowerManager::applyWifiPowerMode(WifiPowerMode mode) {
    this->config.wifiMode = mode;
    
    switch (mode) {
        case WIFI_POWER_FULL:
            WiFi.setSleep(WIFI_PS_NONE);
            Serial.println("[Power] WiFi Power: FULL (Khong tiet kiem, do tre thap nhat)");
            break;
            
        case WIFI_POWER_MIN_MODEM:
            WiFi.setSleep(WIFI_PS_MIN_MODEM);
            Serial.println("[Power] WiFi Power: MIN MODEM SLEEP (Tiet kiem ~20mA, do tre +3ms)");
            break;
            
        case WIFI_POWER_MAX_MODEM:
            WiFi.setSleep(WIFI_PS_MAX_MODEM);
            Serial.println("[Power] WiFi Power: MAX MODEM SLEEP (Tiet kiem ~40mA, do tre +10ms)");
            break;
    }
}

void PowerManager::setBluetoothEnabled(bool enabled) {
    if (enabled && !this->config.bluetoothEnabled) {
        this->enableBluetooth();
    } else if (!enabled && this->config.bluetoothEnabled) {
        this->disableBluetooth();
    }
}

void PowerManager::setLedUpdateHz(uint8_t hz) {
    // Validate
    if (hz != 30 && hz != 50 && hz != 100) {
        Serial.printf("[Power] LOI: LED Hz khong hop le (%d), chi chap nhan 30/50/100\n", hz);
        return;
    }
    
    this->config.ledUpdateHz = hz;
    Serial.printf("[Power] LED refresh rate: %d Hz\n", hz);
}

void PowerManager::disableBluetooth() {
    btStop();
    this->config.bluetoothEnabled = false;
    Serial.println("[Power] Bluetooth: TAT (Tiet kiem ~12mA)");
}

void PowerManager::enableBluetooth() {
    btStart();
    this->config.bluetoothEnabled = true;
    Serial.println("[Power] Bluetooth: BAT (Tieu thu +12mA)");
}

float PowerManager::getEstimatedCurrentDraw() const {
    float basePower = this->calculateBasePower();
    
    // CPU power (theoretical)
    float cpuPower = 0.0f;
    switch (this->config.cpuFreq) {
        case CPU_80MHZ:  cpuPower = 40.0f; break;
        case CPU_160MHZ: cpuPower = 70.0f; break;
        case CPU_240MHZ: cpuPower = 100.0f; break;
    }
    
    // WiFi power
    float wifiPower = 0.0f;
    switch (this->config.wifiMode) {
        case WIFI_POWER_FULL:      wifiPower = 80.0f; break;
        case WIFI_POWER_MIN_MODEM: wifiPower = 60.0f; break;
        case WIFI_POWER_MAX_MODEM: wifiPower = 40.0f; break;
    }
    
    // Bluetooth
    float btPower = this->config.bluetoothEnabled ? 12.0f : 0.0f;
    
    // LED task overhead
    float ledPower = 0.0f;
    switch (this->config.ledUpdateHz) {
        case 30:  ledPower = 3.0f; break;
        case 50:  ledPower = 5.0f; break;
        case 100: ledPower = 8.0f; break;
    }
    
    return basePower + cpuPower + wifiPower + btPower + ledPower;
}

float PowerManager::getEstimatedPowerSavings() const {
    // So sanh voi cau hinh mac dinh (240MHz, Full WiFi)
    float currentPower = this->getEstimatedCurrentDraw();
    float defaultPower = 100.0f + 80.0f + 8.0f + 30.0f; // CPU + WiFi + LED + Base
    
    return defaultPower - currentPower;
}

float PowerManager::calculateBasePower() const {
    // Base power: Flash, RAM, Peripherals
    return 30.0f; // ~30mA baseline
}
