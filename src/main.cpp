#include "Config.h"
#include "drivers/Alarm.h"
#include "drivers/RgbController.h"
#include "drivers/SHT31Sensor.h"
#include "drivers/StressTester.h"
#include "drivers/DataLogger.h"
#include "drivers/PowerManager.h"
#include "network/CaptivePortal.h"
#include "network/StorageManager.h"
#include "network/TelemetryService.h"
#include "network/WiFiService.h"
#include <Arduino.h>

// Khoi tao cac module
SHT31Sensor sensor;
AlarmController alarmSystem(PIN_BUZZER, PIN_LED);
RgbController rgbLed(PIN_RGB_LED);
StorageManager storage;
PowerManager powerManager;
SensorSettings sensorSettings;
WiFiService wifiService(storage);
CaptivePortal portal(wifiService, storage, rgbLed, sensorSettings, powerManager);
TelemetryService telemetry(wifiService, storage);

// Bien quan ly thoi gian (Non-blocking)
uint32_t lastReadTime = 0;
uint32_t lastTelemetryTime = 0;
uint32_t lastLogTime = 0;

float currentTemperature = 0.0f;
float currentHumidity = 0.0f;
bool hasValidData = false;

void setup() {
  Serial.begin(SERIAL_BAUD_RATE);
  delay(500);
  Serial.println("\n========================================");
  Serial.println("   ESP32-S3 ENVIRONMENT MONITOR SYSTEM  ");
  Serial.println("========================================");

  // 1. Khoi tao phan cung co ban
  pinMode(PIN_BOOT, INPUT_PULLUP);
  alarmSystem.init();
  rgbLed.init();
  rgbLed.setPurpleBreathing(); // Mac dinh Mau Tim Cyber Breathing

  // 2. Khoi tao va tai cau hinh tu Flash NVS
  storage.init();
  storage.loadSensorSettings(sensorSettings.tempAlert, sensorSettings.humAlert,
                             sensorSettings.readIntervalSec, sensorSettings.sendIntervalSec);

  uint8_t savedMode = 2, savedR = 168, savedG = 85, savedB = 247, savedBright = 220, savedSpeed = 50;
  storage.loadLedConfig(savedMode, savedR, savedG, savedB, savedBright, savedSpeed);
  rgbLed.applyConfig(savedMode, savedR, savedG, savedB, savedBright, savedSpeed);

  // 3. Khoi tao DataLogger (Circular Ring Buffer 1440 diem & Flash LittleFS)
  DataLogger::getInstance().init();

  // 4. Khoi tao cam bien SHT31
  if (!sensor.init(PIN_SDA, PIN_SCL, SHT31_I2C_ADDR)) {
    Serial.println("[LOI] Khong tim thay cam bien SHT31! Vui long kiem tra day I2C (SDA=8, SCL=9).");
  } else {
    Serial.println("[OK] Cam bien SHT31 da san sang.");
  }

  // 5. Khoi tao dich vu mang WiFi va Web Portal
  powerManager.init();
  wifiService.init();
  portal.init();
  StressTester::getInstance().init();

  // 6. Khoi tao FreeRTOS Task tren Core 0 cho LED RGB (100 FPS muot tuyet doi)
  xTaskCreatePinnedToCore(
    [](void *pvParameters) {
      while (true) {
        rgbLed.update(currentTemperature, currentHumidity, alarmSystem.isAlarming());
        vTaskDelay(pdMS_TO_TICKS(10)); // Tan so lam tuoi 100 Hz co dinh (chu ky 10ms)
      }
    },
    "LedTaskCore0",
    3072,
    NULL,
    1,
    NULL,
    0 // Ghim vao Core 0
  );
}

