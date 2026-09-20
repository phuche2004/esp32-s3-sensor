# ĐẶC TẢ KỸ THUẬT API TELEMETRY (ESP32-S3 -> WEB BACKEND)
### HỆ THỐNG QUAN TRẮC MÔI TRƯỜNG & HIỆU NĂNG PHẦN CỨNG WDP301
**Phiên bản tài liệu:** 2.1  
**Ngày cập nhật:** 20/09/2026  
**Áp dụng cho:** Firmware ESP32-S3 Telemetry v2.1 & Web Backend Server (Node.js / Python / Go)  
**Trạng thái:** CHUẨN XÁC MINH SẢN XUẤT (PRODUCTION READY)

---

## 1. Tổng Quan Kiến Trúc & Kết Nối

### 1.1. Mô hình truyền thông
- **Mô hình kết nối**: HTTP REST Webhook (Client-Initiated Push).
- **Thiết bị trạm đo**: ESP32-S3 Dual-Core Xtensa LX7 @ 240MHz (Tích hợp 8MB PSRAM, 16MB Flash SPI, cảm biến độ chính xác cao Sensirion SHT31).
- **Kiến trúc luồng xử lý phi chặn (Non-blocking Dual-Core)**:
  - **Core 1 (App Core)**: Đọc cảm biến SHT31, tính toán nhiệt động lực học (VPD, Điểm sương), giám sát ngưỡng cảnh báo và đẩy struct dữ liệu vào FreeRTOS Queue (< 10µs).
  - **Core 0 (Network Core)**: Task ngầm `TelemetryTask` (Stack 8192 bytes, Priority 1) rút dữ liệu từ Queue, quản lý bộ đệm PSRAM, giao tiếp mạng HTTPClient / TLS và dọn dẹp bộ nhớ Flash.
- **Tần suất gửi mặc định**: **1 giây / lần (1 Hz)** (có thể tinh chỉnh từ 1s đến 300s qua Web Portal).

### 1.2. Cơ chế lưu trữ đệm PSRAM & Chống mòn Flash (Store-and-Forward)
1. **Giai đoạn đệm PSRAM (Chống mòn chip Flash)**:
   - Khi mất kết nối Wi-Fi hoặc server từ chối kết nối tạm thời, firmware lưu dữ liệu trực tiếp vào bộ đệm trên 8MB PSRAM (`psramBuffer`, sức chứa 60 bản ghi).
   - Chỉ khi gom đủ **60 bản ghi** (tương đương 1 phút mất mạng), firmware mới mở Flash LittleFS để ghi 1 lần. Cơ chế này giảm hơn 98% áp lực ghi/xóa lên chip Flash NOR.
2. **Giai đoạn lưu trữ bền vững Flash LittleFS**:
   - Khi mất mạng kéo dài (> 1 phút), dữ liệu được bảo vệ an toàn trong phân vùng Flash LittleFS. Mất nguồn điện hoặc khởi động lại chip, dữ liệu vẫn còn nguyên vẹn 100%.
   - Hệ thống tự động duy trì cơ chế **FIFO (First In First Out)** khống chế tối đa 3,600 mẫu đo gần nhất (~172 KB Flash), không bao giờ gây phình dung lượng bộ nhớ.
3. **Giai đoạn gửi bù (Batch Ingestion)**:
   - Khi kết nối mạng phục hồi, ESP32 tự động kích hoạt chế độ **Batch Ingestion**, đóng gói các mẫu đo tồn đọng thành mảng JSON (`batch: true`) và gửi theo khối (tối đa 50 - 100 mẫu/khối) lên Backend để xả sạch dữ liệu trong tích tắc.

### 1.3. Cơ chế kết nối & Thời gian chờ (Timeout)
- **Giao thức**: HTTP/1.1 Persistent Connection (`Connection: keep-alive`).
- **Thời gian chờ phản hồi từ Server (HTTP Timeout)**:
  - Gói đơn lẻ Realtime (`batch: false`): **2000 ms**.
  - Gói gửi bù Batch Ingestion (`batch: true`): **4000 ms** (đảm bảo đủ thời gian cho Backend phân tích mảng và thực thi Bulk Insert xuống Database).

