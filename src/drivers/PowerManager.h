#ifndef POWERMANAGER_H
#define POWERMANAGER_H

#include <Arduino.h>
#include "esp_wifi.h"
#include "esp_bt.h"

enum CpuFrequency {
    CPU_80MHZ = 80,
    CPU_160MHZ = 160,
    CPU_240MHZ = 240
};

enum WifiPowerMode {
    WIFI_POWER_FULL = 0,      // No power save
    WIFI_POWER_MIN_MODEM = 1,  // Min modem sleep
    WIFI_POWER_MAX_MODEM = 2   // Max modem sleep
};

struct PowerConfig {
    CpuFrequency cpuFreq;
    WifiPowerMode wifiMode;
    bool bluetoothEnabled;
    bool psramEnabled;
    uint8_t ledUpdateHz;       // LED refresh rate (30, 50, 100 Hz)
};

class PowerManager {
public:
    PowerManager();
    
    void init();
    void applyCpuFrequency(CpuFrequency freq);
    void applyWifiPowerMode(WifiPowerMode mode);
    void setBluetoothEnabled(bool enabled);
    void setLedUpdateHz(uint8_t hz);
    
    // Getters
    CpuFrequency getCurrentCpuFreq() const { return this->config.cpuFreq; }
    WifiPowerMode getWifiPowerMode() const { return this->config.wifiMode; }
    bool isBluetoothEnabled() const { return this->config.bluetoothEnabled; }
    uint8_t getLedUpdateHz() const { return this->config.ledUpdateHz; }
    
    // Power monitoring
    float getEstimatedCurrentDraw() const;
    float getEstimatedPowerSavings() const;
    
private:
    PowerConfig config;
    PowerConfig defaultConfig;
    
    void disableBluetooth();
    void enableBluetooth();
    float calculateBasePower() const;
};

#endif
