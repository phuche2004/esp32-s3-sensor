# Power Management System - Hướng Dẫn Sử Dụng

## 🔋 Tổng Quan

Module Power Management cho phép điều chỉnh mức tiêu thụ điện năng của ESP32-S3 thông qua Web UI **KHÔNG CẦN KHỞI ĐỘNG LẠI**.

---

## 📡 API Endpoints

### GET `/api/power`
Lấy cấu hình hiện tại

**Response:**
```json
{
  "cpuFreq": 240,              // 80, 160, or 240 MHz
  "wifiMode": 0,               // 0=Full, 1=Min Modem, 2=Max Modem
  "bluetooth": false,
  "ledHz": 100,                // 30, 50, or 100 Hz
  "estimatedCurrent": 218.0,   // Ước tính dòng điện (mA)
  "estimatedSavings": 0.0,     // Tiết kiệm so với default (mA)
  "actualCpuFreq": 240         // Tần số CPU thực tế
}
```

### POST `/api/power`
Cập nhật cấu hình (áp dụng NGAY LẬP TỨC)

**Parameters:**
- `cpu_freq`: `80` | `160` | `240` (MHz)
- `wifi_mode`: `0` (Full) | `1` (Min Modem) | `2` (Max Modem)
- `bluetooth`: `true` | `false`
- `led_hz`: `30` | `50` | `100` (Hz)

**Example cURL:**
```bash
curl -X POST http://esp.local/api/power \
  -d "cpu_freq=160&wifi_mode=1&bluetooth=false&led_hz=50"
```

---

## ⚙️ Các Tùy Chọn Tối Ưu

### 1. CPU Frequency (Áp dụng ngay lập tức!)
| Tần Số | Dòng Điện | Nhiệt Độ | Use Case |
|--------|-----------|----------|----------|
| **240MHz** | ~100mA | Cao nhất | Default, stress test |
| **160MHz** ⭐ | ~70mA (-30%) | Trung bình | **Khuyến nghị cho IoT** |
| **80MHz** | ~40mA (-60%) | Thấp nhất | Ultra low power, chỉ sensor |

**Lưu ý:** Thay đổi CPU freq **KHÔNG CẦN** khởi động lại, hiệu quả ngay lập tức!

---

### 2. WiFi Power Save Mode
| Mode | Dòng Điện | Độ Trễ | Use Case |
|------|-----------|--------|----------|
| **Full Power** (0) | ~80mA | 0ms | Web UI real-time, low latency |
| **Min Modem** (1) ⭐ | ~60mA (-25%) | +3ms | **Khuyến nghị cho IoT** |
| **Max Modem** (2) | ~40mA (-50%) | +10ms | Battery powered, telemetry only |

---

### 3. Bluetooth
- **Enabled**: +12mA
- **Disabled** ⭐: Tiết kiệm 12mA (Code này không dùng BT)

---

### 4. LED Update Rate
| Tần Số | Dòng Điện | Độ Mượt | Use Case |
|--------|-----------|---------|----------|
| **100Hz** | ~8mA | Mượt nhất | Default, animations |
| **50Hz** ⭐ | ~5mA (-37%) | Vẫn mượt | **Khuyến nghị** |
| **30Hz** | ~3mA (-62%) | Chấp nhận được | Ultra low power |

---

## 🎯 Presets Khuyến Nghị

### Preset 1: **Balanced** (Khuyến nghị) ⭐
```json
{
  "cpu_freq": 160,
  "wifi_mode": 1,
  "bluetooth": false,
  "led_hz": 50
}
```
- Tiết kiệm: **~45mA (25%)**
- Giảm nhiệt: **~2-3°C**
- Hiệu năng: Vẫn rất mượt

---

### Preset 2: **Max Performance**
```json
{
  "cpu_freq": 240,
  "wifi_mode": 0,
  "bluetooth": false,
  "led_hz": 100
}
```
- Dòng điện: **~218mA** (default)
- Dùng cho: Stress test, real-time Web UI

---

### Preset 3: **Ultra Low Power**
```json
{
  "cpu_freq": 80,
  "wifi_mode": 2,
  "bluetooth": false,
  "led_hz": 30
}
```
- Tiết kiệm: **~95mA (52%)**
- Giảm nhiệt: **~4-5°C**
- Dùng cho: Battery powered, chỉ sensor + telemetry

---

## 💻 Ví Dụ Code JavaScript

```javascript
// Lấy cấu hình hiện tại
fetch('/api/power')
  .then(res => res.json())
  .then(data => {
    console.log('CPU:', data.cpuFreq, 'MHz');
    console.log('Estimated current:', data.estimatedCurrent, 'mA');
  });

// Áp dụng Balanced preset
fetch('/api/power', {
  method: 'POST',
  headers: {'Content-Type': 'application/x-www-form-urlencoded'},
  body: new URLSearchParams({
    cpu_freq: 160,
    wifi_mode: 1,
    led_hz: 50
  })
})
.then(res => res.json())
.then(data => console.log(data.message));
```

---

## 📊 Bảng So Sánh Tổng Hợp

| Config | CPU | WiFi | LED | Total (mA) | Savings | Temp Δ |
|--------|-----|------|-----|------------|---------|--------|
| **Default** | 240MHz | Full | 100Hz | ~218mA | 0mA | +0°C |
| **Balanced** ⭐ | 160MHz | Min | 50Hz | ~173mA | **-45mA** | **-2.5°C** |
| **Ultra** | 80MHz | Max | 30Hz | ~123mA | **-95mA** | **-4.5°C** |

---

## ⚠️ Lưu Ý Quan Trọng

1. **CPU Frequency**: Thay đổi NGAY LẬP TỨC, không cần reboot
2. **WiFi Mode**: Áp dụng ngay, có thể gây flicker Web UI 1-2s
3. **LED Hz**: Cần update code `vTaskDelay()` trong `LedTaskCore0` để thay đổi (TODO)
4. **Không lưu vào NVS**: Sau khi reboot sẽ về default (240MHz, Full WiFi)

---

## 🚀 Roadmap

- [ ] Lưu config vào NVS Flash
- [ ] Dynamic LED Hz (update `vTaskDelay` real-time)
- [ ] Light Sleep mode giữa các chu kỳ sensor
- [ ] Web UI tab "⚡ Tối Ưu" với sliders
- [ ] Real-time power consumption chart

---

## 🔗 Tài Liệu Tham Khảo

- [ESP32-S3 Power Management](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-reference/system/power_management.html)
- [WiFi Power Save](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/wifi.html#esp32-wi-fi-power-saving-mode)
