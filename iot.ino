#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>

// =====================================================
// WIFI SETTINGS
// =====================================================

const char* ssid = "YOUR_WIFI_NAME";
const char* password = "YOUR_WIFI_PASSWORD";

// =====================================================
// WEB SERVER
// =====================================================

WebServer server(80);

// =====================================================
// OLED SETTINGS
// =====================================================

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define OLED_SDA 21
#define OLED_SCL 22

Adafruit_SSD1306 display(
  SCREEN_WIDTH,
  SCREEN_HEIGHT,
  &Wire,
  -1
);

// =====================================================
// DHT11
// =====================================================

#define DHTPIN 27
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);

// =====================================================
// RELAY PINS
// =====================================================

#define FAN1_RELAY     16
#define FAN2_RELAY     17

#define PELTIER1_RELAY 18
#define PELTIER2_RELAY 19

#define DOOR_RELAY     23

#define BULB1_RELAY    25
#define BULB2_RELAY    26

// =====================================================
// RELAY LOGIC
// Most relay modules are ACTIVE LOW.
// Change these if your relay works opposite.
// =====================================================

#define RELAY_ON  LOW
#define RELAY_OFF HIGH

// =====================================================
// DEVICE STATES
// =====================================================

bool fan1State = false;
bool fan2State = false;

bool peltier1State = false;
bool peltier2State = false;

bool doorState = false;

bool bulb1State = false;
bool bulb2State = false;

// =====================================================
// DHT VALUES
// =====================================================

float temperature = 0;
float humidity = 0;

unsigned long lastDHTRead = 0;

// =====================================================
// OLED SCREEN CYCLE TIMER
// =====================================================

unsigned long lastOLED = 0;
bool oledHello = true;

// =====================================================
// SET RELAY
// =====================================================

void setRelay(int pin, bool state)
{
  digitalWrite(pin, state ? RELAY_ON : RELAY_OFF);
}

// =====================================================
// UPDATE ALL RELAYS
// =====================================================

void updateRelays()
{
  setRelay(FAN1_RELAY, fan1State);
  setRelay(FAN2_RELAY, fan2State);

  setRelay(PELTIER1_RELAY, peltier1State);
  setRelay(PELTIER2_RELAY, peltier2State);

  setRelay(DOOR_RELAY, doorState);

  setRelay(BULB1_RELAY, bulb1State);
  setRelay(BULB2_RELAY, bulb2State);
}

// =====================================================
// READ DHT11
// =====================================================

void readDHT()
{
  if (millis() - lastDHTRead >= 2000)
  {
    lastDHTRead = millis();

    float h = dht.readHumidity();
    float t = dht.readTemperature();

    if (!isnan(h) && !isnan(t))
    {
      humidity = h;
      temperature = t;
    }
  }
}

// =====================================================
// OLED DISPLAY
// =====================================================

void updateOLED()
{
  if (millis() - lastOLED >= 1000)
  {
    lastOLED = millis();

    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);

    if (oledHello)
    {
      display.setTextSize(2);
      display.setCursor(12, 18);
      display.println("AURA");
      display.setCursor(12, 42);
      display.println("HOME OS");
    }
    else
    {
      display.setTextSize(1);

      display.setCursor(0, 0);
      display.println("AURA SMART HOME");

      display.setCursor(0, 18);
      display.print("Temp:     ");
      display.print(temperature, 1);
      display.println(" C");

      display.setCursor(0, 32);
      display.print("Humidity: ");
      display.print(humidity, 1);
      display.println(" %");

      display.setCursor(0, 50);
      display.print("IP: ");
      display.print(WiFi.localIP());
    }

    display.display();
    oledHello = !oledHello;
  }
}

// =====================================================
// RENDER INDIVIDUAL APPLIANCE CARD
// =====================================================

String renderDeviceCard(
  const char* id,
  const char* name,
  const char* subtitle,
  const char* category,
  const char* pin,
  const char* imgUrl,
  bool state,
  const char* typeClass
)
{
  String card = "<div class='appliance-card ";
  card += typeClass;
  if (state) card += " active";
  card += "' id='card-";
  card += id;
  card += "'>";

  // Real Image Viewport
  card += "<div class='image-viewport'>";
  card += "<img src='";
  card += imgUrl;
  card += "' alt='";
  card += name;
  card += "' class='real-photo' loading='lazy'>";
  card += "<div class='image-gradient'></div>";
  card += "<div class='card-top-badges'>";
  card += "<span class='device-category-chip'>";
  card += category;
  card += "</span>";
  card += "<span class='live-status-pill' id='badge-";
  card += id;
  card += "'>";
  if (strcmp(id, "door") == 0) {
    card += state ? "UNLOCKED" : "LOCKED";
  } else {
    card += state ? "ONLINE" : "STANDBY";
  }
  card += "</span></div></div>";

  // Card Content Body
  card += "<div class='card-body'>";
  card += "<div class='device-meta'><div>";
  card += "<div class='device-name'>";
  card += name;
  card += "</div>";
  card += "<div class='device-subtitle'>";
  card += subtitle;
  card += " &bull; <span class='pin-badge'>";
  card += pin;
  card += "</span></div></div></div>";

  // Action / Switch Row
  card += "<div class='card-action-row'>";
  card += "<span class='state-label' id='state-";
  card += id;
  card += "'>";
  if (strcmp(id, "door") == 0) {
    card += state ? "Door Unlocked (Open)" : "Perimeter Secured";
  } else {
    card += state ? "Active &amp; Running" : "Device Offline";
  }
  card += "</span>";

  card += "<button class='control-switch-btn' onclick=\"toggleDevice('";
  card += id;
  card += "')\"><span id='btn-icon-";
  card += id;
  card += "'>";
  if (strcmp(id, "door") == 0) {
    card += state ? "&#x1F512;" : "&#x1F513;";
  } else if (strstr(typeClass, "is-cooling")) {
    card += state ? "&#x23FB;" : "&#x2744;&#xFE0F;";
  } else if (strstr(typeClass, "is-bulb")) {
    card += state ? "&#x23FB;" : "&#x1F4A1;";
  } else {
    card += state ? "&#x23FB;" : "&#x26A1;";
  }
  card += "</span><span id='btn-text-";
  card += id;
  card += "'>";
  if (strcmp(id, "door") == 0) {
    card += state ? "LOCK DOOR" : "UNLOCK";
  } else if (strstr(typeClass, "is-cooling")) {
    card += state ? "TURN OFF" : "START CHILL";
  } else if (strstr(typeClass, "is-bulb")) {
    card += state ? "TURN OFF" : "LIGHT UP";
  } else {
    card += state ? "TURN OFF" : "TURN ON";
  }
  card += "</span></button></div></div></div>";

  return card;
}

