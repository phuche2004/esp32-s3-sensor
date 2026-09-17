# ESP32-S3 IoT Environment Monitor & LED Controller

Tài liệu kỹ thuật hệ thống nhúng giám sát môi trường, điều khiển quang học đa nhân và kiểm thử hiệu năng trên nền vi điều khiển ESP32-S3.

---

## 1. Quick Reference: Các Lệnh PlatformIO Thường Dùng

Tất cả các lệnh thực thi trong Terminal tại thư mục gốc của dự án (`nhiet_do`):

```powershell
# 1. Biên dịch kiểm tra cú pháp và liên kết thư viện (Build)
pio run

# 2. Biên dịch và nạp firmware qua cổng COM3 (Upload)
pio run -t upload

# 3. Mở Serial Monitor theo dõi log (115200 baud)
pio device monitor -p COM3 -b 115200

# 4. Lệnh kết hợp: Nạp firmware xong tự động mở Serial Monitor
pio run -t upload -t monitor

# 5. Xóa toàn bộ file build trung gian để build lại từ đầu (Clean)
pio run -t clean

# 6. Liệt kê danh sách các cổng COM đang kết nối
pio device list
```

*Lưu ý: Nhấn tổ hợp phím `Ctrl + C` để đóng Serial Monitor trước khi thực hiện lệnh nạp firmware tiếp theo, tránh xung đột quyền truy cập cổng COM.*

---

## 2. Phần Cứng & Sơ Đồ Chân (Hardware Specs & Pinout)

- **Vi điều khiển**: ESP32-S3 (Xtensa Dual-Core 32-bit LX7 @ 240MHz, 16MB Flash QIO, 8MB PSRAM OPI).
- **Cảm biến môi trường**: Sensirion SHT31-DIS (Độ phân giải: 0.01°C, 0.01% RH; Sai số: ±0.2°C, ±2% RH).
- **Cơ cấu chấp hành**: 1x LED RGB WS2812 (tích hợp trên bo mạch), 1x Active Buzzer, 1x Status LED, 1x Phím nhấn BOOT.

| Chân GPIO | Thiết Bị Ngoại Vi | Giao Thức / Chế Độ Hoạt Động | Mô Tả Kỹ Thuật |
| :--- | :--- | :--- | :--- |
| `GPIO 8` | SHT31 SDA | I2C Fast Mode (400 kHz) | Tuyến dữ liệu I2C |
| `GPIO 9` | SHT31 SCL | I2C Fast Mode (400 kHz) | Tuyến xung nhịp I2C |
| `GPIO 10` | Active Buzzer | Digital Output | Còi báo động khi vượt ngưỡng đo |
| `GPIO 13` | Status LED | Digital Output | Đèn báo trạng thái hệ thống |
| `GPIO 48` | WS2812 RGB LED | RMT Peripheral | Bộ phát xung RMT, dữ liệu 24-bit GRB |
| `GPIO 0` | Phím BOOT | Input Pull-up | Nhấn giữ > 1s để xóa cấu hình Wi-Fi NVS |

---

## 3. Kiến Trúc Đa Nhiệm FreeRTOS (Dual-Core SMP)

Hệ thống phân tách tải xử lý độc lập giữa hai nhân phần cứng nhằm đảm bảo tính tiền định (deterministic timing) cho tín hiệu quang học và giao diện mạng:

```
                    ESP32-S3 (2 Cores @ 240MHz)
            ┌─────────────────────┬─────────────────────┐
            │       Core 0        │       Core 1        │
            ├─────────────────────┼─────────────────────┤
            │ • LedTaskCore0      │ • Arduino loopTask  │
            │   (100 Hz, chu kỳ   │ • WebServer         │
            │    10ms cố định)    │   (HTTP Port 80)    │
            │ • Bảng tra CIE 1931 │ • DNSServer         │
            │   Gamma 2.8 LUT     │   (Captive Portal)  │
            │ • Wi-Fi Driver MAC  │ • SHT31 I2C Driver  │
            │ • Idle Hook Core 0  │ • Telemetry Client  │
            │                     │ • Idle Hook Core 1  │
            └─────────────────────┴─────────────────────┘
```