---

## 2. Thông Số Kỹ Thuật HTTP Request

### 2.1. Endpoint & Giao thức

| Thuộc tính | Giá trị quy chuẩn | Ghi chú |
| :--- | :--- | :--- |
| **HTTP Method** | `POST` | Bắt buộc |
| **URL Path** | `/api/sensor` | Đường dẫn mặc định, có thể cấu hình linh hoạt trên ESP32 |
| **Giao thức** | HTTP/1.1 hoặc HTTPS (TLS 1.2/1.3) | Hỗ trợ Cloudflare Tunnel / SSL Reverse Proxy |
| **Payload Format** | JSON UTF-8 (`application/json`) | Nén chuẩn, không chứa ký tự thừa |

### 2.2. HTTP Request Headers

| Tên Header | Giá trị mẫu | Bắt buộc | Ý nghĩa kỹ thuật |
| :--- | :--- | :---: | :--- |
| `Content-Type` | `application/json` | Có | Định dạng dữ liệu body |
| `User-Agent` | `ESP32-S3-Sensor/2.1` | Có | Nhận diện phiên bản firmware của thiết bị |
| `X-Device-ID` | `ESP32S3_404CCA44C814` | Có | Mã định danh phần cứng duy nhất (dựa trên MAC Address của chip ESP32-S3) |
| `X-Batch-Mode` | `true` hoặc `false` | Có | `true`: Gói gửi bù dữ liệu lưu đệm; `false`: Gói thời gian thực |
| `X-Batch-Count` | `50` | Tùy chọn | Số lượng bản ghi trong mảng `records` (chỉ có khi `X-Batch-Mode: true`) |
| `X-Packet-Seq` | `1052` | Tùy chọn | Số thứ tự gói tin tự tăng (chỉ có khi `X-Batch-Mode: false`) |

> **Quy định xác thực bảo mật (Authentication):**  
> Thiết bị được định danh độc nhất qua Header `X-Device-ID`. Backend xác thực và phân quyền trạm đo dựa trên danh sách trắng (`Whitelist Device ID`) đã đăng ký trong cơ sở dữ liệu kết hợp với lớp bảo mật mạng (HTTPS / Cloudflare Tunnel / Firewall IP). Hệ thống không yêu cầu nhập chuỗi API Key tĩnh thủ công tại trạm để bảo đảm tính cắm-là-chạy (Plug-and-Play) khi lắp đặt.

---

## 3. Cấu Trúc Dữ Liệu Gửi Đi (JSON Payload)

Backend sẽ tiếp nhận 2 định dạng bản tin dựa trên trạng thái kết nối mạng:

---

### 3.1. Định dạng 1: Bản tin thời gian thực đơn lẻ (`batch: false`)
Gửi đều đặn mỗi **1 giây** khi kết nối mạng ổn định.

```json
{
  "device_id": "ESP32S3_404CCA44C814",
  "batch": false,
  "count": 1,
  "timestamp": 1726733520,
  "seq": 1052,
  "metrics": {
    "temperature": 28.45,
    "humidity": 65.20,
    "dew_point": 21.32,
    "vpd": 1.15
  },
  "diagnostics": {
    "chip_temp": 42.5,
    "cpu_load": 2.8,
    "cpu0": 2.1,
    "cpu1": 3.5,
    "free_heap": 184320,
    "uptime_sec": 1250,
    "wifi_rssi": -58
  },
  "status": {
    "alert": false,
    "sensor_valid": true
  }
}
```

---

### 3.2. Định dạng 2: Bản tin gửi bù theo khối (`batch: true`) - BATCH INGESTION
Gửi khi thiết bị vừa khôi phục kết nối Wi-Fi / Internet, mang theo toàn bộ dữ liệu đã được lưu trữ an toàn trong bộ nhớ đệm PSRAM và Flash LittleFS.

