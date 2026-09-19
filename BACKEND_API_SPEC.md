# ĐẶC TẢ KỸ THUẬT API TELEMETRY (ESP32-S3 -> WEB BACKEND)
### HỖ TRỢ REALTIME 1S/LẦN & BATCH INGESTION CHỐNG MẤT DỮ LIỆU BẰNG FLASH LITTLEFS

Tài liệu này dành cho **Team Backend Web Server** để xây dựng API tiếp nhận dữ liệu thời gian thực và dữ liệu gửi bù từ thiết bị quan trắc môi trường ESP32-S3.

---

## 1. Tổng Quan Kiến Trúc & Kết Nối

- **Mô hình**: HTTP REST Webhook (Client-Initiated Push).
- **Thiết bị gửi**: ESP32-S3 Dual-Core Xtensa LX7 @ 240MHz (Kèm cảm biến SHT31, 16MB Flash, 8MB PSRAM).
- **Tần suất bình thường**: **1 giây / lần (1 Hz)**.
- **Cơ chế chống mất dữ liệu (Store-and-Forward & FIFO Pruning)**:
  - Khi rớt Wi-Fi hoặc Backend server tạm thời bị sập: ESP32 tự động ghi từng mẫu đo vào bộ nhớ **Flash LittleFS** bền vững. **Mất điện hoặc reset chip dữ liệu vẫn còn 100%**.
  - **Cơ chế tự động chống tràn bộ nhớ (FIFO)**: Khi mất mạng dài ngày (vài tuần, 1 tháng...), hệ thống tự động đào thải các mẫu đo cũ nhất theo nguyên lý **FIFO (First In First Out)** để luôn duy trì **1 giờ đo đạc mới nhất** (~3,600 mẫu, chiếm cố định ~172 KB Flash). Dung lượng Flash không bao giờ bị phình to và tuyệt đối không bao giờ tràn bộ nhớ.
  - Khi có mạng trở lại: ESP32 tự động kích hoạt chế độ **Batch Ingestion (Gửi gộp theo đợt)**, đóng gói các mẫu đo tồn đọng thành mảng JSON và bắn 1 lần HTTP POST để xả sạch dữ liệu trong 0.1s.
- **Cơ chế kết nối**: HTTP/1.1 Persistent Connection (`Keep-Alive`).
- **Thời gian chờ phản hồi (Timeout)**:
  - Gói đơn lẻ: **800 ms**
  - Gói Batch Ingestion: **1500 ms** (để Backend có đủ thời gian parse mảng và bulk insert).

---

## 2. Thông Số Kỹ Thuật HTTP Request

### 2.1. Endpoint & Phương Thức

| Thuộc tính | Giá trị |
| :--- | :--- |
| **HTTP Method** | `POST` |
| **URL Path** | Được cấu hình linh hoạt trên ESP32 (Mặc định: `/api/sensor`) |
| **Giao thức** | HTTP/1.1 hoặc HTTPS |

### 2.2. HTTP Request Headers

| Tên Header | Giá trị ví dụ | Ý nghĩa |
| :--- | :--- | :--- |
| `Content-Type` | `application/json` | Định dạng body gói tin |
| `User-Agent` | `ESP32-S3-Sensor/2.0` | Nhận diện firmware thiết bị gửi |
| `X-Device-ID` | `ESP32S3_404CCA44C814` | Mã định danh phần cứng duy nhất của chip |
| `X-Batch-Mode` | `true` hoặc `false` | `true` nếu là gói gửi bù dữ liệu offline; `false` nếu là gói realtime |
| `X-Batch-Count` | `50` | Số lượng bản ghi trong đợt Batch (chỉ có khi `X-Batch-Mode: true`) |
| `X-Packet-Seq` | `1052` | Số thứ tự gói tin (chỉ có khi `X-Batch-Mode: false`) |

---

## 3. Cấu Trúc Dữ Liệu Gửi Đi (JSON Payload)

Backend sẽ nhận được **2 định dạng bản tin** tùy vào tình trạng mạng của thiết bị:

