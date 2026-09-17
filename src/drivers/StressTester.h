#pragma once
#include <Arduino.h>
#include <math.h>
#include "esp_freertos_hooks.h"

class StressTester {
public:
    static StressTester& getInstance();

    void init();
    void start();
    void stop();
    void loop();

    bool isRunning() const { return this->running; }
    uint32_t getElapsedSec() const { return this->elapsedSec; }
    float getChipTemperature() const { return temperatureRead(); }
    float getCpuLoad() const { return this->cpuLoadTotal; }
    float getCpuLoadCore0() const { return this->cpuLoadCore0; }
    float getCpuLoadCore1() const { return this->cpuLoadCore1; }

private:
    StressTester();

    volatile bool running;
    bool initialized;
    uint32_t startTime;
    uint32_t elapsedSec;
    TaskHandle_t taskHandle0;
    TaskHandle_t taskHandle1;

    uint32_t lastCpuCheck;
    uint32_t maxIdle0;
    uint32_t maxIdle1;
    float cpuLoadCore0;
    float cpuLoadCore1;
    float cpuLoadTotal;

    static volatile uint32_t idleCounter0;
    static volatile uint32_t idleCounter1;

    static bool idleHook0();
    static bool idleHook1();
    static void taskCore0(void *pvParameters);
    static void taskCore1(void *pvParameters);
};
