#include "RgbController.h"
#include <math.h>

// Bang mau Neon danh cho che do Bar Club
static const uint8_t CLUB_COLORS[][3] = {
    {255, 0, 0},     // Do neon
    {0, 255, 255},   // Cyan electric
    {255, 0, 255},   // Magenta tim bar
    {0, 255, 0},     // Xanh lime
    {255, 255, 0},   // Vang ruc
    {0, 80, 255},    // Xanh duong sau
    {255, 255, 255}, // Trang chop strobe
    {255, 20, 147}   // Hong Deep Pink
};
static const uint8_t NUM_CLUB_COLORS = sizeof(CLUB_COLORS) / sizeof(CLUB_COLORS[0]);

RgbController::RgbController(uint8_t pin)
    : pin(pin), currentMode(RGB_MODE_BREATHING),
      colorR(168), colorG(85), colorB(247), // Tim mac dinh
      brightness(220), speed(50),
      lastUpdateTick(0), animStep(0),
      clubStep(0), clubColorIdx(0), clubStepDuration(35) {}

void RgbController::init() {
    this->lastUpdateTick = millis();
    this->animStep = 0;
}

void RgbController::setMode(RgbMode mode) {
    this->currentMode = mode;
    this->animStep = 0;
    this->clubStep = 0;
    if (mode == RGB_MODE_OFF) {
        neopixelWrite(this->pin, 0, 0, 0);
    }
}

void RgbController::setColor(uint8_t r, uint8_t g, uint8_t b) {
    this->colorR = r;
    this->colorG = g;
    this->colorB = b;
}

void RgbController::setBrightness(uint8_t brightness) {
    this->brightness = brightness;
    if (this->currentMode == RGB_MODE_STATIC) {
        this->setHardwareRgb(this->colorR, this->colorG, this->colorB);
    } else if (this->currentMode == RGB_MODE_OFF) {
        neopixelWrite(this->pin, 0, 0, 0);
    }
}

void RgbController::setSpeed(uint8_t speed) {
    if (speed < 1) speed = 1;
    if (speed > 100) speed = 100;
    this->speed = speed;
}

void RgbController::setPurpleBreathing() {
    this->currentMode = RGB_MODE_BREATHING;
    this->colorR = 168;
    this->colorG = 85;
    this->colorB = 247;
    this->brightness = 220;
    this->speed = 50;
    this->animStep = 0;
}

void RgbController::setRainbow() {
    this->currentMode = RGB_MODE_RAINBOW;
    this->brightness = 200;
    this->speed = 50;
    this->animStep = 0;
}

void RgbController::applyConfig(uint8_t mode, uint8_t r, uint8_t g, uint8_t b, uint8_t brightness, uint8_t speed) {
    RgbMode newMode = (RgbMode)mode;
    if (this->currentMode != newMode) {
        this->currentMode = newMode;
        this->animStep = 0;
        this->clubStep = 0;
    }
    this->colorR = r;
    this->colorG = g;
    this->colorB = b;
    this->brightness = brightness;
    this->setSpeed(speed);

    if (this->currentMode == RGB_MODE_OFF) {
        neopixelWrite(this->pin, 0, 0, 0);
    } else if (this->currentMode == RGB_MODE_STATIC) {
        this->setHardwareRgb(this->colorR, this->colorG, this->colorB);
    }
}

void RgbController::setHardwareRgb(uint8_t r, uint8_t g, uint8_t b) {
    // Scale mau voi do sang chung
    uint8_t outR = (uint16_t(r) * this->brightness) / 255;
    uint8_t outG = (uint16_t(g) * this->brightness) / 255;
    uint8_t outB = (uint16_t(b) * this->brightness) / 255;
    neopixelWrite(this->pin, outR, outG, outB);
}

void RgbController::update(float temp, float hum, bool isAlert) {
    switch (this->currentMode) {
        case RGB_MODE_OFF:
            // Khong can lam gi
            break;
        case RGB_MODE_STATIC:
            // Sáng tĩnh, đã set khi apply
            break;
        case RGB_MODE_BREATHING:
            this->updateBreathing();
            break;
        case RGB_MODE_RAINBOW:
            this->updateRainbow();
            break;
        case RGB_MODE_BAR_CLUB:
            this->updateBarClub();
            break;
        case RGB_MODE_TEMP_REACTIVE:
            this->updateTempReactive(temp, hum, isAlert);
            break;
    }
}