---

### 3.1. Định dạng 1: Bản tin thời gian thực đơn lẻ (`batch: false`)

Gửi đều đặn mỗi **1 giây** khi mạng hoạt động bình thường.

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

### 3.2. Định dạng 2: Bản tin gửi gộp theo đợt (`batch: true`) - BATCH INGESTION

Gửi khi thiết bị vừa có Wi-Fi trở lại sau khi mất mạng, mang theo toàn bộ dữ liệu đã được lưu trữ bền vững trong Flash LittleFS.

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

## 4. Bảng Mô Tả Chi Tiết Các Trường Dữ Liệu

| Trường | Cấp | Kiểu dữ liệu | Ý nghĩa / Đơn vị | Ghi chú |
| :--- | :--- | :--- | :--- | :--- |
| `device_id` | Root | `string` | Định danh phần cứng | Sinh từ MAC Address của chip ESP32-S3 |
| `batch` | Root | `boolean` | Cờ nhận diện Batch | `true`: Gói gửi bù dữ liệu offline; `false`: Gói thời gian thực |
| `count` | Root | `integer` | Số lượng bản ghi | Số lượng bản ghi trong gói tin |
| `timestamp` | Record | `integer` (uint32) | Giây UTC (Epoch) | **CỰC KỲ QUAN TRỌNG: Backend BẮT BUỘC lưu DB theo trường này**, không dùng `Date.now()` của server |
| `seq` | Record | `integer` (uint32) | Số thứ tự gói tin | Tự tăng liên tục, giúp phát hiện mất gói hoặc thống kê tỷ lệ rớt mạng |
| `metrics.temperature` | Record | `float` (°C) | Nhiệt độ môi trường | Cảm biến SHT31 (độ chính xác ±0.2°C) |
| `metrics.humidity` | Record | `float` (%RH) | Độ ẩm không khí | Cảm biến SHT31 (độ chính xác ±2%RH) |
| `metrics.dew_point` | Record | `float` (°C) | Điểm sương | Nhiệt độ ngưng tụ hơi nước (công thức Magnus) |
| `metrics.vpd` | Record | `float` (kPa) | Áp suất hơi bão hòa thiếu hụt | Vapor Pressure Deficit |
| `diagnostics.chip_temp` | Record | `float` (°C) | Nhiệt độ nhân chip | Cảm biến nhiệt độ tích hợp trong ESP32-S3 |
| `diagnostics.cpu_load` | Record | `float` (%) | Mức tải CPU tổng thể | 0.0% đến 100.0% |
| `diagnostics.cpu0` / `cpu1` | Record | `float` (%) | Mức tải từng nhân | Core 0 (Wi-Fi/LED) và Core 1 (App/Sensor) |
| `diagnostics.free_heap` | Record | `integer` (Bytes) | RAM còn trống | Bộ nhớ SRAM khả dụng của chip |
| `diagnostics.uptime_sec` | Record | `integer` (Giây) | Thời gian hoạt động | Tính từ lúc bật nguồn thiết bị |
| `diagnostics.wifi_rssi` | Record | `integer` (dBm) | Cường độ sóng Wi-Fi | `-30` (rất khỏe) đến `-85` (rất yếu) |
| `status.alert` | Record | `boolean` | Cờ cảnh báo | `true` nếu nhiệt độ hoặc độ ẩm vượt ngưỡng cài đặt |
| `status.sensor_valid` | Record | `boolean` | Trạng thái cảm biến | `true` nếu đọc SHT31 bình thường qua I2C |

---

## 5. Quy Chuẩn Phản Hồi Từ Backend (Response Specification)

Để ESP32 ghi nhận việc gửi thành công (và xóa các bản ghi Flash sau khi gửi bù):

- **HTTP Status Code**: Trả về `200 OK` (hoặc `201`, `204`).
- **Header**: `Content-Type: application/json`
- **Body JSON gợi ý**:
  ```json
  {
    "status": "ok",
    "received_count": 50,
    "server_time": 1726733521
  }
  ```

