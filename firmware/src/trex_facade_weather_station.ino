#include <WiFi.h>
#include <WebServer.h>
#include <WiFiUdp.h>
#include <DNSServer.h>
#include <Preferences.h>
#include <Wire.h>

#include <Adafruit_BME280.h>
#include <Adafruit_VEML7700.h>

#include "esp_sleep.h"
#include "driver/gpio.h"

// ============================================================
// T-REX WALL STATION
// Firmware: 1.2.2
// Hardware: 1.0
// ESP32-C3
// ============================================================

#define TREX_STATION_FW_VERSION "1.2.2"
#define TREX_STATION_HW_VERSION "1.0"

// ============================================================
// PINY
// ============================================================

#define SDA_PIN          8
#define SCL_PIN          9
#define RAIN_PIN         4
#define BATTERY_ADC_PIN  3

// ============================================================
// DZIELNIK NAPIĘCIA BATERII
//
// BAT+ ---- 220k ----+---- GPIO3
//                   |
//                  100k
//                   |
//                  GND
//
// Kondensator:
// GPIO3 ---- 100 nF ---- GND
// ============================================================

#define BATTERY_R1 220000.0f
#define BATTERY_R2 100000.0f

// ============================================================
// CZASY
// ============================================================
const uint32_t CONFIG_TIME_MS =
  10UL * 60UL * 1000UL;

const uint32_t AUTO_INTERVAL_SEC =
  10UL * 60UL;

const uint32_t NETWORK_LISTEN_MS =
  8000;

// ============================================================
// UDP
// ============================================================

const uint16_t TREX_UDP_PORT = 4210;

// ============================================================
// OBIEKTY
// ============================================================

Adafruit_BME280 bme;
Adafruit_VEML7700 veml;

WebServer server(80);
DNSServer dnsServer;
WiFiUDP udp;
Preferences prefs;

// ============================================================
// STAN
// ============================================================

bool bmeOK = false;
bool vemlOK = false;
bool configMode = false;

String stationSerial;
String apName;

String savedSSID;
String savedPassword;

String cachedWiFiOptions =
  "<option value=''>Kliknij SKANUJ SIECI</option>";

uint32_t configStartedAt = 0;
uint32_t lastTelemetryAt = 0;

// ============================================================
// TABELA KALIBRACYJNA BATERII
//
// rawVoltage  = odczyt ESP po przeliczeniu dzielnika
// realVoltage = rzeczywiste napięcie z zasilacza / baterii
//
// Dane z kalibracji konkretnego prototypu.
// ============================================================

struct BatteryCalibrationPoint {
  float rawVoltage;
  float realVoltage;
};

const BatteryCalibrationPoint batteryCalibrationTable[] = {

  {3.128f, 3.200f},
  {3.313f, 3.400f},
  {3.536f, 3.600f},
  {3.730f, 3.800f},
  {3.977f, 4.000f},
  {4.179f, 4.200f},
  {4.420f, 4.500f}

};

const int BATTERY_CALIBRATION_POINTS =
  sizeof(batteryCalibrationTable) /
  sizeof(batteryCalibrationTable[0]);

// ============================================================
// NUMER SERYJNY
// ============================================================

String createSerial() {

  uint64_t chipID =
    ESP.getEfuseMac();

  uint32_t shortID =
    (uint32_t)(chipID & 0xFFFFFFFF);

  char buffer[24];

  snprintf(
    buffer,
    sizeof(buffer),
    "TRX-FS-%08lX",
    (unsigned long)shortID
  );

  return String(buffer);
}

// ============================================================
// RAINPOINT
//
// PODLACZENIE:
// ESP 3V3 ---- RainPoint ---- GPIO4
//
// INPUT_PULLDOWN
//
// HIGH = SUCHO   (styk zamkniety)
// LOW  = DESZCZ  (styk otwarty)
// ============================================================

bool isRaining() {

  return digitalRead(RAIN_PIN) == LOW;
}

// ============================================================
// SUROWY POMIAR BATERII
//
// Zwraca napięcie przeliczone przez dzielnik,
// ale jeszcze BEZ tabeli kalibracyjnej.
// ============================================================

float readBatteryVoltageRaw() {

  const int samples = 32;

  uint32_t totalMv = 0;

  for (int i = 0; i < samples; i++) {

    totalMv +=
      analogReadMilliVolts(
        BATTERY_ADC_PIN
      );

    delay(2);
  }

  float averageMv =
    totalMv / (float)samples;

  float adcVoltage =
    averageMv / 1000.0f;

  float dividerRatio =
    (BATTERY_R1 + BATTERY_R2) /
    BATTERY_R2;

  float rawBatteryVoltage =
    adcVoltage *
    dividerRatio;

  return rawBatteryVoltage;
}

