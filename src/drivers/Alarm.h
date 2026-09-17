#pragma once
#include <Arduino.h>

class AlarmController {
public:
    AlarmController(uint8_t buzzerPin, uint8_t ledPin);

    // Khoi tao pinMode va dập tắt còi/LED ngay khi khoi dong
    void init();

    // Bat che do canh bao (Bat coi + sang den LED)
    void trigger();

    // Tat canh bao (Tat coi + tat den LED)
    void silence();

    // Kiem tra trang thai canh bao hien tai
    bool isAlarming() const;

private:
    uint8_t buzzerPin;
    uint8_t ledPin;
    bool active;
};
