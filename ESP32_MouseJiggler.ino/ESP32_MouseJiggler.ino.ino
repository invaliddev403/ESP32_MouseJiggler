#include <BleMouse.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <NimBLEDevice.h> 

// BLE Mouse
BleMouse bleMouse("ESP32 Jiggler", "OpenAI", 100);

// Wi-Fi
const char* ssid = "ESP32-Jiggler";
const char* password = "jiggle123";
WebServer server(80);

// Preferences
Preferences preferences;

// App State
bool isJiggling = false;
bool bleConnected = false;
bool bleEnabled = true;
unsigned long lastJiggle = 0;
uint32_t jiggleInterval = 5000;
int jiggleDistance = 5;

// === Movement Logic ===
void moveMouse() {
  if (!bleMouse.isConnected()) return;

  int x = random(3) - 1;
  int y = random(3) - 1;
  for (int i = 0; i < jiggleDistance; i++) {
    bleMouse.move(x, y);
    delay(20);
  }
  for (int i = 0; i < jiggleDistance; i++) {
    bleMouse.move(-x, -y);
    delay(20);
  }
}

// === HTML Page ===
String htmlPage() {
  String html = "<!DOCTYPE html><html><head><title>ESP32 BLE Jiggler</title></head><body>";
  html += "<h2>ESP32 BLE Mouse Jiggler</h2>";
  html += "<p><b>BLE Enabled:</b> " + String(bleEnabled ? "Yes" : "No") + "</p>";
  html += "<p><b>BLE Connected:</b> " + String(bleConnected ? "Yes" : "No") + "</p>";

  html += "<form method='POST' action='/update'>";
  html += "<label>Enable BLE:</label><input type='checkbox' name='ble' " + String(bleEnabled ? "checked" : "") + "><br><br>";

  if (bleEnabled) {
    html += "<label>Interval (ms):</label><input type='number' name='interval' value='" + String(jiggleInterval) + "'><br><br>";
    html += "<label>Distance (px):</label><input type='number' name='distance' value='" + String(jiggleDistance) + "'><br><br>";
    html += "<label>Jiggling:</label><input type='checkbox' name='jiggle' " + String(isJiggling ? "checked" : "") + "><br><br>";
  }

  html += "<input type='submit' value='Save Settings'>";
  html += "</form></body></html>";
  return html;
}

// === Request Handlers ===
void handleRoot() {
  server.send(200, "text/html", htmlPage());
}

void handleUpdate() {
  bool newBleEnabled = server.hasArg("ble");
  if (newBleEnabled != bleEnabled) {
    bleEnabled = newBleEnabled;
    preferences.putBool("bleEnabled", bleEnabled);

    if (bleEnabled) {
      Serial.println("Enabling BLE...");
      bleMouse.begin();
    } else {
      Serial.println("Disabling BLE...");
      NimBLEDevice::deinit(true);
      bleConnected = false;
      isJiggling = false;
    }
  }

  if (bleEnabled) {
    if (server.hasArg("interval")) {
      uint32_t newInterval = server.arg("interval").toInt();
      if (newInterval >= 100 && newInterval <= 60000) {
        jiggleInterval = newInterval;
        preferences.putUInt("interval", jiggleInterval);
      }
    }

    if (server.hasArg("distance")) {
      int newDist = server.arg("distance").toInt();
      if (newDist >= 1 && newDist <= 50) {
        jiggleDistance = newDist;
        preferences.putInt("distance", jiggleDistance);
      }
    }

    isJiggling = server.hasArg("jiggle");
    preferences.putBool("isrunning", isJiggling);
  }

  server.sendHeader("Location", "/");
  server.send(303);
}

// === Setup ===
void setup() {
  Serial.begin(115200);
  delay(500);

  preferences.begin("config", false);
  isJiggling = preferences.getBool("isrunning", false);
  jiggleInterval = preferences.getUInt("interval", 5000);
  jiggleDistance = preferences.getInt("distance", 5);
  bleEnabled = preferences.getBool("bleEnabled", true);

  // BLE init
  if (bleEnabled) {
    bleMouse.begin();
  }

  // Wi-Fi AP
  WiFi.softAP(ssid, password);
  Serial.print("Wi-Fi AP started: ");
  Serial.println(WiFi.softAPIP());

  // Web server
  server.on("/", handleRoot);
  server.on("/update", HTTP_POST, handleUpdate);
  server.begin();
  Serial.println("Web server running");
}

// === Loop ===
void loop() {
  server.handleClient();

  bool currentConnected = bleMouse.isConnected();
  if (currentConnected != bleConnected) {
    bleConnected = currentConnected;
    Serial.println(bleConnected ? "BLE Connected" : "BLE Disconnected");
  }

  if (isJiggling && bleConnected) {
    unsigned long now = millis();
    if (now - lastJiggle >= jiggleInterval) {
      moveMouse();
      lastJiggle = now;
    }
  }
}
