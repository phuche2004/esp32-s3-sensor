from http.server import HTTPServer, BaseHTTPRequestHandler
import json
from datetime import datetime

HOST = '0.0.0.0'
PORT = 3000

class ProfessionalSensorServer(BaseHTTPRequestHandler):
    def do_POST(self):
        if self.path == '/api/sensor':
            content_length = int(self.headers.get('Content-Length', 0))
            body = self.rfile.read(content_length)
            
            try:
                data = json.loads(body.decode('utf-8'))
                time_str = datetime.now().strftime('%H:%M:%S')
                device_id = data.get('device_id', self.headers.get('X-Device-ID', 'UNKNOWN'))
                
                # 1. Kiem tra xem co phai goi tin Batch Ingestion (gui bu du lieu offline) khong
                if data.get('batch') is True:
                    records = data.get('records', [])
                    count = data.get('count', len(records))
                    first_seq = records[0].get('seq') if records else 'N/A'
                    last_seq = records[-1].get('seq') if records else 'N/A'
                    first_time = datetime.fromtimestamp(records[0].get('timestamp', 0)).strftime('%H:%M:%S') if records else 'N/A'
                    last_time = datetime.fromtimestamp(records[-1].get('timestamp', 0)).strftime('%H:%M:%S') if records else 'N/A'

                    print('------------------------------------------------------------------------------------------------------')
                    print(f"[{time_str}] [BATCH INGESTION] Da nhan thanh cong dot {count} ban ghi offline tu {device_id}!")
                    print(f"             Pham vi Seq: #{first_seq} -> #{last_seq} | Thoi gian do: {first_time} -> {last_time}")
                    print('------------------------------------------------------------------------------------------------------')

                    response_payload = {
                        "status": "ok",
                        "batch": True,
                        "received_count": count,
                        "server_time": int(datetime.now().timestamp())
                    }
                else:
                    # 2. Xu ly ban tin thoi gian thuc don le
                    seq = data.get('seq', self.headers.get('X-Packet-Seq', 0))
                    
                    if 'metrics' in data:
                        metrics = data.get('metrics', {})
                        temp = metrics.get('temperature', 0.0)
                        hum = metrics.get('humidity', 0.0)
                        dew = metrics.get('dew_point', 0.0)
                        vpd = metrics.get('vpd', 0.0)
                    else:
                        temp = data.get('temperature', 0.0)
                        hum = data.get('humidity', 0.0)
                        dew = 0.0
                        vpd = 0.0

                    diag = data.get('diagnostics', {})
                    chip_temp = diag.get('chip_temp', 0.0)
                    cpu_load = diag.get('cpu_load', 0.0)
                    rssi = diag.get('wifi_rssi', 0)

                    status_info = data.get('status', {})
                    alert = status_info.get('alert', False)
                    alert_badge = "[CANH BAO]" if alert else "[OK]"

                    print(f"[{time_str}] #{seq:<5} | {device_id} | {temp:5.2f} C | {hum:5.2f} %RH | Dew: {dew:4.1f} C | VPD: {vpd:4.2f} kPa | CPU: {cpu_load:4.1f}% ({chip_temp:4.1f} C) | RSSI: {rssi:3} dBm | {alert_badge}")

                    response_payload = {
                        "status": "ok",
                        "ack_seq": seq,
                        "server_time": int(datetime.now().timestamp())
                    }

                self.send_response(200)
                self.send_header('Content-Type', 'application/json')
                self.end_headers()
                self.wfile.write(json.dumps(response_payload).encode('utf-8'))
            except Exception as e:
                print(f"Loi phan tich JSON: {e}")
                self.send_response(400)
                self.end_headers()
        else:
            self.send_response(404)
            self.end_headers()

    def do_GET(self):
        self.send_response(200)
        self.send_header('Content-Type', 'text/html; charset=utf-8')
        self.end_headers()
        self.wfile.write(b'<h3>Backend API Server dang chay tren cong 3000! Ho tro ca Single Telemetry va Batch Ingestion.</h3>')

    def log_message(self, format, *args):
        return

if __name__ == '__main__':
    server = HTTPServer((HOST, PORT), ProfessionalSensorServer)
    print('===================================================================================')
    print(f'   BACKEND REST API SERVER CHUYEN NGHIEP DANG CHAY TAI: http://0.0.0.0:{PORT}')
    print(f'   Endpoint nhan du lieu tu ESP32:                    http://<IP_MAY_TINH>:{PORT}/api/sensor')
    print('   Ho tro: Realtime Single Packet & Batch Ingestion (Store-and-Forward)')
    print('   Nhan Ctrl + C de dung server')
    print('===================================================================================')
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print('\nDa dung server.')