// ============================================================
// INTERPOLACJA TABELI KALIBRACYJNEJ
// ============================================================

float calibrateBatteryVoltage(
  float rawVoltage
) {

  // --------------------------------------------------------
  // Poniżej najniższego punktu:
  // ekstrapolacja liniowa z pierwszych dwóch punktów.
  // --------------------------------------------------------

  if (
    rawVoltage <=
    batteryCalibrationTable[0].rawVoltage
  ) {

    float x1 =
      batteryCalibrationTable[0].rawVoltage;

    float y1 =
      batteryCalibrationTable[0].realVoltage;

    float x2 =
      batteryCalibrationTable[1].rawVoltage;

    float y2 =
      batteryCalibrationTable[1].realVoltage;

    return y1 +
      (rawVoltage - x1) *
      (y2 - y1) /
      (x2 - x1);
  }

  // --------------------------------------------------------
  // Szukanie dwóch punktów otaczających odczyt.
  // --------------------------------------------------------

  for (
    int i = 0;
    i < BATTERY_CALIBRATION_POINTS - 1;
    i++
  ) {

    float x1 =
      batteryCalibrationTable[i].rawVoltage;

    float y1 =
      batteryCalibrationTable[i].realVoltage;

    float x2 =
      batteryCalibrationTable[i + 1].rawVoltage;

    float y2 =
      batteryCalibrationTable[i + 1].realVoltage;

    if (
      rawVoltage >= x1 &&
      rawVoltage <= x2
    ) {

      float corrected =
        y1 +
        (rawVoltage - x1) *
        (y2 - y1) /
        (x2 - x1);

      return corrected;
    }
  }

  // --------------------------------------------------------
  // Powyżej najwyższego punktu:
  // ekstrapolacja liniowa z dwóch ostatnich punktów.
  // --------------------------------------------------------

  int last =
    BATTERY_CALIBRATION_POINTS - 1;

  float x1 =
    batteryCalibrationTable[last - 1].rawVoltage;

  float y1 =
    batteryCalibrationTable[last - 1].realVoltage;

  float x2 =
    batteryCalibrationTable[last].rawVoltage;

  float y2 =
    batteryCalibrationTable[last].realVoltage;

  return y1 +
    (rawVoltage - x1) *
    (y2 - y1) /
    (x2 - x1);
}

// ============================================================
// FINALNY POMIAR BATERII
// ============================================================

float readBatteryVoltage() {

  float rawVoltage =
    readBatteryVoltageRaw();

  float correctedVoltage =
    calibrateBatteryVoltage(
      rawVoltage
    );

  return correctedVoltage;
}

// ============================================================
// PROCENT BATERII
//
// Przybliżona charakterystyka 1S Li-Ion.
// ============================================================

int batteryPercent(float v) {

  if (v >= 4.20f) return 100;
  if (v >= 4.10f) return 90;
  if (v >= 4.00f) return 80;
  if (v >= 3.90f) return 70;
  if (v >= 3.82f) return 60;
  if (v >= 3.75f) return 50;
  if (v >= 3.70f) return 40;
  if (v >= 3.65f) return 30;
  if (v >= 3.55f) return 20;
  if (v >= 3.40f) return 10;
  if (v >= 3.20f) return 5;

  return 0;
}

// ============================================================
// STATUS BATERII
// ============================================================

String batteryStatus(float v) {

  if (v >= 3.60f) {
    return "OK";
  }

  if (v >= 3.35f) {
    return "LOW";
  }

  return "CRITICAL";
}

// ============================================================
// JSON
// ============================================================

