# KIẾN TRÚC CI/CD CHO THIẾT BỊ IOT ESP32-S3

Tài liệu thiết kế quy trình Tích hợp liên tục (CI) và Triển khai liên tục (CD) tự động chuẩn công nghiệp cho thiết bị giám sát nhiệt độ/độ ẩm ESP32-S3.

---

## 1. TỔNG QUAN LUỒNG HOẠT ĐỘNG (PIPELINE OVERVIEW)

```
[Developer]
    │
    ▼ git push / merge to main
[GitHub Actions CI]
    ├── 1. Code Lint & Static Check
    ├── 2. PlatformIO Build (esp32s3_n16r8)
    ├── 3. Kiểm tra ngưỡng tràn RAM/Flash
    └── 4. Sinh file firmware.bin + SHA-256
    │
    ▼ Release Artifact
[Firmware Registry / Backend Server]
    │
    ├── Kênh 1: PUSH (Khẩn cấp qua MQTT/WebSocket)
    └── Kênh 2: PULL (Định kỳ / Auto Polling khi Boot)
    │
    ▼ Over-The-Air (OTA)
[ESP32-S3 Device]
    ├── Ghi vào phân vùng OTA phụ (ota_1)
    ├── Kiểm tra toàn vẹn SHA-256
    ├── Khởi động lại & Tự kiểm tra (Self-Test)
    └── Tự động Rollback nếu gặp lỗi (Anti-Brick)
```

---

## 2. CONTINUOUS INTEGRATION (CI)

Chạy tự động trên **GitHub Actions** mỗi khi có Pull Request hoặc push code vào nhánh `main`.

### Các bước trong Pipeline CI:
1. **Kiểm tra môi trường & phụ thuộc**:
   - Cài đặt Python và PlatformIO Core CLI.
   - Tự động tải các thư viện theo cấu hình trong `platformio.ini`.
2. **Biên dịch tự động (Automated Build)**:
   - Chạy lệnh `pio run -e esp32s3_n16r8`.
   - Nếu phát sinh lỗi biên dịch (cú pháp, thiếu thư viện, xung đột header), block merge ngay lập tức.
3. **Phân tích tài nguyên bộ nhớ (Memory Footprint Gate)**:
   - Kiểm tra dung lượng Flash: Cảnh báo nếu > 85% phân vùng app.
   - Kiểm tra dung lượng RAM tĩnh: Cảnh báo nếu Heap khởi điểm < 100 KB.
4. **Đóng gói Artifact**:
   - Xuất file nhị phân: `firmware.bin`.
   - Tính toán mã băm kiểm tra: `SHA-256`.
   - Lưu trữ artifact hoặc tạo GitHub Release theo git tag (VD: `v1.2.0`).

---

## 3. CONTINUOUS DELIVERY & DEPLOYMENT (CD)

Thiết bị IoT là phần cứng vật lý từ xa, do đó việc nạp firmware phải thông qua cơ chế **OTA (Over-The-Air Update)** với 2 phương thức kích hoạt:

### 3.1. Phương thức PULL (Mặc định - An toàn, Tiết kiệm)
- **Cơ chế**: ESP32 chủ động định kỳ gửi request kiểm tra phiên bản mới.
- **Chu kỳ**:
  - Khi thiết bị vừa khởi động và kết nối Wi-Fi thành công.
  - Định kỳ mỗi 6 giờ hoặc vào khung giờ thấp điểm (ví dụ 02:00 - 04:00 sáng).
- **API tương tác**:
  - `GET /api/firmware/latest`
  - Response:
    ```json
    {
      "version": "1.2.0",
      "url": "https://backend.example.com/firmware/esp32_v1.2.0.bin",
      "sha256": "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855",
      "mandatory": false
    }
    ```

### 3.2. Phương thức PUSH (Cập nhật khẩn cấp)
- **Cơ chế**: Sử dụng kết nối hai chiều (MQTT hoặc WebSocket).
- **Áp dụng khi**: Có bản vá bảo mật nghiêm trọng hoặc cần cập nhật tức thời theo lệnh của người quản trị.
- **Payload qua MQTT** (Topic: `device/{mac_address}/cmd`):
  ```json
  {
    "action": "ota_update",
    "version": "1.2.1",
    "url": "https://backend.example.com/firmware/esp32_v1.2.1.bin",
    "sha256": "..."
  }
  ```

---

## 4. TIÊU CHUẨN AN TOÀN TRÁNH BIẾN MẠCH THÀNH CỤC GẠCH (ANTI-BRICKING)

Đây là yêu cầu bắt buộc của mọi hệ thống IoT công nghiệp:

1. **Phân vùng kép (Dual OTA Partition Table)**:
   - Bộ nhớ Flash 16MB được chia thành 2 phân vùng chạy song song: `ota_0` và `ota_1`.
   - Khi cập nhật, code mới sẽ được ghi vào phân vùng dự phòng. Phân vùng đang chạy vẫn hoạt động bình thường cho đến khi quá trình nạp kết thúc.
2. **Xác thực toàn vẹn (Integrity Verification)**:
   - Trước khi nạp, thiết bị tính mã SHA-256 của file tải về và đối chiếu với giá trị server cung cấp. Nếu sai lệch dù chỉ 1 bit, hủy bỏ cập nhật ngay lập tức.
3. **Cơ chế tự động Rollback (OTA Self-Test & Rollback)**:
   - Sau khi nạp firmware mới và reboot, ESP32 phải chạy chế độ thử nghiệm (Testing State).
   - Thiết bị phải kết nối được Wi-Fi và đọc thành công cảm biến SHT31 trong vòng 60 giây.
   - Nếu thành công: Gọi lệnh `esp_ota_mark_app_valid_cancel_rollback()` để xác nhận phiên bản mới ổn định.
   - Nếu gặp lỗi Crash/Panic Loop hoặc không kết nối được mạng trong 60 giây: Chip Watchdog sẽ tự động kích hoạt khởi động lại và rollback về phân vùng firmware cũ hoạt động bình thường.
4. **Triển khai theo giai đoạn (Staged / Canary Rollout)**:
   - Không nâng cấp đồng loạt 100% thiết bị cùng lúc.
   - Triển khai thử nghiệm 5% số lượng thiết bị trước -> Theo dõi 24 giờ -> Mở rộng 25% -> Triển khai 100%.