// =====================================================
// WEB PAGE BUILDER
// =====================================================

String makeWebPage()
{
  String html;
  html.reserve(16384);

  html += R"rawliteral(<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
  <title>AURA™ Home OS | Smart Living Intelligence</title>
  <link rel="preconnect" href="https://fonts.googleapis.com">
  <link rel="preconnect" href="https://fonts.gstatic.com" crossorigin>
  <link href="https://fonts.googleapis.com/css2?family=Plus+Jakarta+Sans:wght@300;400;500;600;700;800&family=Space+Grotesk:wght@500;700&display=swap" rel="stylesheet">
  <style>
    :root {
      --bg-dark: #0a0d14;
      --bg-card: rgba(18, 24, 38, 0.75);
      --bg-card-hover: rgba(26, 34, 54, 0.9);
      --border-subtle: rgba(255, 255, 255, 0.08);
      --border-active: rgba(59, 130, 246, 0.5);
      --text-main: #f8fafc;
      --text-muted: #94a3b8;
      --text-dim: #64748b;
      --accent-blue: #3b82f6;
      --accent-cyan: #06b6d4;
      --accent-emerald: #10b981;
      --accent-amber: #f59e0b;
      --glow-blue: 0 0 25px rgba(59, 130, 246, 0.35);
      --glow-emerald: 0 0 25px rgba(16, 185, 129, 0.35);
      --glow-amber: 0 0 30px rgba(245, 158, 11, 0.4);
      --glow-cyan: 0 0 30px rgba(6, 182, 212, 0.35);
      --radius-lg: 20px;
      --radius-md: 14px;
    }

    * {
      box-sizing: border-box;
      margin: 0;
      padding: 0;
      -webkit-tap-highlight-color: transparent;
    }

    body {
      font-family: 'Plus Jakarta Sans', -apple-system, BlinkMacSystemFont, sans-serif;
      background: var(--bg-dark);
      background-image: 
        radial-gradient(circle at 10% 10%, rgba(59, 130, 246, 0.12) 0%, transparent 40%),
        radial-gradient(circle at 90% 80%, rgba(139, 92, 246, 0.1) 0%, transparent 40%),
        radial-gradient(circle at 50% 50%, rgba(6, 182, 212, 0.05) 0%, transparent 60%);
      color: var(--text-main);
      min-height: 100vh;
      padding-bottom: 50px;
      overflow-x: hidden;
    }

    .app-header {
      position: sticky;
      top: 0;
      z-index: 100;
      backdrop-filter: blur(20px);
      -webkit-backdrop-filter: blur(20px);
      background: rgba(10, 13, 20, 0.85);
      border-bottom: 1px solid var(--border-subtle);
      padding: 16px 24px;
      display: flex;
      justify-content: space-between;
      align-items: center;
      box-shadow: 0 4px 20px rgba(0,0,0,0.4);
    }

    .brand-section {
      display: flex;
      align-items: center;
      gap: 14px;
    }

    .brand-logo {
      width: 44px;
      height: 44px;
      border-radius: 12px;
      background: linear-gradient(135deg, #2563eb, #06b6d4);
      display: flex;
      align-items: center;
      justify-content: center;
      box-shadow: var(--glow-blue);
    }

    .brand-logo svg {
      width: 26px;
      height: 26px;
      fill: none;
      stroke: #ffffff;
      stroke-width: 2;
    }

    .brand-text h1 {
      font-family: 'Space Grotesk', sans-serif;
      font-size: 20px;
      font-weight: 700;
      letter-spacing: -0.5px;
      display: flex;
      align-items: center;
      gap: 8px;
    }

    .brand-badge {
      font-size: 10px;
      text-transform: uppercase;
      letter-spacing: 1px;
      background: linear-gradient(90deg, #3b82f6, #06b6d4);
      color: #fff;
      padding: 2px 7px;
      border-radius: 20px;
      font-weight: 700;
    }

    .brand-sub {
      font-size: 12px;
      color: var(--text-muted);
    }

    .system-pills {
      display: flex;
      align-items: center;
      gap: 10px;
    }

    .status-pill {
      display: flex;
      align-items: center;
      gap: 8px;
      background: rgba(16, 185, 129, 0.12);
      border: 1px solid rgba(16, 185, 129, 0.3);
      padding: 6px 14px;
      border-radius: 30px;
      font-size: 12px;
      font-weight: 600;
      color: #34d399;
    }

    .status-dot {
      width: 8px;
      height: 8px;
      border-radius: 50%;
      background: #10b981;
      box-shadow: 0 0 10px #10b981;
      animation: pulse 2s infinite;
    }

    @keyframes pulse {
      0%, 100% { opacity: 1; transform: scale(1); }
      50% { opacity: 0.4; transform: scale(0.85); }
    }

    .ip-pill {
      font-size: 12px;
      color: var(--text-muted);
      background: rgba(255, 255, 255, 0.05);
      padding: 6px 12px;
      border-radius: 30px;
      border: 1px solid var(--border-subtle);
      font-family: monospace;
    }

    .main-content {
      max-width: 1200px;
      margin: 0 auto;
      padding: 24px 20px;
    }

    .hero-banner {
      position: relative;
      border-radius: var(--radius-lg);
      overflow: hidden;
      margin-bottom: 28px;
      min-height: 230px;
      display: flex;
      flex-direction: column;
      justify-content: flex-end;
      padding: 28px;
      border: 1px solid var(--border-subtle);
      box-shadow: 0 10px 30px rgba(0,0,0,0.5);
    }

    .hero-bg-img {
      position: absolute;
      top: 0;
      left: 0;
      width: 100%;
      height: 100%;
      object-fit: cover;
      filter: brightness(0.38) saturate(1.2);
      z-index: 1;
      transition: transform 10s ease;
    }

    .hero-banner:hover .hero-bg-img {
      transform: scale(1.05);
    }

    .hero-overlay {
      position: absolute;
      inset: 0;
      background: linear-gradient(180deg, rgba(10,13,20,0.2) 0%, rgba(10,13,20,0.92) 100%);
      z-index: 2;
    }

    .hero-content {
      position: relative;
      z-index: 3;
      display: flex;
      flex-wrap: wrap;
      justify-content: space-between;
      align-items: flex-end;
      gap: 20px;
    }

    .hero-info h2 {
      font-size: 26px;
      font-weight: 800;
      letter-spacing: -0.5px;
      margin-bottom: 4px;
      background: linear-gradient(90deg, #ffffff, #cbd5e1);
      -webkit-background-clip: text;
      -webkit-text-fill-color: transparent;
    }

    .hero-info p {
      color: var(--text-muted);
      font-size: 14px;
    }

    .telemetry-row {
      display: flex;
      gap: 16px;
      flex-wrap: wrap;
    }

    .telemetry-badge {
      background: rgba(18, 24, 38, 0.85);
      backdrop-filter: blur(12px);
      border: 1px solid rgba(255, 255, 255, 0.12);
      padding: 14px 20px;
      border-radius: var(--radius-md);
      display: flex;
      align-items: center;
      gap: 16px;
      box-shadow: 0 8px 24px rgba(0,0,0,0.3);
    }

    .telemetry-icon {
      width: 44px;
      height: 44px;
      border-radius: 12px;
      display: flex;
      align-items: center;
      justify-content: center;
    }

    .temp-icon-bg {
      background: rgba(245, 158, 11, 0.15);
      color: #fbbf24;
      box-shadow: var(--glow-amber);
    }

    .humidity-icon-bg {
      background: rgba(6, 182, 212, 0.15);
      color: #22d3ee;
      box-shadow: var(--glow-cyan);
    }

    .telemetry-icon svg {
      width: 24px;
      height: 24px;
      stroke-width: 2;
    }

    .telemetry-label {
      font-size: 11px;
      text-transform: uppercase;
      letter-spacing: 0.8px;
      color: var(--text-muted);
      margin-bottom: 2px;
    }

    .telemetry-value {
      font-size: 24px;
      font-weight: 700;
      font-family: 'Space Grotesk', sans-serif;
    }

    .telemetry-sub {
      font-size: 11px;
      color: #10b981;
      font-weight: 600;
    }

    .quick-actions {
      display: flex;
      gap: 12px;
      margin-bottom: 28px;
      overflow-x: auto;
      padding-bottom: 4px;
    }

    .quick-btn {
      background: rgba(255, 255, 255, 0.04);
      border: 1px solid var(--border-subtle);
      color: var(--text-muted);
      padding: 10px 18px;
      border-radius: 30px;
      font-size: 13px;
      font-weight: 600;
      cursor: pointer;
      display: flex;
      align-items: center;
      gap: 8px;
      white-space: nowrap;
      transition: all 0.25s ease;
    }

    .quick-btn:hover {
      background: rgba(255, 255, 255, 0.08);
      color: var(--text-main);
      border-color: rgba(255, 255, 255, 0.2);
      transform: translateY(-1px);
    }

    .section-title {
      font-size: 18px;
      font-weight: 700;
      letter-spacing: -0.3px;
      margin-bottom: 16px;
      display: flex;
      align-items: center;
      gap: 10px;
    }

    .section-title::before {
      content: '';
      display: inline-block;
      width: 4px;
      height: 18px;
      border-radius: 4px;
      background: var(--accent-blue);
    }

    .device-grid {
      display: grid;
      grid-template-columns: repeat(auto-fill, minmax(280px, 1fr));
      gap: 20px;
      margin-bottom: 32px;
    }

    .appliance-card {
      background: var(--bg-card);
      border: 1px solid var(--border-subtle);
      border-radius: var(--radius-lg);
      overflow: hidden;
      display: flex;
      flex-direction: column;
      transition: all 0.3s cubic-bezier(0.4, 0, 0.2, 1);
      position: relative;
      box-shadow: 0 4px 20px rgba(0,0,0,0.3);
    }

    .appliance-card:hover {
      background: var(--bg-card-hover);
      transform: translateY(-4px);
      box-shadow: 0 12px 30px rgba(0,0,0,0.45);
      border-color: rgba(255, 255, 255, 0.15);
    }

    .appliance-card.active {
      border-color: var(--border-active);
    }

    .appliance-card.active.is-fan {
      box-shadow: var(--glow-blue);
      border-color: rgba(59, 130, 246, 0.6);
    }

    .appliance-card.active.is-cooling {
      box-shadow: var(--glow-cyan);
      border-color: rgba(6, 182, 212, 0.6);
    }

    .appliance-card.active.is-bulb {
      box-shadow: var(--glow-amber);
      border-color: rgba(245, 158, 11, 0.6);
    }

    .appliance-card.active.is-door {
      box-shadow: var(--glow-emerald);
      border-color: rgba(16, 185, 129, 0.6);
    }

    .image-viewport {
      position: relative;
      width: 100%;
      height: 175px;
      overflow: hidden;
      background: #111827;
    }

    .real-photo {
      width: 100%;
      height: 100%;
      object-fit: cover;
      transition: transform 0.6s ease, filter 0.4s ease;
      filter: brightness(0.7) contrast(1.1);
    }

    .appliance-card:hover .real-photo {
      transform: scale(1.06);
    }

    .appliance-card.active .real-photo {
      filter: brightness(1.02) contrast(1.05) saturate(1.2);
    }

    .appliance-card.is-bulb.active .image-viewport::after {
      content: '';
      position: absolute;
      inset: 0;
      background: radial-gradient(circle at center, rgba(245, 158, 11, 0.35) 0%, transparent 70%);
      pointer-events: none;
      mix-blend-mode: screen;
    }

    .appliance-card.is-cooling.active .image-viewport::after {
      content: '';
      position: absolute;
      inset: 0;
      background: radial-gradient(circle at center, rgba(6, 182, 212, 0.3) 0%, transparent 70%);
      pointer-events: none;
      mix-blend-mode: screen;
    }

    .image-gradient {
      position: absolute;
      inset: 0;
      background: linear-gradient(180deg, rgba(0,0,0,0.2) 0%, rgba(18,24,38,0.95) 100%);
    }

    .card-top-badges {
      position: absolute;
      top: 12px;
      left: 12px;
      right: 12px;
      display: flex;
      justify-content: space-between;
      align-items: center;
      z-index: 5;
    }

    .device-category-chip {
      background: rgba(0, 0, 0, 0.65);
      backdrop-filter: blur(8px);
      border: 1px solid rgba(255, 255, 255, 0.12);
      color: #e2e8f0;
      font-size: 11px;
      font-weight: 600;
      padding: 4px 10px;
      border-radius: 20px;
      display: flex;
      align-items: center;
      gap: 6px;
    }

    .live-status-pill {
      font-size: 11px;
      font-weight: 700;
      padding: 4px 10px;
      border-radius: 20px;
      text-transform: uppercase;
      letter-spacing: 0.5px;
      display: flex;
      align-items: center;
      gap: 6px;
      background: rgba(100, 116, 139, 0.4);
      color: #cbd5e1;
      backdrop-filter: blur(8px);
      border: 1px solid rgba(255, 255, 255, 0.1);
      transition: all 0.3s ease;
    }

    .appliance-card.active .live-status-pill {
      background: rgba(16, 185, 129, 0.25);
      color: #34d399;
      border-color: rgba(16, 185, 129, 0.5);
      box-shadow: 0 0 12px rgba(16, 185, 129, 0.4);
    }

    .card-body {
      padding: 16px 18px 20px;
      display: flex;
      flex-direction: column;
      flex: 1;
      justify-content: space-between;
      gap: 16px;
    }

    .device-meta {
      display: flex;
      justify-content: space-between;
      align-items: flex-start;
    }

    .device-name {
      font-size: 16px;
      font-weight: 700;
      color: #ffffff;
      line-height: 1.3;
      margin-bottom: 3px;
    }

    .device-subtitle {
      font-size: 12px;
      color: var(--text-muted);
      display: flex;
      align-items: center;
      gap: 6px;
    }

    .pin-badge {
      font-size: 10px;
      background: rgba(255, 255, 255, 0.07);
      padding: 2px 6px;
      border-radius: 4px;
      font-family: monospace;
      color: #94a3b8;
    }

    .card-action-row {
      display: flex;
      align-items: center;
      justify-content: space-between;
      padding-top: 10px;
      border-top: 1px solid rgba(255, 255, 255, 0.06);
    }

    .state-label {
      font-size: 13px;
      font-weight: 600;
      color: var(--text-muted);
      transition: color 0.3s;
    }

    .appliance-card.active .state-label {
      color: #38bdf8;
    }

    .control-switch-btn {
      border: none;
      outline: none;
      cursor: pointer;
      padding: 10px 22px;
      border-radius: 30px;
      font-size: 13px;
      font-weight: 700;
      letter-spacing: 0.5px;
      display: flex;
      align-items: center;
      gap: 8px;
      transition: all 0.25s cubic-bezier(0.4, 0, 0.2, 1);
      background: rgba(255, 255, 255, 0.08);
      color: #cbd5e1;
      border: 1px solid rgba(255, 255, 255, 0.12);
    }

    .control-switch-btn:hover {
      background: rgba(255, 255, 255, 0.14);
      transform: scale(1.03);
    }

    .control-switch-btn:active {
      transform: scale(0.97);
    }

    .appliance-card.active .control-switch-btn {
      background: linear-gradient(135deg, #2563eb, #06b6d4);
      color: #ffffff;
      border-color: rgba(59, 130, 246, 0.8);
      box-shadow: 0 4px 15px rgba(37, 99, 235, 0.4);
    }

    .appliance-card.is-door .control-switch-btn {
      background: rgba(244, 63, 94, 0.15);
      color: #fb7185;
      border-color: rgba(244, 63, 94, 0.3);
    }

    .appliance-card.is-door.active .control-switch-btn {
      background: linear-gradient(135deg, #059669, #10b981);
      color: #ffffff;
      border-color: rgba(16, 185, 129, 0.8);
      box-shadow: 0 4px 15px rgba(16, 185, 129, 0.4);
    }

    .appliance-card.is-bulb.active .control-switch-btn {
      background: linear-gradient(135deg, #d97706, #f59e0b);
      color: #ffffff;
      border-color: rgba(245, 158, 11, 0.8);
      box-shadow: 0 4px 18px rgba(245, 158, 11, 0.45);
    }

    .app-footer {
      max-width: 1200px;
      margin: 20px auto 0;
      padding: 20px;
      border-top: 1px solid var(--border-subtle);
      display: flex;
      justify-content: space-between;
      align-items: center;
      flex-wrap: wrap;
      gap: 15px;
      font-size: 12px;
      color: var(--text-dim);
    }

    .system-diagnostics {
      display: flex;
      gap: 20px;
    }

    .diag-item {
      display: flex;
      align-items: center;
      gap: 6px;
    }

    @media (max-width: 768px) {
      .app-header { padding: 12px 16px; }
      .brand-text h1 { font-size: 17px; }
      .system-pills { display: none; }
      .hero-banner { padding: 20px; min-height: 220px; }
      .hero-info h2 { font-size: 20px; }
      .device-grid { grid-template-columns: 1fr; }
      .app-footer { flex-direction: column; text-align: center; }
    }
  </style>
</head>
<body>

  <!-- Top Navigation & Brand Header -->
  <header class="app-header">
    <div class="brand-section">
      <div class="brand-logo">
        <svg viewBox="0 0 24 24">
          <path d="M3 9l9-7 9 7v11a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2z"/>
          <polyline points="9 22 9 12 15 12 15 22"/>
        </svg>
      </div>
      <div class="brand-text">
        <h1>AURA™ Home OS <span class="brand-badge">PRO v2.5</span></h1>
        <div class="brand-sub">Enterprise IoT Smart Living System</div>
      </div>
    </div>

    <div class="system-pills">
      <div class="status-pill">
        <span class="status-dot"></span>
        <span>ESP32 Gateway Online</span>
      </div>
      <div class="ip-pill">IP: )rawliteral";

  html += WiFi.localIP().toString();

  html += R"rawliteral(</div>
      <div class="ip-pill" id="live-clock">--:--</div>
    </div>
  </header>

  <main class="main-content">
    
    <!-- Hero Room & Environment -->
    <section class="hero-banner">
      <img src="https://images.unsplash.com/photo-1600585154340-be6161a56a0c?auto=format&fit=crop&w=1200&q=80" alt="Living Suite" class="hero-bg-img">
      <div class="hero-overlay"></div>
      
      <div class="hero-content">
        <div class="hero-info">
          <h2>Living Suite & Environment</h2>
          <p>Real-time microclimate sensors and automation grid</p>
        </div>

        <div class="telemetry-row">
          <!-- Temperature Card -->
          <div class="telemetry-badge">
            <div class="telemetry-icon temp-icon-bg">
              <svg viewBox="0 0 24 24" fill="none" stroke="currentColor">
                <path d="M14 14.76V3.5a2.5 2.5 0 0 0-5 0v11.26a4.5 4.5 0 1 0 5 0z"/>
              </svg>
            </div>
            <div>
              <div class="telemetry-label">Ambient Temp</div>
              <div class="telemetry-value"><span id="temperature">)rawliteral";

  html += String(temperature, 1);

  html += R"rawliteral(</span> °C</div>
              <div class="telemetry-sub" id="temp-status">Comfort Zone</div>
            </div>
          </div>

          <!-- Humidity Card -->
          <div class="telemetry-badge">
            <div class="telemetry-icon humidity-icon-bg">
              <svg viewBox="0 0 24 24" fill="none" stroke="currentColor">
                <path d="M12 2.69l5.66 5.66a8 8 0 1 1-11.31 0z"/>
              </svg>
            </div>
            <div>
              <div class="telemetry-label">Air Humidity</div>
              <div class="telemetry-value"><span id="humidity">)rawliteral";

  html += String(humidity, 1);

  html += R"rawliteral(</span> %</div>
              <div class="telemetry-sub">Optimal Level</div>
            </div>
          </div>
        </div>
      </div>
    </section>

    <!-- Quick Action Scene Bar -->
    <div class="quick-actions">
      <button class="quick-btn" onclick="masterAction('all_on')">
        <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><circle cx="12" cy="12" r="10"/><polyline points="12 6 12 12 16 14"/></svg>
        Activate All Devices
      </button>
      <button class="quick-btn" onclick="masterAction('all_off')">
        <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><circle cx="12" cy="12" r="10"/><line x1="4.93" y1="4.93" x2="19.07" y2="19.07"/></svg>
        Master Standby (All Off)
      </button>
      <button class="quick-btn" onclick="masterAction('lights_off')">
        <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M9 18h6"/><path d="M10 22h4"/><path d="M15.09 14c.18-.98.65-1.74 1.41-2.5A4.65 4.65 0 0 0 18 8 6 6 0 0 0 6 8c0 1 .23 2.23 1.5 3.5A4.61 4.61 0 0 1 8.91 14"/></svg>
        Turn Off All Lights
      </button>
      <button class="quick-btn" onclick="masterAction('cooling_boost')">
        <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M12 2v20M2 12h20M4.93 4.93l14.14 14.14M4.93 19.07L19.07 4.93"/></svg>
        Max Cooling Boost
      </button>
    </div>

    <!-- Section 1: Aeration & Fans -->
    <h3 class="section-title">Ventilation &amp; High-Airflow Fans</h3>
    <div class="device-grid">
)rawliteral";

  html += renderDeviceCard(
    "fan1",
    "Master Aeration Fan 1",
    "Whisper-Quiet Aerofoil",
    "&#x1F300; Ventilation",
    "GPIO 16",
    "https://images.unsplash.com/photo-1595514535311-65f57339d6cb?auto=format&fit=crop&w=600&q=80",
    fan1State,
    "is-fan"
  );

  html += renderDeviceCard(
    "fan2",
    "Living Room Turbo Fan 2",
    "High CFM Air Circulation",
    "&#x1F300; Circulation",
    "GPIO 17",
    "https://images.unsplash.com/photo-1618221195710-dd6b41faaea6?auto=format&fit=crop&w=600&q=80",
    fan2State,
    "is-fan"
  );

  html += R"rawliteral(
    </div>

    <!-- Section 2: Peltier Thermoelectric Climate -->
    <h3 class="section-title">Peltier Active Thermoelectric Cooling</h3>
    <div class="device-grid">
)rawliteral";

  html += renderDeviceCard(
    "peltier1",
    "Peltier AC Cooling Unit #1",
    "Dual TEC Chill Module",
    "&#x2744;&#xFE0F; Chiller Core",
    "GPIO 18",
    "https://images.unsplash.com/photo-1621905251189-08b45d6a269e?auto=format&fit=crop&w=600&q=80",
    peltier1State,
    "is-cooling"
  );

  html += renderDeviceCard(
    "peltier2",
    "Peltier AC Cooling Unit #2",
    "Sub-Ambient Stage",
    "&#x2744;&#xFE0F; Frost Tech",
    "GPIO 19",
    "https://images.unsplash.com/photo-1585338107529-13afc5f02586?auto=format&fit=crop&w=600&q=80",
    peltier2State,
    "is-cooling"
  );

  html += R"rawliteral(
    </div>

    <!-- Section 3: Smart Security Access & Lighting -->
    <div style="display: grid; grid-template-columns: repeat(auto-fit, minmax(280px, 1fr)); gap: 20px; margin-bottom: 24px;">
      
      <!-- Security Column -->
      <div>
        <h3 class="section-title">Biometric Access &amp; Perimeter</h3>
)rawliteral";

  html += renderDeviceCard(
    "door",
    "Main Entrance Smart Deadbolt",
    "Heavy Solenoid Lock",
    "&#x1F6E1;&#xFE0F; Access Security",
    "GPIO 23",
    "https://images.unsplash.com/photo-1558002038-1055907df827?auto=format&fit=crop&w=600&q=80",
    doorState,
    "is-door"
  );

  html += R"rawliteral(
      </div>

      <!-- High-Power Bulbs Column -->
      <div>
        <h3 class="section-title">High-Power Illumination</h3>
        <div style="display: grid; grid-template-columns: 1fr; gap: 20px;">
)rawliteral";

  html += renderDeviceCard(
    "bulb1",
    "Ambient Edison Bulb #1 (100W)",
    "Warm 2700K Filament",
    "&#x1F4A1; 100W Warm Glow",
    "GPIO 25",
    "https://images.unsplash.com/photo-1513506003901-1e6a229e2d15?auto=format&fit=crop&w=600&q=80",
    bulb1State,
    "is-bulb"
  );

  html += renderDeviceCard(
    "bulb2",
    "Lustre Chandelier Bulb #2 (100W)",
    "High Output Lighting",
    "&#x1F4A1; 100W Accent",
    "GPIO 26",
    "https://images.unsplash.com/photo-1507473885765-e6ed057f782c?auto=format&fit=crop&w=600&q=80",
    bulb2State,
    "is-bulb"
  );

  html += R"rawliteral(
        </div>
      </div>

    </div>

  </main>

  <!-- Diagnostics Footer -->
  <footer class="app-footer">
    <div>
      <strong>AURA™ Intelligent Automation Framework</strong> &bull; Powered by ESP32 Dual-Core SoC
    </div>
    <div class="system-diagnostics">
      <div class="diag-item"><span>Relays:</span> <strong style="color: #38bdf8;">7 Channels Active</strong></div>
      <div class="diag-item"><span>OLED:</span> <strong style="color: #34d399;">SSD1306 128x64 I2C</strong></div>
      <div class="diag-item"><span>DHT11:</span> <strong style="color: #fbbf24;">Calibrated</strong></div>
    </div>
  </footer>

  <script>
    // State tracker initialized automatically from active card classes
    const stateMap = {};
    ['fan1', 'fan2', 'peltier1', 'peltier2', 'door', 'bulb1', 'bulb2'].forEach(id => {
      const card = document.getElementById('card-' + id);
      stateMap[id] = (card && card.classList.contains('active')) ? 1 : 0;
    });

    // Real-time live clock
    function updateClock() {
      const now = new Date();
      const timeStr = now.toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' });
      const clockEl = document.getElementById('live-clock');
      if (clockEl) clockEl.textContent = timeStr;
    }
    setInterval(updateClock, 1000);
    updateClock();

    // Visual UI updater for a given device
    function renderDeviceUI(id, state) {
      stateMap[id] = state ? 1 : 0;
      const card = document.getElementById('card-' + id);
      const badge = document.getElementById('badge-' + id);
      const stateLabel = document.getElementById('state-' + id);
      const btnText = document.getElementById('btn-text-' + id);
      const btnIcon = document.getElementById('btn-icon-' + id);

      if (!card) return;

      if (state) {
        card.classList.add('active');
        if (badge) {
          badge.textContent = id === 'door' ? 'UNLOCKED' : 'ONLINE';
          badge.style.color = id === 'door' ? '#f43f5e' : '#34d399';
        }
        if (stateLabel) {
          stateLabel.textContent = id === 'door' ? 'Door Unlocked (Open)' : 'Active & Running';
          stateLabel.style.color = id === 'door' ? '#f43f5e' : '#38bdf8';
        }
        if (btnText) btnText.textContent = id === 'door' ? 'LOCK DOOR' : 'TURN OFF';
        if (btnIcon) btnIcon.innerHTML = id === 'door' ? '&#x1F512;' : '&#x23FB;';
      } else {
        card.classList.remove('active');
        if (badge) {
          badge.textContent = id === 'door' ? 'LOCKED' : 'STANDBY';
          badge.style.color = '#cbd5e1';
        }
        if (stateLabel) {
          stateLabel.textContent = id === 'door' ? 'Perimeter Secured' : 'Device Offline';
          stateLabel.style.color = '#94a3b8';
        }
        if (btnText) {
          if (id === 'door') btnText.textContent = 'UNLOCK';
          else if (id.startsWith('peltier')) btnText.textContent = 'START CHILL';
          else if (id.startsWith('bulb')) btnText.textContent = 'LIGHT UP';
          else btnText.textContent = 'TURN ON';
        }
        if (btnIcon) {
          if (id === 'door') btnIcon.innerHTML = '&#x1F513;';
          else if (id.startsWith('peltier')) btnIcon.innerHTML = '&#x2744;&#xFE0F;';
          else if (id.startsWith('bulb')) btnIcon.innerHTML = '&#x1F4A1;';
          else btnIcon.innerHTML = '&#x26A1;';
        }
      }
    }

    // Instant optimistic toggle + background fetch
    function toggleDevice(id) {
      const currentState = stateMap[id] || 0;
      const targetState = currentState ? 0 : 1;
      
      // Instant visual feedback
      renderDeviceUI(id, targetState);

      // Async fetch to ESP32 /control endpoint
      fetch('/control?device=' + id + '&state=' + targetState)
        .catch(err => {
          console.warn('Network request error:', err);
        });
    }

    // Master Quick Action Scenes
    function masterAction(action) {
      if (action === 'all_on') {
        ['fan1', 'fan2', 'peltier1', 'peltier2', 'bulb1', 'bulb2'].forEach(dev => {
          if (!stateMap[dev]) toggleDevice(dev);
        });
      } else if (action === 'all_off') {
        Object.keys(stateMap).forEach(dev => {
          if (stateMap[dev]) toggleDevice(dev);
        });
      } else if (action === 'lights_off') {
        ['bulb1', 'bulb2'].forEach(dev => {
          if (stateMap[dev]) toggleDevice(dev);
        });
      } else if (action === 'cooling_boost') {
        ['peltier1', 'peltier2', 'fan1', 'fan2'].forEach(dev => {
          if (!stateMap[dev]) toggleDevice(dev);
        });
      }
    }

    // Real-time sensor & state synchronization
    function pollSensor() {
      fetch('/sensor')
        .then(res => res.json())
        .then(data => {
          if (data.temperature !== undefined) {
            const tempEl = document.getElementById('temperature');
            if (tempEl) tempEl.textContent = Number(data.temperature).toFixed(1);
            
            const tempStatus = document.getElementById('temp-status');
            if (tempStatus) {
              if (data.temperature > 30) tempStatus.textContent = 'High Temperature';
              else if (data.temperature < 20) tempStatus.textContent = 'Cool Environment';
              else tempStatus.textContent = 'Comfort Zone';
            }
          }
          if (data.humidity !== undefined) {
            const humEl = document.getElementById('humidity');
            if (humEl) humEl.textContent = Number(data.humidity).toFixed(1);
          }

          // Real-time multi-client synchronization
          ['fan1', 'fan2', 'peltier1', 'peltier2', 'door', 'bulb1', 'bulb2'].forEach(dev => {
            if (data[dev] !== undefined) {
              renderDeviceUI(dev, data[dev] == 1);
            }
          });
        })
        .catch(() => {});
    }

    setInterval(pollSensor, 2500);
  </script>