String buildTelemetryJSON(
  String type = "telemetry"
) {

  float temperature = NAN;
  float humidity = NAN;
  float pressure = NAN;
  float lux = NAN;

  if (bmeOK) {

    temperature =
      bme.readTemperature();

    humidity =
      bme.readHumidity();

    pressure =
      bme.readPressure() /
      100.0F;
  }

  if (vemlOK) {

    lux =
      veml.readLux();
  }

  bool rain =
    isRaining();

  float batteryRaw =
    readBatteryVoltageRaw();

  float batteryVoltage =
    calibrateBatteryVoltage(
      batteryRaw
    );

  int batteryPct =
    batteryPercent(
      batteryVoltage
    );

  String batteryState =
    batteryStatus(
      batteryVoltage
    );

  String json = "{";

  json +=
    "\"protocol\":\"trex-wall/1\",";

  json +=
    "\"type\":\"" +
    type +
    "\",";

  json +=
    "\"serial\":\"" +
    stationSerial +
    "\",";

  json +=
    "\"fw_version\":\"" +
    String(TREX_STATION_FW_VERSION) +
    "\",";

  json +=
    "\"hw_version\":\"" +
    String(TREX_STATION_HW_VERSION) +
    "\",";

  // --------------------------------------------------------
  // IP
  // --------------------------------------------------------

  if (
    WiFi.status() ==
    WL_CONNECTED
  ) {

    json +=
      "\"ip\":\"";

    json +=
      WiFi.localIP().toString();

    json +=
      "\",";

  } else {

    json +=
      "\"ip\":null,";
  }

  // --------------------------------------------------------
  // TEMPERATURA
  // --------------------------------------------------------

  json +=
    "\"temperature_c\":";

  if (isnan(temperature)) {
    json += "null";
  } else {
    json += String(temperature, 2);
  }

  // --------------------------------------------------------
  // WILGOTNOŚĆ
  // --------------------------------------------------------

  json +=
    ",\"humidity_pct\":";

  if (isnan(humidity)) {
    json += "null";
  } else {
    json += String(humidity, 2);
  }

  // --------------------------------------------------------
  // CIŚNIENIE
  // --------------------------------------------------------

  json +=
    ",\"pressure_hpa\":";

  if (isnan(pressure)) {
    json += "null";
  } else {
    json += String(pressure, 2);
  }

  // --------------------------------------------------------
  // LUX
  // --------------------------------------------------------

  json +=
    ",\"lux\":";

  if (isnan(lux)) {
    json += "null";
  } else {
    json += String(lux, 1);
  }

  // --------------------------------------------------------
  // DESZCZ
  // --------------------------------------------------------

  json +=
    ",\"rain\":";

  json +=
    rain ?
    "true" :
    "false";

  // --------------------------------------------------------
  // BATERIA RAW
  //
  // Zostawiamy diagnostycznie.
  // Później można usunąć z produkcyjnego protokołu.
  // --------------------------------------------------------

  json +=
    ",\"battery_raw_v\":";

  json +=
    String(
      batteryRaw,
      3
    );

  // --------------------------------------------------------
  // BATERIA SKALIBROWANA
  // --------------------------------------------------------

  json +=
    ",\"battery_v\":";

  json +=
    String(
      batteryVoltage,
      3
    );

  json +=
    ",\"battery_pct\":";

  json +=
    String(
      batteryPct
    );

  json +=
    ",\"battery_status\":\"";

  json +=
    batteryState;

  json +=
    "\"";

  // --------------------------------------------------------
  // RSSI
  // --------------------------------------------------------

  json +=
    ",\"rssi\":";

  if (
    WiFi.status() ==
    WL_CONNECTED
  ) {

    json +=
      String(
        WiFi.RSSI()
      );

  } else {

    json +=
      "null";
  }

  json += "}";

  return json;
}

// ============================================================
// BROADCAST
// ============================================================

IPAddress getBroadcastAddress() {

  IPAddress ip =
    WiFi.localIP();

  IPAddress mask =
    WiFi.subnetMask();

  IPAddress broadcast;

  for (int i = 0; i < 4; i++) {

    broadcast[i] =
      ip[i] |
      (~mask[i] & 0xFF);
  }

  return broadcast;
}

// ============================================================
// WYSYŁANIE TELEMETRII
// ============================================================

void broadcastTelemetry() {

  if (
    WiFi.status() !=
    WL_CONNECTED
  ) {

    Serial.println(
      "Brak WiFi - telemetryka niewyslana."
    );

    return;
  }

  String json =
    buildTelemetryJSON(
      "telemetry"
    );

  IPAddress broadcast =
    getBroadcastAddress();

  udp.beginPacket(
    broadcast,
    TREX_UDP_PORT
  );

  udp.print(json);

  udp.endPacket();

  Serial.println();
  Serial.println(
    "===== T-REX TX ====="
  );

  Serial.println(json);

  Serial.println(
    "====================="
  );

  Serial.println();
}

// ============================================================
// UDP / T-REX HOME
// ============================================================