```json
{
  "device_id": "ESP32S3_404CCA44C814",
  "batch": true,
  "count": 3,
  "records": [
    {
      "timestamp": 1726733500,
      "seq": 101,
      "metrics": {
        "temperature": 28.30,
        "humidity": 65.10,
        "dew_point": 21.15,
        "vpd": 1.14
      },
      "diagnostics": {
        "chip_temp": 42.1,
        "cpu_load": 2.5,
        "cpu0": 2.0,
        "cpu1": 3.0,
        "free_heap": 185000,
        "uptime_sec": 1230,
        "wifi_rssi": 0
      },
      "status": {
        "alert": false,
        "sensor_valid": true
      }
    },
    {
      "timestamp": 1726733501,
      "seq": 102,
      "metrics": {
        "temperature": 28.35,
        "humidity": 65.12,
        "dew_point": 21.19,
        "vpd": 1.14
      },
      "diagnostics": {
        "chip_temp": 42.2,
        "cpu_load": 2.6,
        "cpu0": 2.0,
        "cpu1": 3.2,
        "free_heap": 184900,
        "uptime_sec": 1231,
        "wifi_rssi": 0
      },
      "status": {
        "alert": false,
        "sensor_valid": true
      }
    },
    {
      "timestamp": 1726733502,
      "seq": 103,
      "metrics": {
        "temperature": 28.40,
        "humidity": 65.15,
        "dew_point": 21.25,
        "vpd": 1.15
      },
      "diagnostics": {
        "chip_temp": 42.3,
        "cpu_load": 2.7,
        "cpu0": 2.1,
        "cpu1": 3.3,
        "free_heap": 184800,
        "uptime_sec": 1232,
        "wifi_rssi": 0
      },
      "status": {
        "alert": false,
        "sensor_valid": true
      }
    }
  ]
}
```

---

## 4. Bảng Đặc Tả Chi Tiết Các Trường Dữ Liệu (Data Dictionary)

| Tên trường | Cấp dữ liệu | Kiểu dữ liệu | Đơn vị / Định dạng | Ý nghĩa kỹ thuật | Quy định xử lý phía Backend |
| :--- | :--- | :--- | :--- | :--- | :--- |
| `device_id` | Root | `string` | Text (`ESP32S3_xxxx`) | Định danh phần cứng thiết bị | Dùng làm khóa ngoại (`foreign key`) tra cứu trạm đo trong DB |
| `batch` | Root | `boolean` | `true` / `false` | Cờ nhận diện chế độ gửi | `false`: Realtime; `true`: Batch Ingestion gửi bù |
| `count` | Root | `integer` | Số nguyên >= 1 | Số bản ghi trong gói tin | Realtime luôn = 1; Batch = số phần tử mảng `records` |
| `timestamp` | Record | `integer` (uint32) | Unix Epoch (Giây) | **Thời điểm lấy mẫu cảm biến** | **QUY TẮC BẮT BUỘC:**<br>1. Nếu `timestamp > 1700000000`: Lưu đúng mốc thời gian này vào DB.<br>2. Nếu `timestamp <= 0` (thiết bị mới boot chưa có giờ): **Backend tự gán `Date.now() / 1000`** của server. |
| `seq` | Record | `integer` (uint32) | Số nguyên tự tăng | Số thứ tự mẫu đo | Dùng kiểm tra mất gói hoặc tính toán độ ổn định truyền thông |
| `metrics.temperature` | Record | `float` | °C (Ví dụ: `28.45`) | Nhiệt độ môi trường | Đọc từ SHT31 qua I2C (độ chính xác cao ±0.2°C) |
| `metrics.humidity` | Record | `float` | %RH (Ví dụ: `65.20`) | Độ ẩm không khí | Đọc từ SHT31 qua I2C (độ chính xác cao ±2%RH) |
| `metrics.dew_point` | Record | `float` | °C (Ví dụ: `21.32`) | Điểm sương (Dew Point) | Tính toán theo công thức Magnus-Tetens |
| `metrics.vpd` | Record | `float` | kPa (Ví dụ: `1.15`) | Độ hụt áp suất hơi nước | Vapor Pressure Deficit (quan trọng cho bảo quản dược phẩm) |
| `diagnostics.chip_temp` | Record | `float` | °C (Ví dụ: `42.5`) | Nhiệt độ lõi vi điều khiển | Giám sát tình trạng quá nhiệt phần cứng ESP32-S3 |
| `diagnostics.cpu_load` | Record | `float` | % (Ví dụ: `2.8`) | Tải CPU tổng hợp | Tỷ lệ sử dụng CPU thực tế (0.0% - 100.0%) |
| `diagnostics.cpu0` | Record | `float` | % (Ví dụ: `2.1`) | Tải Core 0 | Mức sử dụng của nhân mạng Wi-Fi/Telemetry |
| `diagnostics.cpu1` | Record | `float` | % (Ví dụ: `3.5`) | Tải Core 1 | Mức sử dụng của nhân đo lường cảm biến/Logic |
| `diagnostics.free_heap` | Record | `integer` | Bytes (Ví dụ: `184320`) | Bộ nhớ RAM khả dụng | Giám sát rò rỉ bộ nhớ (Memory Leak) |
| `diagnostics.uptime_sec` | Record | `integer` | Giây | Thời gian chạy liên tục | Uptime kể từ thời điểm bật nguồn thiết bị |
| `diagnostics.wifi_rssi` | Record | `integer` | dBm (Ví dụ: `-58`) | Cường độ tín hiệu Wi-Fi | `-30` đến `-60`: Rất tốt; `-75` đến `-90`: Yếu |
| `status.alert` | Record | `boolean` | `true` / `false` | Cờ cảnh báo vượt ngưỡng | `true`: Nhiệt độ hoặc độ ẩm vượt ngưỡng cấu hình |
| `status.sensor_valid` | Record | `boolean` | `true` / `false` | Trạng thái cảm biến | `false`: Cảm biến SHT31 bị lỏng dây hoặc lỗi I2C |