void loop() {
  // 1. Xu ly cac tac vu mang, Stress Test va DataLogger
  wifiService.loop();
  portal.loop();
  StressTester::getInstance().loop();
  DataLogger::getInstance().loop();

  uint32_t currentMillis = millis();

  // 2. Doc cam bien dinh ky (theo chu ky nguoi dung cai dat, non-blocking)
  if (currentMillis - lastReadTime >= sensorSettings.readIntervalSec * 1000) {
    lastReadTime = currentMillis;

    float t = 0.0f;
    float h = 0.0f;

    if (sensor.read(t, h)) {
      currentTemperature = t;
      currentHumidity = h;
      hasValidData = true;

      // Luu diem do vao Ring Buffer dinh ky moi 60 giay (1 diem/phut)
      if (currentMillis - lastLogTime >= 60000 || lastLogTime == 0) {
        lastLogTime = currentMillis;
        DataLogger::getInstance().addPoint(currentTemperature, currentHumidity);
      }

      Serial.printf("[Sensor] Nhiet do: %.2f *C | Do am: %.2f %% | Chip: %.1f *C | CPU: %.1f%% (C0:%.0f%% C1:%.0f%%)",
                    t, h, StressTester::getInstance().getChipTemperature(),
                    StressTester::getInstance().getCpuLoad(),
                    StressTester::getInstance().getCpuLoadCore0(),
                    StressTester::getInstance().getCpuLoadCore1());
      if (StressTester::getInstance().isRunning()) {
        Serial.printf(" [STRESS: %us]", StressTester::getInstance().getElapsedSec());
      }

      // Kiem tra dieu kien canh bao theo nguong da cau hinh
      if (t > sensorSettings.tempAlert || h > sensorSettings.humAlert) {
        alarmSystem.trigger();
        Serial.println(" -> [CANH BAO]");
      } else {
        alarmSystem.silence();
        Serial.println(" -> [BINH THUONG]");
      }
      portal.updateSensorData(currentTemperature, currentHumidity, true);
    } else {
      hasValidData = false;
      Serial.println("[Sensor] Loi doc du lieu tu cam bien SHT31!");
      alarmSystem.silence();
      portal.updateSensorData(0.0f, 0.0f, false);
    }
  }

  // 3. Gui du lieu len Backend API dinh ky (theo chu ky nguoi dung cai dat)
  if (currentMillis - lastTelemetryTime >= sensorSettings.sendIntervalSec * 1000) {
    lastTelemetryTime = currentMillis;

    if (wifiService.isConnected() && hasValidData) {
      telemetry.sendData(currentTemperature, currentHumidity);
    }
  }

  // 4. Kiem tra nhan giu nut BOOT (GPIO 0) de xoa cau hinh WiFi
  static uint32_t bootPressStartTime = 0;
  static bool bootWasPressed = false;

  if (digitalRead(PIN_BOOT) == LOW) {
    if (!bootWasPressed) {
      bootWasPressed = true;
      bootPressStartTime = currentMillis;
      Serial.println("[SYSTEM] Dang nhan nut BOOT... Giu 1 giay de xoa WiFi.");
    } else if (currentMillis - bootPressStartTime >= BOOT_HOLD_RESET_MS) {
      Serial.println("[SYSTEM] XAC NHAN: Da giu nut BOOT du 1 giay!");
      Serial.println("[SYSTEM] Dang xoa cau hinh WiFi trong Flash (NVS)...");

      // Coi bip va LED bat sang bao hieu
      digitalWrite(PIN_BUZZER, HIGH);
      digitalWrite(PIN_LED, HIGH);
      delay(500);
      digitalWrite(PIN_BUZZER, LOW);
      digitalWrite(PIN_LED, LOW);

      storage.clear();
      storage.saveLedConfig(RGB_MODE_BREATHING, 168, 85, 247, 220, 50);
      storage.saveSensorSettings(TEMP_ALERT_THRESHOLD, HUM_ALERT_THRESHOLD, 1, 5);
      rgbLed.setPurpleBreathing();
      WiFi.disconnect(true, true);
      delay(300);

      Serial.println("[SYSTEM] Khoi dong lai chip de phat Access Point...");
      ESP.restart();
    }
  } else {
    if (bootWasPressed) {
      bootWasPressed = false;
      if (currentMillis - bootPressStartTime < BOOT_HOLD_RESET_MS) {
        Serial.println("[SYSTEM] Da tha nut BOOT truoc 1 giay -> Huy bo xoa WiFi.");
      }
    }
  }
}
