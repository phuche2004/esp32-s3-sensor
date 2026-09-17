#pragma once
#include <Arduino.h>

enum RgbMode : uint8_t {
    RGB_MODE_OFF = 0,
    RGB_MODE_STATIC = 1,
    RGB_MODE_BREATHING = 2,
    RGB_MODE_RAINBOW = 3,
    RGB_MODE_BAR_CLUB = 4,
    RGB_MODE_TEMP_REACTIVE = 5
};

class RgbController {
public:
    RgbController(uint8_t pin);

    void init();
    void setMode(RgbMode mode);
    void setColor(uint8_t r, uint8_t g, uint8_t b);
    void setBrightness(uint8_t brightness);
    void setSpeed(uint8_t speed); // 1 (rat cham) den 100 (sieu toc)

    void setPurpleBreathing();
    void setRainbow();
    void applyConfig(uint8_t mode, uint8_t r, uint8_t g, uint8_t b, uint8_t brightness, uint8_t speed);

    RgbMode getMode() const { return this->currentMode; }
    uint8_t getR() const { return this->colorR; }
    uint8_t getG() const { return this->colorG; }
    uint8_t getB() const { return this->colorB; }
    uint8_t getBrightness() const { return this->brightness; }
    uint8_t getSpeed() const { return this->speed; }

    // Non-blocking update goi trong loop()
    void update(float temp = 0.0f, float hum = 0.0f, bool isAlert = false);

private:
    uint8_t pin;
    RgbMode currentMode;
    uint8_t colorR;
    uint8_t colorG;
    uint8_t colorB;
    uint8_t brightness;
    uint8_t speed;

    // Bien quan ly thoi gian non-blocking
    uint32_t lastUpdateTick;
    uint16_t animStep;
    uint8_t clubStep;
    uint8_t clubColorIdx;
    uint16_t clubStepDuration;

    void updateBreathing();
    void updateRainbow();
    void updateBarClub();
    void updateTempReactive(float temp, float hum, bool isAlert);
    void setHardwareRgb(uint8_t r, uint8_t g, uint8_t b);
};