void handleUDP() {

  int packetSize =
    udp.parsePacket();

  if (!packetSize) {
    return;
  }

  char buffer[256];

  int len =
    udp.read(
      buffer,
      sizeof(buffer) - 1
    );

  if (len <= 0) {
    return;
  }

  buffer[len] = 0;

  String message =
    String(buffer);

  message.trim();

  Serial.print(
    "UDP RX: "
  );

  Serial.println(
    message
  );

  if (
    message ==
    "TREX_DISCOVER_V1"
  ) {

    String answer =
      buildTelemetryJSON(
        "announce"
      );

    udp.beginPacket(
      udp.remoteIP(),
      udp.remotePort()
    );

    udp.print(answer);

    udp.endPacket();

    Serial.println(
      "HOME DISCOVERY -> odpowiedz wyslana"
    );

    return;
  }

  if (
    message ==
    "TREX_GET:*"
  ) {

    String answer =
      buildTelemetryJSON(
        "telemetry"
      );

    udp.beginPacket(
      udp.remoteIP(),
      udp.remotePort()
    );

    udp.print(answer);

    udp.endPacket();

    return;
  }

  String request =
    "TREX_GET:" +
    stationSerial;

  if (
    message ==
    request
  ) {

    String answer =
      buildTelemetryJSON(
        "telemetry"
      );

    udp.beginPacket(
      udp.remoteIP(),
      udp.remotePort()
    );

    udp.print(answer);

    udp.endPacket();

    Serial.println(
      "HOME -> odpowiedz dla serialu"
    );
  }
}

// ============================================================
// WIFI CONFIG
// ============================================================

void loadWiFiConfig() {

  prefs.begin(
    "trexwall",
    true
  );

  savedSSID =
    prefs.getString(
      "ssid",
      ""
    );

  savedPassword =
    prefs.getString(
      "pass",
      ""
    );

  prefs.end();

  if (
    savedSSID.length() > 0
  ) {

    Serial.print(
      "Zapamietana siec: "
    );

    Serial.println(
      savedSSID
    );

  } else {

    Serial.println(
      "Brak zapisanej sieci WiFi."
    );
  }
}

// ============================================================
// ŁĄCZENIE Z WIFI
// ============================================================

bool connectWiFi(
  uint32_t timeoutMs = 10000
) {

  if (
    savedSSID.length() == 0
  ) {

    Serial.println(
      "Brak danych domowego WiFi."
    );

    return false;
  }

  Serial.print(
    "Proba polaczenia z: "
  );

  Serial.println(
    savedSSID
  );

  WiFi.begin(
    savedSSID.c_str(),
    savedPassword.c_str()
  );

  uint32_t start =
    millis();

  while (
    WiFi.status() !=
      WL_CONNECTED &&
    millis() - start <
      timeoutMs
  ) {

    delay(250);
    Serial.print(".");
  }

  Serial.println();

  if (
    WiFi.status() ==
    WL_CONNECTED
  ) {

    Serial.println(
      "WiFi DOMOWE: POLACZONE"
    );

    Serial.print(
      "SSID: "
    );

    Serial.println(
      WiFi.SSID()
    );

    Serial.print(
      "IP: "
    );

    Serial.println(
      WiFi.localIP()
    );

    Serial.print(
      "RSSI: "
    );

    Serial.print(
      WiFi.RSSI()
    );

    Serial.println(
      " dBm"
    );

    return true;
  }

  Serial.println(
    "WiFi DOMOWE: POLACZENIE NIEUDANE"
  );

  WiFi.disconnect(
    false,
    false
  );

  delay(300);

  return false;
}

// ============================================================
// SKANOWANIE WIFI
// ============================================================

void scanWiFiNetworks() {

  Serial.println(
    "Skanowanie WiFi..."
  );

  int networks =
    WiFi.scanNetworks(
      false,
      true
    );

  cachedWiFiOptions = "";

  if (
    networks <= 0
  ) {

    cachedWiFiOptions =
      "<option value=''>Brak znalezionych sieci</option>";

    Serial.println(
      "Brak znalezionych sieci."
    );

    return;
  }

  Serial.print(
    "Znaleziono sieci: "
  );

  Serial.println(
    networks
  );

  for (
    int i = 0;
    i < networks;
    i++
  ) {

    String ssid =
      WiFi.SSID(i);

    if (
      ssid.length() == 0
    ) {
      continue;
    }

    cachedWiFiOptions +=
      "<option value='";

    cachedWiFiOptions +=
      ssid;

    cachedWiFiOptions +=
      "'>";

    cachedWiFiOptions +=
      ssid;

    cachedWiFiOptions +=
      " (";

    cachedWiFiOptions +=
      String(
        WiFi.RSSI(i)
      );

    cachedWiFiOptions +=
      " dBm)</option>";
  }

  WiFi.scanDelete();
}

