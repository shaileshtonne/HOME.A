import http.server
import socketserver
import urllib.parse
import json
import random
import os
import webbrowser
import threading
import time

PORT = 8080
DIRECTORY = os.path.dirname(os.path.abspath(__file__))

# Simulated device states (same as ESP32)
device_states = {
    "fan1": 0,
    "fan2": 0,
    "peltier1": 0,
    "peltier2": 0,
    "door": 0,
    "bulb1": 0,
    "bulb2": 0
}

current_temp = 24.5
current_humidity = 58.0

class ESP32MockHandler(http.server.SimpleHTTPRequestHandler):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, directory=DIRECTORY, **kwargs)

    def do_GET(self):
        global current_temp, current_humidity
        parsed_url = urllib.parse.urlparse(self.path)
        path = parsed_url.path
        query = urllib.parse.parse_qs(parsed_url.query)

        if path == "/" or path == "/index.html":
            file_path = os.path.join(DIRECTORY, "index.html")
            if os.path.exists(file_path):
                self.send_response(200)
                self.send_header("Content-type", "text/html; charset=utf-8")
                self.end_headers()
                with open(file_path, "rb") as f:
                    self.wfile.write(f.read())
            else:
                self.send_error(404, "File not found")

        elif path == "/sensor":
            # Simulate slight environmental fluctuation
            current_temp = round(24.0 + random.uniform(-0.5, 0.8), 1)
            current_humidity = round(56.0 + random.uniform(-1.0, 2.0), 1)

            payload = {
                "temperature": current_temp,
                "humidity": current_humidity,
                **device_states
            }
            data = json.dumps(payload).encode("utf-8")

            self.send_response(200)
            self.send_header("Content-type", "application/json")
            self.send_header("Access-Control-Allow-Origin", "*")
            self.end_headers()
            self.wfile.write(data)

        elif path == "/control":
            device = query.get("device", [None])[0]
            state = query.get("state", [None])[0]

            if device and device in device_states and state is not None:
                device_states[device] = int(state)
                print(f"[AURA Gateway] Relay state updated: {device} -> {state}")

            self.send_response(200)
            self.send_header("Content-type", "text/plain")
            self.send_header("Access-Control-Allow-Origin", "*")
            self.end_headers()
            self.wfile.write(b"OK")

        else:
            super().do_GET()

def start_server():
    socketserver.TCPServer.allow_reuse_address = True
    with socketserver.TCPServer(("", PORT), ESP32MockHandler) as httpd:
        print(f"[AURA Gateway Simulator] Server active at http://localhost:{PORT}")
        httpd.serve_forever()

if __name__ == "__main__":
    t = threading.Thread(target=start_server, daemon=True)
    t.start()
    time.sleep(0.5)
    
    url = f"http://localhost:{PORT}"
    print(f"Opening {url} in your default browser...")
    webbrowser.open(url)
    
    # Keep main thread alive
    try:
        while True:
            time.sleep(1)
    except KeyboardInterrupt:
        print("Server stopped.")