---

## 5. Quy Chuẩn HTTP Response & Hợp Đồng Xử Lý Lỗi (Critical Response Contract)

Để firmware ESP32-S3 hoạt động ổn định 24/7, an toàn cho bộ nhớ Flash và không gây nghẽn mạng, Backend **BẮT BUỘC** tuân thủ các quy định phân loại mã HTTP dưới đây:

### 5.1. Bảng phân loại HTTP Status Code phía Backend

| Mã HTTP Response | Bản chất | Hành vi xử lý của ESP32 Firmware | Yêu cầu đối với Backend |
| :---: | :--- | :--- | :--- |
| **`200 OK`**<br>`201 Created`<br>`204 No Content` | **Thành công** | Ghi nhận hoàn tất.<br>Nếu là gói Batch: **Xóa ngay batch vừa gửi khỏi Flash LittleFS** để giải phóng bộ nhớ. | Trả về nhanh chóng kèm theo trường `"server_time"` trong JSON body. |
| **`400 Bad Request`**<br>`401 Unauthorized`<br>`403 Forbidden`<br>`413 Payload Too Large`<br>`422 Unprocessable` | **Lỗi Client / Lỗi Dữ Liệu Vĩnh Viễn** | Firmware xác định gói dữ liệu bị sai hoặc backend từ chối vĩnh viễn.<br>**LẬP TỨC HỦY BỎ (DROP) BATCH VÀ XÓA KHỎI FLASH** để chống deadlock loop, in log cảnh báo Serial. | Khi payload sai schema, backend chủ động trả về mã `4xx`. Không trả về `500` cho lỗi do payload của client. |
| **`500 Internal Error`**<br>`502 Bad Gateway`<br>`503 Service Unavailable`<br>`504 Gateway Timeout`<br>hoặc **Network Timeout** | **Lỗi Tạm Thời (Transient Error)** | Firmware xác định máy chủ đang bảo trì hoặc mạng chập chờn.<br>**GIỮ NGUYÊN DỮ LIỆU TRONG FLASH** để tiếp tục thử lại ở chu kỳ tiếp theo. | Sử dụng khi database tạm thời bị khóa hoặc backend đang khởi động lại. |