> **LƯU Ý:** Nếu Backend trả về mã lỗi (`500`, `502`, `503`, `504`) hoặc timeout, ESP32 sẽ **KHÔNG XÓA** file Flash mà giữ nguyên dữ liệu để tiếp tục gửi lại ở chu kỳ tiếp theo!

---

## 6. Code Mẫu Backend Sẵn Sàng Chạy (Hỗ Trợ Cả Single & Batch)

### 6.1. Node.js (Express + PostgreSQL / MySQL Bulk Insert)

```javascript
const express = require('express');
const app = express();

app.use(express.json({ limit: '1mb' }));

app.post('/api/sensor', async (req, res) => {
    const data = req.body;
    const deviceId = data.device_id;
    const isBatch = data.batch === true;

    // Chuyen ve mang cac ban ghi de xu ly thong nhat
    let records = [];
    if (isBatch) {
        records = data.records || [];
        console.log(`[BATCH INGESTION] Nhan ${records.length} ban ghi offline tu ${deviceId}`);
    } else {
        // Goi tin don le
        records = [{
            timestamp: data.timestamp,
            seq: data.seq,
            metrics: data.metrics,
            diagnostics: data.diagnostics,
            status: data.status
        }];
        console.log(`[REALTIME] #${data.seq} | ${deviceId} | ${data.metrics.temperature}°C - ${data.metrics.humidity}%`);
    }

    // 1. Luu vao Database theo batch (vi du PostgreSQL / MySQL)
    // Luon luu theo r.timestamp (thoi gian thuc cua thiet bi), KHONG dung Date.now()!
    /*
    const values = records.map(r => [
        deviceId,
        new Date(r.timestamp * 1000),
        r.seq,
        r.metrics.temperature,
        r.metrics.humidity,
        r.metrics.dew_point,
        r.metrics.vpd,
        r.diagnostics.chip_temp,
        r.diagnostics.cpu_load,
        r.status.alert
    ]);
    await db.query(`
        INSERT INTO sensor_telemetry 
        (device_id, measured_at, seq, temp, hum, dew_point, vpd, chip_temp, cpu_load, alert)
        VALUES ?
    `, [values]);
    */

    // 2. Day du lieu realtime ra WebSocket Dashboard (chi day goi realtime)
    if (!isBatch && typeof wsBroadcast === 'function') {
        wsBroadcast(JSON.stringify(data));
    }

    // 3. Phan hoi ngay lap tuc de ESP32 xoa cache Flash
    return res.status(200).json({
        status: 'ok',
        batch: isBatch,
        received_count: records.length
    });
});

app.listen(3000, '0.0.0.0', () => {
    console.log('Backend Telemetry API dang chay tai cong 3000');
});
```

---

### 6.2. Python (FastAPI + Async Processing)

```python
from fastapi import FastAPI, Request
from pydantic import BaseModel
from typing import List, Optional
import uvicorn
from datetime import datetime

app = FastAPI()

@app.post("/api/sensor")
async def receive_telemetry(request: Request):
    data = await request.json()
    device_id = data.get("device_id")
    is_batch = data.get("batch", False)

    if is_batch:
        records = data.get("records", [])
        count = data.get("count", len(records))
        print(f"[BATCH] Nhan {count} ban ghi tu {device_id} (Seq #{records[0].get('seq')} -> #{records[-1].get('seq')})")
        
        # Thuc hien Bulk Insert vao Time-Series DB (TimescaleDB / InfluxDB)
        # for r in records: save_to_db(r)
    else:
        seq = data.get("seq")
        temp = data.get("metrics", {}).get("temperature")
        hum = data.get("metrics", {}).get("humidity")
        print(f"[REALTIME] #{seq} | {device_id} | {temp}°C - {hum}%")

    return {"status": "ok", "batch": is_batch, "received": len(data.get("records", [1]))}

if __name__ == "__main__":
    uvicorn.run(app, host="0.0.0.0", port=3000)
```

---

### 6.3. Golang (Gin Framework)

```go
package main

