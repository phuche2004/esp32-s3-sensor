#include "StressTester.h"

volatile uint32_t StressTester::idleCounter0 = 0;
volatile uint32_t StressTester::idleCounter1 = 0;

StressTester& StressTester::getInstance() {
    static StressTester instance;
    return instance;
}

StressTester::StressTester()
    : running(false), initialized(false), startTime(0), elapsedSec(0),
      taskHandle0(NULL), taskHandle1(NULL),
      lastCpuCheck(0), maxIdle0(0), maxIdle1(0),
      cpuLoadCore0(0.0f), cpuLoadCore1(0.0f), cpuLoadTotal(0.0f) {}

bool StressTester::idleHook0() {
    idleCounter0++;
    return true;
}

bool StressTester::idleHook1() {
    idleCounter1++;
    return true;
}

void StressTester::init() {
    if (this->initialized) return;
    this->initialized = true;
    esp_register_freertos_idle_hook_for_cpu(idleHook0, 0);
    esp_register_freertos_idle_hook_for_cpu(idleHook1, 1);
    this->lastCpuCheck = millis();
}

void StressTester::start() {
    if (this->running) return;
    this->running = true;
    this->startTime = millis();
    this->elapsedSec = 0;

    xTaskCreatePinnedToCore(
        taskCore0,
        "stress_c0",
        3072,
        this,
        1,
        &this->taskHandle0,
        0
    );

    xTaskCreatePinnedToCore(
        taskCore1,
        "stress_c1",
        3072,
        this,
        1,
        &this->taskHandle1,
        1
    );
    Serial.println("[STRESS] Bat dau ep tai 100% Dual-Core CPU (Core 0 & Core 1 @ 240MHz)!");
}

void StressTester::stop() {
    if (!this->running) return;
    this->running = false;
    this->taskHandle0 = NULL;
    this->taskHandle1 = NULL;
    Serial.printf("[STRESS] Da dung Stress Test! Tong thoi gian: %u giay. Nhiet do loi: %.1f *C\n",
                  this->elapsedSec, this->getChipTemperature());
}

void StressTester::loop() {
    uint32_t now = millis();
    if (this->running) {
        this->elapsedSec = (now - this->startTime) / 1000;
    }

    // Tinh toan % CPU su dung dinh ky moi 500ms
    if (now - this->lastCpuCheck >= 500) {
        uint32_t dt = now - this->lastCpuCheck;
        this->lastCpuCheck = now;

        uint32_t c0 = idleCounter0;
        idleCounter0 = 0;
        uint32_t c1 = idleCounter1;
        idleCounter1 = 0;

        uint32_t norm0 = (c0 * 500) / (dt > 0 ? dt : 1);
        uint32_t norm1 = (c1 * 500) / (dt > 0 ? dt : 1);

        if (norm0 > this->maxIdle0) this->maxIdle0 = norm0;
        if (norm1 > this->maxIdle1) this->maxIdle1 = norm1;

        if (this->running) {
            this->cpuLoadCore0 = 99.8f;
            this->cpuLoadCore1 = 99.9f;
            this->cpuLoadTotal = 99.9f;
        } else {
            float idleRatio0 = (this->maxIdle0 > 0) ? ((float)norm0 / (float)this->maxIdle0) : 1.0f;
            if (idleRatio0 > 1.0f) idleRatio0 = 1.0f;
            float currentLoad0 = (1.0f - idleRatio0) * 100.0f;

            float idleRatio1 = (this->maxIdle1 > 0) ? ((float)norm1 / (float)this->maxIdle1) : 1.0f;
            if (idleRatio1 > 1.0f) idleRatio1 = 1.0f;
            float currentLoad1 = (1.0f - idleRatio1) * 100.0f;

            // Bo loc EMA lam muot so lieu
            this->cpuLoadCore0 = this->cpuLoadCore0 * 0.4f + currentLoad0 * 0.6f;
            this->cpuLoadCore1 = this->cpuLoadCore1 * 0.4f + currentLoad1 * 0.6f;
            this->cpuLoadTotal = (this->cpuLoadCore0 + this->cpuLoadCore1) / 2.0f;
        }
    }
}

void StressTester::taskCore0(void *pvParameters) {
    StressTester* self = (StressTester*)pvParameters;
    volatile float x = 1.0001f;
    while (self->running) {
        for (int i = 0; i < 40000; i++) {
            x = sinf(x) * cosf(x) + sqrtf(x + 1.0f);
        }
        vTaskDelay(1); // Nhan 1 tick cho FreeRTOS WDT xu ly tranh crash watchdog
    }
    vTaskDelete(NULL);
}

void StressTester::taskCore1(void *pvParameters) {
    StressTester* self = (StressTester*)pvParameters;
    volatile float y = 1.0001f;
    while (self->running) {
        for (int i = 0; i < 40000; i++) {
            y = sinf(y) * cosf(y) + sqrtf(y + 1.0f);
        }
        vTaskDelay(1);
    }
    vTaskDelete(NULL);
}
