#pragma once
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_SHT31.h>

class SHT31Sensor {
public:
    SHT31Sensor();
    
    // Khoi tao I2C va ket noi toi chip SHT31
    bool init(uint8_t sdaPin, uint8_t sclPin, uint8_t i2cAddr = 0x44);

    // Doc gia tri nhiet do va do am, tra ve true neu thanh cong
    bool read(float &temperature, float &humidity);

    // Kiem tra trang thai cam bien co san sang khong
    bool isConnected() const;

private:
    Adafruit_SHT31 sensor;
    uint8_t address;
    bool ready;
};