// ============================================================
// PANEL WWW
// ============================================================

String htmlPage() {

  float batteryRaw =
    readBatteryVoltageRaw();

  float batteryVoltage =
    calibrateBatteryVoltage(
      batteryRaw
    );

  int batteryPct =
    batteryPercent(
      batteryVoltage
    );

  String batteryState =
    batteryStatus(
      batteryVoltage
    );

  String html;

  html.reserve(9500);

  html +=
    "<!DOCTYPE html><html><head>";

  html +=
    "<meta charset='UTF-8'>";

  html +=
    "<meta name='viewport' content='width=device-width,initial-scale=1'>";

  html +=
    "<title>T-REX Wall Station</title>";

  html +=
    "<style>";

  html +=
    "body{font-family:Arial;background:#101010;color:#eee;margin:0;padding:20px;}";

  html +=
    ".wrap{max-width:600px;margin:auto;}";

  html +=
    ".box{background:#1c1c1c;padding:18px;margin-bottom:14px;border-radius:12px;}";

  html +=
    ".serial{font-size:22px;font-weight:bold;}";

  html +=
    ".ok{color:#6ee787;}";

  html +=
    ".warn{color:#ffd166;}";

  html +=
    ".bad{color:#ff6b6b;}";

  html +=
    ".battery{font-size:30px;font-weight:bold;margin-top:8px;}";

  html +=
    ".muted{color:#999;font-size:13px;}";

  html +=
    "button,input,select{width:100%;padding:13px;margin-top:9px;box-sizing:border-box;font-size:16px;border:0;border-radius:7px;}";

  html +=
    "button{font-weight:bold;cursor:pointer;}";

  html +=
    "a{text-decoration:none;}";

  html +=
    "</style>";

  html +=
    "</head><body><div class='wrap'>";

  html +=
    "<h1>T-REX Wall Station</h1>";

  // --------------------------------------------------------
  // IDENTYFIKACJA
  // --------------------------------------------------------

  html +=
    "<div class='box'>";

  html +=
    "Numer seryjny";

  html +=
    "<div class='serial'>";

  html +=
    stationSerial;

  html +=
    "</div><br>";

  html +=
    "Firmware: <b>";

  html +=
    TREX_STATION_FW_VERSION;

  html +=
    "</b><br>";

  html +=
    "Hardware: <b>";

  html +=
    TREX_STATION_HW_VERSION;

  html +=
    "</b><br><br>";

  html +=
    "Siec serwisowa:<br><b>";

  html +=
    apName;

  html +=
    "</b></div>";

  // --------------------------------------------------------
  // CZUJNIKI
  // --------------------------------------------------------

  html +=
    "<div class='box'><h3>Czujniki</h3>";

  if (bmeOK) {

    html +=
      "Temperatura: <b>";

    html +=
      String(
        bme.readTemperature(),
        1
      );

    html +=
      " C</b><br>";

    html +=
      "Wilgotnosc: <b>";

    html +=
      String(
        bme.readHumidity(),
        1
      );

    html +=
      " %</b><br>";

    html +=
      "Cisnienie: <b>";

    html +=
      String(
        bme.readPressure() /
        100.0F,
        1
      );

    html +=
      " hPa</b><br>";

  } else {

    html +=
      "<span class='bad'>BME280 BLAD</span><br>";
  }

  if (vemlOK) {

    html +=
      "Swiatlo: <b>";

    html +=
      String(
        veml.readLux(),
        1
      );

    html +=
      " lux</b><br>";

  } else {

    html +=
      "<span class='bad'>VEML7700 BLAD</span><br>";
  }

  html +=
    "Deszcz: <b>";

  html +=
    isRaining() ?
    "TAK" :
    "NIE";

  html +=
    "</b></div>";

  // --------------------------------------------------------
  // BATERIA
  // --------------------------------------------------------

  html +=
    "<div class='box'><h3>Bateria</h3>";

  html +=
    "<div class='battery'>";

  html +=
    String(
      batteryPct
    );

  html +=
    " %</div>";

  html +=
    "Napiecie: <b>";

  html +=
    String(
      batteryVoltage,
      3
    );

  html +=
    " V</b><br>";

  html +=
    "Stan: <b>";

  html +=
    batteryState;

  html +=
    "</b><br><br>";

  html +=
    "<span class='muted'>RAW ADC po dzielniku: ";

  html +=
    String(
      batteryRaw,
      3
    );

  html +=
    " V</span>";

  html +=
    "</div>";

  // --------------------------------------------------------
  // WIFI
  // --------------------------------------------------------

  html +=
    "<div class='box'><h3>Status WiFi</h3>";

  if (
    WiFi.status() ==
    WL_CONNECTED
  ) {

    html +=
      "<span class='ok'>POLACZONE</span><br><br>";

    html +=
      "SSID: <b>";

    html +=
      WiFi.SSID();

    html +=
      "</b><br>";

    html +=
      "IP: <b>";

    html +=
      WiFi.localIP().toString();

    html +=
      "</b><br>";

    html +=
      "RSSI: <b>";

    html +=
      String(
        WiFi.RSSI()
      );

    html +=
      " dBm</b>";

  } else {

    html +=
      "<span class='bad'>BRAK POLACZENIA</span>";

    if (
      savedSSID.length() > 0
    ) {

      html +=
        "<br><br>Zapamietane SSID: <b>";

      html +=
        savedSSID;

      html +=
        "</b>";
    }
  }

  html +=
    "</div>";

  // --------------------------------------------------------
  // WIFI CONFIG
  // --------------------------------------------------------

  html +=
    "<div class='box'><h3>Konfiguracja WiFi</h3>";

  html +=
    "<form method='GET' action='/scan'>";

  html +=
    "<button type='submit'>SKANUJ SIECI</button>";

  html +=
    "</form>";

  html +=
    "<form method='POST' action='/savewifi'>";

  html +=
    "<select name='ssid' required>";

  html +=
    cachedWiFiOptions;

  html +=
    "</select>";

  html +=
    "<input type='password' name='password' placeholder='Haslo WiFi'>";

  html +=
    "<button type='submit'>ZAPISZ I POLACZ</button>";

  html +=
    "</form></div>";

  // --------------------------------------------------------
  // DIAGNOSTYKA
  // --------------------------------------------------------

  html +=
    "<div class='box'><h3>Diagnostyka</h3>";

  html +=
    "<form method='POST' action='/send'>";

  html +=
    "<button type='submit'>WYSLIJ DANE TERAZ</button>";

  html +=
    "</form>";

  html +=
    "<br>";

  html +=
    "<a href='/data'><button>POKAZ JSON</button></a>";

  html +=
    "</div>";

  html +=
    "</div></body></html>";

  return html;
}