---

### 5.2. Cấu trúc JSON Response Body chuẩn (Kèm đồng bộ RTC)

Backend trả về JSON phản hồi chuẩn như sau:

```json
{
  "status": "ok",
  "received_count": 50,
  "server_time": 1726733521
}
```

> **CƠ CHẾ ĐỒNG BỘ RTC NGƯỢC (TỰ ĐỘNG KHẮC PHỤC LỖI CỔNG NTP BỊ CHẶN):**  
> Khi ESP32 nhận được trường `"server_time": 1726733521` trong HTTP Response 200:
> - Nếu RTC của ESP32 chưa được đồng bộ (do mạng trường học/doanh nghiệp chặn cổng UDP 123 NTP), firmware tự động lấy giá trị `server_time` này để cập nhật đồng hồ phần cứng của ESP32 qua hàm `settimeofday()`.
> - Firmware tự động quét và quy đổi toàn bộ các mẫu đo lưu trữ cũ sang mốc thời gian thực chuẩn xác.
> - **Khuyến nghị:** Backend luôn luôn đính kèm trường `"server_time"` trong mọi phản hồi HTTP 200.

---

## 6. Mã Nguồn Mẫu Triển Khai Backend (Production-Ready)

### 6.1. Node.js (Express + Time-Series Bulk Insert)

```javascript
const express = require('express');
const app = express();

app.use(express.json({ limit: '2mb' }));

app.post('/api/sensor', async (req, res) => {
    const data = req.body;
    const deviceId = req.headers['x-device-id'] || data.device_id;
    const isBatch = data.batch === true || req.headers['x-batch-mode'] === 'true';
    const nowSec = Math.floor(Date.now() / 1000);

    // 1. Kiem tra tinh hop le co ban cua Payload
    if (!deviceId) {
        // Tra ve 400 de ESP32 huy bo batch loi, khong ket Flash
        return res.status(400).json({ status: 'error', error: 'Missing device_id' });
    }

    try {
        let recordsToInsert = [];

        if (isBatch) {
            const rawRecords = data.records || [];
            if (!Array.isArray(rawRecords) || rawRecords.length === 0) {
                return res.status(422).json({ status: 'error', error: 'Batch records array is empty or invalid' });
            }

            recordsToInsert = rawRecords.map(r => ({
                deviceId: deviceId,
                // Neu thiet bi chua sync gio (timestamp <= 0), dung nowSec cua server
                timestamp: (r.timestamp && r.timestamp > 1700000000) ? r.timestamp : nowSec,
                seq: r.seq || 0,
                temperature: r.metrics?.temperature,
                humidity: r.metrics?.humidity,
                dewPoint: r.metrics?.dew_point,
                vpd: r.metrics?.vpd,
                chipTemp: r.diagnostics?.chip_temp,
                cpuLoad: r.diagnostics?.cpu_load,
                alert: r.status?.alert || false
            }));

            console.log(`[BATCH INGESTION] Nhan ${recordsToInsert.length} mau offline tu ${deviceId}`);
        } else {
            // Goi tin thoi gian thuc don le
            recordsToInsert = [{
                deviceId: deviceId,
                timestamp: (data.timestamp && data.timestamp > 1700000000) ? data.timestamp : nowSec,
                seq: data.seq || 0,
                temperature: data.metrics?.temperature,
                humidity: data.metrics?.humidity,
                dewPoint: data.metrics?.dew_point,
                vpd: data.metrics?.vpd,
                chipTemp: data.diagnostics?.chip_temp,
                cpuLoad: data.diagnostics?.cpu_load,
                alert: data.status?.alert || false
            }];

            console.log(`[REALTIME 1Hz] #${data.seq} | ${deviceId} | ${data.metrics?.temperature}°C - ${data.metrics?.humidity}%`);
        }

        // 2. Thuc hien Bulk Insert vao Database (PostgreSQL / TimescaleDB / MySQL)
        // await db.sensorTelemetry.bulkCreate(recordsToInsert);

        // 3. Day Realtime ra WebSocket / Server-Sent Events (chi voi goi Realtime)
        if (!isBatch && global.io) {
            global.io.emit(`sensor:${deviceId}`, recordsToInsert[0]);
        }

        // 4. Phan hoi 200 OK kem server_time de ESP32 xoa Flash va tu dong sync RTC
        return res.status(200).json({
            status: 'ok',
            batch: isBatch,
            received_count: recordsToInsert.length,
            server_time: nowSec
        });

    } catch (err) {
        console.error('[DB Error]', err);
        // Loi server database -> Tra ve 500 de ESP32 giu lai Flash va thu lai sau
        return res.status(500).json({ status: 'error', message: 'Internal Server Error' });
    }
});