import (
	"net/http"
	"github.com/gin-gonic/gin"
)

func main() {
	r := gin.Default()

	r.POST("/api/sensor", func(c *gin.Context) {
		var body map[string]interface{}
		if err := c.ShouldBindJSON(&body); err != nil {
			c.JSON(http.StatusBadRequest, gin.H{"error": err.Error()})
			return
		}

		isBatch, _ := body["batch"].(bool)
		deviceId, _ := body["device_id"].(string)

		if isBatch {
			records, _ := body["records"].([]interface{})
			// Xu ly batch insert vao database
			c.JSON(http.StatusOK, gin.H{"status": "ok", "batch": true, "count": len(records)})
			return
		}

		// Xu ly goi tin don le
		c.JSON(http.StatusOK, gin.H{"status": "ok", "batch": false, "device_id": deviceId})
	})

	r.Run("0.0.0.0:3000")
}
```

---

## 7. Script Kiểm Thử Nhanh (server.py)

Dự án đã tích hợp sẵn script Python giả lập Backend server đầy đủ hỗ trợ cả 2 chế độ:

```bash
python server.py
```

Khi chạy, server lắng nghe tại `http://0.0.0.0:3000/api/sensor`. Mày có thể rút nguồn Wi-Fi router hoặc ngắt server trong vài phút rồi bật lại, terminal của `server.py` sẽ hiển thị banner:
```
------------------------------------------------------------------------------------------------------
[16:15:30] [BATCH INGESTION] Da nhan thanh cong dot 50 ban ghi offline tu ESP32S3_404CCA44C814!
             Pham vi Seq: #120 -> #170 | Thoi gian do: 16:14:40 -> 16:15:30
------------------------------------------------------------------------------------------------------
```
Toàn bộ dữ liệu được bảo toàn nguyên vẹn 100%.

---

## 8. Triển Khai Thực Tế Trên AndroidServer (Live Deployment)

Hệ thống Backend và Web Dashboard đã được triển khai chính thức và đang hoạt động 24/7 trên máy chủ AndroidServer:

### 8.1. Địa chỉ tiếp nhận Telemetry từ ESP32-S3
- **Qua Internet (Cloudflare Tunnel HTTPS)**:
  `https://bhair.site/api/sensor`
- **Qua mạng nội bộ Wi-Fi (LAN)**:
  `http://172.16.10.245:3000/api/sensor`
- **Qua mạng Tailscale**:
  `http://100.91.43.5:3000/api/sensor`

### 8.2. Web Dashboard Giám Sát Thời Gian Thực (Không Cần Đăng Nhập)
- **Đường dẫn truy cập trực tiếp**:
  `https://bhair.site/esp/`
- **Tính năng hệ thống**:
  - Giao diện Dark Glassmorphism cao cấp, chuẩn Responsive cho cả máy tính và điện thoại.
  - Tích hợp **8 đồ thị mini (Sparklines)** chạy thời gian thực trên từng thẻ chỉ số (Nhiệt độ, Độ ẩm, Điểm sương, VPD, Nhiệt độ chip, Tải CPU, RAM Heap, Sóng Wi-Fi).
  - **Master Interactive Chart**: Cho phép xem toàn cảnh Môi Trường, Phần Cứng hoặc soi chi tiết từng chỉ số riêng lẻ theo các mốc **5 Phút (Realtime 1Hz)**, **1 Giờ** và **24 Giờ**.
  - **Lưu trữ lịch sử bền vững (Persistent Downsampling)**: Tự động gom mẫu 1 phút/điểm và lưu vào `history_store.json` để bảo toàn dữ liệu lịch sử ngay cả khi máy chủ khởi động lại.
  - **Hỗ trợ Store-and-Forward (Batch Ingestion)**: Tiếp nhận các gói tin gửi bù từ Flash LittleFS khi thiết bị có Wi-Fi trở lại, lưu trữ chính xác theo timestamp của thiết bị và vẽ lấp đầy khoảng trống lịch sử trên biểu đồ.