</body>
</html>
)rawliteral";

  return html;
}

// =====================================================
// ROOT PAGE HANDLER
// =====================================================

void handleRoot()
{
  server.send(
    200,
    "text/html",
    makeWebPage()
  );
}

// =====================================================
// CONTROL DEVICES HANDLER
// =====================================================

void handleControl()
{
  String device = server.arg("device");
  String state = server.arg("state");

  bool value = (state == "1");

  if (device == "fan1")
  {
    fan1State = value;
  }
  else if (device == "fan2")
  {
    fan2State = value;
  }
  else if (device == "peltier1")
  {
    peltier1State = value;
  }
  else if (device == "peltier2")
  {
    peltier2State = value;
  }
  else if (device == "door")
  {
    doorState = value;
  }
  else if (device == "bulb1")
  {
    bulb1State = value;
  }
  else if (device == "bulb2")
  {
    bulb2State = value;
  }

  updateRelays();

  server.send(
    200,
    "text/plain",
    "OK"
  );
}

// =====================================================
// SENSOR & TELEMETRY API
// =====================================================

void handleSensor()
{
  String json = "{";

  json += "\"temperature\":" + String(temperature, 1);
  json += ",\"humidity\":" + String(humidity, 1);
  json += ",\"fan1\":" + String(fan1State ? 1 : 0);
  json += ",\"fan2\":" + String(fan2State ? 1 : 0);
  json += ",\"peltier1\":" + String(peltier1State ? 1 : 0);
  json += ",\"peltier2\":" + String(peltier2State ? 1 : 0);
  json += ",\"door\":" + String(doorState ? 1 : 0);
  json += ",\"bulb1\":" + String(bulb1State ? 1 : 0);
  json += ",\"bulb2\":" + String(bulb2State ? 1 : 0);

  json += "}";

  server.send(
    200,
    "application/json",
    json
  );
}