- **Core 0**:
  - `LedTaskCore0` (Priority 1): Cập nhật trạng thái màu sắc chu kỳ cố định 10ms (**tần số làm tươi 100 Hz**), hiệu chỉnh phi tuyến Gamma 2.8. Tách biệt hoàn toàn khỏi Core 1, loại bỏ hiện tượng giật nhịp quang học khi có tải mạng.
  - Wi-Fi Driver Task của ESP-IDF (Priority 23): Xử lý lớp MAC và quản lý khung phát sóng Beacon.
  - Idle Hook Core 0: Đếm chu kỳ nhàn rỗi phục vụ tính toán mức tải % CPU Core 0.
- **Core 1**:
  - `loopTask` (Priority 1): Điều phối máy chủ HTTP, DNS Server phân giải tên miền Captive Portal, lấy mẫu cảm biến I2C non-blocking và truyền dữ liệu telemetry.
  - Idle Hook Core 1: Đếm chu kỳ nhàn rỗi phục vụ tính toán mức tải % CPU Core 1.

---

## 4. Các Module Chức Năng

### 4.1. Cảm Biến SHT31 & Giám Sát Cảnh Báo
- Giao tiếp I2C 400 kHz, xác minh tính toàn vẹn dữ liệu phần cứng bằng mã kiểm tra CRC-8.
- Xử lý ngưỡng cảnh báo độc lập: Nhiệt độ (20°C - 80°C, mặc định 37.0°C) và Độ ẩm (30% - 99%, mặc định 87.0%).
- Cơ chế phản hồi khi vượt ngưỡng: Kích hoạt còi buzzer ngắt quãng, chuyển LED sang chớp đỏ khẩn cấp, bật cờ `alert: true` trong gói JSON.
- Cấu hình linh hoạt: Cho phép thay đổi chu kỳ đọc cảm biến (1 - 60s) và chu kỳ phát telemetry (2 - 300s), lưu trữ trực tiếp vào Flash NVS.

### 4.2. Bộ Điều Khiển LED WS2812 (RMT)
- 6 chế độ hoạt động:
  - `0`: Tắt LED (tiết kiệm điện năng).
  - `1`: Màu tĩnh (Static Color).
  - `2`: Nhịp thở (Breathing) - Tần số làm tươi 100 Hz (Mặc định: Tím Cyber `#A855F7`).
  - `3`: Cầu vồng quang phổ (Rainbow 256 bước).
  - `4`: Strobe vũ trường (Bar Club EDM 135 BPM).
  - `5`: Biểu thị mức nhiệt (Temp Reactive: Xanh -> Vàng -> Đỏ).
- Hiệu chỉnh màu sắc: Sử dụng bảng tra cứu 256 phần tử Gamma 2.8 theo không gian thị giác chuẩn CIE 1931.

### 4.3. Captive Portal & Web UI
- Cơ chế mạng: SoftAP (SSID: `ESP32-S3`, Open) hoạt động song song Station (hỗ trợ WPA2-Personal và WPA2-Enterprise 802.1x EAP-PEAP).
- Phân giải tên miền: Tích hợp mDNS Responder `http://esp.local`.
- Tối ưu hóa Web di động:
  - Bánh xe màu 260px sử dụng `transform: translate3d` và `requestAnimationFrame`, đồng bộ **60 - 120 FPS** theo tần số quét màn hình client (chuẩn màn hình thường và ProMotion / AMOLED 120Hz).
  - Phân tách vùng cảm ứng hình học: Bắt sự kiện chọn màu trong bán kính `r <= R_wheel`; khu vực ngoài hình tròn tự do cuộn trang (CSS `touch-action: pan-y`).