// ============================================================
// WEB SERVER
// ============================================================

void setupWebServer() {

  server.on(
    "/",
    HTTP_GET,
    []() {

      server.send(
        200,
        "text/html; charset=utf-8",
        htmlPage()
      );
    }
  );

  server.on(
    "/scan",
    HTTP_GET,
    []() {

      scanWiFiNetworks();

      server.sendHeader(
        "Location",
        "/"
      );

      server.send(
        303,
        "text/plain",
        ""
      );
    }
  );

  server.on(
    "/data",
    HTTP_GET,
    []() {

      server.send(
        200,
        "application/json",
        buildTelemetryJSON()
      );
    }
  );

  server.on(
    "/send",
    HTTP_POST,
    []() {

      broadcastTelemetry();

      server.sendHeader(
        "Location",
        "/"
      );

      server.send(
        303,
        "text/plain",
        ""
      );
    }
  );

  server.on(
    "/savewifi",
    HTTP_POST,
    []() {

      String newSSID =
        server.arg(
          "ssid"
        );

      String newPassword =
        server.arg(
          "password"
        );

      if (
        newSSID.length() == 0
      ) {

        server.send(
          400,
          "text/plain",
          "Brak SSID"
        );

        return;
      }

      prefs.begin(
        "trexwall",
        false
      );

      prefs.putString(
        "ssid",
        newSSID
      );

      prefs.putString(
        "pass",
        newPassword
      );

      prefs.end();

      savedSSID =
        newSSID;

      savedPassword =
        newPassword;

      server.send(
        200,
        "text/html; charset=utf-8",
        "<!DOCTYPE html>"
        "<html>"
        "<head>"
        "<meta name='viewport' content='width=device-width,initial-scale=1'>"
        "</head>"
        "<body style='font-family:Arial;background:#111;color:white;padding:25px'>"
        "<h2>T-REX Wall Station</h2>"
        "<p>Konfiguracja WiFi zostala zapisana.</p>"
        "<p>Stacja probuje sie polaczyc.</p>"
        "<br>"
        "<a href='/' style='color:white'>POWROT</a>"
        "</body>"
        "</html>"
      );

      delay(500);

      WiFi.disconnect(
        false,
        false
      );

      delay(300);

      connectWiFi(
        12000
      );

      if (
        WiFi.status() ==
        WL_CONNECTED
      ) {

        broadcastTelemetry();
      }
    }
  );

  server.onNotFound(
    []() {

      server.sendHeader(
        "Location",
        "http://192.168.4.1/",
        true
      );

      server.send(
        302,
        "text/plain",
        ""
      );
    }
  );

  server.begin();

  Serial.println(
    "HTTP server: OK"
  );
}

