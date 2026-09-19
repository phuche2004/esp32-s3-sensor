# DANH SÁCH TỔNG HỢP LINH KIỆN VÀ PHỤ KIỆN IOT

Bảng tổng hợp toàn bộ các module cảm biến, thiết bị đo lường và phụ kiện mở rộng cho hệ thống ESP32-S3 đã được khảo sát.

---

## 1. Bảng Thống Kê Tất Cả Các Món IoT

| STT | Tên Linh Kiện / Module | Mô Tả & Chức Năng Chính | Giá Tham Khảo | Sẽ Mua | Không Mua | Ghi Chú / Trạng Thái |
| :---: | :--- | :--- | :---: | :---: | :---: | :--- |
| **I** | **NHÓM NHIỆT ĐỘ, ĐỘ ẨM VÀ THÂN NHIỆT** | | | | | |
| 1 | **DS18B20 bọc Inox** | Đầu dò tiếp xúc chống nước 100%, dây 1m - 3m, 1-Wire (GPIO 10) | 30.000 VNĐ | [x] | [ ] | **ĐÃ CHỐT MUA** (Đo nước, máy móc) |
| 2 | **Sensirion SHT31-DIS** | Cảm biến nhiệt độ - độ ẩm độ chính xác cao (sai số ±0.2°C) | Có sẵn | - | - | Đã tích hợp sẵn trên bo mạch ESP32-S3 |
| 3 | **MAX30205** | Cảm biến thân nhiệt lâm sàng y tế ASTM E1112 (±0.1°C), I2C | 60.000 VNĐ | [ ] | [x] | Không cần đo áp da trực tiếp |
| 4 | **TI TMP117** | Chip đo nhiệt chuẩn xác nhất thế giới của TI (±0.1°C toàn dải), I2C | 110.000 VNĐ | [ ] | [x] | Giá cao, không thiết thực bằng DS18B20 |
| 5 | **Cặp nhiệt điện Type-K + MAX6675** | Đo nhiệt độ siêu cao 0°C đến 1024°C (lò nung, đầu phun in 3D) | 65.000 VNĐ | [ ] | [x] | Không có nhu cầu đo lò nung |
| 6 | **Đầu dò RTD Pt100/Pt1000 + MAX31865** | Đầu dò điện trở bạch kim chuẩn phòng thí nghiệm và hóa chất | 160.000 VNĐ | [ ] | [x] | Chi phí cao, cồng kềnh |
| **II** | **NHÓM BỨC XẠ HỒNG NGOẠI VÀ CAMERA NHIỆT** | | | | | |
| 7 | **MLX90614 BAA** | Đo nhiệt 1 điểm từ xa bằng hồng ngoại, trường nhìn 90 độ, I2C | 110.000 VNĐ | [ ] | [x] | Đo tiếp xúc bằng DS18B20 chuẩn hơn |
| 8 | **MLX90614 DCI** | Bản thấu kính hội tụ góc hẹp 5 độ, bắn xa 50cm - 1m không loãng nhiệt | 320.000 VNĐ | [ ] | [x] | Giá đắt |
| 9 | **AMG8833 (Grid-EYE)** | Camera nhiệt ma trận 8x8 (64 pixel), I2C, bắt điểm nóng từ xa | 250.000 VNĐ | [ ] | [x] | Độ phân giải thấp so với giá tiền |
| 10 | **MLX90640** | Camera nhiệt 32x24 (768 pixel), soi bo mạch PCB chập cháy | 800.000 VNĐ | [ ] | [x] | Giá quá đắt |
| **III** | **NHÓM CHẤT LƯỢNG KHÔNG KHÍ, KHÍ ĐỘC VÀ BỤI MỊN** | | | | | |
| 11 | **Plantower PMS7003** | Đo bụi mịn PM1.0, PM2.5, PM10 bằng tán xạ laser có quạt hút, UART | 280.000 VNĐ | [ ] | [x] | **TẠM HOÃN / BỎ** (To, chưa thực sự cần) |
| 12 | **Plantower PMS5003ST** | 4-trong-1: Bụi mịn + Khí độc Formaldehyde (keo gỗ ép) + Nhiệt ẩm | 650.000 VNĐ | [ ] | [x] | Giá cao |
| 13 | **Sensirion SPS30** | Cảm biến bụi mịn chuẩn công nghiệp châu Âu, tuổi thọ 10 năm | 850.000 VNĐ | [ ] | [x] | Quá đắt |
| 14 | **Sensirion SGP40** | Đo khí độc, mùi hôi, cồn, hóa chất (VOC Index 0-500), I2C | 130.000 VNĐ | [ ] | [x] | Không có nhu cầu đo mùi |
| 15 | **Sensirion SGP41** | Đo VOC Index + NOx Index (khói xe máy, ô tô, bếp than), I2C | 180.000 VNĐ | [ ] | [x] | Không cần thiết |
| 16 | **Winsen MH-Z19C** | Đo khí ngạt CO2 thật bằng buồng quang hồng ngoại NDIR, UART | 310.000 VNĐ | [ ] | [x] | Không cần đo phòng máy lạnh |
| 17 | **Sensirion SCD40 / SCD41** | Đo CO2 thật bằng công nghệ quang âm siêu nhỏ, I2C | 350.000 VNĐ | [ ] | [x] | Giá cao |
| 18 | **Bosch BME680 / BME688** | 4-trong-1: Nhiệt độ, Độ ẩm, Áp suất khí quyển (báo bão), Khí VOC | 190.000 VNĐ | [ ] | [x] | Trùng lặp tính năng SHT31 |
| 19 | **Sensirion SEN54 / SEN55** | Cụm cảm biến All-in-One 5-trong-1 / 6-trong-1 công nghiệp Thụy Sĩ | 1.200.000 VNĐ | [ ] | [x] | Giá quá cao |
| 20 | **Sharp GP2Y1010 / MQ series** | Cảm biến bụi/khí cổ điển (MQ-135, MQ-2), nóng và sai số lớn | 35.000 VNĐ | [ ] | [x] | Chất lượng kém, tốn điện |
| **IV** | **NHÓM SỨC KHỎE, TIM MẠCH VÀ TINH THẦN** | | | | | |
| 21 | **MAX30102** | Mắt quang đặt trên mặt hộp, chạm tay 30s đo Nhịp tim, SpO2, Stress HRV | 50.000 VNĐ | [x] | [ ] | **ĐÃ CHỐT MUA** (Đo tinh thần, thực dụng) |
| 22 | **Màng áp điện PVDF LDT0-028K** | Lót dưới đệm ghế/nệm, đo nhịp tim, thở, giấc ngủ không chạm | 110.000 VNĐ | [ ] | [x] | Không thích kéo dây ra ghế |
| 23 | **Cảm biến phản ứng da GSR / EDA** | Xỏ 2 ngón tay đo mồ hôi bắt lo âu, hồi hộp (máy phát hiện nói dối) | 75.000 VNĐ | [ ] | [x] | Vướng tay không gõ phím được |
| 24 | **TI ADS1292R** | Y tế chuyên dụng: Điện tim ECG 24-bit + đo nhịp thở lồng ngực | 400.000 VNĐ | [ ] | [x] | Bất tiện phải dán miếng keo |
| 25 | **AD8232** | Điện tâm đồ ECG 1 kênh analog kèm dây dán ngực | 90.000 VNĐ | [ ] | [x] | Vướng víu, không dùng hàng ngày |
| 26 | **TI ADS1299** | Sóng não EEG 24-bit 8 kênh chuẩn nghiên cứu (giao diện BCI) | 750.000 VNĐ | [ ] | [x] | Phải bôi gel, đội mũ điện cực |
| 27 | **NeuroSky TGAM** | Module sóng não 1 kênh đeo trán (đo tập trung và thiền) | 280.000 VNĐ | [ ] | [x] | Không thực tế để dùng lâu dài |
| 28 | **MyoWare 2.0 (EMG)** | Cảm biến điện cơ bắt hiện tượng nghiến răng, gồng cơ vai gáy | 190.000 VNĐ | [ ] | [x] | Không phù hợp |
| 29 | **Màng rung siêu âm 108kHz** | Phun sương tinh dầu thơm tự động khi căng thẳng | 18.000 VNĐ | [ ] | [x] | Không thích kiểu tác động cải thiện |
| **V** | **NHÓM ĐÈN LASER VÀ KÍNH NGẮM** | | | | | |
| 30 | **Ống Laser Đỏ xịn 635nm** | Vỏ đồng thau CNC, thấu kính thủy tinh, chấm tròn đanh thép | 65.000 VNĐ | [ ] | [x] | **ĐÃ CHỐT BỎ** (Không cần thiết) |
| 31 | **Module Laser Đỏ KY-008** | Laser đỏ 5mW thấu kính nhựa meca đồ chơi giá rẻ | 8.000 VNĐ | [ ] | [x] | Tia nhòe, không xài |
| 32 | **Laser Xanh Lá 520nm / 532nm** | Tia xanh sáng rực, thấy rõ luồng tia trong không khí | 80.000 VNĐ | [ ] | [x] | Đắt tiền hơn, không cần |
| 33 | **Laser Tím Blu-ray 405nm** | Tia tím huyền bí, phát quang tem, soi tiền giả, khô keo UV | 55.000 VNĐ | [ ] | [x] | Mắt người nhìn thấy mờ nhạt |
| 34 | **Cụm Laser ngắm ốc X-Y vỏ nhôm** | Ống ngắm chuyên dụng có 2 ốc lục giác vi chỉnh tâm cơ khí | 130.000 VNĐ | [ ] | [x] | Không cần |
| 35 | **Kính ngắm Red Dot Mini (RMR)** | Kính phản xạ quang học chấm đỏ vô cực vỏ hợp kim nhôm | 160.000 VNĐ | [ ] | [x] | Không dùng |
| 36 | **Laser chiếu hồng tâm (Circle Dot)** | Chiếu ra vòng tròn hồng tâm hoặc chữ thập lên tường | 50.000 VNĐ | [ ] | [x] | Không dùng |
| **VI** | **NHÓM MÀN HÌNH HIỂN THỊ** | | | | | |
| 37 | **Màn hình IPS 1.47 inch ST7789** | 172x320 pixel, bo cong 4 góc cực đẹp, 247 PPI, giao tiếp SPI | 74.500 VNĐ | [ ] | [ ] | **ĐANG CÂN NHẮC** (Vừa hộp cũ, siêu nét) |
| 38 | **Màn hình TFT 2.4 inch ILI9341 Touch** | 240x320 pixel, màn to phủ kín mặt hộp, CẢM ỨNG chạm vuốt | 155.000 VNĐ | [ ] | [ ] | **ĐANG CÂN NHẮC** (Hợp với hộp to hơn) |
| 39 | **Màn hình OLED 0.96 / 1.3 inch** | Đơn sắc (trắng/xanh), I2C chung chân SHT31, ăn điện cực ít | 45.000 VNĐ | [ ] | [x] | Đơn sắc, nhỏ, không sinh động |
| 40 | **Màn hình mực điện tử E-Paper 2.13"** | Chữ rõ như sách in, rút điện vẫn lưu số liệu vĩnh viễn | 110.000 VNĐ | [ ] | [x] | Tốc độ làm tươi chậm |
| **VII** | **NHÓM VỎ HỘP ĐỰNG THIẾT BỊ** | | | | | |
| 41 | **Hộp nhựa hiện tại (Đã khoét lỗ)** | Hộp đang sử dụng, đã cắt phíp lỗ xanh bắt ốc vừa in | Có sẵn | [ ] | [ ] | **ĐANG CÂN NHẮC** (Giữ được vì đã bỏ PMS) |
| 42 | **Hộp nhựa sâu lòng 100x68x50 mm** | Hộp nhựa ABS chống nước sâu 50mm, chia 2 tầng thoáng đãng | 30.000 VNĐ | [ ] | [ ] | **ĐANG CÂN NHẮC** (Nếu muốn thoáng dễ đi dây) |
| 43 | **Hộp nhựa mặt nghiêng để bàn** | Mặt vát nghiêng 20 độ hướng thẳng vào mắt người ngồi | 55.000 VNĐ | [ ] | [ ] | **ĐANG CÂN NHẮC** (Nếu muốn để bàn đẹp) |
| 44 | **Hộp in 3D theo yêu cầu** | Thiết kế đo ni đóng giày theo linh kiện | 50.000 VNĐ | [ ] | [ ] | **ĐANG CÂN NHẮC** |
| **VIII** | **NHÓM LƯU TRỮ, NGUỒN VÀ NGOẠI VI KHÁC** | | | | | |
| 45 | **Module thẻ nhớ MicroSD** | Ghi nhật ký CSV ngoại tuyến, lưu trữ file Web Server | 15.000 VNĐ | [ ] | [ ] | **ĐANG CHỜ** (Rẻ, tiện dụng) |
| 46 | **Mạch UPS Mini 5V tự động** | Nuôi nguồn ESP32 chạy 8-12 tiếng khi bị cúp điện | 25.000 VNĐ | [ ] | [ ] | **ĐANG CHỜ** |
| 47 | **Tự chế sạc dự phòng** | Tự hàn cell pin 18650 + mạch sạc nhanh | 200.000 VNĐ | [ ] | [x] | **BỎ** (Mua sạc ngoài rẻ và an toàn hơn) |
| 48 | **Module SIM 4G LTE A7670C** | Nhắn tin SMS cảnh báo cháy/nhiệt, nhá máy cuộc gọi | 180.000 VNĐ | [ ] | [x] | Không có nhu cầu viễn thông |
| 49 | **Module LoRa SX1278 E32-433T20D** | Truyền tín hiệu không dây xuyên tường 2-3km | 80.000 VNĐ | [ ] | [x] | Không cần thiết |
| 50 | **Module bộ đàm SA818** | Nói chuyện trực tiếp vào máy bộ đàm ngoài đời | 180.000 VNĐ | [ ] | [x] | Không cần |
| 51 | **Bộ đàm số I2S (Mic + Ampli + Loa)** | Đàm thoại Push-to-Talk qua WiFi / Web Audio | 60.000 VNĐ | [ ] | [x] | Không cần |
| 52 | **Radar vi sóng 24GHz LD2410C** | Bắt nhịp thở người ngồi bất động trong phòng, xuyên hộp | 55.000 VNĐ | [ ] | [x] | Không chọn |
| 53 | **Cảm biến sét AS3935** | Bắt sóng điện từ báo dông sét cách xa 40km | 280.000 VNĐ | [ ] | [x] | Không cần |
| 54 | **Cảm biến Laser ToF VL53L0X** | Đo khoảng cách vật lý bằng laser 2cm đến 2m | 50.000 VNĐ | [ ] | [x] | Không cần |
| 55 | **Mắt hồng ngoại IR Transceiver** | Bật tắt máy lạnh, quạt, TV tự động từ Web UI | 15.000 VNĐ | [ ] | [x] | Không cần |
| 56 | **Cảm biến cử chỉ tay APDS-9960** | Vẫy tay trong không khí điều khiển đèn LED | 50.000 VNĐ | [ ] | [x] | Không cần |
| 57 | **Đầu đọc thẻ từ RFID RC522 / PN532** | Quẹt thẻ bảo mật mở khóa hệ thống | 35.000 VNĐ | [ ] | [x] | Không cần |
| 58 | **Cảm biến con quay gia tốc MPU-6050** | Đo rung lắc, báo động chống trộm di dời hộp | 28.000 VNĐ | [ ] | [x] | Không cần |
| 59 | **Module đo dòng điện & công suất INA219**| Đo điện áp, dòng tiêu thụ mA và công suất Watt | 30.000 VNĐ | [ ] | [x] | Không cần |
| 60 | **Module bắt sóng cửa cuốn CC1101** | Bắt và sao chép remote RF 433MHz mở cửa cuốn | 80.000 VNĐ | [ ] | [x] | Không cần |
| 61 | **Ống đếm bức xạ hạt nhân Geiger** | Đo phóng xạ Chernobyl qua ống J305 kêu lách tách | 380.000 VNĐ | [ ] | [x] | Không cần |
| 62 | **Cảm biến quang phổ 11 kênh AMS AS7341**| Cân màu màn hình, đo chỉ số hoàn màu đèn CRI | 300.000 VNĐ | [ ] | [x] | Không cần |
| 63 | **Cảm biến màu cơ bản TCS34725** | Sao chép màu sắc vật thể đổi màu LED WS2812 | 50.000 VNĐ | [ ] | [x] | Không cần |
| 64 | **Núm xoay cơ khí Rotary Encoder EC11** | Núm vặn vô cực tạch tạch chỉnh chế độ, độ sáng | 12.000 VNĐ | [ ] | [x] | Không cần |
| 65 | **Đồng hồ thời gian thực RTC DS3231** | Lưu giờ thực vĩnh viễn bằng pin cúc áo | 28.000 VNĐ | [ ] | [x] | Không cần |
| 66 | **Định vị vệ tinh GPS ATGM336H** | Định vị tọa độ, lấy giờ nguyên tử vệ tinh | 70.000 VNĐ | [ ] | [x] | Không cần |

---

## 2. Tổng Kết Các Món Đang Giữ Lại Để Bàn Tiếp

1. **Món đã chốt chắc chắn mua (2 món)**:
   * **DS18B20 bọc Inox** (~30.000 VNĐ): Đo tiếp xúc nước, máy móc.
   * **MAX30102** (~50.000 VNĐ): Đặt ngón tay 30s đo Nhịp tim, SpO2, chỉ số Stress HRV.
   * *(Tổng tiền đã chốt: ~80.000 VNĐ)*

2. **Món còn bỏ ngỏ cần chốt nốt**:
   * **Màn hình**: Chọn con **1.47 inch IPS bo cong (74.500 VNĐ)** hay con to **2.4 inch Touch (155.000 VNĐ)**?
   * **Hộp**: Dùng lại **Hộp cũ hiện tại (0 VNĐ)** hay lên **Hộp mới (30.000 - 55.000 VNĐ)**?
   * **Phụ trợ**: Có thêm **Thẻ nhớ MicroSD (15.000 VNĐ)** hoặc **Mạch UPS mini (25.000 VNĐ)** không?