app.listen(3000, '0.0.0.0', () => {
    console.log('Backend Telemetry Ingestion API dang khoi chay tai cong 3000');
});
```

---

### 6.2. Python (FastAPI + Async TimescaleDB / MongoDB)

```python
from fastapi import FastAPI, Request, Response, status
from pydantic import BaseModel
from typing import List, Optional
import time
import uvicorn

app = FastAPI(title="ESP32-S3 Telemetry Ingestion Service")

@app.post("/api/sensor")
async def receive_sensor_telemetry(request: Request, response: Response):
    now_sec = int(time.time())
    
    try:
        body = await request.json()
    except Exception:
        response.status_code = status.HTTP_400_BAD_REQUEST
        return {"status": "error", "message": "Invalid JSON format"}

    device_id = request.headers.get("x-device-id") or body.get("device_id")
    if not device_id:
        response.status_code = status.HTTP_400_BAD_REQUEST
        return {"status": "error", "message": "Missing device_id"}

    is_batch = body.get("batch", False)

    if is_batch:
        records = body.get("records", [])
        if not records:
            response.status_code = status.HTTP_422_UNPROCESSABLE_ENTITY
            return {"status": "error", "message": "Empty records array"}

        # Xu ly chuan hoa timestamp neu thiet bi chua sync gio
        for r in records:
            if not r.get("timestamp") or r.get("timestamp") <= 0:
                r["timestamp"] = now_sec

        print(f"[BATCH] Nhan {len(records)} ban ghi offline tu {device_id}")
        # await bulk_insert_database(device_id, records)
        count = len(records)
    else:
        record_ts = body.get("timestamp")
        if not record_ts or record_ts <= 0:
            body["timestamp"] = now_sec

        temp = body.get("metrics", {}).get("temperature")
        hum = body.get("metrics", {}).get("humidity")
        print(f"[REALTIME] #{body.get('seq')} | {device_id} | {temp}°C - {hum}%")
        # await insert_single_database(device_id, body)
        count = 1

    # Luon phan hoi kem server_time de ESP32 hieu chuan RTC
    return {
        "status": "ok",
        "batch": is_batch,
        "received_count": count,
        "server_time": now_sec
    }

if __name__ == "__main__":
    uvicorn.run(app, host="0.0.0.0", port=3000)
```

---

### 6.3. Golang (Gin Engine)

```go
package main

import (
	"net/http"
	"time"

	"github.com/gin-gonic/gin"
)

type TelemetryPayload struct {
	DeviceID string                   `json:"device_id"`
	Batch    bool                     `json:"batch"`
	Count    int                      `json:"count"`
	Seq      uint32                   `json:"seq"`
	Timestamp int64                   `json:"timestamp"`
	Metrics  map[string]interface{}   `json:"metrics"`
	Records  []map[string]interface{} `json:"records"`
}

