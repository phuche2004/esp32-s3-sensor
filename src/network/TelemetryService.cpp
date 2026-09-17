#include "TelemetryService.h"

TelemetryService::TelemetryService(WiFiService &wifiService, StorageManager &storage)
    : wifiService(wifiService), storage(storage) {}

void TelemetryService::sendData(float temperature, float humidity) {
    if (!this->wifiService.isConnected()) {
        return;
    }

    String dummySSID, dummyPass, dummyUser, backendUrl;
    bool dummyIsEnt = false;
    this->storage.loadCredentials(dummySSID, dummyPass, dummyUser, dummyIsEnt, backendUrl);

    if (backendUrl.length() == 0) {
        return;
    }

    HTTPClient http;
    http.setTimeout(3000); // 3s timeout
    
    if (http.begin(backendUrl)) {
        http.addHeader("Content-Type", "application/json");

        String payload = "{\"temperature\":" + String(temperature, 2) + 
                         ",\"humidity\":" + String(humidity, 2) + "}";

        int httpCode = http.POST(payload);
        if (httpCode > 0) {
            Serial.printf("[Backend] Da gui du lieu len API (%s) -> Ma HTTP: %d\n", backendUrl.c_str(), httpCode);
        } else {
            Serial.printf("[Backend] Gui du lieu that bai! Loi: %s\n", http.errorToString(httpCode).c_str());
        }
        http.end();
    } else {
        Serial.println("[Backend] Khong the khoi tao ket noi toi URL!");
    }
}
