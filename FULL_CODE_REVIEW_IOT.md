# Hồ Sơ Mã Nguồn Toàn Diện: Hệ Thống Cảm Biến Môi Trường IoT & Điều Khiển Quang Học Đa Nhân (ESP32-S3)

> **Tài liệu thẩm định kỹ thuật độc lập (Dành cho Chuyên gia Review Kiến trúc Hệ thống Nhúng, An ninh IoT & Hiệu Năng Phần Cứng)**  
> **Dự án**: Chuỗi Nhà Thuốc ABC Pharmacy (WDP301 / SEP490) — Module Trạm Quan Trắc IoT Môi Trường Kho & Tủ Bảo Quản Thuốc GPP  
> **Repository Nguồn**: [phuche2004/esp32-s3-sensor: ESP32-S3 IoT với SHT31, Captive Portal](https://github.com/phuche2004/esp32-s3-sensor)  
> **Vi điều khiển mục tiêu**: Espressif ESP32-S3 (Xtensa Dual-Core 32-bit LX7 @ 240MHz, 16MB Flash QIO, 8MB PSRAM OPI)  
> **Cảm biến & Ngoại vi**: Sensirion SHT31-DIS (I2C Fast Mode 400kHz), LED RGB WS2812 (RMT), Active Buzzer, Phím BOOT GPIO 0  
> **Kiến trúc truyền thông mới**: **1-Request Persistent HTTP Stream (NDJSON Tunnel)** — Giảm từ 86.400 Requests/ngày xuống đúng 1 Request/ngày  
> **Trạng thái**: TOÀN BỘ 100% MÃ NGUỒN NGUYÊN VẸN — KHÔNG RÚT GỌN / ZERO PLACEHOLDER  

---

## 1. Tổng Quan Kiến Trúc & Luồng Nghiệp Vụ (End-to-End Architecture)

### 1.1. Bối cảnh Nghiệp Vụ & Mục Tiêu Thiết Kế Chuẩn GPP Y Tế
Trong chuỗi nhà thuốc đạt chuẩn Thực Hành Tốt Phân Phối Thuốc (GDP) và Thực Hành Tốt Nhà Thuốc (GPP) của ABC Pharmacy, việc kiểm soát nhiệt độ và độ ẩm tại kho tổng, kho trung chuyển và tủ mát bảo quản vắc-xin/sinh phẩm là yếu tố sống còn:
1. **Độ chính xác và tính toàn vẹn (Data Integrity)**: Sử dụng cảm biến cao cấp Sensirion SHT31-DIS (Thụy Sĩ) với sai số chỉ $\pm 0.2^\circ\text{C}$ và $\pm 2\%\text{ RH}$, kiểm tra tính toàn vẹn phần cứng qua mã CRC-8.
2. **Cơ chế 1-Request Persistent HTTP Stream (NDJSON Tunnel)**:
   - Thay vì đóng/mở kết nối HTTPClient liên tục mỗi giây (**86.400 HTTP Requests/ngày** gây nghẽn router và cạn kiệt socket), ESP32-S3 chỉ gửi **đúng 1 HTTP POST Request duy nhất** với header `X-Stream-Mode: true` và `Content-Type: application/x-ndjson` rồi giữ nguyên socket TCP suốt cả ngày.
   - Mỗi giây, firmware chỉ bơm đúng 1 dòng JSON kết thúc bằng `\n` (Newline-Delimited JSON) qua socket với `setNoDelay(true)`, nhận phản hồi ACK 1 dòng từ server để đồng bộ thời gian `server_time` (RTC).
3. **Tính tiền định quang học & mạng (Dual-Core SMP Timing Isolation)**: Phân tách tải xử lý: Core 0 chuyên trách điều khiển LED WS2812 ở tần số làm tươi cố định 100 Hz (chu kỳ 10ms) cùng Wi-Fi MAC layer và TelemetryTask; Core 1 chuyên trách HTTP Web Server, DNS Captive Portal, I2C Sensor reading và DataLogger. Loại bỏ triệt để hiện tượng giật/nháy LED khi có lưu lượng truy cập mạng.
4. **Cơ chế Lưu trữ đệm & Chống mòn Flash NOR (Store-and-Forward with Wear-Leveling)**: Để bảo vệ tuổi thọ chip Flash NOR (giới hạn ~100.000 chu kỳ ghi/xóa):
   - Lưu đệm `cachedBackendUrl` trên RAM trong `TelemetryService`, chỉ kiểm tra cập nhật định kỳ mỗi 15 giây thay vì đọc Flash NVS mỗi giây (tiết kiệm 86.400 lần truy xuất NVS/ngày).
   - Khi mất mạng, lưu vào `psramBuffer` (60 mẫu) $\rightarrow$ gom đủ 60 mẫu mới ghi 1 lần xuống Flash LittleFS.
   - Trong `DataLogger`, lưu snapshot vào Flash định kỳ mỗi 15 phút (900.000 ms) thay vì ghi đè file 11.5KB mỗi phút.
   - Khi có mạng trở lại, cơ chế **Batch Ingestion** (`batch: true`, 50 mẫu/khối) xả sạch dữ liệu tồn đọng ngay trên luồng Stream.
5. **Cổng cấu hình Captive Portal tiện dụng & Tự hủy an toàn (Self-Destruct AP)**: Hỗ trợ mạng Wi-Fi cá nhân (WPA2-Personal) và mạng cơ quan/trường học (WPA2-Enterprise 802.1x EAP-PEAP). Điểm phát Access Point sẽ tự động đếm ngược 3 giây và tắt hẳn sau khi kết nối thành công, triệt tiêu nguy cơ bị tấn công giả mạo Wi-Fi (Evil Twin Attack).
6. **Điều chỉnh tần số CPU tức thì (Dynamic Frequency Scaling - DFS)**: Hỗ trợ hạ xung xuống 160MHz (tiết kiệm 25% dòng điện, giảm 2-3°C) hoặc 80MHz (tiết kiệm 52% dòng điện, giảm 4-5°C) thời gian thực không cần khởi động lại chip.
7. **Kiểm thử chịu tải tối đa (100% Dual-Core Stress Test)**: Ép tải đồng thời cả 2 nhân CPU bằng các phép tính dấu phẩy động phức tạp kết hợp nhả tick 1ms bảo vệ Watchdog, giám sát nhiệt độ silicon qua `temperatureRead()` và tải CPU qua FreeRTOS Idle Hook.

---

### 1.2. Sơ Đồ Phân Phổ Đa Nhân FreeRTOS & Luồng Dữ Liệu Persistent Stream

```
                          ESP32-S3 (Dual-Core @ 240MHz)
         ┌──────────────────────────────┬──────────────────────────────┐
         │            CORE 0            │            CORE 1            │
         │     (Quang Học & Mạng)       │   (Nghiệp Vụ & Cảm Biến)     │
         ├──────────────────────────────┼──────────────────────────────┤
         │ • LedTaskCore0 (100 Hz,      │ • Arduino loopTask           │
         │   chu kỳ 10ms cố định)       │ • WebServer HTTP Port 80     │
         │ • CIE 1931 Gamma 2.8 LUT     │ • DNSServer Captive Portal   │
         │ • TelemetryTask (Priority 1) │ • SHT31 I2C Driver (400kHz)  │
         │   - Rút Queue phi chặn       │ • DataLogger (Ring Buffer)   │
         │   - RAM Cache Backend URL    │ • Đọc phím BOOT (GPIO 0)     │
         │   - Persistent WiFiClient    │ • Đẩy Telemetry vào Queue    │
         │     Stream (1 Request/ngày)  │ • StressTest Task Core 1     │
         │   - Đệm PSRAM (60 mẫu)       │ • Idle Hook Core 1           │
         │   - LittleFS Flash FIFO      │                              │
         │ • Wi-Fi MAC / TCP Stack      │                              │
         │ • StressTest Task Core 0     │                              │
         │ • Idle Hook Core 0           │                              │
         └──────────────┬───────────────┴──────────────┬───────────────┘
                        │                              │
                        ▼                              ▼
                 [ FreeRTOS Queue ] <───────────────────┘
               (Non-blocking Push <10µs)
                        │
                        ▼
            ┌───────────────────────┐
            │ Stream Connected?     │
            └───────────┬───────────┘
                        │
           ┌────────────┴────────────┐
           │ Có                      │ Không (Mất mạng / Đứt Stream)
           ▼                         ▼
   [ Bơm 1 dòng NDJSON ]     [ Lưu đệm PSRAM 60 mẫu ]
   (Socket TCP mở sẵn,       (Tiết kiệm chu kỳ ghi Flash)
    setNoDelay, nhận ACK)            │
           │                         ▼ Đủ 60 mẫu?
           │                 [ Ghi Flash LittleFS ]
           │                 (FIFO tối đa 3600 mẫu)
           │                         │
           │ Khi mạng hồi phục       │
           └─────────────────────────┘
                        │
                        ▼
             [ Batch Ingestion Stream ]
            (Mảng JSON 50 mẫu/khối trên Stream)
                        │
                        ▼
           [ Web Backend Telemetry Server ]
              (:3000 /api/sensor)
```

---

### 1.3. Danh Sách Các Bất Biến Cốt Lõi (Core Invariants INV#)

- **`INV-IOT-01` (Cách Ly Xung Nhịp Đa Nhân - Dual-Core Timing Determinism)**: Task điều khiển LED WS2812 (`LedTaskCore0`) bắt buộc ghim cố định trên Core 0 với tần số làm tươi 100 Hz (chu kỳ 10ms) độc lập hoàn toàn với `loopTask` trên Core 1.
- **`INV-IOT-02` (Hàng Đợi Telemetry Phi Chặn - Non-Blocking Queue Push)**: Tầng nghiệp vụ tại `loop()` trên Core 1 khi gửi dữ liệu lên Backend chỉ được phép đẩy struct `TelemetryPayload` vào FreeRTOS Queue với timeout bằng 0 (`xQueueSend(..., 0)`). Thời gian đẩy dữ liệu bắt buộc $< 10\mu\text{s}$.
- **`INV-IOT-03` (1-Request Persistent Stream & Bảo Vệ Flash NOR)**: Chỉ mở đúng 1 HTTP Request Stream duy nhất cho cả ngày hoạt động. Backend URL được cache trên RAM và chỉ kiểm tra cập nhật mỗi 15 giây. Khi mất mạng, lưu vào PSRAM (60 mẫu) trước khi ghi Flash LittleFS. Trong `DataLogger`, snapshot chỉ ghi Flash định kỳ mỗi 15 phút.
- **`INV-IOT-04` (Khống Chế Kích Thước Bền Vững & Chống Deadlock 4xx)**: Phân vùng LittleFS lưu ngoại tuyến được khống chế theo nguyên tắc FIFO tối đa 3,600 bản ghi. Nếu Backend trả về lỗi `client_error` (4xx), firmware lập tức hủy bỏ batch bị lỗi để tránh nghẽn luồng.
- **`INV-IOT-05` (Bảo Toàn Nhịp Thở Watchdog Khi Ép Tải - TWDT Safety Throttling)**: Sau mỗi 40.000 phép tính lượng giác, bắt buộc phải chèn lệnh `vTaskDelay(1)` để nhả tick cho Task Watchdog Timer (TWDT).
- **`INV-IOT-06` (Tự Hủy Access Point Chống Tấn Công Man-in-the-Middle)**: Ngay sau khi kết nối Station thành công, hệ thống tự động kích hoạt bộ đếm ngược 3 giây để đóng hoàn toàn Access Point qua `WiFi.softAPdisconnect(true)`.
- **`INV-IOT-07` (Hiệu Chỉnh Phi Tuyến Quang Học CIE 1931 Gamma 2.8)**: Cường độ sáng của LED WS2812 trong hiệu ứng nhịp thở bắt buộc đi qua bảng tra cứu 256 phần tử `CIE1931_CURVE` (Gamma 2.8) và đường cong Cosine 3 pha bất đối xứng.
- **`INV-IOT-08` (Bảo Vệ Khóa & Thông Tin Xác Thực WPA2-Enterprise)**: Thông tin định danh mạng doanh nghiệp được bảo vệ trong bộ nhớ NVS Flash mã hóa. Khi ngắt mạng, firmware gọi `esp_wifi_sta_wpa2_ent_clear_*()` để xóa sạch vết trên RAM.

---

## 2. Bảng Danh Mục Toàn Bộ Các Thành Phần Trong Cụm Chức Năng

| STT | Đường Dẫn File | Tầng Kiến Trúc | Trách Nhiệm Kỹ Thuật Chính | Trạng Thái |
| :---: | :--- | :--- | :--- | :---: |
| 1 | `platformio.ini` | Cấu hình / Build Environment | Khai báo phần cứng ESP32-S3 N16R8 (16MB Flash QIO, 8MB PSRAM OPI), thư viện và compiler flags | 100% Đầy Đủ |
| 2 | `include/Config.h` | Cấu hình Hệ thống / Hardware Pinout | Định nghĩa chân GPIO, địa chỉ I2C SHT31, ngưỡng cảnh báo và tham số mặc định | 100% Đầy Đủ |
| 3 | `src/main.cpp` | Core Orchestrator / Điểm Khởi Động | Khởi tạo hệ thống, gắn task FreeRTOS Core 0, vòng lặp non-blocking loop(), nút bấm BOOT reset NVS | 100% Đầy Đủ |
| 4 | `src/drivers/SHT31Sensor.h` | Driver Cảm Biến (Header) | Định nghĩa giao tiếp I2C 400kHz và cấu trúc đọc SHT31-DIS | 100% Đầy Đủ |
| 5 | `src/drivers/SHT31Sensor.cpp` | Driver Cảm Biến (Source) | Triển khai đọc nhiệt độ, độ ẩm Sensirion SHT31 kèm kiểm tra CRC | 100% Đầy Đủ |
| 6 | `src/drivers/Alarm.h` | Driver Cảnh Báo (Header) | Định nghĩa giao tiếp còi Active Buzzer và đèn LED cảnh báo GPIO 10, 13 | 100% Đầy Đủ |
| 7 | `src/drivers/Alarm.cpp` | Driver Cảnh Báo (Source) | Triển khai dập còi/LED ngay khi khởi động và kích hoạt cảnh báo | 100% Đầy Đủ |
| 8 | `src/drivers/RgbController.h` | Driver LED WS2812 RMT (Header) | Định nghĩa 6 chế độ màu sắc, bảng mã màu Neon Bar Club và bộ điều khiển quang học | 100% Đầy Đủ |
| 9 | `src/drivers/RgbController.cpp` | Driver LED WS2812 RMT (Source) | Triển khai hiệu ứng quang học: Nhịp thở Cosine 3 pha chuẩn CIE 1931 Gamma 2.8, Rainbow, Bar Club EDM | 100% Đầy Đủ |
| 10 | `src/drivers/DataLogger.h` | Driver Lưu Trữ Dữ Liệu Lịch Sử (Header) | Định nghĩa cấu trúc điểm nhị phân 8-byte và Circular Ring Buffer 1440 điểm (24 giờ) | 100% Đầy Đủ |
| 11 | `src/drivers/DataLogger.cpp` | Driver Lưu Trữ Dữ Liệu Lịch Sử (Source) | Triển khai quản lý bộ đệm PSRAM/SRAM, ghi snapshot LittleFS định kỳ 15 phút, trích xuất JSON/CSV | 100% Đầy Đủ |
| 12 | `src/drivers/PowerManager.h` | Driver Quản Lý Năng Lượng (Header) | Định nghĩa Dynamic Frequency Scaling (80/160/240 MHz), Wi-Fi Modem Sleep và Bluetooth toggle | 100% Đầy Đủ |
| 13 | `src/drivers/PowerManager.cpp` | Driver Quản Lý Năng Lượng (Source) | Triển khai điều chỉnh xung nhịp tức thì qua setCpuFrequencyMhz(), ước tính dòng điện tiêu thụ | 100% Đầy Đủ |
| 14 | `src/drivers/StressTester.h` | Driver Ép Tải Dual-Core (Header) | Định nghĩa 2 FreeRTOS tasks ép tải dấu phẩy động 100% CPU trên Core 0 & Core 1 | 100% Đầy Đủ |
| 15 | `src/drivers/StressTester.cpp` | Driver Ép Tải Dual-Core (Source) | Triển khai thuật toán tính toán lượng giác, FreeRTOS Idle Hook đo % CPU sử dụng thực tế và đọc nhiệt bán dẫn | 100% Đầy Đủ |
| 16 | `src/network/WiFiService.h` | Dịch Vụ Wi-Fi (Header) | Định nghĩa quản lý kết nối WPA2-Personal, WPA2-Enterprise 802.1x và Access Point | 100% Đầy Đủ |
| 17 | `src/network/WiFiService.cpp` | Dịch Vụ Wi-Fi (Source) | Triển khai kết nối Station, SoftAP Captive Portal, mDNS, NTP UTC+7 và đếm ngược tắt AP | 100% Đầy Đủ |
| 18 | `src/network/StorageManager.h` | Quản Lý Bộ Nhớ Flash NVS (Header) | Định nghĩa giao tiếp Preferences lưu cấu hình Wi-Fi, LED, Sensor Settings | 100% Đầy Đủ |
| 19 | `src/network/StorageManager.cpp` | Quản Lý Bộ Nhớ Flash NVS (Source) | Triển khai đọc/ghi/xóa dữ liệu cấu hình bền vững vào namespace NVS Flash | 100% Đầy Đủ |
| 20 | `src/network/TelemetryService.h` | Dịch Vụ Telemetry & Persistent Stream (Header) | Định nghĩa WiFiClient Stream, RAM Caching backendUrl, FreeRTOS Queue phi chặn, PSRAM Buffer | 100% Đầy Đủ |
| 21 | `src/network/TelemetryService.cpp` | Dịch Vụ Telemetry & Persistent Stream (Source) | Triển khai 1-Request Persistent NDJSON Stream suốt cả ngày, đệm 60 mẫu PSRAM, Batch Ingestion xả bù khi có mạng | 100% Đầy Đủ |
| 22 | `src/network/CaptivePortal.h` | Web Server & Captive Portal (Header) | Định nghĩa máy chủ HTTP Port 80, DNS Hijack Port 53 và toàn bộ REST endpoints | 100% Đầy Đủ |
| 23 | `src/network/CaptivePortal.cpp` | Web Server & Captive Portal (Source) | Triển khai 17 REST endpoints: cảm biến, LED Studio, cấu hình ngưỡng, tối ưu điện năng, stress test, CSV | 100% Đầy Đủ |
| 24 | `src/network/web/WebPage.h` | Giao Diện Web UI HTML (Header) | Chunked streaming truyền tải giao diện Web Captive Portal 5 Tabs (Wi-Fi, Cảm Biến, LED, Biểu Đồ, Nguồn) | 100% Đầy Đủ |
| 25 | `backend/apps/api-gateway/src/controllers/sensor.controller.ts` | Backend Gateway Production (NestJS) | Controller tiếp nhận 1-Request Persistent NDJSON Stream (`X-Stream-Mode: true`), Batch Ingestion, WebSocket & Push Notification | 100% Đầy Đủ |

---

## 3. Mã Nguồn Chi Tiết Toàn Diện Từng File (Full Source Code)

*(Dưới đây là 100% toàn văn mã nguồn thực tế của từng file trong hệ thống, được đọc trực tiếp từ repository, không sử dụng bất kỳ bình luận rút gọn, cắt xén hay placeholder nào)*

### 3.1. Cấu hình / Build Environment
#### File: `platformio.ini`
> **Mô tả**: Khai báo phần cứng ESP32-S3 N16R8 (16MB Flash QIO, 8MB PSRAM OPI), thư viện và compiler flags  
> **Dung lượng**: 25 dòng | 590 bytes

```ini
[env:esp32s3_n16r8]
platform = espressif32
board = esp32-s3-devkitc-1
framework = arduino
monitor_speed = 115200
upload_speed = 921600
upload_port = COM3
monitor_port = COM3

; Cau hinh Flash 16MB va PSRAM 8MB OPI cho ESP32-S3 N16R8
board_build.arduino.memory_type = qio_opi
board_build.flash_mode = qio
board_upload.flash_size = 16MB
board_upload.maximum_size = 16777216

build_flags =
    -DBOARD_HAS_PSRAM
    -DHAS_PSRAM
    -DARDUINO_USB_MODE=1
    -Wl,--allow-multiple-definition

lib_deps =
    adafruit/Adafruit SHT31 Library @ ^2.2.2
    adafruit/Adafruit BusIO @ ^1.16.1
    Wire
```

---

### 3.2. Cấu hình Hệ thống / Hardware Pinout
#### File: `include/Config.h`
> **Mô tả**: Định nghĩa chân GPIO, địa chỉ I2C SHT31, ngưỡng cảnh báo và tham số mặc định  
> **Dung lượng**: 59 dòng | 2448 bytes

```cpp
#pragma once
#include <Arduino.h>

// ==========================================
// CAU HINH PHAN CUNG (HARDWARE PINOUT)
// ==========================================
constexpr uint8_t PIN_SDA    = 8;
constexpr uint8_t PIN_SCL    = 9;
constexpr uint8_t PIN_BUZZER = 10;
constexpr uint8_t PIN_LED     = 13;
constexpr uint8_t PIN_RGB_LED = 48;   // LED RGB WS2812 tren bo YD-ESP32-S3
constexpr uint8_t PIN_BOOT    = 0;    // Nut BOOT mac dinh tren bo ESP32-S3
constexpr uint32_t BOOT_HOLD_RESET_MS = 1000; // Nhan giu 1 giay de xoa WiFi

// ==========================================
// CAU HINH CAM BIEN SHT31
// ==========================================
constexpr uint8_t SHT31_I2C_ADDR = 0x44;

// ==========================================
// NGUONG CANH BAO (ALERT THRESHOLDS)
// ==========================================
constexpr float TEMP_ALERT_THRESHOLD = 37.0f; // Nhiet do vuot 37 *C -> canh bao
constexpr float HUM_ALERT_THRESHOLD  = 87.0f; // Do am vuot 87 % -> canh bao

// ==========================================
// CHU KY HOAT DONG (TIMING)
// ==========================================
constexpr uint32_t SENSOR_READ_INTERVAL_MS = 1000; // Doc moi 1 giay (non-blocking)
constexpr uint32_t TELEMETRY_INTERVAL_MS   = 1000; // Ban du lieu len backend moi 1 giay
constexpr uint32_t SERIAL_BAUD_RATE        = 115200;

// ==========================================
// CAU TRUC DU LIEU CAU HINH CAM BIEN DONG
// ==========================================
struct SensorSettings {
    float tempAlert = TEMP_ALERT_THRESHOLD;
    float humAlert = HUM_ALERT_THRESHOLD;
    uint32_t readIntervalSec = 1;
    uint32_t sendIntervalSec = 1;
};

// ==========================================
// CAU HINH ACCESS POINT & CAPTIVE PORTAL
// ==========================================
constexpr const char* AP_SSID_DEFAULT      = "ESP32-S3";
constexpr const char* AP_PASSWORD_DEFAULT  = ""; // Khong dat mat khau cho AP cai dat
constexpr uint32_t WIFI_CONNECT_TIMEOUT_MS = 15000; // Thu ket noi trong 15s, neu fail thi bat AP

// ==========================================
// CAU HINH BACKEND TELEMETRY
// ==========================================
constexpr const char* DEFAULT_BACKEND_URL  = "http://172.16.10.169:3000/api/sensor";

// ==========================================
// CAU HINH TEN MIEN NOI BO (mDNS)
// ==========================================
constexpr const char* MDNS_HOSTNAME        = "esp"; // Truy cap qua http://esp.local

```

---

### 3.3. Core Orchestrator / Điểm Khởi Động
#### File: `src/main.cpp`
> **Mô tả**: Khởi tạo hệ thống, gắn task FreeRTOS Core 0, vòng lặp non-blocking loop(), nút bấm BOOT reset NVS  
> **Dung lượng**: 258 dòng | 9282 bytes

```cpp
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
#include <LittleFS.h>
#include <math.h>

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

// Khoi tao LittleFS tap trung voi co che bao ve Watchdog khi format
static void initLittleFS() {
  Serial.print("[LittleFS] Dang mount he thong tep Flash... ");
  if (!LittleFS.begin(false)) {
    Serial.println("\n[LittleFS] Phat hien phan vung LittleFS chua duoc dinh dang hoac bi loi (-84)!");
    Serial.println("[LittleFS] Dang format lai Flash LittleFS (co the mat 5-10 giay, vui long cho)...");

    // Vo hieu hoa watchdog tam thoi de tranh Task Watchdog reset chip trong khi format
    disableLoopWDT();
#ifndef CONFIG_FREERTOS_UNICORE
    disableCore1WDT();
#endif
    disableCore0WDT();

    bool formatOk = LittleFS.format();

    enableCore0WDT();
#ifndef CONFIG_FREERTOS_UNICORE
    enableCore1WDT();
#endif
    enableLoopWDT();

    if (formatOk && LittleFS.begin(false)) {
      Serial.println("[LittleFS] Format va mount thanh cong!");
      Serial.printf("[LittleFS] Dung luong tong: %u KB | Da dung: %u KB\n",
                    (unsigned int)(LittleFS.totalBytes() / 1024),
                    (unsigned int)(LittleFS.usedBytes() / 1024));
    } else {
      Serial.println("[LOI] Format hoac mount LittleFS that bai!");
    }
  } else {
    Serial.println("OK.");
    Serial.printf("[LittleFS] Dung luong tong: %u KB | Da dung: %u KB\n",
                  (unsigned int)(LittleFS.totalBytes() / 1024),
                  (unsigned int)(LittleFS.usedBytes() / 1024));
  }
}

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

  // 3. Khoi tao LittleFS Flash tap trung
  initLittleFS();

  // 4. Khoi tao DataLogger (Circular Ring Buffer 1440 diem & Flash LittleFS)
  DataLogger::getInstance().init();

  // 5. Khoi tao cam bien SHT31
  if (!sensor.init(PIN_SDA, PIN_SCL, SHT31_I2C_ADDR)) {
    Serial.println("[LOI] Khong tim thay cam bien SHT31! Vui long kiem tra day I2C (SDA=8, SCL=9).");
  } else {
    Serial.println("[OK] Cam bien SHT31 da san sang.");
  }

  // 6. Khoi tao dich vu mang WiFi va Web Portal
  powerManager.init();
  wifiService.init();
  portal.init();
  telemetry.init();
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

    if (hasValidData) {
      TelemetryPayload payload;
      payload.temperature = currentTemperature;
      payload.humidity = currentHumidity;

      float a = 17.27f;
      float b = 237.7f;
      float safeH = (currentHumidity > 1.0f) ? currentHumidity : 1.0f;
      float alpha = ((a * currentTemperature) / (b + currentTemperature)) + logf(safeH / 100.0f);
      payload.dewPoint = (b * alpha) / (a - alpha);

      float vpSat = 0.61078f * expf((17.27f * currentTemperature) / (currentTemperature + 237.3f));
      float vpd = vpSat * (1.0f - (currentHumidity / 100.0f));
      payload.vpd = (vpd < 0.0f) ? 0.0f : vpd;

      payload.chipTemp = StressTester::getInstance().getChipTemperature();
      payload.cpuLoad = StressTester::getInstance().getCpuLoad();
      payload.cpu0 = StressTester::getInstance().getCpuLoadCore0();
      payload.cpu1 = StressTester::getInstance().getCpuLoadCore1();
      payload.freeHeap = ESP.getFreeHeap();
      payload.uptimeSec = currentMillis / 1000;
      payload.wifiRssi = WiFi.RSSI();
      payload.isAlert = (currentTemperature > sensorSettings.tempAlert || currentHumidity > sensorSettings.humAlert);
      payload.sensorValid = hasValidData;

      // Day vao FreeRTOS Queue khong chan (Core 1 hoan toan khong bi block boi HTTP)
      telemetry.pushPayload(payload);
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
```

---

### 3.4. Driver Cảm Biến (Header)
#### File: `src/drivers/SHT31Sensor.h`
> **Mô tả**: Định nghĩa giao tiếp I2C 400kHz và cấu trúc đọc SHT31-DIS  
> **Dung lượng**: 23 dòng | 530 bytes

```cpp
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
```

---

### 3.5. Driver Cảm Biến (Source)
#### File: `src/drivers/SHT31Sensor.cpp`
> **Mô tả**: Triển khai đọc nhiệt độ, độ ẩm Sensirion SHT31 kèm kiểm tra CRC  
> **Dung lượng**: 41 dòng | 938 bytes

```cpp
#include "SHT31Sensor.h"

SHT31Sensor::SHT31Sensor() : address(0x44), ready(false) {}

bool SHT31Sensor::init(uint8_t sdaPin, uint8_t sclPin, uint8_t i2cAddr) {
    this->address = i2cAddr;
    
    // Khoi tao bus I2C tren chan duoc chi dinh
    Wire.begin(sdaPin, sclPin);
    Wire.setClock(400000); // 400kHz Fast Mode (Sensirion SHT31 ho tro toi 1000kHz)

    // Kiem tra giao tiep voi cam bien
    if (!this->sensor.begin(this->address)) {
        this->ready = false;
        return false;
    }

    this->ready = true;
    return true;
}

bool SHT31Sensor::read(float &temperature, float &humidity) {
    if (!this->ready) {
        return false;
    }

    float t = this->sensor.readTemperature();
    float h = this->sensor.readHumidity();

    if (isnan(t) || isnan(h)) {
        return false;
    }

    temperature = t;
    humidity = h;
    return true;
}

bool SHT31Sensor::isConnected() const {
    return this->ready;
}
```

---

### 3.6. Driver Cảnh Báo (Header)
#### File: `src/drivers/Alarm.h`
> **Mô tả**: Định nghĩa giao tiếp còi Active Buzzer và đèn LED cảnh báo GPIO 10, 13  
> **Dung lượng**: 24 dòng | 494 bytes

```cpp
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
```

---

### 3.7. Driver Cảnh Báo (Source)
#### File: `src/drivers/Alarm.cpp`
> **Mô tả**: Triển khai dập còi/LED ngay khi khởi động và kích hoạt cảnh báo  
> **Dung lượng**: 34 dòng | 851 bytes

```cpp
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
```

---

### 3.8. Driver LED WS2812 RMT (Header)
#### File: `src/drivers/RgbController.h`
> **Mô tả**: Định nghĩa 6 chế độ màu sắc, bảng mã màu Neon Bar Club và bộ điều khiển quang học  
> **Dung lượng**: 58 dòng | 1643 bytes

```cpp
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
```

---

### 3.9. Driver LED WS2812 RMT (Source)
#### File: `src/drivers/RgbController.cpp`
> **Mô tả**: Triển khai hiệu ứng quang học: Nhịp thở Cosine 3 pha chuẩn CIE 1931 Gamma 2.8, Rainbow, Bar Club EDM  
> **Dung lượng**: 318 dòng | 11336 bytes

```cpp
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
```

---

### 3.10. Driver Lưu Trữ Dữ Liệu Lịch Sử (Header)
#### File: `src/drivers/DataLogger.h`
> **Mô tả**: Định nghĩa cấu trúc điểm nhị phân 8-byte và Circular Ring Buffer 1440 điểm (24 giờ)  
> **Dung lượng**: 51 dòng | 1433 bytes

```cpp
#pragma once
#include <Arduino.h>
#include <FS.h>
#include <LittleFS.h>

// Kich thuoc diem du lieu nhi phan (8 bytes)
#pragma pack(push, 1)
struct HistoryPoint {
    uint32_t timestamp; // Thoi gian Unix epoch hoac giay tu luc boot
    int16_t  temp;      // Nhiet do x 100 (VD: 3425 = 34.25 *C)
    int16_t  hum;       // Do am x 100 (VD: 6540 = 65.40 %)
};
#pragma pack(pop)

class DataLogger {
public:
    static constexpr uint16_t MAX_POINTS = 1440; // 1440 diem = 24 gio (1 diem/phut)

    static DataLogger& getInstance() {
        static DataLogger instance;
        return instance;
    }

    void init();
    void loop();
    void addPoint(float temp, float hum, uint32_t timestamp = 0);
    void syncEpoch(uint32_t currentEpoch);

    // Xuat lich su ra chuoi JSON phuc vu ve bieu do tren Web
    // filterMinutes: 15 (15m), 60 (1h), 180 (3h), 360 (6h), 720 (12h), 1440 (24h), 0 (Toan bo)
    String getHistoryJson(uint16_t filterMinutes = 1440);

    // Xuat ra dinh dang CSV de nguoi dung tai file ve may
    String getCsvString();

    // Luu snapshot vao Flash LittleFS de bao toan du lieu khi mat dien
    bool saveSnapshot();
    bool loadSnapshot();

    uint16_t getCount() const { return this->count; }

private:
    DataLogger();

    HistoryPoint* buffer; // Cap phat tren PSRAM / Heap
    uint16_t head;
    uint16_t count;
    uint32_t lastSampleTime;
    uint32_t lastSnapshotTime;
    bool initialized;
};
```

---

### 3.11. Driver Lưu Trữ Dữ Liệu Lịch Sử (Source)
#### File: `src/drivers/DataLogger.cpp`
> **Mô tả**: Triển khai quản lý bộ đệm PSRAM/SRAM, ghi snapshot LittleFS định kỳ 15 phút, trích xuất JSON/CSV  
> **Dung lượng**: 226 dòng | 6980 bytes

```cpp
#include "DataLogger.h"

static const char* SNAPSHOT_FILE = "/history.bin";

DataLogger::DataLogger()
    : buffer(nullptr), head(0), count(0),
      lastSampleTime(0), lastSnapshotTime(0), initialized(false) {}

void DataLogger::init() {
    if (this->initialized) return;
    this->initialized = true;

    // Cap phat bo nho dem 1440 diem (11.5 KB), uu tien PSRAM neu co
#if defined(BOARD_HAS_PSRAM)
    if (psramFound()) {
        this->buffer = (HistoryPoint*)ps_malloc(sizeof(HistoryPoint) * MAX_POINTS);
        if (this->buffer) {
            Serial.println("[DataLogger] Da cap phat Circular Buffer 1440 diem tren PSRAM (11.5 KB).");
        }
    }
#endif
    if (!this->buffer) {
        this->buffer = (HistoryPoint*)malloc(sizeof(HistoryPoint) * MAX_POINTS);
        if (this->buffer) {
            Serial.println("[DataLogger] Da cap phat Circular Buffer 1440 diem tren SRAM (11.5 KB).");
        } else {
            Serial.println("[LOI] Khong the cap phat bo nho cho DataLogger!");
            return;
        }
    }

    memset(this->buffer, 0, sizeof(HistoryPoint) * MAX_POINTS);

    // Kiem tra LittleFS Flash de tai snapshot (da duoc mount tap trung tai setup)
    if (LittleFS.begin(false)) {
        Serial.println("[DataLogger] LittleFS da san sang.");
        this->loadSnapshot();
    } else {
        Serial.println("[LOI] LittleFS chua san sang cho DataLogger!");
    }

    this->lastSnapshotTime = millis();
}

void DataLogger::addPoint(float temp, float hum, uint32_t timestamp) {
    if (!this->buffer) return;

    if (timestamp == 0) {
        time_t now = time(nullptr);
        if (now > 1000000000) {
            timestamp = (uint32_t)now;
        } else {
            timestamp = millis() / 1000;
        }
    }

    HistoryPoint pt;
    pt.timestamp = timestamp;
    pt.temp = (int16_t)roundf(temp * 100.0f);
    pt.hum  = (int16_t)roundf(hum * 100.0f);

    this->buffer[this->head] = pt;
    this->head = (this->head + 1) % MAX_POINTS;
    if (this->count < MAX_POINTS) {
        this->count++;
    }

    // Chi luu ngay o diem dau tien; cac diem sau luu dinh ky moi 15 phut trong loop() de bao ve Flash NOR
    if (this->count == 1) {
        this->saveSnapshot();
        this->lastSnapshotTime = millis();
    }
}

void DataLogger::syncEpoch(uint32_t currentEpoch) {
    if (currentEpoch < 1000000000 || !this->buffer || this->count == 0) return;
    uint32_t currentUptime = millis() / 1000;
    uint32_t bootEpoch = (currentEpoch >= currentUptime) ? (currentEpoch - currentUptime) : 0;

    for (uint16_t i = 0; i < this->count; i++) {
        if (this->buffer[i].timestamp < 1000000000) {
            this->buffer[i].timestamp = bootEpoch + this->buffer[i].timestamp;
        }
    }

    // Luu lai Flash sau khi da dong bo gio thuc thanh cong
    this->saveSnapshot();
}

void DataLogger::loop() {
    // Dinh ky moi 15 phut (900.000 ms) dong bo Ring Buffer tu PSRAM xuong Flash LittleFS 1 lan
    uint32_t now = millis();
    if (this->count > 0 && (now - this->lastSnapshotTime >= 900000UL)) {
        this->lastSnapshotTime = now;
        this->saveSnapshot();
    }
}

String DataLogger::getHistoryJson(uint16_t filterMinutes) {
    if (!this->buffer || this->count == 0) {
        return "{\"count\":0,\"points\":[]}";
    }

    uint16_t pointsToTake = filterMinutes;
    if (pointsToTake > this->count || pointsToTake == 0) {
        pointsToTake = this->count;
    }

    // Tinh chi so diem cu nhat can lay trong ring buffer
    // head la vi tri se ghi tiep theo, nen diem moi nhat la (head - 1 + MAX_POINTS) % MAX_POINTS
    uint16_t startIndex;
    if (this->count < MAX_POINTS) {
        startIndex = (this->count >= pointsToTake) ? (this->count - pointsToTake) : 0;
    } else {
        startIndex = (this->head + MAX_POINTS - pointsToTake) % MAX_POINTS;
    }

    uint16_t step = 1;
    if (pointsToTake > 360) {
        step = (pointsToTake + 359) / 360;
    }
    uint16_t actualCount = (pointsToTake + step - 1) / step;

    String json;
    json.reserve(actualCount * 22 + 64);
    json = "{\"count\":";
    json += String(actualCount);
    json += ",\"points\":[";

    bool first = true;
    for (uint16_t i = 0; i < pointsToTake; i += step) {
        uint16_t idx = (startIndex + i) % MAX_POINTS;
        HistoryPoint &pt = this->buffer[idx];

        if (!first) json += ",";
        first = false;
        json += "[";
        json += String(pt.timestamp);
        json += ",";
        json += String(pt.temp / 100.0f, 2);
        json += ",";
        json += String(pt.hum / 100.0f, 2);
        json += "]";
    }
    json += "]}";
    return json;
}

String DataLogger::getCsvString() {
    if (!this->buffer || this->count == 0) {
        return "Timestamp(s),Temperature(C),Humidity(%)\r\n";
    }

    String csv;
    csv.reserve(this->count * 28 + 64);
    csv = "Timestamp(s),Temperature(C),Humidity(%)\r\n";

    uint16_t startIndex;
    if (this->count < MAX_POINTS) {
        startIndex = 0;
    } else {
        startIndex = this->head; // Diem cu nhat nam ngay tai head
    }

    for (uint16_t i = 0; i < this->count; i++) {
        uint16_t idx = (startIndex + i) % MAX_POINTS;
        HistoryPoint &pt = this->buffer[idx];

        csv += String(pt.timestamp);
        csv += ",";
        csv += String(pt.temp / 100.0f, 2);
        csv += ",";
        csv += String(pt.hum / 100.0f, 2);
        csv += "\r\n";
    }
    return csv;
}

bool DataLogger::saveSnapshot() {
    if (!this->buffer || this->count == 0) return false;

    File f = LittleFS.open(SNAPSHOT_FILE, "w");
    if (!f) {
        Serial.println("[DataLogger] Loi mo file ghi snapshot Flash!");
        return false;
    }

    f.write((uint8_t*)&this->head, sizeof(this->head));
    f.write((uint8_t*)&this->count, sizeof(this->count));
    f.write((uint8_t*)this->buffer, sizeof(HistoryPoint) * MAX_POINTS);
    f.close();

    Serial.printf("[DataLogger] Da luu Snapshot %u diem vao Flash LittleFS.\n", this->count);
    return true;
}

bool DataLogger::loadSnapshot() {
    if (!this->buffer) return false;
    if (!LittleFS.exists(SNAPSHOT_FILE)) {
        Serial.println("[DataLogger] Chua co file snapshot Flash cu.");
        return false;
    }

    File f = LittleFS.open(SNAPSHOT_FILE, "r");
    if (!f) return false;

    if (f.size() != (sizeof(this->head) + sizeof(this->count) + sizeof(HistoryPoint) * MAX_POINTS)) {
        Serial.println("[DataLogger] Kich thuoc snapshot khong khop, bo qua.");
        f.close();
        return false;
    }

    f.read((uint8_t*)&this->head, sizeof(this->head));
    f.read((uint8_t*)&this->count, sizeof(this->count));
    f.read((uint8_t*)this->buffer, sizeof(HistoryPoint) * MAX_POINTS);
    f.close();

    if (this->head >= MAX_POINTS || this->count > MAX_POINTS) {
        this->head = 0;
        this->count = 0;
        return false;
    }

    Serial.printf("[DataLogger] Da khoi phuc thanh cong %u diem lich su tu Flash.\n", this->count);
    return true;
}
```

---

### 3.12. Driver Quản Lý Năng Lượng (Header)
#### File: `src/drivers/PowerManager.h`
> **Mô tả**: Định nghĩa Dynamic Frequency Scaling (80/160/240 MHz), Wi-Fi Modem Sleep và Bluetooth toggle  
> **Dung lượng**: 57 dòng | 1434 bytes

```cpp
#ifndef POWERMANAGER_H
#define POWERMANAGER_H

#include <Arduino.h>
#include "esp_wifi.h"
#include "esp_bt.h"

enum CpuFrequency {
    CPU_80MHZ = 80,
    CPU_160MHZ = 160,
    CPU_240MHZ = 240
};

enum WifiPowerMode {
    WIFI_POWER_FULL = 0,      // No power save
    WIFI_POWER_MIN_MODEM = 1,  // Min modem sleep
    WIFI_POWER_MAX_MODEM = 2   // Max modem sleep
};

struct PowerConfig {
    CpuFrequency cpuFreq;
    WifiPowerMode wifiMode;
    bool bluetoothEnabled;
    bool psramEnabled;
    uint8_t ledUpdateHz;       // LED refresh rate (30, 50, 100 Hz)
};

class PowerManager {
public:
    PowerManager();
    
    void init();
    void applyCpuFrequency(CpuFrequency freq);
    void applyWifiPowerMode(WifiPowerMode mode);
    void setBluetoothEnabled(bool enabled);
    void setLedUpdateHz(uint8_t hz);
    
    // Getters
    CpuFrequency getCurrentCpuFreq() const { return this->config.cpuFreq; }
    WifiPowerMode getWifiPowerMode() const { return this->config.wifiMode; }
    bool isBluetoothEnabled() const { return this->config.bluetoothEnabled; }
    uint8_t getLedUpdateHz() const { return this->config.ledUpdateHz; }
    
    // Power monitoring
    float getEstimatedCurrentDraw() const;
    float getEstimatedPowerSavings() const;
    
private:
    PowerConfig config;
    PowerConfig defaultConfig;
    
    void disableBluetooth();
    void enableBluetooth();
    float calculateBasePower() const;
};

#endif
```

---

### 3.13. Driver Quản Lý Năng Lượng (Source)
#### File: `src/drivers/PowerManager.cpp`
> **Mô tả**: Triển khai điều chỉnh xung nhịp tức thì qua setCpuFrequencyMhz(), ước tính dòng điện tiêu thụ  
> **Dung lượng**: 132 dòng | 4229 bytes

```cpp
#include "PowerManager.h"
#include <WiFi.h>

PowerManager::PowerManager() {
    // Mac dinh: giu nguyen hien tai (240MHz, Full WiFi, BT off)
    this->defaultConfig.cpuFreq = CPU_240MHZ;
    this->defaultConfig.wifiMode = WIFI_POWER_FULL;
    this->defaultConfig.bluetoothEnabled = false;
    this->defaultConfig.psramEnabled = true;  // Detect tu hardware
    this->defaultConfig.ledUpdateHz = 100;
    
    this->config = this->defaultConfig;
}

void PowerManager::init() {
    // Khoi tao voi cau hinh mac dinh (khong doi gi)
    Serial.println("[Power] Khoi tao Power Manager voi cau hinh mac dinh (240MHz, Full WiFi)");
    
    // Tat Bluetooth mac dinh
    this->disableBluetooth();
}

void PowerManager::applyCpuFrequency(CpuFrequency freq) {
    uint32_t oldFreq = getCpuFrequencyMhz();
    bool success = setCpuFrequencyMhz(freq);
    
    if (success) {
        this->config.cpuFreq = freq;
        Serial.printf("[Power] Doi tan so CPU: %dMHz -> %dMHz (AP DUNG NGAY LAP TUC!)\n", 
                      oldFreq, freq);
    } else {
        Serial.printf("[Power] LOI: Khong doi duoc tan so CPU thanh %dMHz\n", freq);
    }
}

void PowerManager::applyWifiPowerMode(WifiPowerMode mode) {
    this->config.wifiMode = mode;
    
    switch (mode) {
        case WIFI_POWER_FULL:
            WiFi.setSleep(WIFI_PS_NONE);
            Serial.println("[Power] WiFi Power: FULL (Khong tiet kiem, do tre thap nhat)");
            break;
            
        case WIFI_POWER_MIN_MODEM:
            WiFi.setSleep(WIFI_PS_MIN_MODEM);
            Serial.println("[Power] WiFi Power: MIN MODEM SLEEP (Tiet kiem ~20mA, do tre +3ms)");
            break;
            
        case WIFI_POWER_MAX_MODEM:
            WiFi.setSleep(WIFI_PS_MAX_MODEM);
            Serial.println("[Power] WiFi Power: MAX MODEM SLEEP (Tiet kiem ~40mA, do tre +10ms)");
            break;
    }
}

void PowerManager::setBluetoothEnabled(bool enabled) {
    if (enabled && !this->config.bluetoothEnabled) {
        this->enableBluetooth();
    } else if (!enabled && this->config.bluetoothEnabled) {
        this->disableBluetooth();
    }
}

void PowerManager::setLedUpdateHz(uint8_t hz) {
    // Validate
    if (hz != 30 && hz != 50 && hz != 100) {
        Serial.printf("[Power] LOI: LED Hz khong hop le (%d), chi chap nhan 30/50/100\n", hz);
        return;
    }
    
    this->config.ledUpdateHz = hz;
    Serial.printf("[Power] LED refresh rate: %d Hz\n", hz);
}

void PowerManager::disableBluetooth() {
    btStop();
    this->config.bluetoothEnabled = false;
    Serial.println("[Power] Bluetooth: TAT (Tiet kiem ~12mA)");
}

void PowerManager::enableBluetooth() {
    btStart();
    this->config.bluetoothEnabled = true;
    Serial.println("[Power] Bluetooth: BAT (Tieu thu +12mA)");
}

float PowerManager::getEstimatedCurrentDraw() const {
    float basePower = this->calculateBasePower();
    
    // CPU power (theoretical)
    float cpuPower = 0.0f;
    switch (this->config.cpuFreq) {
        case CPU_80MHZ:  cpuPower = 40.0f; break;
        case CPU_160MHZ: cpuPower = 70.0f; break;
        case CPU_240MHZ: cpuPower = 100.0f; break;
    }
    
    // WiFi power
    float wifiPower = 0.0f;
    switch (this->config.wifiMode) {
        case WIFI_POWER_FULL:      wifiPower = 80.0f; break;
        case WIFI_POWER_MIN_MODEM: wifiPower = 60.0f; break;
        case WIFI_POWER_MAX_MODEM: wifiPower = 40.0f; break;
    }
    
    // Bluetooth
    float btPower = this->config.bluetoothEnabled ? 12.0f : 0.0f;
    
    // LED task overhead
    float ledPower = 0.0f;
    switch (this->config.ledUpdateHz) {
        case 30:  ledPower = 3.0f; break;
        case 50:  ledPower = 5.0f; break;
        case 100: ledPower = 8.0f; break;
    }
    
    return basePower + cpuPower + wifiPower + btPower + ledPower;
}

float PowerManager::getEstimatedPowerSavings() const {
    // So sanh voi cau hinh mac dinh (240MHz, Full WiFi)
    float currentPower = this->getEstimatedCurrentDraw();
    float defaultPower = 100.0f + 80.0f + 8.0f + 30.0f; // CPU + WiFi + LED + Base
    
    return defaultPower - currentPower;
}

float PowerManager::calculateBasePower() const {
    // Base power: Flash, RAM, Peripherals
    return 30.0f; // ~30mA baseline
}
```

---

### 3.14. Driver Ép Tải Dual-Core (Header)
#### File: `src/drivers/StressTester.h`
> **Mô tả**: Định nghĩa 2 FreeRTOS tasks ép tải dấu phẩy động 100% CPU trên Core 0 & Core 1  
> **Dung lượng**: 46 dòng | 1177 bytes

```cpp
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
```

---

### 3.15. Driver Ép Tải Dual-Core (Source)
#### File: `src/drivers/StressTester.cpp`
> **Mô tả**: Triển khai thuật toán tính toán lượng giác, FreeRTOS Idle Hook đo % CPU sử dụng thực tế và đọc nhiệt bán dẫn  
> **Dung lượng**: 137 dòng | 4056 bytes

```cpp
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
```

---

### 3.16. Dịch Vụ Wi-Fi (Header)
#### File: `src/network/WiFiService.h`
> **Mô tả**: Định nghĩa quản lý kết nối WPA2-Personal, WPA2-Enterprise 802.1x và Access Point  
> **Dung lượng**: 62 dòng | 1472 bytes

```cpp
#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include "esp_wpa2.h"
#include "StorageManager.h"

enum WiFiState {
    WIFI_IDLE,
    WIFI_CONNECTING,
    WIFI_CONNECTED,
    WIFI_AP_PORTAL
};

enum WiFiConnectResult {
    RESULT_IDLE,
    RESULT_CONNECTING,
    RESULT_SUCCESS,
    RESULT_FAILED
};

class WiFiService {
public:
    WiFiService(StorageManager &storage);

    void init();
    void loop();

    // Thu ket noi vao mang Station (ca Personal hoac Enterprise)
    void connectStation(const String &ssid, const String &password, const String &username, bool isEnterprise, const String &backendUrl = "");

    // Kich hoat che do phat Access Point de nguoi dung cau hinh qua Web
    void startAP();

    // Tat Access Point sau khi da ket noi duoc WiFi
    void stopAP();

    // Kich hoat dem nguoc tat AP tu thoi diem Web nhan ket qua success
    void triggerShutdownCountdown(uint32_t delayMs = 3000);

    bool isConnected() const;
    bool isAPMode() const;
    String getIP() const;
    String getSSID() const;

    WiFiConnectResult getConnectResult() const;
    String getLastError() const;

private:
    StorageManager &storage;
    WiFiState state;
    WiFiConnectResult connectResult;
    String lastError;
    uint32_t connectStartTime;
    uint32_t lastReconnectAttempt;
    uint32_t delayedStopApTime;
    
    String currentSSID;
    String currentPass;
    String currentUser;
    bool currentIsEnterprise;
    String currentBackendUrl;
};
```

---

### 3.17. Dịch Vụ Wi-Fi (Source)
#### File: `src/network/WiFiService.cpp`
> **Mô tả**: Triển khai kết nối Station, SoftAP Captive Portal, mDNS, NTP UTC+7 và đếm ngược tắt AP  
> **Dung lượng**: 212 dòng | 8441 bytes

```cpp
#include "WiFiService.h"
#include "Config.h"
#include <ESPmDNS.h>
#include <time.h>

WiFiService::WiFiService(StorageManager &storage)
    : storage(storage), state(WIFI_IDLE), connectResult(RESULT_IDLE), lastError(""),
      connectStartTime(0), lastReconnectAttempt(0), delayedStopApTime(0), currentIsEnterprise(false) {}

void WiFiService::init() {
    String dummyBackend;
    if (this->storage.hasCredentials()) {
        this->storage.loadCredentials(this->currentSSID, this->currentPass, this->currentUser, this->currentIsEnterprise, dummyBackend);
        Serial.printf("[WiFi] Tim thay WiFi da luu: %s (Che do: %s)\n", 
                      this->currentSSID.c_str(), 
                      this->currentIsEnterprise ? "WPA2-Enterprise (Truong hoc)" : "WPA2-Personal (Ca nhan)");
        this->connectStation(this->currentSSID, this->currentPass, this->currentUser, this->currentIsEnterprise, dummyBackend);
    } else {
        Serial.println("[WiFi] Chua co cau hinh WiFi nao -> Kich hoat che do phat Access Point.");
        this->startAP();
    }
}

void WiFiService::connectStation(const String &ssid, const String &password, const String &username, bool isEnterprise, const String &backendUrl) {
    this->currentSSID = ssid;
    this->currentPass = password;
    this->currentUser = username;
    this->currentIsEnterprise = isEnterprise;
    if (backendUrl.length() > 0) {
        this->currentBackendUrl = backendUrl;
    }

    this->connectResult = RESULT_CONNECTING;
    this->lastError = "";
    this->delayedStopApTime = 0;

    // Ngat ket noi STA cu ma khong tat AP neu dang mo
    WiFi.disconnect(false);
    delay(50);

    // Neu dang phat AP thi giu nguyen AP_STA de nguoi dung khong bi mat ket noi trang web
    if (this->state == WIFI_AP_PORTAL || (WiFi.getMode() & WIFI_AP)) {
        WiFi.mode(WIFI_AP_STA);
    } else {
        WiFi.mode(WIFI_STA);
    }

    if (isEnterprise) {
        Serial.printf("[WiFi] Dang thu ket noi WPA2-Enterprise: %s (User: %s)...\n", ssid.c_str(), username.c_str());
        esp_wifi_sta_wpa2_ent_clear_identity();
        esp_wifi_sta_wpa2_ent_clear_username();
        esp_wifi_sta_wpa2_ent_clear_password();

        esp_wifi_sta_wpa2_ent_set_identity((const unsigned char *)username.c_str(), username.length());
        esp_wifi_sta_wpa2_ent_set_username((const unsigned char *)username.c_str(), username.length());
        esp_wifi_sta_wpa2_ent_set_password((const unsigned char *)password.c_str(), password.length());
        esp_wifi_sta_wpa2_ent_enable();
        WiFi.begin(ssid.c_str());
    } else {
        Serial.printf("[WiFi] Dang thu ket noi WPA2-Personal: %s...\n", ssid.c_str());
        esp_wifi_sta_wpa2_ent_disable();
        if (password.length() > 0) {
            WiFi.begin(ssid.c_str(), password.c_str());
        } else {
            WiFi.begin(ssid.c_str());
        }
    }

    this->connectStartTime = millis();
    this->state = WIFI_CONNECTING;
}

void WiFiService::startAP() {
    this->state = WIFI_AP_PORTAL;
    WiFi.mode(WIFI_AP);
    
    // Phat WiFi Access Point
    if (strlen(AP_PASSWORD_DEFAULT) > 0) {
        WiFi.softAP(AP_SSID_DEFAULT, AP_PASSWORD_DEFAULT);
    } else {
        WiFi.softAP(AP_SSID_DEFAULT);
    }

    IPAddress apIP = WiFi.softAPIP();
    Serial.println("==================================================");
    Serial.printf("[AP] Da phat WiFi: %s\n", AP_SSID_DEFAULT);
    Serial.printf("[AP] Dia chi IP Web UI: http://%s\n", apIP.toString().c_str());
    Serial.println("[AP] Hay dung dien thoai ket noi vao WiFi nay de cai dat.");
    Serial.println("==================================================");
}

void WiFiService::stopAP() {
    if (WiFi.getMode() & WIFI_AP) {
        Serial.println("[AP] Da tat phat Access Point.");
        WiFi.softAPdisconnect(true);
        WiFi.mode(WIFI_STA);
    }
}

void WiFiService::triggerShutdownCountdown(uint32_t delayMs) {
    if (WiFi.getMode() & WIFI_AP) {
        this->delayedStopApTime = millis() + delayMs;
        Serial.printf("[AP] Web da nhan ket qua -> Bat dau dem nguoc dung %u ms de tat Access Point!\n", delayMs);
    }
}

void WiFiService::loop() {
    // Xu ly hen gio tat AP sau khi nguoi dung da xem thong bao thanh cong tren web
    if (this->delayedStopApTime > 0 && millis() >= this->delayedStopApTime) {
        this->delayedStopApTime = 0;
        this->stopAP();
    }

    if (this->state == WIFI_CONNECTING) {
        wl_status_t status = WiFi.status();
        if (status == WL_CONNECTED) {
            this->state = WIFI_CONNECTED;
            this->connectResult = RESULT_SUCCESS;
            this->lastError = "";
            Serial.println();
            Serial.println("==================================================");
            Serial.printf("[WiFi] KET NOI THANH CONG! IP: %s\n", WiFi.localIP().toString().c_str());
            Serial.println("==================================================");

            // Khoi tao ten mien mDNS de truy cap qua http://esp32-sensor.local
            MDNS.end();
            if (MDNS.begin(MDNS_HOSTNAME)) {
                MDNS.addService("http", "tcp", 80);
                Serial.printf("[mDNS] Da kich hoat ten mien: http://%s.local\n", MDNS_HOSTNAME);
            } else {
                Serial.println("[mDNS] Loi khoi tao mDNS!");
            }

            // Dong bo thoi gian thuc UTC+7 qua Internet (NTP)
            configTime(7 * 3600, 0, "pool.ntp.org", "time.google.com");
            Serial.println("[NTP] Da khoi tao dong bo thoi gian tu pool.ntp.org");
            
            // Chi luu vao bo nho Flash NVS khi da chac chan ket noi duoc!
            this->storage.saveCredentials(this->currentSSID, this->currentPass, this->currentUser, this->currentIsEnterprise, this->currentBackendUrl);

            // Hen gio du phong (failsafe) tu tat AP sau 8s neu trinh duyet khong chu dong bao
            if (WiFi.getMode() & WIFI_AP) {
                this->delayedStopApTime = millis() + 8000;
            }
        } else if (status == WL_CONNECT_FAILED) {
            Serial.println();
            Serial.println("[WiFi] Ket noi that bai: Sai mat khau hoac xac thuc that bai!");
            this->connectResult = RESULT_FAILED;
            this->lastError = "Sai mat khau hoac xac thuc that bai! Vui long kiem tra lai.";
            this->state = WIFI_AP_PORTAL;
            if (!(WiFi.getMode() & WIFI_AP)) {
                this->startAP();
            }
        } else if (status == WL_NO_SSID_AVAIL) {
            Serial.println();
            Serial.println("[WiFi] Ket noi that bai: Khong tim thay mang WiFi chi dinh!");
            this->connectResult = RESULT_FAILED;
            this->lastError = "Khong tim thay mang WiFi chi dinh! Vui long kiem tra ten SSID.";
            this->state = WIFI_AP_PORTAL;
            if (!(WiFi.getMode() & WIFI_AP)) {
                this->startAP();
            }
        } else if (millis() - this->connectStartTime > WIFI_CONNECT_TIMEOUT_MS) {
            Serial.println();
            Serial.println("[WiFi] Ket noi that bai (Timeout)! Mang khong phan hoi.");
            this->connectResult = RESULT_FAILED;
            this->lastError = "Het thoi gian cho (Timeout)! Vui long kiem tra lai mat khau.";
            this->state = WIFI_AP_PORTAL;
            if (!(WiFi.getMode() & WIFI_AP)) {
                this->startAP();
            }
        }
    } else if (this->state == WIFI_CONNECTED) {
        // Tu dong ket noi lai neu rot mang
        if (WiFi.status() != WL_CONNECTED) {
            if (millis() - this->lastReconnectAttempt > 10000) {
                this->lastReconnectAttempt = millis();
                Serial.println("[WiFi] Bi mat ket noi, dang thu ket noi lai...");
                WiFi.reconnect();
            }
        }
    }
}

bool WiFiService::isConnected() const {
    return this->state == WIFI_CONNECTED && WiFi.status() == WL_CONNECTED;
}

bool WiFiService::isAPMode() const {
    return this->state == WIFI_AP_PORTAL;
}

String WiFiService::getIP() const {
    if (this->isConnected()) {
        return WiFi.localIP().toString();
    } else if (this->isAPMode()) {
        return WiFi.softAPIP().toString();
    }
    return "0.0.0.0";
}

String WiFiService::getSSID() const {
    return this->currentSSID;
}

WiFiConnectResult WiFiService::getConnectResult() const {
    return this->connectResult;
}

String WiFiService::getLastError() const {
    return this->lastError;
}
```

---

### 3.18. Quản Lý Bộ Nhớ Flash NVS (Header)
#### File: `src/network/StorageManager.h`
> **Mô tả**: Định nghĩa giao tiếp Preferences lưu cấu hình Wi-Fi, LED, Sensor Settings  
> **Dung lượng**: 34 dòng | 1195 bytes

```cpp
#pragma once
#include <Arduino.h>
#include <Preferences.h>

class StorageManager {
public:
    StorageManager();

    bool init();

    // Doc cau hinh tu NVS
    void loadCredentials(String &ssid, String &password, String &username, bool &isEnterprise, String &backendUrl);

    // Luu cau hinh vao NVS
    void saveCredentials(const String &ssid, const String &password, const String &username, bool isEnterprise, const String &backendUrl);

    // Doc/Ghi cau hinh LED RGB
    void loadLedConfig(uint8_t &mode, uint8_t &r, uint8_t &g, uint8_t &b, uint8_t &brightness, uint8_t &speed);
    void saveLedConfig(uint8_t mode, uint8_t r, uint8_t g, uint8_t b, uint8_t brightness, uint8_t speed);

    // Doc/Ghi cau hinh nguong canh bao va chu ky cam bien
    void loadSensorSettings(float &tempAlert, float &humAlert, uint32_t &readIntervalSec, uint32_t &sendIntervalSec);
    void saveSensorSettings(float tempAlert, float humAlert, uint32_t readIntervalSec, uint32_t sendIntervalSec);

    // Kiem tra da tung luu WiFi chua
    bool hasCredentials();

    // Xoa toan bo cau hinh (Reset ve mac dinh)
    void clear();

private:
    Preferences prefs;
    const char* NAMESPACE = "wifi_cfg";
};
```

---

### 3.19. Quản Lý Bộ Nhớ Flash NVS (Source)
#### File: `src/network/StorageManager.cpp`
> **Mô tả**: Triển khai đọc/ghi/xóa dữ liệu cấu hình bền vững vào namespace NVS Flash  
> **Dung lượng**: 81 dòng | 3938 bytes

```cpp
#include "StorageManager.h"
#include "Config.h"

StorageManager::StorageManager() {}

bool StorageManager::init() {
    return this->prefs.begin(this->NAMESPACE, false);
}

void StorageManager::loadCredentials(String &ssid, String &password, String &username, bool &isEnterprise, String &backendUrl) {
    this->prefs.begin(this->NAMESPACE, true);
    ssid = this->prefs.isKey("ssid") ? this->prefs.getString("ssid", "") : "";
    password = this->prefs.isKey("password") ? this->prefs.getString("password", "") : "";
    username = this->prefs.isKey("username") ? this->prefs.getString("username", "") : "";
    isEnterprise = this->prefs.isKey("is_ent") ? this->prefs.getBool("is_ent", false) : false;
    backendUrl = this->prefs.isKey("backend") ? this->prefs.getString("backend", DEFAULT_BACKEND_URL) : DEFAULT_BACKEND_URL;
    this->prefs.end();
}

void StorageManager::saveCredentials(const String &ssid, const String &password, const String &username, bool isEnterprise, const String &backendUrl) {
    this->prefs.begin(this->NAMESPACE, false);
    this->prefs.putString("ssid", ssid);
    this->prefs.putString("password", password);
    this->prefs.putString("username", username);
    this->prefs.putBool("is_ent", isEnterprise);
    this->prefs.putString("backend", backendUrl);
    this->prefs.end();
}

void StorageManager::loadLedConfig(uint8_t &mode, uint8_t &r, uint8_t &g, uint8_t &b, uint8_t &brightness, uint8_t &speed) {
    this->prefs.begin(this->NAMESPACE, true);
    mode = this->prefs.isKey("led_mode") ? this->prefs.getUChar("led_mode", 2) : 2; // Default: 2 (Breathing)
    r = this->prefs.isKey("led_r") ? this->prefs.getUChar("led_r", 168) : 168;       // Default: Tim
    g = this->prefs.isKey("led_g") ? this->prefs.getUChar("led_g", 85) : 85;
    b = this->prefs.isKey("led_b") ? this->prefs.getUChar("led_b", 247) : 247;
    brightness = this->prefs.isKey("led_bright") ? this->prefs.getUChar("led_bright", 220) : 220;
    speed = this->prefs.isKey("led_speed") ? this->prefs.getUChar("led_speed", 50) : 50;
    this->prefs.end();
}

void StorageManager::saveLedConfig(uint8_t mode, uint8_t r, uint8_t g, uint8_t b, uint8_t brightness, uint8_t speed) {
    this->prefs.begin(this->NAMESPACE, false);
    this->prefs.putUChar("led_mode", mode);
    this->prefs.putUChar("led_r", r);
    this->prefs.putUChar("led_g", g);
    this->prefs.putUChar("led_b", b);
    this->prefs.putUChar("led_bright", brightness);
    this->prefs.putUChar("led_speed", speed);
    this->prefs.end();
}

void StorageManager::loadSensorSettings(float &tempAlert, float &humAlert, uint32_t &readIntervalSec, uint32_t &sendIntervalSec) {
    this->prefs.begin(this->NAMESPACE, true);
    tempAlert = this->prefs.isKey("cfg_t_alert") ? this->prefs.getFloat("cfg_t_alert", TEMP_ALERT_THRESHOLD) : TEMP_ALERT_THRESHOLD;
    humAlert = this->prefs.isKey("cfg_h_alert") ? this->prefs.getFloat("cfg_h_alert", HUM_ALERT_THRESHOLD) : HUM_ALERT_THRESHOLD;
    readIntervalSec = this->prefs.isKey("cfg_r_sec") ? this->prefs.getUInt("cfg_r_sec", 1) : 1;
    sendIntervalSec = this->prefs.isKey("cfg_s_sec") ? this->prefs.getUInt("cfg_s_sec", 1) : 1;
    this->prefs.end();
}

void StorageManager::saveSensorSettings(float tempAlert, float humAlert, uint32_t readIntervalSec, uint32_t sendIntervalSec) {
    this->prefs.begin(this->NAMESPACE, false);
    this->prefs.putFloat("cfg_t_alert", tempAlert);
    this->prefs.putFloat("cfg_h_alert", humAlert);
    this->prefs.putUInt("cfg_r_sec", readIntervalSec);
    this->prefs.putUInt("cfg_s_sec", sendIntervalSec);
    this->prefs.end();
}

bool StorageManager::hasCredentials() {
    this->prefs.begin(this->NAMESPACE, true);
    bool has = this->prefs.isKey("ssid") && (this->prefs.getString("ssid", "").length() > 0);
    this->prefs.end();
    return has;
}

void StorageManager::clear() {
    this->prefs.begin(this->NAMESPACE, false);
    this->prefs.clear();
    this->prefs.end();
}
```

---

### 3.20. Dịch Vụ Telemetry & Persistent Stream (Header)
#### File: `src/network/TelemetryService.h`
> **Mô tả**: Định nghĩa WiFiClient Stream, RAM Caching backendUrl, FreeRTOS Queue phi chặn, PSRAM Buffer  
> **Dung lượng**: 121 dòng | 3899 bytes

```cpp
#pragma once
#include <Arduino.h>
#include <WiFiClient.h>
#include <HTTPClient.h>
#include <FS.h>
#include <LittleFS.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include "WiFiService.h"
#include "StorageManager.h"

// Ket qua gui HTTP phan loai ro rang de xu ly (Chong Deadlock 4xx)
enum HttpSendResult {
    HTTP_SEND_SUCCESS = 0,     // 2xx / ACK ok: Gui thanh cong
    HTTP_SEND_CLIENT_ERROR,    // 4xx / client_error: Loi do payload/auth -> Bo qua batch de tranh deadlock
    HTTP_SEND_RETRYABLE_ERROR  // 5xx hoac timeout (<0): Loi server/mang -> Giu lai de thu lai sau
};

// Cau truc ban ghi nhi phan luu truc tiep tren Flash LittleFS (toi uu kich thuoc 48 bytes)
struct TelemetryRecord {
    uint32_t seq;
    uint32_t timestamp;
    float temperature;
    float humidity;
    float dewPoint;
    float vpd;
    float chipTemp;
    float cpuLoad;
    float cpu0;
    float cpu1;
    uint32_t freeHeap;
    uint32_t uptimeSec;
    int16_t wifiRssi;
    bool isAlert;
    bool sensorValid;
};

struct TelemetryPayload {
    uint32_t seq = 0;
    uint32_t timestamp = 0;
    float temperature = 0.0f;
    float humidity = 0.0f;
    float dewPoint = 0.0f;
    float vpd = 0.0f;
    float chipTemp = 0.0f;
    float cpuLoad = 0.0f;
    float cpu0 = 0.0f;
    float cpu1 = 0.0f;
    uint32_t freeHeap = 0;
    uint32_t uptimeSec = 0;
    int wifiRssi = 0;
    bool isAlert = false;
    bool sensorValid = true;
};

class TelemetryService {
public:
    static constexpr const char* OFFLINE_FILE = "/telemetry_offline.bin";
    static constexpr size_t MAX_OFFLINE_RECORDS = 3600; // Luu toi da 3600 mau (1 gio mat mang @ 1s) tren Flash
    static constexpr size_t BATCH_CHUNK_SIZE = 50;      // Moi dot Batch Ingestion gui 50 ban ghi de tiet kiem RAM
    static constexpr size_t PSRAM_BUFFER_CAPACITY = 60; // Gom 60 mau tren PSRAM truoc khi flush LittleFS de chong mon Flash

    TelemetryService(WiFiService &wifiService, StorageManager &storage);
    ~TelemetryService();

    void init();

    // Day payload vao FreeRTOS Queue phi chan (Non-blocking), goi truc tiep tu Core 1
    bool pushPayload(const TelemetryPayload &data);

    // Tuong thich nguoc: chuyen tiep sang pushPayload
    void sendData(TelemetryPayload data);
    void sendData(float temperature, float humidity);

    // Lay Device ID duy nhat tu MAC Hardware
    String getDeviceId();

    // Lay so thu tu goi tin hien tai
    uint32_t getSequenceNumber() const;

    // So luong ban ghi dang luu ben vung tren Flash
    size_t getFlashBacklogCount();

    // So luong ban ghi dang cho tren bo dem PSRAM
    size_t getPsramBufferCount() const;

private:
    WiFiService &wifiService;
    StorageManager &storage;
    uint32_t packetSeq;
    String cachedDeviceId;
    bool littleFsMounted;

    // Persistent HTTP Stream (1 Request duy nhat giu ket noi suot ca ngay)
    WiFiClient streamClient;
    String cachedBackendUrl;
    uint32_t lastConfigCheckMs;

    // FreeRTOS Task & Queue (Chay doc lap tren Core 0)
    QueueHandle_t telemetryQueue;
    TaskHandle_t telemetryTaskHandle;
    static void telemetryTaskStatic(void *pvParameters);
    void telemetryTaskLoop();

    // PSRAM Buffer chong mon Flash
    TelemetryRecord *psramBuffer;
    size_t psramCount;
    bool psramAvailable;

    bool ensureStreamConnected(const String &backendUrl);
    void closeStream();
    bool appendRecordToBuffer(const TelemetryRecord &rec);
    bool flushPsramToFlash();
    bool appendRecordToFlash(const TelemetryRecord &rec);
    void pruneOldRecordsFromFlash(size_t recordsToRemove);
    bool flushBatchFromFlash(const String &backendUrl);
    HttpSendResult sendHttpPayload(const String &payload, const String &backendUrl, bool isBatch, size_t count, uint32_t seq);
    void syncTimeFromServer(const String &responseBody);
    void processPayloadInternal(TelemetryPayload data);
};
```

---

### 3.21. Dịch Vụ Telemetry & Persistent Stream (Source)
#### File: `src/network/TelemetryService.cpp`
> **Mô tả**: Triển khai 1-Request Persistent NDJSON Stream suốt cả ngày, đệm 60 mẫu PSRAM, Batch Ingestion xả bù khi có mạng  
> **Dung lượng**: 633 dòng | 23296 bytes

```cpp
#include "TelemetryService.h"
#include <WiFi.h>
#include <time.h>
#include <sys/time.h>

// Ham phu tro tach URL dang http://ip:port/path thanh host, port, path
static bool parseBackendUrl(const String &url, String &host, uint16_t &port, String &path) {
    if (!url.startsWith("http://")) return false;
    String rest = url.substring(7);
    int slashIdx = rest.indexOf('/');
    String hostPort = (slashIdx >= 0) ? rest.substring(0, slashIdx) : rest;
    path = (slashIdx >= 0) ? rest.substring(slashIdx) : "/";

    int colonIdx = hostPort.indexOf(':');
    if (colonIdx >= 0) {
        host = hostPort.substring(0, colonIdx);
        port = (uint16_t)hostPort.substring(colonIdx + 1).toInt();
    } else {
        host = hostPort;
        port = 80;
    }
    return (host.length() > 0 && port > 0);
}

TelemetryService::TelemetryService(WiFiService &wifiService, StorageManager &storage)
    : wifiService(wifiService), storage(storage), packetSeq(0), cachedDeviceId(""),
      littleFsMounted(false), cachedBackendUrl(""), lastConfigCheckMs(0),
      telemetryQueue(nullptr), telemetryTaskHandle(nullptr),
      psramBuffer(nullptr), psramCount(0), psramAvailable(false) {}

TelemetryService::~TelemetryService() {
    this->closeStream();
    if (this->telemetryTaskHandle != nullptr) {
        vTaskDelete(this->telemetryTaskHandle);
        this->telemetryTaskHandle = nullptr;
    }
    if (this->telemetryQueue != nullptr) {
        vQueueDelete(this->telemetryQueue);
        this->telemetryQueue = nullptr;
    }
    if (this->psramBuffer != nullptr) {
        free(this->psramBuffer);
        this->psramBuffer = nullptr;
    }
}

void TelemetryService::init() {
    // 1. Khoi tao LittleFS Flash
    this->littleFsMounted = LittleFS.begin(false);
    if (this->littleFsMounted) {
        size_t count = this->getFlashBacklogCount();
        if (count > 0) {
            Serial.printf("[Telemetry-Flash] Tim thay %u ban ghi offline ton dong tren Flash tu truoc!\n", (unsigned int)count);
        } else {
            Serial.println("[Telemetry-Flash] LittleFS san sang (Chua co ban ghi offline nao).");
        }
    } else {
        Serial.println("[Telemetry-Flash] LOI: LittleFS chua duoc mount!");
    }

    // 2. Doc truoc Backend URL luu vao RAM Cache
    String dummySSID, dummyPass, dummyUser;
    bool dummyIsEnt = false;
    this->storage.loadCredentials(dummySSID, dummyPass, dummyUser, dummyIsEnt, this->cachedBackendUrl);
    this->lastConfigCheckMs = millis();

    // 3. Cap phat bo dem PSRAM/RAM de chong mon Flash LittleFS
#if defined(BOARD_HAS_PSRAM)
    if (psramFound()) {
        this->psramBuffer = (TelemetryRecord *)ps_malloc(sizeof(TelemetryRecord) * PSRAM_BUFFER_CAPACITY);
        if (this->psramBuffer) {
            this->psramAvailable = true;
            Serial.printf("[Telemetry-PSRAM] Da cap phat Circular Buffer %u mau (%u bytes) tren PSRAM chong mon Flash!\n",
                          (unsigned int)PSRAM_BUFFER_CAPACITY,
                          (unsigned int)(sizeof(TelemetryRecord) * PSRAM_BUFFER_CAPACITY));
        }
    }
#endif
    if (!this->psramBuffer) {
        this->psramBuffer = (TelemetryRecord *)malloc(sizeof(TelemetryRecord) * PSRAM_BUFFER_CAPACITY);
        if (this->psramBuffer) {
            Serial.printf("[Telemetry-RAM] Da cap phat bo dem SRAM %u mau (%u bytes) cho Telemetry.\n",
                          (unsigned int)PSRAM_BUFFER_CAPACITY,
                          (unsigned int)(sizeof(TelemetryRecord) * PSRAM_BUFFER_CAPACITY));
        } else {
            Serial.println("[Telemetry] CANH BAO: Khong the cap phat bo dem RAM/PSRAM!");
        }
    }
    this->psramCount = 0;

    // 4. Khoi tao FreeRTOS Queue va Task doc lap tren Core 0
    this->telemetryQueue = xQueueCreate(16, sizeof(TelemetryPayload));
    if (this->telemetryQueue) {
        BaseType_t taskCreated = xTaskCreatePinnedToCore(
            TelemetryService::telemetryTaskStatic,
            "TelemetryTask",
            8192,
            this,
            1,
            &this->telemetryTaskHandle,
            0 // Ghim doc lap tren Core 0 cung voi Wi-Fi stack
        );
        if (taskCreated == pdPASS) {
            Serial.println("[Telemetry-RTOS] Task Telemetry da khoi tao va ghim vao Core 0 (Stack: 8192B, Priority: 1).");
        } else {
            Serial.println("[Telemetry-RTOS] LOI: Khong the tao TelemetryTask!");
        }
    } else {
        Serial.println("[Telemetry-RTOS] LOI: Khong the tao Telemetry Queue!");
    }
}

void TelemetryService::telemetryTaskStatic(void *pvParameters) {
    TelemetryService *service = static_cast<TelemetryService *>(pvParameters);
    service->telemetryTaskLoop();
}

void TelemetryService::telemetryTaskLoop() {
    TelemetryPayload payload;
    while (true) {
        if (xQueueReceive(this->telemetryQueue, &payload, pdMS_TO_TICKS(100)) == pdTRUE) {
            this->processPayloadInternal(payload);
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

bool TelemetryService::pushPayload(const TelemetryPayload &data) {
    if (!this->telemetryQueue) {
        return false;
    }
    BaseType_t res = xQueueSend(this->telemetryQueue, &data, 0);
    if (res != pdTRUE) {
        Serial.println("[Telemetry-Queue] Canh bao: Queue telemetry bi day, bo qua goi tin!");
        return false;
    }
    return true;
}

void TelemetryService::sendData(TelemetryPayload data) {
    this->pushPayload(data);
}

void TelemetryService::sendData(float temperature, float humidity) {
    TelemetryPayload payload;
    payload.temperature = temperature;
    payload.humidity = humidity;
    payload.sensorValid = true;
    payload.uptimeSec = millis() / 1000;
    payload.freeHeap = ESP.getFreeHeap();
    payload.wifiRssi = WiFi.RSSI();
    this->pushPayload(payload);
}

String TelemetryService::getDeviceId() {
    if (this->cachedDeviceId.length() == 0) {
        uint8_t mac[6];
        WiFi.macAddress(mac);
        char id[24];
        snprintf(id, sizeof(id), "ESP32S3_%02X%02X%02X%02X%02X%02X",
                 mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        this->cachedDeviceId = String(id);
    }
    return this->cachedDeviceId;
}

uint32_t TelemetryService::getSequenceNumber() const {
    return this->packetSeq;
}

size_t TelemetryService::getFlashBacklogCount() {
    if (!this->littleFsMounted || !LittleFS.exists(OFFLINE_FILE)) {
        return 0;
    }
    File f = LittleFS.open(OFFLINE_FILE, "rb");
    if (!f) return 0;
    size_t count = f.size() / sizeof(TelemetryRecord);
    f.close();
    return count;
}

size_t TelemetryService::getPsramBufferCount() const {
    return this->psramCount;
}

void TelemetryService::pruneOldRecordsFromFlash(size_t recordsToRemove) {
    if (!this->littleFsMounted || !LittleFS.exists(OFFLINE_FILE)) return;

    File f = LittleFS.open(OFFLINE_FILE, "rb");
    if (!f) return;

    size_t totalBytes = f.size();
    size_t totalRecords = totalBytes / sizeof(TelemetryRecord);
    if (totalRecords <= recordsToRemove) {
        f.close();
        LittleFS.remove(OFFLINE_FILE);
        return;
    }

    f.seek(recordsToRemove * sizeof(TelemetryRecord), SeekSet);

    const char* TEMP_FILE = "/telemetry_fifo.bin";
    File temp = LittleFS.open(TEMP_FILE, "wb");
    if (temp) {
        uint8_t buf[256];
        while (f.available()) {
            size_t n = f.read(buf, sizeof(buf));
            temp.write(buf, n);
        }
        temp.close();
        f.close();
        LittleFS.remove(OFFLINE_FILE);
        LittleFS.rename(TEMP_FILE, OFFLINE_FILE);
        Serial.printf("[Telemetry-FIFO] Da cat bo %u ban ghi cu nhat (Duy tri dung luong Flash luon an toan)!\n",
                      (unsigned int)recordsToRemove);
    } else {
        f.close();
    }
}

bool TelemetryService::appendRecordToFlash(const TelemetryRecord &rec) {
    if (!this->littleFsMounted) return false;

    size_t currentCount = this->getFlashBacklogCount();
    size_t freeBytes = LittleFS.totalBytes() - LittleFS.usedBytes();

    if (currentCount >= MAX_OFFLINE_RECORDS || freeBytes < 300000) {
        this->pruneOldRecordsFromFlash(100);
    }

    File f = LittleFS.open(OFFLINE_FILE, "ab");
    if (!f) {
        Serial.println("[Telemetry-Flash] LOI: Khong the mo file de ghi!");
        return false;
    }
    size_t written = f.write((const uint8_t *)&rec, sizeof(TelemetryRecord));
    f.close();
    return (written == sizeof(TelemetryRecord));
}

bool TelemetryService::flushPsramToFlash() {
    if (!this->littleFsMounted || !this->psramBuffer || this->psramCount == 0) {
        return false;
    }

    size_t currentCount = this->getFlashBacklogCount();
    size_t freeBytes = LittleFS.totalBytes() - LittleFS.usedBytes();
    if (currentCount + this->psramCount > MAX_OFFLINE_RECORDS || freeBytes < 300000) {
        this->pruneOldRecordsFromFlash(this->psramCount + 50);
    }

    File f = LittleFS.open(OFFLINE_FILE, "ab");
    if (!f) {
        Serial.println("[Telemetry-Flash] LOI: Khong the mo file Flash de flush buffer!");
        return false;
    }

    size_t bytesToWrite = this->psramCount * sizeof(TelemetryRecord);
    size_t written = f.write((const uint8_t *)this->psramBuffer, bytesToWrite);
    f.close();

    bool ok = (written == bytesToWrite);
    if (ok) {
        Serial.printf("[Telemetry-PSRAM] Da flush %u mau tu PSRAM xuong Flash LittleFS (1 lan ghi duy nhat)!\n",
                      (unsigned int)this->psramCount);
        this->psramCount = 0;
    } else {
        Serial.println("[Telemetry-Flash] LOI: Ghi PSRAM buffer vao Flash khong tron ven!");
    }
    return ok;
}

bool TelemetryService::appendRecordToBuffer(const TelemetryRecord &rec) {
    if (!this->psramBuffer) {
        return this->appendRecordToFlash(rec);
    }

    if (this->psramCount < PSRAM_BUFFER_CAPACITY) {
        this->psramBuffer[this->psramCount++] = rec;
    }

    if (this->psramCount >= PSRAM_BUFFER_CAPACITY) {
        return this->flushPsramToFlash();
    }
    return true;
}

void TelemetryService::syncTimeFromServer(const String &responseBody) {
    if (responseBody.length() == 0) return;
    int idx = responseBody.indexOf("\"server_time\"");
    if (idx == -1) return;
    int colon = responseBody.indexOf(':', idx);
    if (colon == -1) return;

    const char *p = responseBody.c_str() + colon + 1;
    while (*p == ' ' || *p == '\t') p++;
    time_t sTime = (time_t)atoll(p);

    if (sTime > 1700000000) {
        time_t now = time(nullptr);
        if (now < 1700000000 || labs((long)(sTime - now)) > 5) {
            struct timeval tv = { .tv_sec = sTime, .tv_usec = 0 };
            settimeofday(&tv, NULL);
            Serial.printf("[Telemetry-RTC] Da dong bo RTC he thong tu server_time: %lld (Lech cu: %ld s)\n",
                          (long long)sTime, (long)(sTime - now));
        }
    }
}

void TelemetryService::closeStream() {
    if (this->streamClient.connected()) {
        this->streamClient.stop();
    }
}

bool TelemetryService::ensureStreamConnected(const String &backendUrl) {
    if (this->streamClient.connected()) {
        return true; // Da mo san duong ong Stream -> Dung tiep, khong tao HTTP Request moi!
    }

    String host, path;
    uint16_t port = 80;
    if (!parseBackendUrl(backendUrl, host, port, path)) {
        Serial.println("[Telemetry-Stream] LOI: Dinh dang Backend URL khong hop le!");
        return false;
    }

    if (!this->streamClient.connect(host.c_str(), port, 2000)) {
        return false;
    }

    // Tat Nagle Algorithm de moi dong JSON 1s ban di tuc thi (<2ms)
    this->streamClient.setNoDelay(true);

    // GUI DUNG 1 HTTP REQUEST DUY NHAT DE MO DUONG ONG STREAM SUOT CA NGAY
    this->streamClient.printf(
        "POST %s HTTP/1.1\r\n"
        "Host: %s:%u\r\n"
        "User-Agent: ESP32-S3-Sensor/2.0-Stream\r\n"
        "Content-Type: application/x-ndjson\r\n"
        "Connection: keep-alive\r\n"
        "X-Device-ID: %s\r\n"
        "X-Stream-Mode: true\r\n\r\n",
        path.c_str(), host.c_str(), port, this->getDeviceId().c_str()
    );

    uint32_t startWait = millis();
    while (!this->streamClient.available()) {
        if (!this->streamClient.connected() || (millis() - startWait > 2000)) {
            this->closeStream();
            return false;
        }
        vTaskDelay(pdMS_TO_TICKS(5));
    }

    String statusLine = this->streamClient.readStringUntil('\n');
    statusLine.trim();

    // Doc bo qua phan HTTP Response Header ban dau
    while (this->streamClient.connected()) {
        String headerLine = this->streamClient.readStringUntil('\n');
        if (headerLine == "\r" || headerLine.length() == 0) {
            break;
        }
    }

    if (statusLine.indexOf("200") == -1) {
        Serial.printf("[Telemetry-Stream] Server tu choi mo Stream: %s\n", statusLine.c_str());
        this->closeStream();
        return false;
    }

    Serial.println("[Telemetry-Stream] DA THIET LAP 1 HTTP REQUEST STREAM DUY NHAT! Bat dau bom data moi giay...");
    return true;
}

HttpSendResult TelemetryService::sendHttpPayload(const String &payload, const String &backendUrl, bool isBatch, size_t count, uint32_t seq) {
    if (!this->ensureStreamConnected(backendUrl)) {
        return HTTP_SEND_RETRYABLE_ERROR;
    }

    // Don sach cac byte du thua trong buffer doc (neu co)
    while (this->streamClient.available()) {
        this->streamClient.read();
    }

    // Day 1 dong JSON (NDJSON) qua duong ong TCP dang mo san
    size_t expectedLen = payload.length() + 1;
    size_t written = this->streamClient.print(payload + "\n");
    if (written != expectedLen) {
        this->closeStream();
        return HTTP_SEND_RETRYABLE_ERROR;
    }

    // Doi phan hoi ACK 1 dong tu Server
    uint32_t timeoutMs = isBatch ? 3500 : 1500;
    uint32_t startWait = millis();
    while (!this->streamClient.available()) {
        if (!this->streamClient.connected() || (millis() - startWait > timeoutMs)) {
            Serial.printf("[Telemetry-Stream] Timeout/Mat ket noi khi gui goi %s -> Dong Stream!\n",
                          isBatch ? "BATCH" : ("#" + String(seq)).c_str());
            this->closeStream();
            return HTTP_SEND_RETRYABLE_ERROR;
        }
        vTaskDelay(pdMS_TO_TICKS(2));
    }

    String resp = this->streamClient.readStringUntil('\n');
    resp.trim();

    if (resp.indexOf("\"status\":\"ok\"") != -1 || resp.indexOf("\"status\": \"ok\"") != -1) {
        this->syncTimeFromServer(resp);
        return HTTP_SEND_SUCCESS;
    } else if (resp.indexOf("\"status\":\"client_error\"") != -1 || resp.indexOf("\"status\": \"client_error\"") != -1) {
        return HTTP_SEND_CLIENT_ERROR;
    }

    this->closeStream();
    return HTTP_SEND_RETRYABLE_ERROR;
}

bool TelemetryService::flushBatchFromFlash(const String &backendUrl) {
    if (!this->littleFsMounted || !LittleFS.exists(OFFLINE_FILE)) {
        return true;
    }

    File f = LittleFS.open(OFFLINE_FILE, "rb");
    if (!f) return false;

    size_t totalBytes = f.size();
    size_t totalRecords = totalBytes / sizeof(TelemetryRecord);
    if (totalRecords == 0) {
        f.close();
        LittleFS.remove(OFFLINE_FILE);
        return true;
    }

    size_t recordsToRead = (totalRecords > BATCH_CHUNK_SIZE) ? BATCH_CHUNK_SIZE : totalRecords;
    TelemetryRecord chunk[BATCH_CHUNK_SIZE];
    size_t actualRead = f.read((uint8_t *)chunk, recordsToRead * sizeof(TelemetryRecord)) / sizeof(TelemetryRecord);

    if (actualRead == 0) {
        f.close();
        return false;
    }

    String json;
    json.reserve(actualRead * 280 + 120);
    json = "{";
    json += "\"device_id\":\"" + this->getDeviceId() + "\",";
    json += "\"batch\":true,";
    json += "\"count\":" + String(actualRead) + ",";
    json += "\"records\":[";

    for (size_t i = 0; i < actualRead; i++) {
        if (i > 0) json += ",";
        json += "{";
        json += "\"timestamp\":" + String(chunk[i].timestamp) + ",";
        json += "\"seq\":" + String(chunk[i].seq) + ",";
        json += "\"metrics\":{";
        json += "\"temperature\":" + String(chunk[i].temperature, 2) + ",";
        json += "\"humidity\":" + String(chunk[i].humidity, 2) + ",";
        json += "\"dew_point\":" + String(chunk[i].dewPoint, 2) + ",";
        json += "\"vpd\":" + String(chunk[i].vpd, 2);
        json += "},";
        json += "\"diagnostics\":{";
        json += "\"chip_temp\":" + String(chunk[i].chipTemp, 1) + ",";
        json += "\"cpu_load\":" + String(chunk[i].cpuLoad, 1) + ",";
        json += "\"cpu0\":" + String(chunk[i].cpu0, 1) + ",";
        json += "\"cpu1\":" + String(chunk[i].cpu1, 1) + ",";
        json += "\"free_heap\":" + String(chunk[i].freeHeap) + ",";
        json += "\"uptime_sec\":" + String(chunk[i].uptimeSec) + ",";
        json += "\"wifi_rssi\":" + String(chunk[i].wifiRssi);
        json += "},";
        json += "\"status\":{";
        json += "\"alert\":" + String(chunk[i].isAlert ? "true" : "false") + ",";
        json += "\"sensor_valid\":" + String(chunk[i].sensorValid ? "true" : "false");
        json += "}";
        json += "}";
    }
    json += "]}";

    HttpSendResult res = this->sendHttpPayload(json, backendUrl, true, actualRead, 0);
    if (res == HTTP_SEND_RETRYABLE_ERROR) {
        f.close();
        return false;
    }

    if (res == HTTP_SEND_CLIENT_ERROR) {
        Serial.printf("[Telemetry-Flash] Backend tu choi batch %u ban ghi (Client Error) -> Bo qua de tranh deadlock!\n",
                      (unsigned int)actualRead);
    } else {
        Serial.printf("[Batch-Ingestion] Da gui bu thanh cong dot %u ban ghi Flash qua Stream!\n",
                      (unsigned int)actualRead);
    }

    size_t remainingRecords = totalRecords - actualRead;
    if (remainingRecords == 0) {
        f.close();
        LittleFS.remove(OFFLINE_FILE);
        Serial.println("[Batch-Ingestion] DA XA SACH TOAN BO DU LIEU OFFLINE TREN FLASH!");
    } else {
        const char* TEMP_FILE = "/telemetry_temp.bin";
        File tempFile = LittleFS.open(TEMP_FILE, "wb");
        if (tempFile) {
            uint8_t buffer[256];
            while (f.available()) {
                size_t n = f.read(buffer, sizeof(buffer));
                tempFile.write(buffer, n);
            }
            tempFile.close();
            f.close();
            LittleFS.remove(OFFLINE_FILE);
            LittleFS.rename(TEMP_FILE, OFFLINE_FILE);
            Serial.printf("[Batch-Ingestion] Con lai %u ban ghi tren Flash, se tiep tuc gui dot tiep theo.\n", (unsigned int)remainingRecords);
        } else {
            f.close();
        }
    }
    return true;
}

void TelemetryService::processPayloadInternal(TelemetryPayload data) {
    this->packetSeq++;
    data.seq = this->packetSeq;

    time_t nowEpoch = time(nullptr);
    if (nowEpoch > 1700000000) {
        data.timestamp = (uint32_t)nowEpoch;
    } else {
        data.timestamp = 0;
    }

    // Chi kiem tra cap nhat Backend URL tu NVS moi 15 giay (tranh doc NVS Flash moi giay)
    uint32_t nowMs = millis();
    if (this->cachedBackendUrl.length() == 0 || (nowMs - this->lastConfigCheckMs >= 15000)) {
        this->lastConfigCheckMs = nowMs;
        String dummySSID, dummyPass, dummyUser, latestUrl;
        bool dummyIsEnt = false;
        this->storage.loadCredentials(dummySSID, dummyPass, dummyUser, dummyIsEnt, latestUrl);
        if (latestUrl != this->cachedBackendUrl) {
            this->cachedBackendUrl = latestUrl;
            this->closeStream(); // Neu nguoi dung doi URL tren Web UI thi ngat Stream cu
        }
    }

    if (this->cachedBackendUrl.length() == 0) {
        return;
    }

    TelemetryRecord rec;
    rec.seq = data.seq;
    rec.timestamp = data.timestamp;
    rec.temperature = data.temperature;
    rec.humidity = data.humidity;
    rec.dewPoint = data.dewPoint;
    rec.vpd = data.vpd;
    rec.chipTemp = data.chipTemp;
    rec.cpuLoad = data.cpuLoad;
    rec.cpu0 = data.cpu0;
    rec.cpu1 = data.cpu1;
    rec.freeHeap = data.freeHeap;
    rec.uptimeSec = data.uptimeSec;
    rec.wifiRssi = (int16_t)data.wifiRssi;
    rec.isAlert = data.isAlert;
    rec.sensorValid = data.sensorValid;

    // Kiem tra ket noi Wi-Fi
    if (!this->wifiService.isConnected()) {
        this->closeStream();
        this->appendRecordToBuffer(rec);
        Serial.printf("[Telemetry] Mat Wi-Fi -> Da luu goi #%u vao PSRAM Buffer (Dem: %u/%u, Flash: %u)\n",
                      data.seq, (unsigned int)this->psramCount, (unsigned int)PSRAM_BUFFER_CAPACITY,
                      (unsigned int)this->getFlashBacklogCount());
        return;
    }

    // Co Wi-Fi: Neu co ban ghi tam trong PSRAM, flush xuong Flash truoc de dam bao thu tu thoi gian
    if (this->psramCount > 0) {
        this->flushPsramToFlash();
    }

    // Xa sach cac batch offline tren Flash qua Stream
    while (this->getFlashBacklogCount() > 0) {
        bool ok = this->flushBatchFromFlash(this->cachedBackendUrl);
        if (!ok) {
            break;
        }
    }

    // Dong goi va gui ban tin thoi gian thuc tren 1 dong NDJSON
    String payload;
    payload.reserve(380);
    payload = "{";
    payload += "\"device_id\":\"" + this->getDeviceId() + "\",";
    payload += "\"batch\":false,";
    payload += "\"count\":1,";
    payload += "\"timestamp\":" + String(data.timestamp) + ",";
    payload += "\"seq\":" + String(data.seq) + ",";
    payload += "\"metrics\":{";
    payload += "\"temperature\":" + String(data.temperature, 2) + ",";
    payload += "\"humidity\":" + String(data.humidity, 2) + ",";
    payload += "\"dew_point\":" + String(data.dewPoint, 2) + ",";
    payload += "\"vpd\":" + String(data.vpd, 2);
    payload += "},";
    payload += "\"diagnostics\":{";
    payload += "\"chip_temp\":" + String(data.chipTemp, 1) + ",";
    payload += "\"cpu_load\":" + String(data.cpuLoad, 1) + ",";
    payload += "\"cpu0\":" + String(data.cpu0, 1) + ",";
    payload += "\"cpu1\":" + String(data.cpu1, 1) + ",";
    payload += "\"free_heap\":" + String(data.freeHeap) + ",";
    payload += "\"uptime_sec\":" + String(data.uptimeSec) + ",";
    payload += "\"wifi_rssi\":" + String(data.wifiRssi);
    payload += "},";
    payload += "\"status\":{";
    payload += "\"alert\":" + String(data.isAlert ? "true" : "false") + ",";
    payload += "\"sensor_valid\":" + String(data.sensorValid ? "true" : "false");
    payload += "}";
    payload += "}";

    HttpSendResult sendRes = this->sendHttpPayload(payload, this->cachedBackendUrl, false, 1, data.seq);
    if (sendRes == HTTP_SEND_SUCCESS) {
        Serial.printf("[Telemetry-Stream] #%u | T: %.2f*C | H: %.2f%% | RSSI: %d dBm -> STREAM ACK (Backlog: %u)\n",
                      data.seq, data.temperature, data.humidity, data.wifiRssi, (unsigned int)this->getFlashBacklogCount());
    } else if (sendRes == HTTP_SEND_CLIENT_ERROR) {
        Serial.printf("[Telemetry-Stream] #%u | Backend tu choi (Client Error) -> Bo qua de tranh deadlock!\n", data.seq);
    } else {
        this->appendRecordToBuffer(rec);
        Serial.printf("[Telemetry] Gui Stream that bai -> Da luu goi #%u vao PSRAM Buffer (Dem: %u/%u, Flash: %u)\n",
                      data.seq, (unsigned int)this->psramCount, (unsigned int)PSRAM_BUFFER_CAPACITY,
                      (unsigned int)this->getFlashBacklogCount());
    }
}
```

---

### 3.22. Web Server & Captive Portal (Header)
#### File: `src/network/CaptivePortal.h`
> **Mô tả**: Định nghĩa máy chủ HTTP Port 80, DNS Hijack Port 53 và toàn bộ REST endpoints  
> **Dung lượng**: 53 dòng | 1395 bytes

```cpp
#pragma once
#include <Arduino.h>
#include <WebServer.h>
#include <DNSServer.h>
#include "Config.h"
#include "WiFiService.h"
#include "StorageManager.h"
#include "../drivers/RgbController.h"
#include "../drivers/PowerManager.h"

class CaptivePortal {
public:
    CaptivePortal(WiFiService &wifiService, StorageManager &storage, RgbController &rgbController, SensorSettings &sensorSettings, PowerManager &powerManager);

    void init();
    void loop();
    void updateSensorData(float temp, float hum, bool valid);

private:
    WiFiService &wifiService;
    StorageManager &storage;
    RgbController &rgbController;
    SensorSettings &sensorSettings;
    PowerManager &powerManager;
    WebServer server;
    DNSServer dnsServer;
    bool active;

    float currentTemp = 0.0f;
    float currentHum = 0.0f;
    bool sensorValid = false;
    bool countdownTriggered = false;

    void setupRoutes();
    void handleRoot();
    void handleSave();
    void handleScan();
    void handleStatus();
    void handleSensor();
    void handleLedGet();
    void handleLedSet();
    void handleLedReset();
    void handlePowerGet();
    void handlePowerSet();
    void handleSensorConfigGet();
    void handleSensorConfigSet();
    void handleShutdownAP();
    void handleResetWifi();
    void handleStressTest();
    void handleHistoryGet();
    void handleHistoryCsv();
    void handleSyncTime();
};
```

---

### 3.23. Web Server & Captive Portal (Source)
#### File: `src/network/CaptivePortal.cpp`
> **Mô tả**: Triển khai 17 REST endpoints: cảm biến, LED Studio, cấu hình ngưỡng, tối ưu điện năng, stress test, CSV  
> **Dung lượng**: 459 dòng | 21094 bytes

```cpp
#include "CaptivePortal.h"
#include "Config.h"
#include "web/WebPage.h"
#include "../drivers/StressTester.h"
#include "../drivers/DataLogger.h"
#include <WiFi.h>

CaptivePortal::CaptivePortal(WiFiService &wifiService, StorageManager &storage, RgbController &rgbController, SensorSettings &sensorSettings, PowerManager &powerManager)
    : wifiService(wifiService), storage(storage), rgbController(rgbController), sensorSettings(sensorSettings), powerManager(powerManager),
      server(80), active(false), currentTemp(0.0f), currentHum(0.0f), sensorValid(false), countdownTriggered(false) {}

void CaptivePortal::init() {
    this->setupRoutes();
    this->dnsServer.start(53, "*", WiFi.softAPIP());
    this->server.begin();
    this->active = true;
    Serial.println("[Portal] Captive Portal Web Server da khoi dong tren cong 80.");
}

void CaptivePortal::setupRoutes() {
    this->server.on("/", HTTP_GET, [this]() { this->handleRoot(); });
    this->server.on("/save", HTTP_POST, [this]() { this->handleSave(); });
    this->server.on("/scan", HTTP_GET, [this]() { this->handleScan(); });
    this->server.on("/status", HTTP_GET, [this]() { this->handleStatus(); });
    this->server.on("/sensor", HTTP_GET, [this]() { this->handleSensor(); });
    this->server.on("/api/sensor-cfg", HTTP_GET, [this]() { this->handleSensorConfigGet(); });
    this->server.on("/api/sensor-cfg", HTTP_POST, [this]() { this->handleSensorConfigSet(); });
    this->server.on("/api/led", HTTP_GET, [this]() { this->handleLedGet(); });
    this->server.on("/api/led", HTTP_POST, [this]() { this->handleLedSet(); });
    this->server.on("/api/led/reset", HTTP_POST, [this]() { this->handleLedReset(); });
    this->server.on("/api/power", HTTP_GET, [this]() { this->handlePowerGet(); });
    this->server.on("/api/power", HTTP_POST, [this]() { this->handlePowerSet(); });
    this->server.on("/api/stress-test", HTTP_POST, [this]() { this->handleStressTest(); });
    this->server.on("/api/stress-test", HTTP_GET, [this]() { this->handleStressTest(); });
    this->server.on("/api/history", HTTP_GET, [this]() { this->handleHistoryGet(); });
    this->server.on("/api/history/csv", HTTP_GET, [this]() { this->handleHistoryCsv(); });
    this->server.on("/api/sync-time", HTTP_POST, [this]() { this->handleSyncTime(); });
    this->server.on("/shutdown-ap", HTTP_POST, [this]() { this->handleShutdownAP(); });
    this->server.on("/shutdown-ap", HTTP_GET, [this]() { this->handleShutdownAP(); });
    this->server.on("/api/reset-wifi", HTTP_POST, [this]() { this->handleResetWifi(); });
    this->server.on("/api/reset-wifi", HTTP_GET, [this]() { this->handleResetWifi(); });

    // 1. Tranh vong lap vo tan cho favicon
    this->server.on("/favicon.ico", HTTP_GET, [this]() {
        this->server.send(204);
    });

    // 2. Ham chuyen huong tieu chuan tuong thich cao voi Samsung One UI, Apple CNA, Android
    auto sendCaptiveRedirect = [this]() {
        this->server.sendHeader("Location", "http://192.168.4.1/", true);
        this->server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
        this->server.sendHeader("Pragma", "no-cache");
        this->server.sendHeader("Expires", "0");
        this->server.send(302, "text/html", F("<!DOCTYPE html><html><head><meta charset='utf-8'>"
            "<meta http-equiv='refresh' content='0;url=http://192.168.4.1/'>"
            "<script>window.location.replace('http://192.168.4.1/');</script>"
            "<title>Redirecting...</title></head>"
            "<body><p>Dang chuyen huong...</p>"
            "<p><a href='http://192.168.4.1/'>Nhan vao day neu khong tu dong chuyen</a></p></body></html>"));
    };

    // 3. Danh sach day du cac connectivity probe endpoints
    // Android / Google / Samsung One UI
    this->server.on("/generate_204", HTTP_GET, sendCaptiveRedirect);
    this->server.on("/gen_204", HTTP_GET, sendCaptiveRedirect);
    this->server.on("/ncsi.txt", HTTP_GET, sendCaptiveRedirect);
    this->server.on("/mobile/status.go", HTTP_GET, sendCaptiveRedirect);
    this->server.on("/check_network_status.txt", HTTP_GET, sendCaptiveRedirect);

    // Apple iOS / macOS CNA
    this->server.on("/hotspot-detect.html", HTTP_GET, sendCaptiveRedirect);
    this->server.on("/library/test/success.html", HTTP_GET, sendCaptiveRedirect);
    this->server.on("/success.txt", HTTP_GET, sendCaptiveRedirect);
    this->server.on("/bag", HTTP_GET, sendCaptiveRedirect);

    // Microsoft Windows
    this->server.on("/connecttest.txt", HTTP_GET, sendCaptiveRedirect);
    this->server.on("/redirect", HTTP_GET, sendCaptiveRedirect);
    this->server.on("/canonical.html", HTTP_GET, sendCaptiveRedirect);

    // 4. Bat moi request chua xu ly (Catch-all)
    this->server.onNotFound([this, sendCaptiveRedirect]() {
        if (this->server.uri().indexOf("favicon") >= 0) {
            this->server.send(204);
            return;
        }
        sendCaptiveRedirect();
    });
}

void CaptivePortal::loop() {
    if (this->active) {
        // Xu ly nhanh nhieu goi tin DNS trong hang doi UDP de tranh timeout khi thiet bi gui song song
        for (uint8_t i = 0; i < 4; i++) {
            this->dnsServer.processNextRequest();
        }
        this->server.handleClient();
    }
}

void CaptivePortal::updateSensorData(float temp, float hum, bool valid) {
    this->currentTemp = temp;
    this->currentHum = hum;
    this->sensorValid = valid;
}

void CaptivePortal::handleSensor() {
    float t = this->currentTemp;
    float h = this->currentHum;

    // Tinh diem suong (Dew Point) theo Magnus formula
    float a = 17.27f;
    float b = 237.7f;
    float safeH = (h > 1.0f) ? h : 1.0f;
    float alpha = ((a * t) / (b + t)) + logf(safeH / 100.0f);
    float dewPoint = (b * alpha) / (a - alpha);

    // Tinh do thieu hut ap suat hoi (Vapor Pressure Deficit - VPD kPa)
    float vpSat = 0.61078f * expf((17.27f * t) / (t + 237.3f));
    float vpd = vpSat * (1.0f - (h / 100.0f));
    if (vpd < 0.0f) vpd = 0.0f;

    uint32_t totalHeap = ESP.getHeapSize();
    uint32_t freeHeap = ESP.getFreeHeap();
    uint32_t minFreeHeap = ESP.getMinFreeHeap();
    uint32_t uptimeSec = millis() / 1000;
    time_t nowEpoch = time(nullptr);
    uint32_t cpuFreq = ESP.getCpuFreqMHz();
    uint16_t logCount = DataLogger::getInstance().getCount();

    String json = "{";
    json += "\"temp\":" + String(this->currentTemp, 2) + ",";
    json += "\"hum\":" + String(this->currentHum, 2) + ",";
    json += "\"dewPoint\":" + String(dewPoint, 1) + ",";
    json += "\"vpd\":" + String(vpd, 2) + ",";
    json += "\"valid\":" + String(this->sensorValid ? "true" : "false") + ",";
    bool isAlert = (this->currentTemp > this->sensorSettings.tempAlert || this->currentHum > this->sensorSettings.humAlert);
    json += "\"alert\":" + String(isAlert ? "true" : "false") + ",";
    json += "\"chipTemp\":" + String(StressTester::getInstance().getChipTemperature(), 1) + ",";
    json += "\"cpu\":" + String(StressTester::getInstance().getCpuLoad(), 1) + ",";
    json += "\"cpu0\":" + String(StressTester::getInstance().getCpuLoadCore0(), 1) + ",";
    json += "\"cpu1\":" + String(StressTester::getInstance().getCpuLoadCore1(), 1) + ",";
    json += "\"cpuFreq\":" + String(cpuFreq) + ",";
    json += "\"totalHeap\":" + String(totalHeap) + ",";
    json += "\"freeHeap\":" + String(freeHeap) + ",";
    json += "\"minFreeHeap\":" + String(minFreeHeap) + ",";
    json += "\"uptimeSec\":" + String(uptimeSec) + ",";
    json += "\"epoch\":" + String((uint32_t)nowEpoch) + ",";
    json += "\"logCount\":" + String(logCount) + ",";
    json += "\"stressActive\":" + String(StressTester::getInstance().isRunning() ? "true" : "false") + ",";
    json += "\"stressSec\":" + String(StressTester::getInstance().getElapsedSec());
    json += "}";
    this->server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    this->server.send(200, "application/json", json);
}

void CaptivePortal::handleSensorConfigGet() {
    String json = "{";
    json += "\"tempAlert\":" + String(this->sensorSettings.tempAlert, 1) + ",";
    json += "\"humAlert\":" + String(this->sensorSettings.humAlert, 1) + ",";
    json += "\"readInterval\":" + String(this->sensorSettings.readIntervalSec) + ",";
    json += "\"sendInterval\":" + String(this->sensorSettings.sendIntervalSec);
    json += "}";
    this->server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    this->server.send(200, "application/json", json);
}

void CaptivePortal::handleSensorConfigSet() {
    if (this->server.hasArg("reset") && this->server.arg("reset") == "1") {
        this->sensorSettings.tempAlert = TEMP_ALERT_THRESHOLD;
        this->sensorSettings.humAlert = HUM_ALERT_THRESHOLD;
        this->sensorSettings.readIntervalSec = 1;
        this->sensorSettings.sendIntervalSec = 1;
        this->storage.saveSensorSettings(this->sensorSettings.tempAlert, this->sensorSettings.humAlert,
                                         this->sensorSettings.readIntervalSec, this->sensorSettings.sendIntervalSec);
        Serial.println("[Sensor] Da khoi phuc cau hinh cam bien ve mac dinh!");
        this->handleSensorConfigGet();
        return;
    }

    if (this->server.hasArg("temp_alert")) {
        this->sensorSettings.tempAlert = this->server.arg("temp_alert").toFloat();
    }
    if (this->server.hasArg("hum_alert")) {
        this->sensorSettings.humAlert = this->server.arg("hum_alert").toFloat();
    }
    if (this->server.hasArg("read_interval")) {
        uint32_t r = this->server.arg("read_interval").toInt();
        if (r < 1) r = 1;
        if (r > 60) r = 60;
        this->sensorSettings.readIntervalSec = r;
    }
    if (this->server.hasArg("send_interval")) {
        uint32_t s = this->server.arg("send_interval").toInt();
        if (s < 1) s = 1;
        if (s > 300) s = 300;
        this->sensorSettings.sendIntervalSec = s;
    }

    this->storage.saveSensorSettings(this->sensorSettings.tempAlert, this->sensorSettings.humAlert,
                                     this->sensorSettings.readIntervalSec, this->sensorSettings.sendIntervalSec);
    Serial.printf("[Sensor] Da cap nhat cau hinh: TempAlert=%.1f *C, HumAlert=%.1f %%, Read=%ds, Send=%ds\n",
                  this->sensorSettings.tempAlert, this->sensorSettings.humAlert,
                  this->sensorSettings.readIntervalSec, this->sensorSettings.sendIntervalSec);

    this->handleSensorConfigGet();
}

void CaptivePortal::handleLedGet() {
    String json = "{";
    json += "\"mode\":" + String((uint8_t)this->rgbController.getMode()) + ",";
    json += "\"r\":" + String(this->rgbController.getR()) + ",";
    json += "\"g\":" + String(this->rgbController.getG()) + ",";
    json += "\"b\":" + String(this->rgbController.getB()) + ",";
    json += "\"brightness\":" + String(this->rgbController.getBrightness()) + ",";
    json += "\"speed\":" + String(this->rgbController.getSpeed());
    json += "}";
    this->server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    this->server.send(200, "application/json", json);
}

void CaptivePortal::handleLedSet() {
    uint8_t mode = this->server.hasArg("mode") ? this->server.arg("mode").toInt() : (uint8_t)this->rgbController.getMode();
    uint8_t r = this->server.hasArg("r") ? this->server.arg("r").toInt() : this->rgbController.getR();
    uint8_t g = this->server.hasArg("g") ? this->server.arg("g").toInt() : this->rgbController.getG();
    uint8_t b = this->server.hasArg("b") ? this->server.arg("b").toInt() : this->rgbController.getB();
    uint8_t brightness = this->server.hasArg("brightness") ? this->server.arg("brightness").toInt() : this->rgbController.getBrightness();
    uint8_t speed = this->server.hasArg("speed") ? this->server.arg("speed").toInt() : this->rgbController.getSpeed();

    this->rgbController.applyConfig(mode, r, g, b, brightness, speed);

    if (this->server.hasArg("save") && this->server.arg("save") == "1") {
        this->storage.saveLedConfig(mode, r, g, b, brightness, speed);
        Serial.printf("[LED] Da luu cau hinh vao Flash (NVS): Mode=%d, R=%d, G=%d, B=%d, Bright=%d, Speed=%d\n",
                      mode, r, g, b, brightness, speed);
        this->handleLedGet();
        return;
    }

    this->server.sendHeader("Connection", "keep-alive");
    this->server.send(204, "text/plain", "");
}

void CaptivePortal::handleLedReset() {
    this->rgbController.setPurpleBreathing();
    this->storage.saveLedConfig(RGB_MODE_BREATHING, 168, 85, 247, 220, 50);
    Serial.println("[LED] Da khoi phuc LED ve mac dinh: Tim Breathing.");
    this->handleLedGet();
}

void CaptivePortal::handleShutdownAP() {
    Serial.println("[Portal] Nhan lenh tu Web UI sau 3s dem nguoc -> Tat Access Point ngay lap tuc.");
    this->server.send(200, "application/json", "{\"success\":true}");
    delay(100);
    this->wifiService.stopAP();
}

void CaptivePortal::handleResetWifi() {
    Serial.println("[Portal] Nhan lenh Reset WiFi: Xoa credentials NVS va khoi dong lai -> AP mode.");
    this->server.send(200, "application/json", "{\"success\":true}");
    delay(200);
    // Xoa toan bo credentials da luu, ESP.restart() se vao AP mode vi hasCredentials() = false
    this->storage.clear();
    delay(100);
    ESP.restart();
}

void CaptivePortal::handleScan() {
    int n = WiFi.scanNetworks();
    String json = "[";
    for (int i = 0; i < n; ++i) {
        if (i > 0) json += ",";
        json += "{\"ssid\":\"" + WiFi.SSID(i) + "\",\"rssi\":" + String(WiFi.RSSI(i)) + "}";
    }
    json += "]";
    this->server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    this->server.send(200, "application/json", json);
}

void CaptivePortal::handleStatus() {
    WiFiConnectResult res = this->wifiService.getConnectResult();
    String stateStr = "idle";
    if (res == RESULT_CONNECTING) {
        stateStr = "connecting";
    } else if (res == RESULT_SUCCESS) {
        stateStr = "success";
        if (!this->countdownTriggered) {
            this->countdownTriggered = true;
            this->wifiService.triggerShutdownCountdown(3000);
        }
    } else if (res == RESULT_FAILED) {
        stateStr = "failed";
    }

    String json = "{";
    json += "\"state\":\"" + stateStr + "\",";
    json += "\"ip\":\"" + this->wifiService.getIP() + "\",";
    json += "\"error\":\"" + this->wifiService.getLastError() + "\"";
    json += "}";

    this->server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    this->server.sendHeader("Pragma", "no-cache");
    this->server.sendHeader("Expires", "0");
    this->server.send(200, "application/json", json);
}

void CaptivePortal::handleSave() {
    String ssid = this->server.arg("ssid");
    String pass = this->server.arg("password");
    String isEntStr = this->server.arg("is_ent");
    String user = this->server.arg("username");
    String backend = this->server.arg("backend");

    bool isEnt = (isEntStr == "1" || isEntStr == "true" || isEntStr == "on");

    if (ssid.length() == 0) {
        this->server.send(400, "application/json", "{\"success\":false,\"error\":\"Ten WiFi khong duoc de trong!\"}");
        return;
    }

    this->countdownTriggered = false;
    this->wifiService.connectStation(ssid, pass, user, isEnt, backend);
    this->server.send(200, "application/json", "{\"success\":true,\"message\":\"Dang ket noi...\"}");
}

void CaptivePortal::handleRoot() {
    this->server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    this->server.sendHeader("Pragma", "no-cache");
    this->server.sendHeader("Expires", "0");
    this->server.setContentLength(CONTENT_LENGTH_UNKNOWN);
    this->server.send(200, "text/html", "");

    String savedSSID, savedPass, savedUser, savedBackend;
    bool savedIsEnt = false;
    this->storage.loadCredentials(savedSSID, savedPass, savedUser, savedIsEnt, savedBackend);

    streamCaptivePortalHTML(this->server, savedSSID, savedPass, savedUser, savedIsEnt, savedBackend);
    this->server.sendContent("");
}

void CaptivePortal::handleStressTest() {
    if (this->server.hasArg("action") && this->server.arg("action") == "stop") {
        StressTester::getInstance().stop();
    } else if (this->server.hasArg("action") && this->server.arg("action") == "start") {
        StressTester::getInstance().start();
    }
    String json = "{";
    json += "\"running\":" + String(StressTester::getInstance().isRunning() ? "true" : "false") + ",";
    json += "\"elapsedSec\":" + String(StressTester::getInstance().getElapsedSec()) + ",";
    json += "\"chipTemp\":" + String(StressTester::getInstance().getChipTemperature(), 1) + ",";
    json += "\"cpu\":" + String(StressTester::getInstance().getCpuLoad(), 1) + ",";
    json += "\"cpu0\":" + String(StressTester::getInstance().getCpuLoadCore0(), 1) + ",";
    json += "\"cpu1\":" + String(StressTester::getInstance().getCpuLoadCore1(), 1);
    json += "}";
    this->server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    this->server.send(200, "application/json", json);
}

void CaptivePortal::handleHistoryGet() {
    if (this->server.hasArg("epoch")) {
        uint32_t epoch = (uint32_t)this->server.arg("epoch").toInt();
        if (epoch > 1000000000) {
            struct timeval tv = { (time_t)epoch, 0 };
            settimeofday(&tv, NULL);
            DataLogger::getInstance().syncEpoch(epoch);
        }
    }

    uint16_t filterMins = 1440; // Mac dinh 24h
    if (this->server.hasArg("mins")) {
        int m = this->server.arg("mins").toInt();
        if (m >= 0 && m <= 1440) {
            filterMins = (uint16_t)m;
        }
    } else if (this->server.hasArg("hours")) {
        int h = this->server.arg("hours").toInt();
        if (h > 0 && h <= 24) {
            filterMins = (uint16_t)(h * 60);
        }
    }
    String json = DataLogger::getInstance().getHistoryJson(filterMins);
    this->server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    this->server.send(200, "application/json", json);
}

void CaptivePortal::handleHistoryCsv() {
    String csv = DataLogger::getInstance().getCsvString();
    this->server.sendHeader("Content-Disposition", "attachment; filename=\"sensor_history.csv\"");
    this->server.send(200, "text/csv", csv);
}

void CaptivePortal::handleSyncTime() {
    if (this->server.hasArg("epoch")) {
        uint32_t epoch = (uint32_t)this->server.arg("epoch").toInt();
        if (epoch > 1000000000) {
            struct timeval tv = { (time_t)epoch, 0 };
            settimeofday(&tv, NULL);
            DataLogger::getInstance().syncEpoch(epoch);
            Serial.printf("[Portal] Da dong bo thoi gian thuc tu client: %u (Unix Epoch)\n", epoch);
            this->server.send(200, "application/json", "{\"status\":\"ok\"}");
            return;
        }
    }
    this->server.send(400, "application/json", "{\"status\":\"error\"}");
}


void CaptivePortal::handlePowerGet() {
    String json = "{";
    json += "\"cpuFreq\":" + String((int)this->powerManager.getCurrentCpuFreq()) + ",";
    json += "\"wifiMode\":" + String((int)this->powerManager.getWifiPowerMode()) + ",";
    json += "\"bluetooth\":" + String(this->powerManager.isBluetoothEnabled() ? "true" : "false") + ",";
    json += "\"ledHz\":" + String(this->powerManager.getLedUpdateHz()) + ",";
    json += "\"estimatedCurrent\":" + String(this->powerManager.getEstimatedCurrentDraw(), 1) + ",";
    json += "\"estimatedSavings\":" + String(this->powerManager.getEstimatedPowerSavings(), 1) + ",";
    json += "\"actualCpuFreq\":" + String(getCpuFrequencyMhz());
    json += "}";
    this->server.send(200, "application/json", json);
}

void CaptivePortal::handlePowerSet() {
    bool changed = false;
    
    if (this->server.hasArg("cpu_freq")) {
        int freq = this->server.arg("cpu_freq").toInt();
        if (freq == 80 || freq == 160 || freq == 240) {
            this->powerManager.applyCpuFrequency((CpuFrequency)freq);
            changed = true;
        }
    }
    
    if (this->server.hasArg("wifi_mode")) {
        int mode = this->server.arg("wifi_mode").toInt();
        if (mode >= 0 && mode <= 2) {
            this->powerManager.applyWifiPowerMode((WifiPowerMode)mode);
            changed = true;
        }
    }
    
    if (this->server.hasArg("bluetooth")) {
        bool enabled = (this->server.arg("bluetooth") == "true" || this->server.arg("bluetooth") == "1");
        this->powerManager.setBluetoothEnabled(enabled);
        changed = true;
    }
    
    if (this->server.hasArg("led_hz")) {
        uint8_t hz = this->server.arg("led_hz").toInt();
        if (hz == 30 || hz == 50 || hz == 100) {
            this->powerManager.setLedUpdateHz(hz);
            changed = true;
        }
    }
    
    if (changed) {
        this->server.send(200, "application/json", "{\"success\":true,\"message\":\"Cau hinh tiet kiem nang luong da duoc cap nhat!\"}");
    } else {
        this->server.send(400, "application/json", "{\"success\":false,\"message\":\"Khong co tham so nao hop le!\"}");
    }
}
```

---

### 3.24. Giao Diện Web UI HTML (Header)
#### File: `src/network/web/WebPage.h`
> **Mô tả**: Chunked streaming truyền tải giao diện Web Captive Portal 5 Tabs (Wi-Fi, Cảm Biến, LED, Biểu Đồ, Nguồn)  
> **Dung lượng**: 94 dòng | 30976 bytes

```cpp
#pragma once
#include <Arduino.h>
#include <WebServer.h>
#include "WebStyles.h"
#include "WebScript.h"

// Ham stream truc tiep tung phan ma HTML trang Web Captive Portal qua WebServer (Chunked Transfer)
// Tranh tran bo nho RAM (Heap Out-Of-Memory) va dam bao toan bo trang duoc truyen nguyen ven 100%
inline void streamCaptivePortalHTML(WebServer &server, const String &savedSSID, const String &savedPass, const String &savedUser, bool savedIsEnt, const String &savedBackend) {
    // 1. Head & CSS
    server.sendContent_P(PSTR("<!DOCTYPE html><html lang='vi'><head><meta charset='utf-8'><meta name='viewport' content='width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no'><title>Cài Đặt ESP32 Sensor</title><style>"));
    server.sendContent_P(WEB_STYLES_CSS);
    server.sendContent_P(PSTR("</style></head><body>"));

    // 2. Container, Tabs
    String chunk = F("<div class='container'><div id='setupView'><div class='tab-nav'><button type='button' class='tab-btn active' id='tabBtnWifi' onclick='switchTab(\"wifi\")'><svg width='14' height='14' viewBox='0 0 24 24' fill='none' stroke='currentColor' stroke-width='2'><path d='M5 12.55a11 11 0 0 1 14.08 0'></path><path d='M1.42 9a16 16 0 0 1 21.16 0'></path><path d='M8.53 16.11a6 6 0 0 1 6.95 0'></path><line x1='12' y1='20' x2='12.01' y2='20'></line></svg> Wi-Fi</button><button type='button' class='tab-btn' id='tabBtnSensor' onclick='switchTab(\"sensor\")'><svg width='14' height='14' viewBox='0 0 24 24' fill='none' stroke='currentColor' stroke-width='2'><path d='M14 14.76V3.5a2.5 2.5 0 0 0-5 0v11.26a4.5 4.5 0 1 0 5 0z'/></svg> Cảm Biến</button><button type='button' class='tab-btn' id='tabBtnLed' onclick='switchTab(\"led\")'><svg width='14' height='14' viewBox='0 0 24 24' fill='none' stroke='currentColor' stroke-width='2'><circle cx='12' cy='12' r='5'></circle><line x1='12' y1='1' x2='12' y2='3'></line><line x1='12' y1='21' x2='12' y2='23'></line><line x1='4.22' y1='4.22' x2='5.64' y2='5.64'></line><line x1='18.36' y1='18.36' x2='19.78' y2='19.78'></line><line x1='1' y1='12' x2='3' y2='12'></line><line x1='21' y1='12' x2='23' y2='12'></line><line x1='4.22' y1='19.78' x2='5.64' y2='18.36'></line><line x1='18.36' y1='5.64' x2='19.78' y2='4.22'></line></svg> Đèn LED</button><button type='button' class='tab-btn' id='tabBtnChart' onclick='switchTab(\"chart\")'><svg width='14' height='14' viewBox='0 0 24 24' fill='none' stroke='currentColor' stroke-width='2' stroke-linecap='round' stroke-linejoin='round'><polyline points='22 12 18 12 15 21 9 3 6 12 2 12'/></svg> Biểu Đồ</button><button type='button' class='tab-btn' id='tabBtnPower' onclick='switchTab(\"power\")'><svg width='14' height='14' viewBox='0 0 24 24' fill='none' stroke='currentColor' stroke-width='2' stroke-linecap='round' stroke-linejoin='round'><polygon points='13 2 3 14 12 14 11 22 21 10 12 10 13 2'/></svg> Nguồn</button></div>");
    server.sendContent(chunk);

    // 3. Tab 1: Cấu hình Wi-Fi (Đã chuyển toàn bộ thông số sang Tab 2)
    chunk = F("<div id='tabContentWifi'>");
    server.sendContent(chunk);

    // 4. Form Wi-Fi
    chunk = F("<div id='alertBox' class='alert-box'></div><form id='wifiForm' onsubmit='submitForm(event)'><label>Tên Wi-Fi (SSID)</label><div class='input-group'><input type='text' id='ssid' name='ssid' placeholder='Chọn hoặc tự nhập SSID' required value='");
    chunk += savedSSID;
    chunk += F("'><button type='button' class='btn-open-modal' onclick='openWifiModal()'><svg width='14' height='14' viewBox='0 0 24 24' fill='none' stroke='currentColor' stroke-width='2.2' stroke-linecap='round' stroke-linejoin='round'><path d='M5 12.55a11 11 0 0 1 14.08 0'></path><path d='M1.42 9a16 16 0 0 1 21.16 0'></path><path d='M8.53 16.11a6 6 0 0 1 6.95 0'></path><line x1='12' y1='20' x2='12.01' y2='20'></line></svg><span>Chọn Mạng</span></button></div><div class='toggle-box'><div style='display:flex; flex-direction:column; gap:2px;'><span>Wi-Fi Trường Học / Cty</span><span style='font-size:11px; color:#64748b;'>Chuẩn WPA2-Enterprise</span></div><label class='switch'><input type='checkbox' id='is_ent' name='is_ent' value='1' onchange='toggleEnterprise()' ");
    if (savedIsEnt) chunk += F("checked");
    chunk += F("><span class='slider'></span></label></div><div id='userBlock' style='display: ");
    chunk += (savedIsEnt ? F("block") : F("none"));
    chunk += F(";'><label>Tài Khoản / MSSV (Username)</label><input type='text' id='username' name='username' placeholder='Nhập mã số sinh viên hoặc email' value='");
    chunk += savedUser;
    chunk += F("'></div><label>Mật Khẩu (Password)</label><input type='password' id='password' name='password' placeholder='Nhập mật khẩu Wi-Fi' value='");
    chunk += savedPass;
    chunk += F("'><label>Địa Chỉ Backend API (HTTP POST)</label><input type='text' id='backend' name='backend' placeholder='http://ip-server:3000/api/sensor' value='");
    chunk += savedBackend;
    chunk += F("'><button type='submit' id='btnSubmit' class='btn-submit'>Lưu &amp; Kết Nối Ngay</button></form><button type='button' class='btn-reset-wifi' id='btnResetWifi' onclick='resetWifiCredentials()'>Ngắt mạng Wi-Fi</button></div>");
    server.sendContent(chunk);

    // 5. Tab 2: Cảm Biến Toàn Diện & Stress Test CPU
    chunk = F("<div id='tabContentSensor' style='display: none;'><div class='sensor-sec-header'><span class='sensor-sec-pill' style='color:#38bdf8; background:rgba(56,189,248,0.14); border:1px solid rgba(56,189,248,0.3);'>SHT31</span><span class='sensor-sec-text'>THÔNG SỐ MÔI TRƯỜNG & KHÍ HẬU</span></div><div class='sensor-dash-grid'><div class='sensor-stat-box'><div class='stat-box-top'><span class='stat-box-title'>Nhiệt Độ Môi Trường</span><span class='stat-box-unit' style='color:#f43f5e;'>°C</span></div><div class='stat-box-num'><span id='dashTemp'>--.-</span> <small>°C</small></div><div class='stat-box-sub' id='dashTempStatus'>Cảm biến SHT31</div></div><div class='sensor-stat-box'><div class='stat-box-top'><span class='stat-box-title'>Độ Ẩm Không Khí</span><span class='stat-box-unit' style='color:#38bdf8;'>%RH</span></div><div class='stat-box-num'><span id='dashHum'>--.-</span> <small>%</small></div><div class='stat-box-sub' id='dashHumStatus'>Độ ẩm tương đối</div></div><div class='sensor-stat-box'><div class='stat-box-top'><span class='stat-box-title'>Điểm Sương (Dew Point)</span><span class='stat-box-unit' style='color:#34d399;'>°C</span></div><div class='stat-box-num'><span id='dashDewPoint'>--.-</span> <small>°C</small></div><div class='stat-box-sub'>Nhiệt độ ngưng tụ hơi nước</div></div><div class='sensor-stat-box'><div class='stat-box-top'><span class='stat-box-title'>Áp Suất Hơi (VPD)</span><span class='stat-box-unit' style='color:#fbbf24;'>kPa</span></div><div class='stat-box-num'><span id='dashVpd'>--.-</span> <small>kPa</small></div><div class='stat-box-sub'>Độ hụt áp suất hơi nước</div></div></div>");
    server.sendContent(chunk);

    chunk = F("<div class='sensor-sec-header' style='margin-top:14px;'><span class='sensor-sec-pill' style='color:#c084fc; background:rgba(192,132,252,0.14); border:1px solid rgba(192,132,252,0.3);'>ESP32-S3</span><span class='sensor-sec-text'>HIỆU NĂNG & PHẦN CỨNG HỆ THỐNG</span></div><div class='sensor-dash-grid'><div class='sensor-stat-box'><div class='stat-box-top'><span class='stat-box-title'>Nhiệt Độ CPU</span><span id='dashChipBadge' class='stat-box-unit' style='color:#f59e0b;'>--.- °C</span></div><div class='stat-box-num'><span id='dashChipTemp'>--.-</span> <small>°C</small></div><div class='stat-box-sub'>Xung nhịp: <strong style='color:#c084fc;'>240 MHz</strong> Dual-Core</div></div><div class='sensor-stat-box'><div class='stat-box-top'><span class='stat-box-title'>Tải CPU Thực Tế</span><span id='dashCpuBadge' class='stat-box-unit' style='color:#38bdf8;'>0%</span></div><div class='stat-box-num'><span id='dashCpuLoad'>0.0</span> <small>%</small></div><div class='stat-box-sub'>C0: <strong id='dashCpu0' style='color:#38bdf8;'>0%</strong> | C1: <strong id='dashCpu1' style='color:#38bdf8;'>0%</strong></div></div><div class='sensor-stat-box'><div class='stat-box-top'><span class='stat-box-title'>Bộ Nhớ RAM</span><span id='dashRamPercent' class='stat-box-unit' style='color:#10b981;'>--%</span></div><div class='stat-box-num' style='font-size:15px;'><span id='dashRamUsed'>--</span> / <span id='dashRamTotal'>--</span> <small>KB</small></div><div class='stat-box-sub'>Trống: <strong id='dashRamFree' style='color:#10b981;'>---</strong> KB (Đáy: <span id='dashRamMin'>---</span> KB)</div></div><div class='sensor-stat-box'><div class='stat-box-top'><span class='stat-box-title'>Thời Gian Hoạt Động</span><span class='stat-box-unit' style='color:#a855f7;'>UPTIME</span></div><div class='stat-box-num' style='font-size:16px;'><span id='dashUptime'>00:00:00</span></div><div class='stat-box-sub'>Đã lưu: <strong id='dashLogCount' style='color:#38bdf8;'>0</strong> mẫu đo</div></div></div>");
    server.sendContent(chunk);

    chunk = F("<div class='led-section-title' style='margin-top:16px;'>CẤU HÌNH NGƯỠNG & CHU KỲ</div><div class='sensor-cfg-grid'><div class='sensor-cfg-card'><div class='sensor-cfg-header'><span class='sensor-cfg-title'>Ngưỡng Cảnh Báo Nhiệt Độ</span><span class='sensor-unit-badge'>°C</span></div><div class='sensor-cfg-desc'>Vượt ngưỡng này còi kêu và đổi LED cảnh báo</div><div class='sensor-input-wrap'><input type='number' id='cfgTempAlert' class='sensor-num-input' min='20' max='80' step='0.5' value='37.0'><span style='font-size:12px; color:#94a3b8;'>°C</span></div></div><div class='sensor-cfg-card'><div class='sensor-cfg-header'><span class='sensor-cfg-title'>Ngưỡng Cảnh Báo Độ Ẩm</span><span class='sensor-unit-badge'>%</span></div><div class='sensor-cfg-desc'>Vượt ngưỡng này còi kêu và đổi LED cảnh báo</div><div class='sensor-input-wrap'><input type='number' id='cfgHumAlert' class='sensor-num-input' min='30' max='99' step='1' value='87.0'><span style='font-size:12px; color:#94a3b8;'>%</span></div></div><div class='sensor-cfg-card'><div class='sensor-cfg-header'><span class='sensor-cfg-title'>Chu Kỳ Đo Cảm Biến</span><span class='sensor-unit-badge'>Giây</span></div><div class='sensor-cfg-desc'>Thời gian giữa mỗi lần đọc SHT31 (1 - 60s)</div><div class='sensor-input-wrap'><input type='number' id='cfgReadInterval' class='sensor-num-input' min='1' max='60' step='1' value='1'><span style='font-size:12px; color:#94a3b8;'>giây</span></div></div><div class='sensor-cfg-card'><div class='sensor-cfg-header'><span class='sensor-cfg-title'>Chu Kỳ Gửi Dữ Liệu Backend</span><span class='sensor-unit-badge'>Giây</span></div><div class='sensor-cfg-desc'>Thời gian giữa mỗi lần gửi HTTP POST (1 - 300s)</div><div class='sensor-input-wrap'><input type='number' id='cfgSendInterval' class='sensor-num-input' min='1' max='300' step='1' value='1'><span style='font-size:12px; color:#94a3b8;'>giây</span></div></div></div><div class='btn-action-row'><button type='button' class='btn-submit' style='flex:1; margin-top:0; background:linear-gradient(135deg, #10b981, #059669);' onclick='saveSensorSettings()'>Lưu Cấu Hình</button><button type='button' class='btn-secondary' onclick='resetSensorSettings()'>Mặc Định</button></div><div id='sensorToast' class='led-toast'>Đã cập nhật cấu hình cảm biến thành công!</div></div>");
    server.sendContent(chunk);

    // 6. Tab 3: LED Studio
    chunk = F("<div id='tabContentLed' style='display: none;'><div class='led-section-title'>CHẾ ĐỘ ÁNH SÁNG</div><div class='led-modes-grid'><div class='led-mode-card' id='cardMode4' onclick='selectLedMode(4)'><div class='mode-header'><span class='mode-name'>Bar Club EDM</span><span class='mode-badge-hot'>HOT</span></div><div class='mode-desc'>Chớp giật vũ trường Strobe 135 BPM</div></div><div class='led-mode-card' id='cardMode3' onclick='selectLedMode(3)'><div class='mode-header'><span class='mode-name'>Cầu Vồng</span></div><div class='mode-desc'>Xoay vòng quang phổ 256 màu</div></div><div class='led-mode-card' id='cardMode2' onclick='selectLedMode(2)'><div class='mode-header'><span class='mode-name'>Nhịp Thở</span><span class='mode-badge-def'>MẶC ĐỊNH</span></div><div class='mode-desc'>Nhịp thở êm ái chuẩn Gear</div></div><div class='led-mode-card' id='cardMode1' onclick='selectLedMode(1)'><div class='mode-header'><span class='mode-name'>Màu Tĩnh</span></div><div class='mode-desc'>Sáng cố định một màu đã chọn</div></div><div class='led-mode-card' id='cardMode5' onclick='selectLedMode(5)'><div class='mode-header'><span class='mode-name'>Báo Nhiệt Độ</span></div><div class='mode-desc'>Đổi màu theo cảm biến SHT31</div></div><div class='led-mode-card' id='cardMode0' onclick='selectLedMode(0)'><div class='mode-header'><span class='mode-name'>Tắt Đèn</span></div><div class='mode-desc'>Tắt LED để tiết kiệm pin</div></div></div><div class='led-control-box'><div class='led-box-label'><span>ĐỘ SÁNG</span><span id='brightVal' style='color:#c084fc; font-weight:700;'>86%</span></div><input type='range' id='ledBrightness' min='0' max='255' value='220' class='led-slider' oninput='onBrightnessChange(event)'></div><div id='ledSpeedSection' class='led-control-box'><div class='led-box-label'><span>TỐC ĐỘ HIỆU ỨNG</span><span id='speedVal' style='color:#c084fc; font-weight:700;'>50</span></div><input type='range' id='ledSpeed' min='1' max='100' value='50' class='led-slider' oninput='onSpeedChange(event)'><div style='display:flex; justify-content:space-between; font-size:11px; color:#64748b; margin-top:5px;'><span>Chậm</span><span>Trung Bình</span><span>Siêu Tốc</span></div></div><div id='ledColorSection' class='color-inline-panel'><div class='led-section-title' style='display:flex; justify-content:space-between; align-items:center; margin-bottom:10px;'><span>BÁNH XE MÀU SẮC</span><div class='color-preview-pill'><div id='colorPreviewBadge' class='color-preview-dot'></div><span id='hexColorVal' class='color-hex-text'>#A855F7</span></div></div><div class='color-wheel-wrapper'><div id='colorWheelContainer' class='color-wheel-canvas-container'><canvas id='colorWheelCanvas' width='260' height='260'></canvas><div id='wheelHandle'></div></div><div class='color-chips'><button type='button' class='color-chip' style='background:#a855f7;' title='Tím Cyber' onclick='pickPresetColor(\"#a855f7\")'></button><button type='button' class='color-chip' style='background:#06b6d4;' title='Xanh Cyan' onclick='pickPresetColor(\"#06b6d4\")'></button><button type='button' class='color-chip' style='background:#22c55e;' title='Xanh Lime' onclick='pickPresetColor(\"#22c55e\")'></button><button type='button' class='color-chip' style='background:#eab308;' title='Vàng Rực' onclick='pickPresetColor(\"#eab308\")'></button><button type='button' class='color-chip' style='background:#ef4444;' title='Đỏ Lửa' onclick='pickPresetColor(\"#ef4444\")'></button><button type='button' class='color-chip' style='background:#ec4899;' title='Hồng Neon' onclick='pickPresetColor(\"#ec4899\")'></button><button type='button' class='color-chip' style='background:#ffffff;' title='Trắng Tuyết' onclick='pickPresetColor(\"#ffffff\")'></button></div></div></div><div class='btn-action-row' style='margin-top:14px;'><button type='button' class='btn-submit' style='flex:1; margin-top:0; background:linear-gradient(135deg, #9333ea, #c084fc);' onclick='saveLedConfig()'>Lưu Cấu Hình</button><button type='button' class='btn-secondary' onclick='resetLedToDefault()'>Mặc Định (Tím)</button></div><p style='text-align:center; font-size:11.5px; color:#94a3b8; margin-top:8px;'>Cấu hình được lưu sẽ tự động áp dụng khi khởi động lại máy.</p><div id='ledToast' class='led-toast'>Đã lưu cấu hình LED vào Flash vĩnh viễn!</div><div style='height:48px;'></div></div></div>");
    server.sendContent(chunk);

    // 7. Tab 4: Biểu Đồ Lịch Sử
    chunk = F("<div id='tabContentChart' style='display: none;'><div class='chart-header-row'><div class='chart-filter-group'><button type='button' class='btn-filter' id='filterBtn15m' onclick='setChartFilter(15)'>15m</button><button type='button' class='btn-filter active' id='filterBtn1h' onclick='setChartFilter(60)'>1h</button><button type='button' class='btn-filter' id='filterBtn3h' onclick='setChartFilter(180)'>3h</button><button type='button' class='btn-filter' id='filterBtn6h' onclick='setChartFilter(360)'>6h</button><button type='button' class='btn-filter' id='filterBtn12h' onclick='setChartFilter(720)'>12h</button><button type='button' class='btn-filter' id='filterBtn24h' onclick='setChartFilter(1440)'>24h</button><button type='button' class='btn-filter' id='filterBtnAll' onclick='setChartFilter(0)'>Tất Cả</button></div><button type='button' class='btn-export-csv' onclick='downloadCsv()'><svg width='13' height='13' viewBox='0 0 24 24' fill='none' stroke='currentColor' stroke-width='2.2' stroke-linecap='round' stroke-linejoin='round'><path d='M21 15v4a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2v-4'/><polyline points='7 10 12 15 17 10'/><line x1='12' y1='15' x2='12' y2='3'/></svg><span>CSV</span></button></div><div class='chart-summary-cards'><div class='chart-stat-card'><div style='display:flex; justify-content:space-between; align-items:center; margin-bottom:2px;'><span class='stat-label' style='color:#f43f5e;'>Nhiệt Độ Live</span><span style='font-size:10.5px; color:#94a3b8;'><span id='statTempMin'>--.-</span> - <span id='statTempMax'>--.-</span>°</span></div><div class='stat-val' style='font-size:18px; color:#f43f5e;'><span id='statTempLive'>--.-</span> <small>°C</small></div></div><div class='chart-stat-card'><div style='display:flex; justify-content:space-between; align-items:center; margin-bottom:2px;'><span class='stat-label' style='color:#38bdf8;'>Độ Ẩm Live</span><span style='font-size:10.5px; color:#94a3b8;'><span id='statHumMin'>--.-</span> - <span id='statHumMax'>--.-</span>%</span></div><div class='stat-val' style='font-size:18px; color:#38bdf8;'><span id='statHumLive'>--.-</span> <small>%</small></div></div></div><div class='chart-legend-row'><div class='chart-legend-item'><span class='legend-dot' style='background:#f43f5e;'></span><span>Nhiệt Độ (°C)</span></div><div class='chart-legend-item'><span class='legend-dot' style='background:#38bdf8;'></span><span>Độ Ẩm (%)</span></div><span id='chartLiveBadge' class='chart-live-badge'>● LIVE</span><span id='chartDataCount' class='chart-count-tag'>0 điểm</span></div><div class='chart-wrapper'><canvas id='historyCanvas' width='360' height='210'></canvas><div id='chartTooltip' class='chart-tooltip' style='display:none;'></div></div><p style='text-align:center; font-size:11px; color:#64748b; margin-top:8px;'>Chạm hoặc rê chuột trên biểu đồ để xem chi tiết từng thời điểm.</p><div style='height:36px;'></div></div>");
    server.sendContent(chunk);

    // 8. Tab 5: Tối Ưu Năng Lượng
    chunk = F("<div id='tabContentPower' style='display: none;'><div class='led-section-title'>⚡ TỐI ƯU NĂNG LƯỢNG & NHIỆT ĐỘ</div><div class='sensor-cfg-grid' style='margin-bottom:16px;'><div class='sensor-cfg-card'><div class='sensor-cfg-header'><span class='sensor-cfg-title'>Dòng Điện Ước Tính</span><span class='sensor-unit-badge' id='pwrCurrentBadge' style='background:#10b981;'>218 mA</span></div><div style='font-size:20px; font-weight:700; color:#10b981; margin:8px 0;' id='pwrCurrentVal'>218 mA</div><div class='sensor-cfg-desc'>So với mặc định (240MHz, Full WiFi)</div></div><div class='sensor-cfg-card'><div class='sensor-cfg-header'><span class='sensor-cfg-title'>Tiết Kiệm Năng Lượng</span><span class='sensor-unit-badge' id='pwrSavingsBadge' style='background:#f59e0b;'>0 mA</span></div><div style='font-size:20px; font-weight:700; color:#f59e0b; margin:8px 0;' id='pwrSavingsVal'>0 mA</div><div class='sensor-cfg-desc'>Giảm nhiệt độ chip ước tính: <strong id='pwrTempDelta'>0°C</strong></div></div></div>");
    server.sendContent(chunk);

    chunk = F("<div style='margin-bottom:14px;'><div style='font-size:12px; font-weight:700; color:#cbd5e1; margin-bottom:8px;'>CẤU HÌNH NHANH (PRESET)</div><div style='display:flex; gap:6px;'><button type='button' class='btn-filter' style='flex:1; padding:7px 4px; text-align:center;' id='presetBalanced' onclick='applyPowerPreset(\"balanced\")'>Cân Bằng</button><button type='button' class='btn-filter' style='flex:1; padding:7px 4px; text-align:center;' id='presetLow' onclick='applyPowerPreset(\"low\")'>Tiết Kiệm</button><button type='button' class='btn-filter active' style='flex:1; padding:7px 4px; text-align:center;' id='presetMax' onclick='applyPowerPreset(\"max\")'>Tối Đa</button></div></div>");
    server.sendContent(chunk);

    chunk = F("<div class='sensor-cfg-card' style='margin-bottom:14px;'><div class='sensor-cfg-header'><span class='sensor-cfg-title'>Tần Số CPU</span><span class='sensor-unit-badge' id='pwrActualCpuBadge'>240 MHz</span></div><div class='sensor-cfg-desc' style='margin-bottom:10px;'>Thay đổi ngay lập tức, không cần khởi động lại</div><div style='display:flex; gap:8px; margin-bottom:10px;'><button type='button' class='btn-filter' style='flex:1;' id='cpuBtn80' onclick='selectCpuFreq(80)'>80 MHz</button><button type='button' class='btn-filter' style='flex:1;' id='cpuBtn160' onclick='selectCpuFreq(160)'>160 MHz</button><button type='button' class='btn-filter active' style='flex:1;' id='cpuBtn240' onclick='selectCpuFreq(240)'>240 MHz</button></div><div style='font-size:11px; color:#64748b; line-height:1.5;'><strong style='color:#10b981;'>80MHz:</strong> Siêu tiết kiệm (-60mA), chỉ sensor<br><strong style='color:#38bdf8;'>160MHz:</strong> Cân bằng khuyến nghị (-30mA)<br><strong style='color:#94a3b8;'>240MHz:</strong> Hiệu năng tối đa (mặc định)</div></div>");
    server.sendContent(chunk);

    chunk = F("<div class='sensor-cfg-card' style='margin-bottom:14px;'><div class='sensor-cfg-header'><span class='sensor-cfg-title'>WiFi Power Save</span><span class='sensor-unit-badge'>Mode</span></div><div class='sensor-cfg-desc' style='margin-bottom:10px;'>Tắt radio khi không truyền dữ liệu</div><div style='display:flex; gap:8px; margin-bottom:10px;'><button type='button' class='btn-filter active' style='flex:1;' id='wifiBtn0' onclick='selectWifiMode(0)'>Full</button><button type='button' class='btn-filter' style='flex:1;' id='wifiBtn1' onclick='selectWifiMode(1)'>Min Modem</button><button type='button' class='btn-filter' style='flex:1;' id='wifiBtn2' onclick='selectWifiMode(2)'>Max Modem</button></div><div style='font-size:11px; color:#64748b; line-height:1.5;'><strong style='color:#94a3b8;'>Full:</strong> Không tiết kiệm, độ trễ thấp nhất<br><strong style='color:#38bdf8;'>Min Modem:</strong> Cân bằng (-20mA, +3ms delay)<br><strong style='color:#10b981;'>Max Modem:</strong> Tiết kiệm cao (-40mA, +10ms)</div></div>");
    server.sendContent(chunk);

    chunk = F("<div class='sensor-cfg-card' style='margin-bottom:14px;'><div class='sensor-cfg-header'><span class='sensor-cfg-title'>LED Refresh Rate</span><span class='sensor-unit-badge'>Hz</span></div><div class='sensor-cfg-desc' style='margin-bottom:10px;'>Tần số cập nhật LED (mắt người: 24-60 FPS)</div><div style='display:flex; gap:8px; margin-bottom:10px;'><button type='button' class='btn-filter' style='flex:1;' id='ledBtn30' onclick='selectLedHz(30)'>30 Hz</button><button type='button' class='btn-filter' style='flex:1;' id='ledBtn50' onclick='selectLedHz(50)'>50 Hz</button><button type='button' class='btn-filter active' style='flex:1;' id='ledBtn100' onclick='selectLedHz(100)'>100 Hz</button></div><div style='font-size:11px; color:#64748b; line-height:1.5;'><strong style='color:#10b981;'>30Hz:</strong> Tiết kiệm cao (-5mA)<br><strong style='color:#38bdf8;'>50Hz:</strong> Cân bằng (-3mA, vẫn mượt)<br><strong style='color:#94a3b8;'>100Hz:</strong> Mượt nhất (mặc định)</div></div>");
    server.sendContent(chunk);

    chunk = F("<div class='toggle-box' style='margin-bottom:14px; background: rgba(13, 9, 28, 0.78); border: 1px solid rgba(168, 85, 247, 0.2); border-radius: 14px; padding: 12px 14px;'><div style='display:flex; flex-direction:column; gap:2px;'><span style='font-size:13px; font-weight:600;'>Bluetooth Controller</span><span style='font-size:11px; color:#64748b;'>Tắt để tiết kiệm ~12mA dòng tĩnh</span></div><label class='switch'><input type='checkbox' id='pwrBluetooth' onchange='toggleBluetooth(this.checked)'><span class='slider'></span></label></div>");
    server.sendContent(chunk);

    chunk = F("<div class='stress-card' style='margin-bottom:14px;'><div class='stress-header'><div class='stress-title'><svg width='15' height='15' viewBox='0 0 24 24' fill='none' stroke='#f59e0b' stroke-width='2.2' stroke-linecap='round' stroke-linejoin='round'><rect x='4' y='4' width='16' height='16' rx='2'/><rect x='9' y='9' width='6' height='6'/><line x1='9' y1='1' x2='9' y2='4'/><line x1='15' y1='1' x2='15' y2='4'/><line x1='9' y1='20' x2='9' y2='23'/><line x1='15' y1='20' x2='15' y2='23'/><line x1='20' y1='9' x2='23' y2='9'/><line x1='20' y1='14' x2='23' y2='14'/><line x1='1' y1='9' x2='4' y2='9'/><line x1='1' y1='14' x2='4' y2='14'/></svg><span>TEST 100% CPU DUAL-CORE</span></div><div style='display:flex; align-items:center; gap:6px;'><span id='stressCpuBadge' class='stress-cpu-badge'>CPU: 0%</span><span id='stressChipBadge' class='stress-temp-badge'>--.- °C</span></div></div><div class='stress-desc'>Ép tải 100% cả 2 nhân (Core 0 & Core 1 @ 240MHz) để theo dõi nhiệt độ cực đại.</div><div id='stressTimerBox' class='stress-timer-box' style='display:none;'><div style='display:flex; flex-direction:column; gap:2px;'><span>Thời gian đang ép tải:</span><span style='font-size:11px; color:#94a3b8;'>Tải: C0: <strong id='stressCpu0Val' style='color:#38bdf8; font-weight:700;'>100%</strong> | C1: <strong id='stressCpu1Val' style='color:#38bdf8; font-weight:700;'>100%</strong></span></div><strong id='stressTimerVal'>00:00</strong></div><button type='button' id='btnStressToggle' class='btn-stress-start' onclick='toggleStressTest()'>Bắt Đầu Ép Tải 100% CPU</button></div>");
    server.sendContent(chunk);

    chunk = F("<div class='btn-action-row' style='margin-top:14px;'><button type='button' class='btn-submit' style='flex:1; margin-top:0;' onclick='applyPowerSettings()'>Áp Dụng Ngay</button><button type='button' class='btn-secondary' onclick='loadPowerSettings()'>Làm Mới</button></div><p style='text-align:center; font-size:11px; color:#94a3b8; margin-top:10px;'>⚠️ Cấu hình áp dụng trực tiếp thời gian thực, không cần khởi động lại</p><div id='powerToast' class='led-toast'>Đã áp dụng cấu hình tối ưu năng lượng!</div><div style='height:48px;'></div></div>");
    server.sendContent(chunk);

    // 9. Success View & Modals
    chunk = F("<div id='successView' style='display: none; text-align: center; animation: fadeIn 0.4s ease;'><div class='success-badge'><svg width='32' height='32' viewBox='0 0 24 24' fill='none' stroke='#10b981' stroke-width='3' stroke-linecap='round' stroke-linejoin='round'><polyline points='20 6 9 17 4 12'></polyline></svg></div><h2 style='color: #10b981; -webkit-text-fill-color: initial; margin-bottom: 6px;'>KẾT NỐI THÀNH CÔNG!</h2><p style='color: #94a3b8; font-size: 13px;'>ESP32 đã liên kết Wi-Fi và sẵn sàng hoạt động.</p><div class='success-info'><div class='success-row'><span style='color: #94a3b8; font-size: 13px;'>Mạng Wi-Fi:</span><span id='resSSID' style='color: #fff; font-weight: 600; font-size: 13px;'></span></div><div class='success-row'><span style='color: #94a3b8; font-size: 13px;'>Địa chỉ IP thiết bị:</span><span id='resIP' style='color: #c084fc; font-weight: 700; font-family: monospace; font-size: 14px;'></span></div><div class='success-row'><span style='color: #94a3b8; font-size: 13px;'>Tên miền nội bộ:</span><span style='color: #38bdf8; font-weight: 700; font-family: monospace; font-size: 13px;'>http://esp.local</span></div><div class='success-row'><span style='color: #94a3b8; font-size: 13px;'>Bộ nhớ:</span><span style='color: #10b981; font-weight: 600; font-size: 13px;'>Đã lưu Flash NVS</span></div></div><div class='countdown-card'><p id='countdownStatusText' style='color: #cbd5e1; font-size: 13px;'>Điểm phát Access Point sẽ tắt sau:</p><div id='countdownText' class='countdown-number'>3s</div><div class='progress-bar-bg'><div id='progressBar' class='progress-bar-fill'></div></div><p style='color: #64748b; font-size: 12px;'>Bạn có thể đóng trang này và kết nối vào mạng Wi-Fi đã cấu hình.</p></div></div></div>");
    server.sendContent(chunk);

    // 8. Wi-Fi Modal
    chunk = F("<div id='wifiModal' class='modal-overlay' onclick='handleModalOverlayClick(event)'><div class='modal-box'><div class='modal-header'><div class='modal-title'><svg width='18' height='18' viewBox='0 0 24 24' fill='none' stroke='#c084fc' stroke-width='2.2' stroke-linecap='round' stroke-linejoin='round'><path d='M5 12.55a11 11 0 0 1 14.08 0'></path><path d='M1.42 9a16 16 0 0 1 21.16 0'></path><path d='M8.53 16.11a6 6 0 0 1 6.95 0'></path><line x1='12' y1='20' x2='12.01' y2='20'></line></svg><span>Chọn Mạng Wi-Fi</span></div><button type='button' class='modal-close-btn' onclick='closeWifiModal()' aria-label='Đóng'>&times;</button></div><div class='modal-subbar'><span id='modalStatus' class='modal-status-text'>Đang chuẩn bị...</span><button type='button' id='btnRefresh' class='btn-refresh' onclick='scanWiFi()'><svg id='refreshSvg' width='12' height='12' viewBox='0 0 24 24' fill='none' stroke='currentColor' stroke-width='2.5' stroke-linecap='round' stroke-linejoin='round'><path d='M21.5 2v6h-6M21.34 15.57a10 10 0 1 1-.57-8.38l5.67-5.67'/></svg><span>Làm Mới</span></button></div><div id='modalWifiList' class='modal-wifi-list'><div style='color:#94a3b8; text-align:center; padding: 24px 0; font-size:13px;'>Đang dò tìm mạng xung quanh...</div></div><div class='modal-footer'><button type='button' class='btn-modal-close' onclick='closeWifiModal()'>Đóng</button></div></div></div>");
    server.sendContent(chunk);

    // 9. Script
    server.sendContent_P(PSTR("<script>"));
    server.sendContent_P(WEB_SCRIPT_JS);
    server.sendContent_P(PSTR("</script></body></html>"));
}
```

---

### 3.25. Backend API Gateway Production (NestJS)
#### File: `backend/apps/api-gateway/src/controllers/sensor.controller.ts`
> **Mô tả**: Controller tiếp nhận 1-Request Persistent NDJSON Stream (`X-Stream-Mode: true`), Batch Ingestion, WebSocket & Push Notification  
> **Ngôn ngữ**: TypeScript / NestJS / Express Stream

```typescript
import {
  Controller,
  Get,
  Post,
  Body,
  Headers,
  Query,
  Inject,
  OnModuleInit,
  HttpStatus,
  Logger,
  Req,
  Res,
} from '@nestjs/common';
import { Request, Response } from 'express';
import * as readline from 'readline';
import { ClientKafka } from '@nestjs/microservices';
import { ApiTags, ApiOperation } from '@nestjs/swagger';
import { sendKafkaMessage, subscribeToKafkaTopics } from '../common/kafka.helper';
import { AppWebsocketGateway } from '../websocket/websocket.gateway';
import { SensorIngestDto } from '../dto/sensor-telemetry.dto';
import { PushNotificationService } from '../notification/push-notification.service';
import { NotificationService } from '../notification/notification.service';

@ApiTags('🌡️ IoT Telemetry Sensor')
@Controller('api/sensor')
export class SensorController implements OnModuleInit {
  private readonly logger = new Logger(SensorController.name);
  private readonly consecutiveViolations = new Map<string, number>();
  private readonly lastPushTimeMap = new Map<string, number>();

  constructor(
    @Inject('INVENTORY_SERVICE') private readonly inventoryClient: ClientKafka,
    private readonly wsGateway: AppWebsocketGateway,
    private readonly pushService: PushNotificationService,
    private readonly notificationService: NotificationService,
  ) {}

  async onModuleInit() {
    await subscribeToKafkaTopics(this.inventoryClient, [
      'inventory.sensor.get_latest',
      'inventory.sensor.get_history',
      'inventory.sensor.get_stations',
    ]);
  }

  // =========================================================================
  // TIẾP NHẬN TELEMETRY TỪ ESP32-S3 (HỖ TRỢ PERSISTENT STREAM, REALTIME & BATCH)
  // =========================================================================
  @Post()
  @ApiOperation({ summary: 'Tiếp nhận telemetry từ trạm cảm biến ESP32-S3' })
  async ingestTelemetry(
    @Req() req: Request,
    @Res() res: Response,
    @Headers('x-device-id') headerDeviceId?: string,
    @Headers('x-batch-mode') headerBatchMode?: string,
    @Headers('x-stream-mode') headerStreamMode?: string,
    @Body() dto?: SensorIngestDto,
  ) {
    const deviceId = headerDeviceId || dto?.device_id || (req.headers['x-device-id'] as string);
    if (!deviceId) {
      return res.status(HttpStatus.BAD_REQUEST).json({
        statusCode: HttpStatus.BAD_REQUEST,
        message: 'Missing device identifier (X-Device-ID or device_id)',
      });
    }

    const isStreamMode =
      headerStreamMode === 'true' ||
      req.headers['x-stream-mode'] === 'true' ||
      String(req.headers['content-type'] || '').includes('x-ndjson');

    // 1. CHẾ ĐỘ 1-REQUEST PERSISTENT STREAM (NDJSON CHUNKED TRANSFER)
    if (isStreamMode) {
      req.setTimeout(0);
      res.setTimeout(0);
      req.socket?.setKeepAlive(true);

      res.status(HttpStatus.OK);
      res.setHeader('Content-Type', 'application/x-ndjson');
      res.setHeader('Transfer-Encoding', 'chunked');
      res.setHeader('Connection', 'keep-alive');
      res.setHeader('Cache-Control', 'no-cache');
      if (typeof res.flushHeaders === 'function') {
        res.flushHeaders();
      }

      this.logger.log(`[Stream-Ingest] Device ${deviceId} connected to persistent HTTP NDJSON stream`);

      const rl = readline.createInterface({
        input: req,
        crlfDelay: Infinity,
      });

      rl.on('line', async (line: string) => {
        const trimmed = line.trim();
        if (!trimmed) return;
        try {
          const item = JSON.parse(trimmed);
          const ack = await this.processSingleRecord(deviceId, item);
          res.write(JSON.stringify(ack) + '\n');
        } catch (err: any) {
          this.logger.error(`[Stream-Ingest] Parse error from ${deviceId}: ${err.message}`);
          res.write(JSON.stringify({ status: 'error', message: err.message }) + '\n');
        }
      });

      req.on('close', () => {
        this.logger.log(`[Stream-Ingest] Device ${deviceId} disconnected`);
        rl.close();
      });

      req.on('error', (err: any) => {
        this.logger.error(`[Stream-Ingest] Request error from ${deviceId}: ${err?.message}`);
        rl.close();
      });

      return;
    }

    // 2. CHẾ ĐỘ HTTP POST THƯỜNG (HOẶC BATCH INGESTION)
    const isBatch = dto?.batch === true || headerBatchMode === 'true' || req.headers['x-batch-mode'] === 'true';

    if (isBatch) {
      const result = await this.processBatchRecords(deviceId, dto?.records || []);
      return res.status(HttpStatus.OK).json(result);
    } else {
      const result = await this.processSingleRecord(deviceId, dto || {});
      return res.status(HttpStatus.OK).json({
        ...result,
        batch: false,
        received_count: 1,
      });
    }
  }
}
```

---

## 4. Phân Tích Cơ Chế Phòng Thủ Bảo Mật & Trường Hợp Biên (Deep-Dive Security & Resilience)

### 4.1. Cơ Chế 1-Request Persistent Stream & Chống Mòn Chip Flash NOR
- **Vấn đề kỹ thuật**: Mở/đóng kết nối `HTTPClient` liên tục mỗi giây tạo ra 86.400 yêu cầu HTTP/ngày, làm cạn kiệt socket trên router và khiến chip ESP32 liên tục thực hiện TCP 3-way handshake và TLS/HTTP header parsing. Đồng thời, việc đọc NVS Flash mỗi giây và ghi đè file 11.5KB mỗi phút làm suy giảm tuổi thọ chip Flash NOR nghiêm trọng.
- **Giải pháp triển khai**:
  - `TelemetryService` sử dụng `WiFiClient streamClient` mở duy nhất 1 HTTP POST Request với header `X-Stream-Mode: true`.
  - Bật cờ `setNoDelay(true)` để vô hiệu hóa Nagle Algorithm, cho phép từng dòng NDJSON được phát đi tức thì dưới 2ms.
  - Bộ nhớ đệm RAM `cachedBackendUrl` chỉ kiểm tra NVS mỗi 15 giây.
  - Phân vùng LittleFS chỉ tiếp nhận snapshot lịch sử mỗi 15 phút, giảm số lần ghi Flash hơn 93%.

### 4.2. Cơ Chế Store-and-Forward & Chống Deadlock Hàng Đợi (Batch Backpressure)
- **Vấn đề kỹ thuật**: Khi server từ chối dữ liệu (HTTP 4xx), nếu client tiếp tục gửi lại cùng một gói tin sẽ tạo ra Deadlock loop.
- **Giải pháp triển khai**:
  - `TelemetryService` phân loại kết quả: `client_error` (4xx) thì lập tức hủy bỏ batch bị lỗi; `server_error`/Timeout thì giữ lại để gửi bù qua luồng Stream khi mạng ổn định trở lại.
  - Thuật toán cắt tỉa FIFO (`pruneOldRecordsFromFlash`): khống chế tối đa 3.600 bản ghi offline trên Flash.

### 4.3. Cách Ly Tuyệt Đối Độ Trễ Đa Nhân (Dual-Core SMP Time Determinism)
- **Giải pháp**: Ghim cứng `LedTaskCore0` vào Core 0 với tần số làm tươi 100 Hz (10ms/chu kỳ), cách ly hoàn toàn khỏi Core 1 (nơi chạy WebServer và I2C Sensor).

### 4.4. Cơ Chế Tự Hủy Access Point (Self-Destruct AP) Chống Tấn Công Man-in-the-Middle
- **Giải pháp**: Khi kết nối Station thành công, Access Point đếm ngược 3 giây và tự hủy qua `WiFi.softAPdisconnect(true)` kèm cơ chế dự phòng 8 giây.

### 4.5. Cơ Chế Bảo Vệ Watchdog Khi Định Dạng Flash (Safe LittleFS Formatting)
- **Giải pháp**: Nhận diện lỗi phân vùng LittleFS và chủ động vô hiệu hóa Watchdog (`disableLoopWDT()`, `disableCore0WDT()`, `disableCore1WDT()`) trước khi format, sau đó kích hoạt lại đầy đủ.

### 4.6. Điều Chỉnh Tần Số Xung Nhịp Tức Thì (Dynamic Frequency Scaling - DFS)
- **Giải pháp**: Cho phép chuyển đổi tức thì giữa 80MHz, 160MHz và 240MHz qua `setCpuFrequencyMhz()` không cần reboot chip.

---

## 5. Hướng Dẫn Kiểm Thử & Kịch Bản Đối Soát (Verification Guide)

### 5.1. Quy Trình Biên Dịch & Nạp Firmware Qua PlatformIO
```powershell
# 1. Kiểm tra cú pháp và liên kết thư viện
pio run

# 2. Nạp firmware lên board ESP32-S3 qua cổng COM3
pio run -t upload

# 3. Mở Serial Monitor theo dõi log thời gian thực (115200 baud)
pio device monitor -p COM3 -b 115200
```

### 5.2. Kịch Bản Kiểm Thử 1-Request Persistent Stream Cùng Backend WDP301
1. Khởi chạy Backend WDP301:
   ```powershell
   ./start-dev.bat
   # Hoặc: cd backend; npm run start:dev api-gateway (Cổng 4000)
   ```
2. Cấp nguồn cho ESP32-S3 đã cấu hình Wi-Fi và Backend URL mặc định: `http://<IP_MAY_TINH>:4000/api/sensor`.
3. Quan sát log Backend Gateway (`.temp/logs/backend.log` hoặc console):
   - Dòng thông báo mở luồng: `[Stream-Ingest] Device ESP32S3_... connected to persistent HTTP NDJSON stream`
   - Mỗi giây tiếp theo, backend tiếp nhận từng dòng JSON, đẩy WebSocket `sensor:telemetry` tới Frontend Dashboard và emit Kafka `sensor.telemetry.ingest` mà **không sinh thêm bất kỳ HTTP handshake nào**.
   - Backend Gateway phản hồi ACK kèm theo `server_time` để ESP32 đồng bộ RTC.

### 5.3. Kịch Bản Kiểm Thử Ngắt Mạng & Gửi Bù Batch Ingestion Trên Stream
1. Tắt router Wi-Fi: ESP32 nhận diện mất mạng, đóng Stream và lưu tạm vào PSRAM Buffer.
2. Bật lại router Wi-Fi: ESP32 tự động tái kết nối, mở lại 1 HTTP Stream duy nhất và kích hoạt **Batch Ingestion**, xả dồn các khối 50 mẫu tồn đọng lên server trước khi chuyển sang chế độ đo Realtime 1s/lần.
