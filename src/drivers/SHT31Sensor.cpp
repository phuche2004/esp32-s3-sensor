#include "SHT31Sensor.h"

SHT31Sensor::SHT31Sensor() : address(0x44), ready(false) {}

bool SHT31Sensor::init(uint8_t sdaPin, uint8_t sclPin, uint8_t i2cAddr) {
    this->address = i2cAddr;
    
    // Khoi tao bus I2C tren chan duoc chi dinh
    Wire.begin(sdaPin, sclPin);
    Wire.setClock(400000); // 400kHz Fast Mode (Sensirion SHT31 ho tro toi 1000kHz)

    // Kiem tra giao tiep voi cam bien
    if (!this->sensor.begin(this->address)) {
        this->ready = false;
        return false;
    }

    this->ready = true;
    return true;
}

bool SHT31Sensor::read(float &temperature, float &humidity) {
    if (!this->ready) {
        return false;
    }

    float t = this->sensor.readTemperature();
    float h = this->sensor.readHumidity();

    if (isnan(t) || isnan(h)) {
        return false;
    }

    temperature = t;
    humidity = h;
    return true;
}

bool SHT31Sensor::isConnected() const {
    return this->ready;
}