func main() {
	r := gin.Default()

	r.POST("/api/sensor", func(c *gin.Context) {
		var payload TelemetryPayload
		if err := c.ShouldBindJSON(&payload); err != nil {
			c.JSON(http.StatusBadRequest, gin.H{"status": "error", "message": "Malformed JSON"})
			return
		}

		nowSec := time.Now().Unix()
		deviceID := c.GetHeader("X-Device-ID")
		if deviceID == "" {
			deviceID = payload.DeviceID
		}

		if deviceID == "" {
			c.JSON(http.StatusBadRequest, gin.H{"status": "error", "message": "Missing device identifier"})
			return
		}

		if payload.Batch {
			// Xu ly batch ingestion
			c.JSON(http.StatusOK, gin.H{
				"status":         "ok",
				"batch":          true,
				"received_count": len(payload.Records),
				"server_time":    nowSec,
			})
			return
		}

		// Xu ly goi tin don le Realtime
		c.JSON(http.StatusOK, gin.H{
			"status":         "ok",
			"batch":          false,
			"received_count": 1,
			"server_time":    nowSec,
		})
	})

	r.Run("0.0.0.0:3000")
}
```

---

## 7. Kịch Bản Kiểm Thử Bằng Lệnh cURL (Testing Commands)

### 7.1. Giả lập gói Realtime đơn lẻ (1Hz)
```bash
curl -X POST https://bhair.site/api/sensor \
  -H "Content-Type: application/json" \
  -H "X-Device-ID: ESP32S3_TEST01" \
  -H "X-Batch-Mode: false" \
  -d '{
    "device_id": "ESP32S3_TEST01",
    "batch": false,
    "count": 1,
    "timestamp": 1726733520,
    "seq": 1,
    "metrics": {
      "temperature": 28.5,
      "humidity": 65.0,
      "dew_point": 21.3,
      "vpd": 1.15
    },
    "diagnostics": {
      "chip_temp": 42.0,
      "cpu_load": 3.0,
      "uptime_sec": 100,
      "wifi_rssi": -55
    },
    "status": {
      "alert": false,
      "sensor_valid": true
    }
  }'
```

### 7.2. Giả lập gói Batch Ingestion gửi bù (Khi vừa khôi phục mạng)
```bash
curl -X POST https://bhair.site/api/sensor \
  -H "Content-Type: application/json" \
  -H "X-Device-ID: ESP32S3_TEST01" \
  -H "X-Batch-Mode: true" \
  -H "X-Batch-Count: 2" \
  -d '{
    "device_id": "ESP32S3_TEST01",
    "batch": true,
    "count": 2,
    "records": [
      {
        "timestamp": 1726733500,
        "seq": 10,
        "metrics": { "temperature": 28.2, "humidity": 65.1, "dew_point": 21.0, "vpd": 1.12 },
        "diagnostics": { "chip_temp": 41.5, "cpu_load": 2.5, "uptime_sec": 80, "wifi_rssi": 0 },
        "status": { "alert": false, "sensor_valid": true }
      },
      {
        "timestamp": 1726733501,
        "seq": 11,
        "metrics": { "temperature": 28.3, "humidity": 65.2, "dew_point": 21.1, "vpd": 1.13 },
        "diagnostics": { "chip_temp": 41.6, "cpu_load": 2.6, "uptime_sec": 81, "wifi_rssi": 0 },
        "status": { "alert": false, "sensor_valid": true }
      }
    ]
  }'
```

---

## 8. Môi Trường Triển Khai Thực Tế (Live Deployment)

Hệ thống Backend đã được cấu hình và vận hành 24/7 phục vụ kết nối:

- **Endpoint Internet (HTTPS qua Cloudflare Tunnel)**:  
  `https://bhair.site/api/sensor`
- **Endpoint Mạng Nội Bộ (LAN Wi-Fi)**:  
  `http://172.16.10.245:3000/api/sensor`
- **Web Dashboard Giám Sát Real-time (Truy cập trực tiếp)**:  
  `https://bhair.site/esp/`

Mọi thắc mắc kỹ thuật trong quá trình tích hợp API, team Backend vui lòng liên hệ team IoT để phối hợp kiểm thử kết nối trực tiếp trên thiết bị thực tế.