// Bang tra quang sai thi giac CIE 1931 (Gamma 2.8) danh cho mat nguoi (256 gia tri)
static const uint8_t CIE1931_CURVE[256] = {
    0,0,0,0,0,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,3,3,3,3,3,3,3,3,4,
    4,4,4,4,4,5,5,5,5,5,6,6,6,6,6,7,7,7,7,8,8,8,8,9,9,9,10,10,10,10,
    11,11,11,12,12,12,13,13,13,14,14,15,15,15,16,16,17,17,17,18,18,19,19,20,
    20,21,21,22,22,23,23,24,24,25,25,26,26,27,28,28,29,29,30,31,31,32,32,33,
    34,34,35,36,37,37,38,39,39,40,41,42,43,43,44,45,46,47,47,48,49,50,51,52,
    53,54,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,70,71,72,73,74,75,76,
    77,79,80,81,82,83,85,86,87,88,90,91,92,94,95,96,98,99,100,102,103,105,106,108,
    109,110,112,113,115,116,118,120,121,123,124,126,128,129,131,132,134,136,138,139,141,143,
    145,146,148,150,152,154,155,157,159,161,163,165,167,169,171,173,175,177,179,181,183,185,
    187,189,191,193,196,198,200,202,204,207,209,211,214,216,218,220,223,225,228,230,232,235,
    237,240,242,245,247,250,252,255
};

void RgbController::updateBreathing() {
    // Chu ky tho theo toc do speed (1..100): 5500ms o speed=1 xuong 1200ms o speed=100
    const uint16_t TOTAL_STEPS = 500;
    uint32_t cycleMs = map(this->speed, 1, 100, 5500, 1200);
    uint32_t stepDelay = cycleMs / TOTAL_STEPS;
    if (stepDelay < 2) stepDelay = 2;

    uint32_t now = millis();
    if (now - this->lastUpdateTick < stepDelay) {
        return;
    }
    this->lastUpdateTick = now;

    // Nhip tho bat doi xung 3 pha (Apple/Razer Standard):
    // Pha 1: Hit vao (Inhale 35% -> step 0..174)
    // Pha 2: Tho ra (Exhale 50% -> step 175..424)
    // Pha 3: Nghi day (Rest 15% -> step 425..499)
    float linearFactor = 0.0f;
    const float minLuma = 0.035f; // Giu 3.5% do sang o day de LED luon co mot dom sang diu dang

    if (this->animStep < 175) {
        // Hit vao: tang tu minLuma len 1.0 theo nua chu ky hinh Cosine
        float progress = (float)this->animStep / 175.0f;
        float curve = (1.0f - cosf(progress * 3.14159265f)) * 0.5f;
        linearFactor = minLuma + curve * (1.0f - minLuma);
    } else if (this->animStep < 425) {
        // Tho ra: giam tu 1.0 ve minLuma cham rai va thu thai
        float progress = (float)(this->animStep - 175) / 250.0f;
        float curve = (1.0f + cosf(progress * 3.14159265f)) * 0.5f;
        linearFactor = minLuma + curve * (1.0f - minLuma);
    } else {
        // Nghi day: giu nguyen dom sang mo am ap truoc khi bat dau nhip moi
        linearFactor = minLuma;
    }

    // Tinh gia tri do sang toan hoc [0..255]
    uint8_t rawLuma = (uint8_t)(linearFactor * 255.0f);
    // Hieu chinh qua bang quang sai mat nguoi CIE 1931
    uint8_t correctedLuma = CIE1931_CURVE[rawLuma];

    // Scale theo mau sac va do sang tong the (brightness)
    uint16_t effBrightness = ((uint16_t)this->brightness * correctedLuma) / 255;
    uint8_t r = (uint16_t(this->colorR) * effBrightness) / 255;
    uint8_t g = (uint16_t(this->colorG) * effBrightness) / 255;
    uint8_t b = (uint16_t(this->colorB) * effBrightness) / 255;

    neopixelWrite(this->pin, r, g, b);

    this->animStep = (this->animStep + 1) % TOTAL_STEPS;
}

void RgbController::updateRainbow() {
    uint32_t stepDelay = map(this->speed, 1, 100, 20, 2);
    uint32_t now = millis();
    if (now - this->lastUpdateTick < stepDelay) {
        return;
    }
    this->lastUpdateTick = now;

    uint8_t pos = 255 - (uint8_t)(this->animStep & 0xFF);
    uint8_t r = 0, g = 0, b = 0;
    if (pos < 85) {
        r = 255 - pos * 3;
        g = 0;
        b = pos * 3;
    } else if (pos < 170) {
        pos -= 85;
        r = 0;
        g = pos * 3;
        b = 255 - pos * 3;
    } else {
        pos -= 170;
        r = pos * 3;
        g = 255 - pos * 3;
        b = 0;
    }

    this->setHardwareRgb(r, g, b);

    uint8_t stepInc = (this->speed > 75) ? 2 : 1;
    this->animStep = (this->animStep + stepInc) % 256;
}