// ============================================================
// CZUJNIKI
// ============================================================

void setupSensors() {

  Wire.begin(
    SDA_PIN,
    SCL_PIN
  );

  pinMode(
    RAIN_PIN,
    INPUT_PULLDOWN
  );

  analogSetPinAttenuation(
    BATTERY_ADC_PIN,
    ADC_11db
  );

  bmeOK =
    bme.begin(
      0x76,
      &Wire
    );

  vemlOK =
    veml.begin(
      &Wire
    );

  if (vemlOK) {

    veml.setGain(
      VEML7700_GAIN_1
    );

    veml.setIntegrationTime(
      VEML7700_IT_100MS
    );
  }

  Serial.print(
    "BME280: "
  );

  Serial.println(
    bmeOK ?
    "OK" :
    "BLAD"
  );

  Serial.print(
    "VEML7700: "
  );

  Serial.println(
    vemlOK ?
    "OK" :
    "BLAD"
  );

  Serial.print(
    "RainPoint: "
  );

  Serial.println(
    isRaining() ?
    "DESZCZ" :
    "SUCHO"
  );

  float rawBattery =
    readBatteryVoltageRaw();

  float correctedBattery =
    calibrateBatteryVoltage(
      rawBattery
    );

  Serial.print(
    "Battery RAW: "
  );

  Serial.print(
    rawBattery,
    3
  );

  Serial.println(
    " V"
  );

  Serial.print(
    "Battery corrected: "
  );

  Serial.print(
    correctedBattery,
    3
  );

  Serial.println(
    " V"
  );
}

// ============================================================
// TRYB KONFIGURACJI
// ============================================================

void startConfigMode() {

  configMode =
    true;

  Serial.println();

  Serial.println(
    "================================"
  );

  Serial.println(
    "TRYB KONFIGURACJI - 20 MINUT"
  );

  Serial.println(
    "================================"
  );

  WiFi.mode(
    WIFI_AP_STA
  );

  WiFi.setSleep(
    false
  );

  delay(500);

  bool apOK =
    WiFi.softAP(
      apName.c_str()
    );

  delay(300);

  Serial.print(
    "AP: "
  );

  Serial.println(
    apName
  );

  Serial.print(
    "AP status: "
  );

  Serial.println(
    apOK ?
    "OK" :
    "BLAD"
  );

  Serial.print(
    "AP IP: "
  );

  Serial.println(
    WiFi.softAPIP()
  );

  Serial.println(
    "Panel: http://192.168.4.1"
  );

  dnsServer.start(
    53,
    "*",
    WiFi.softAPIP()
  );

  setupWebServer();

  udp.begin(
    TREX_UDP_PORT
  );

  configStartedAt =
    millis();

  if (
    savedSSID.length() > 0
  ) {

    connectWiFi(
      8000
    );

  } else {

    Serial.println(
      "Pierwsza konfiguracja - brak zapisanego WiFi."
    );
  }

  if (
    WiFi.status() ==
    WL_CONNECTED
  ) {

    broadcastTelemetry();
  }

  lastTelemetryAt =
    millis();
}

// ============================================================
// DEEP SLEEP
// ============================================================

