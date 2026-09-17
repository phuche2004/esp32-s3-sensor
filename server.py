from http.server import HTTPServer, BaseHTTPRequestHandler
import json
from datetime import datetime

HOST = '0.0.0.0'
PORT = 3000

class SimpleSensorServer(BaseHTTPRequestHandler):
    def do_POST(self):
        if self.path == '/api/sensor':
            content_length = int(self.headers.get('Content-Length', 0))
            body = self.rfile.read(content_length)
            
            try:
                data = json.loads(body.decode('utf-8'))
                temp = data.get('temperature', 0.0)
                hum = data.get('humidity', 0.0)
                time_str = datetime.now().strftime('%H:%M:%S')

                print(f'[{time_str}] [NHẬN DỮ LIỆU] Nhiệt độ: {temp:.2f} °C  |  Độ ẩm: {hum:.2f} %')

                self.send_response(200)
                self.send_header('Content-Type', 'application/json')
                self.end_headers()
                self.wfile.write(b'{"status":"ok"}')
            except Exception as e:
                print(f'Lỗi phân tích JSON: {e}')
                self.send_response(400)
                self.end_headers()
        else:
            self.send_response(404)
            self.end_headers()

    def do_GET(self):
        self.send_response(200)
        self.send_header('Content-Type', 'text/html; charset=utf-8')
        self.end_headers()
        self.wfile.write(b'<h3>Backend API dang chay tren cong 3000! San sang nhan du lieu tu ESP32 tai /api/sensor</h3>')

    def log_message(self, format, *args):
        # Tắt bớt log HTTP mặc định cho đỡ rối mắt
        return

if __name__ == '__main__':
    server = HTTPServer((HOST, PORT), SimpleSensorServer)
    print('========================================================')
    print(f'   BACKEND SERVER ĐANG CHẠY TẠI: http://172.16.10.169:{PORT}')
    print(f'   API Nhận dữ liệu ESP32:     http://172.16.10.169:{PORT}/api/sensor')
    print('   Nhấn Ctrl + C để dừng server')
    print('========================================================')
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print('\nĐã dừng server.')