void RgbController::updateBarClub() {
    uint32_t now = millis();
    if (now - this->lastUpdateTick < this->clubStepDuration) {
        return;
    }
    this->lastUpdateTick = now;

    // Tinh ti le thoi gian theo toc do speed (1-100)
    float speedFactor = map(this->speed, 1, 100, 150, 50) / 100.0f;

    switch (this->clubStep) {
        case 0: // Nhip 1: Chop sang mau A
            this->setHardwareRgb(CLUB_COLORS[this->clubColorIdx][0],
                                 CLUB_COLORS[this->clubColorIdx][1],
                                 CLUB_COLORS[this->clubColorIdx][2]);
            this->clubStepDuration = (uint16_t)(35 * speedFactor);
            this->clubStep++;
            break;
        case 1: // Tat toi dot ngot
            neopixelWrite(this->pin, 0, 0, 0);
            this->clubStepDuration = (uint16_t)(30 * speedFactor);
            this->clubStep++;
            break;
        case 2: // Nhip 2: Chop nhay lai mau A
            this->setHardwareRgb(CLUB_COLORS[this->clubColorIdx][0],
                                 CLUB_COLORS[this->clubColorIdx][1],
                                 CLUB_COLORS[this->clubColorIdx][2]);
            this->clubStepDuration = (uint16_t)(35 * speedFactor);
            this->clubStep++;
            break;
        case 3: // Tat toi ngan
            neopixelWrite(this->pin, 0, 0, 0);
            this->clubStepDuration = (uint16_t)(35 * speedFactor);
            this->clubStep++;
            break;
        case 4: // Nhip 3: Chop trang Strobe sieu sang
            neopixelWrite(this->pin, this->brightness, this->brightness, this->brightness);
            this->clubStepDuration = (uint16_t)(25 * speedFactor);
            this->clubStep++;
            break;
        case 5: // Nghi ngan giu nhip bass
            neopixelWrite(this->pin, 0, 0, 0);
            this->clubStepDuration = (uint16_t)(90 * speedFactor);
            this->clubStep++;
            break;
        case 6: // Nhip 4: Bass drop bung no mau Neon ngau nhien
            this->clubColorIdx = (this->clubColorIdx + 1 + (esp_random() % (NUM_CLUB_COLORS - 1))) % NUM_CLUB_COLORS;
            this->setHardwareRgb(CLUB_COLORS[this->clubColorIdx][0],
                                 CLUB_COLORS[this->clubColorIdx][1],
                                 CLUB_COLORS[this->clubColorIdx][2]);
            this->clubStepDuration = (uint16_t)(80 * speedFactor);
            this->clubStep++;
            break;
        case 7: // Tat nghi het nhip
            neopixelWrite(this->pin, 0, 0, 0);
            this->clubStepDuration = (uint16_t)(80 * speedFactor);
            this->clubStep = 0;
            break;
    }
}

void RgbController::updateTempReactive(float temp, float hum, bool isAlert) {
    uint32_t now = millis();

    // Neu dang co canh bao: Chop do gap gap (200ms)
    if (isAlert) {
        if (now - this->lastUpdateTick >= 200) {
            this->lastUpdateTick = now;
            this->animStep = !this->animStep;
            if (this->animStep) {
                this->setHardwareRgb(255, 0, 0); // Do ruc
            } else {
                neopixelWrite(this->pin, 0, 0, 0);
            }
        }
        return;
    }

    // Neu khong co canh bao: Cap nhat mau theo dai nhiet do moi 500ms
    if (now - this->lastUpdateTick < 500) {
        return;
    }
    this->lastUpdateTick = now;

    if (temp < 25.0f) {
        this->setHardwareRgb(6, 182, 212); // Xanh Cyan mat me (<25*C)
    } else if (temp <= 30.0f) {
        this->setHardwareRgb(34, 197, 94); // Xanh La de chiu (25-30*C)
    } else if (temp <= 34.0f) {
        this->setHardwareRgb(245, 158, 11); // Vang / Cam hoi am (30-34*C)
    } else {
        this->setHardwareRgb(239, 68, 68); // Do bao dong (>34*C)
    }
}