// =====================================================
// WIFI CONNECT
// =====================================================

void connectWiFi()
{
  WiFi.mode(WIFI_STA);

  WiFi.begin(
    ssid,
    password
  );

  Serial.print("Connecting to WiFi");

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("AURA Home OS WiFi Connected!");
  Serial.print("Gateway IP Address: http://");
  Serial.println(WiFi.localIP());
}

// =====================================================
// SETUP
// =====================================================

void setup()
{
  Serial.begin(115200);

  // Relay pin directions
  pinMode(FAN1_RELAY, OUTPUT);
  pinMode(FAN2_RELAY, OUTPUT);

  pinMode(PELTIER1_RELAY, OUTPUT);
  pinMode(PELTIER2_RELAY, OUTPUT);

  pinMode(DOOR_RELAY, OUTPUT);

  pinMode(BULB1_RELAY, OUTPUT);
  pinMode(BULB2_RELAY, OUTPUT);

  // Initialize all relays to OFF state
  fan1State = false;
  fan2State = false;

  peltier1State = false;
  peltier2State = false;

  doorState = false;

  bulb1State = false;
  bulb2State = false;

  updateRelays();

  // DHT sensor initialization
  dht.begin();

  // OLED display initialization
  Wire.begin(
    OLED_SDA,
    OLED_SCL
  );

  if (!display.begin(
        SSD1306_SWITCHCAPVCC,
        0x3C))
  {
    Serial.println(
      "OLED not found!"
    );
  }
  else
  {
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);

    display.setTextSize(2);
    display.setCursor(12, 18);
    display.println("AURA");

    display.setCursor(12, 42);
    display.println("HOME OS");

    display.display();
    delay(2000);
  }

  // Connect to Local WiFi network
  connectWiFi();

  // Web server routing
  server.on(
    "/",
    handleRoot
  );

  server.on(
    "/control",
    handleControl
  );

  server.on(
    "/sensor",
    handleSensor
  );

  server.begin();

  Serial.println(
    "AURA Home OS Web Server Started!"
  );
}

// =====================================================
// MAIN LOOP
// =====================================================

void loop()
{
  server.handleClient();

  readDHT();

  updateOLED();
}
