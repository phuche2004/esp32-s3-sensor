#include "Alarm.h"

AlarmController::AlarmController(uint8_t buzzerPin, uint8_t ledPin) 
    : buzzerPin(buzzerPin), ledPin(ledPin), active(false) {}

void AlarmController::init() {
    pinMode(this->buzzerPin, OUTPUT);
    pinMode(this->ledPin, OUTPUT);

    // Dam bao tat coi va tat LED ngay khi khoi dong
    digitalWrite(this->buzzerPin, LOW);
    digitalWrite(this->ledPin, LOW);
    this->active = false;
}

void AlarmController::trigger() {
    if (!this->active) {
        digitalWrite(this->buzzerPin, HIGH);
        digitalWrite(this->ledPin, HIGH);
        this->active = true;
    }
}

void AlarmController::silence() {
    if (this->active) {
        digitalWrite(this->buzzerPin, LOW);
        digitalWrite(this->ledPin, LOW);
        this->active = false;
    }
}

bool AlarmController::isAlarming() const {
    return this->active;
}