void goToSleep() {

  bool rainNow = isRaining();

  Serial.println();
  Serial.println("================================");
  Serial.println("T-REX -> DEEP SLEEP");
  Serial.println("================================");

  Serial.print("GPIO4 przed snem: ");
  Serial.println(digitalRead(RAIN_PIN));

  Serial.print("RainPoint: ");
  Serial.println(rainNow ? "DESZCZ" : "SUCHO");

  esp_sleep_enable_timer_wakeup(
    (uint64_t)AUTO_INTERVAL_SEC * 1000000ULL
  );

  esp_err_t wakeResult;

  if (rainNow) {

    // Teraz pada: GPIO4 = LOW.
    // Budzimy stacje, gdy RainPoint przejdzie na HIGH = SUCHO.
    Serial.println("Uzbrajam GPIO4 wake na HIGH - SUCHO");

    wakeResult =
      esp_deep_sleep_enable_gpio_wakeup(
        1ULL << RAIN_PIN,
        ESP_GPIO_WAKEUP_GPIO_HIGH
      );

  } else {

    // Teraz sucho: GPIO4 = HIGH.
    // Budzimy stacje, gdy RainPoint przejdzie na LOW = DESZCZ.
    Serial.println("Uzbrajam GPIO4 wake na LOW - DESZCZ");

    wakeResult =
      esp_deep_sleep_enable_gpio_wakeup(
        1ULL << RAIN_PIN,
        ESP_GPIO_WAKEUP_GPIO_LOW
      );
  }

  Serial.print("GPIO wake result: ");

  if (wakeResult == ESP_OK) {
    Serial.println("ESP_OK");
  } else {
    Serial.print("BLAD = ");
    Serial.println((int)wakeResult);
  }

  Serial.println("Timer wake -> 10 minut");

  Serial.flush();
  delay(1000);

  esp_deep_sleep_start();
}

// ============================================================
// NORMALNY CYKL
// ============================================================

void normalWakeCycle() {

  Serial.println();

  Serial.println(
    "NORMALNY CYKL STACJI"
  );

  WiFi.mode(
    WIFI_STA
  );

  if (
    !connectWiFi(
      10000
    )
  ) {

    Serial.println(
      "Brak WiFi - wracam do snu."
    );

    goToSleep();

    return;
  }

  udp.begin(
    TREX_UDP_PORT
  );

  broadcastTelemetry();

  uint32_t start =
    millis();

  while (
    millis() - start <
    NETWORK_LISTEN_MS
  ) {

    handleUDP();

    delay(5);
  }

  goToSleep();
}

// ============================================================
// SETUP
// ============================================================

void setup() {

  Serial.begin(
    115200
  );

  delay(1200);

  Serial.println();

  Serial.println(
    "================================="
  );

  Serial.println(
    "T-REX WALL STATION"
  );

  Serial.print(
    "Firmware: "
  );

  Serial.println(
    TREX_STATION_FW_VERSION
  );

  Serial.print(
    "Hardware: "
  );

  Serial.println(
    TREX_STATION_HW_VERSION
  );

  Serial.println(
    "================================="
  );

  stationSerial =
    createSerial();

  apName =
    "T-REX-FS-" +
    stationSerial.substring(
      stationSerial.length() - 8
    );

  Serial.print(
    "Serial: "
  );

  Serial.println(
    stationSerial
  );

  setupSensors();

  loadWiFiConfig();

  esp_sleep_wakeup_cause_t wakeCause =
    esp_sleep_get_wakeup_cause();

  Serial.print(
    "Wake cause: "
  );

  Serial.println(
    (int)wakeCause
  );

  // Fizyczny start / reset
  if (
    wakeCause ==
    ESP_SLEEP_WAKEUP_UNDEFINED
  ) {

    startConfigMode();

    return;
  }

  // Timer / RainPoint
  normalWakeCycle();
}

// ============================================================
// LOOP
// ============================================================

void loop() {

  if (
    !configMode
  ) {

    delay(10);

    return;
  }

  dnsServer.processNextRequest();

  server.handleClient();

  handleUDP();

  // --------------------------------------------------------
  // TELEMETRIA CO 10 MIN W CONFIG MODE
  // --------------------------------------------------------

  if (
    WiFi.status() ==
      WL_CONNECTED &&
    millis() -
      lastTelemetryAt >=
      AUTO_INTERVAL_SEC *
      1000UL
  ) {

    lastTelemetryAt =
      millis();

    broadcastTelemetry();
  }

  // --------------------------------------------------------
  // KONIEC 20 MIN TRYBU KONFIGURACJI
  // --------------------------------------------------------

  if (
    millis() -
    configStartedAt >=
    CONFIG_TIME_MS
  ) {

    Serial.println();

    Serial.println(
      "Koniec 20-minutowego trybu konfiguracji."
    );

    if (
      WiFi.status() ==
      WL_CONNECTED
    ) {

      broadcastTelemetry();
    }

    delay(300);

    goToSleep();
  }

  delay(5);
}