### 4.4. Module Kiểm Thử Ép Tải 100% CPU Dual-Core & Giám Sát Phần Cứng
- **Cơ chế ép tải**: Kích hoạt 2 FreeRTOS Task tính toán dấu phẩy động (`sinf`, `cosf`, `sqrtf`) ghim đồng thời trên Core 0 và Core 1.
- **An toàn hệ thống**: Nhả 1 tick `vTaskDelay(1)` sau mỗi 40.000 phép tính để Task Watchdog Timer (TWDT) không bị ngắt, duy trì tải thực tế ~99.9% không gây crash/reset chip.
- **Bộ đếm thời gian xuôi**: Đếm thời gian thực thi tăng dần (`00:00 -> MM:SS`), cho phép bắt đầu/dừng thủ công.
- **Giám sát tải CPU thực tế**: Sử dụng FreeRTOS Idle Hook trên cả Core 0 và Core 1 kèm bộ lọc trung bình động EMA:
  $$\text{CPU Load} = \left(1.0 - \frac{\text{Idle Count}}{\text{Max Idle Count}}\right) \times 100\%$$
- **Giám sát nhiệt độ bán dẫn**: Đọc trực tiếp cảm biến nhiệt độ tích hợp trong die silicon ESP32-S3 qua hàm `temperatureRead()`.

---

## 5. Danh Sách REST API Endpoints

Máy chủ Web lắng nghe trên cổng 80, hỗ trợ các endpoint sau:

| Endpoint | Phương Thức | Tham Số Truyền Vào | Chức Năng / Định Dạng Dữ Liệu |
| :--- | :--- | :--- | :--- |
| `/` | `GET` | - | Trả về giao diện Web Captive Portal (Chunked streaming) |
| `/scan` | `GET` | - | Dò quét Wi-Fi lân cận, trả về JSON: `[{ssid, rssi}]` |
| `/save` | `POST` | `ssid, password, is_ent, username, backend` | Lưu thông số Wi-Fi vào NVS và kết nối Station |
| `/status` | `GET` | - | Trạng thái kết nối: `{"state":"...", "ip":"...", "error":"..."}` |
| `/sensor` | `GET` | - | Dữ liệu thời gian thực: `{temp, hum, valid, alert, chipTemp, cpu, cpu0, cpu1, stressActive, stressSec}` |
| `/api/sensor-cfg` | `GET` | - | Đọc thông số: `{tempAlert, humAlert, readInterval, sendInterval}` |
| `/api/sensor-cfg` | `POST` | `temp_alert, hum_alert, read_interval, send_interval, reset` | Cập nhật ngưỡng và chu kỳ vào Flash NVS |
| `/api/led` | `GET` | - | Đọc cấu hình LED: `{mode, r, g, b, brightness, speed}` |
| `/api/led` | `POST` | `mode, r, g, b, brightness, speed` | Áp dụng cấu hình LED và ghi NVS |
| `/api/led/reset` | `POST` | - | Đưa cấu hình LED về mặc định (Nhịp thở Tím) |
| `/api/stress-test` | `POST` | `action=start` hoặc `action=stop` | Kích hoạt hoặc dừng ép tải 100% CPU Dual-Core |
| `/api/stress-test` | `GET` | - | Trạng thái test: `{running, elapsedSec, chipTemp, cpu, cpu0, cpu1}` |
| `/shutdown-ap` | `POST` | - | Tắt điểm phát Access Point, giữ kết nối Station |

---

## 6. Dịch Vụ Telemetry Backend

- Giao thức truyền: HTTP POST định kỳ (Content-Type: `application/json`).
- Cấu trúc bản tin Telemetry:
  ```json
  {
    "temperature": 34.21,
    "humidity": 64.75,
    "chip_temperature": 44.5,
    "cpu_load": 2.1,
    "alert": false,
    "timestamp": 123456
  }
  ```
- Script giả lập backend server (`server.py`) trên cổng 3000:
  ```powershell
  python server.py
  ```
