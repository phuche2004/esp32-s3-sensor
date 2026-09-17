#include <Wire.h>
#include "Adafruit_SHT31.h"

Adafruit_SHT31 sht31 = Adafruit_SHT31();

// Định nghĩa chân cắm
#define SDA_PIN 8
#define SCL_PIN 9
#define BUZZER_PIN 10

// Đèn cảnh báo duy nhất
#define LED_RED 13  

void setup() {
  Serial.begin(115200);
  while (!Serial) delay(10);
  
  // Cài đặt các chân OUTPUT
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  
  // Đảm bảo tắt hết lúc khởi động
  digitalWrite(BUZZER_PIN, LOW);
  digitalWrite(LED_RED, LOW);

  Wire.begin(SDA_PIN, SCL_PIN);
  if (!sht31.begin(0x44)) {
    Serial.println("Loi: Khong tim thay SHT31!");
    while (1) delay(1);
  }
  Serial.println("SHT31 OK! He thong da san sang...");
}

void loop() {
  float t = sht31.readTemperature();
  float h = sht31.readHumidity();

  // In dữ liệu ra Serial
  if (!isnan(t) && !isnan(h)) {
    Serial.print("Nhiet do: "); Serial.print(t); Serial.print(" *C | ");
    Serial.print("Do am: "); Serial.print(h); Serial.println(" %");
  } else {
    Serial.println("Loi doc cam bien!");
  }

  // LOGIC CẢNH BÁO
  if (t > 33.0 || h > 78.0) {
    digitalWrite(LED_RED, HIGH);   
    digitalWrite(BUZZER_PIN, HIGH);
    delay(1000);
  } else {
    // An toàn: Tắt đèn, tắt còi
    digitalWrite(LED_RED, LOW);   
    digitalWrite(BUZZER_PIN, LOW);
    
    // Đợi đủ 1s rồi lặp lại
    delay(1000); 
  }
}