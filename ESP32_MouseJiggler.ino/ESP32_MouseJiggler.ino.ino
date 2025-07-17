#include <BleMouse.h>
#include <WiFi.h>
#include <WebServer.h>

BleMouse bleMouse("ESP32 Jiggler", "OpenAI", 100);

// Wi-Fi credentials
const char* ssid = "ESP32-Jiggler";
const char* password = "jiggle123";

// Web server
WebServer server(80);

// Jiggle state
bool isJiggling = false;
unsigned long lastJiggle = 0;
unsigned long jiggleInterval = 5000;
int jiggleDistance = 5;

// BLE connection status
bool bleConnected = false;

// Jiggling logic (based on your example)
void moveMouse() {
  if (!bleMouse.isConnected()) return;

  int distance = jiggleDistance;
  int x = random(3) - 1;  // -1, 0, or 1
  int y = random(3) - 1;

  for (int i = 0; i < distance; i++) {
    bleMouse.move(x, y);
    delay(20);
  }

  for (int i = 0; i < distance; i++) {
    bleMouse.move(-x, -y);
    delay(20);
  }
}

// Web UI HTML generator
String htmlPage() {
  String html = "<!DOCTYPE html><html><head><title>ESP32 BLE Jiggler</title></head><body>";
  html += "<h2>ESP32 BLE Mouse Jiggler</h2>";
  html += "<p><b>Status:</b> " + String(isJiggling ? "Running" : "Stopped") + "</p>";
  html += "<p><b>BLE Connected:</b> " + String(bleConnected ? "Yes" : "No") + "</p>";
  html += "<form method='POST' action='/update'>";
  html += "<label>Interval (ms):</label><input type='number' name='interval' value='" + String(jiggleInterval) + "'><br><br>";
  html += "<label>Distance (px):</label><input type='number' name='distance' value='" + String(jiggleDistance) + "'><br><br>";
  html += "<label>Jiggling:</label><input type='checkbox' name='jiggle' " + String(isJiggling ? "checked" : "") + "><br><br>";
  html += "<input type='submit' value='Update'>";
  html += "</form></body></html>";
  return html;
}

// Root page handler
void handleRoot() {
  server.send(200, "text/html", htmlPage());
}

// Form POST handler
void handleUpdate() {
  if (server.hasArg("interval")) {
    int newInterval = server.arg("interval").toInt();
    if (newInterval >= 100 && newInterval <= 60000) {
      jiggleInterval = newInterval;
    }
  }

  if (server.hasArg("distance")) {
    int newDist = server.arg("distance").toInt();
    if (newDist >= 1 && newDist <= 50) {
      jiggleDistance = newDist;
    }
  }

  isJiggling = server.hasArg("jiggle");

  server.sendHeader("Location", "/");
  server.send(303);
}

// Setup BLE and Wi-Fi
void setup() {
  Serial.begin(115200);

  bleMouse.begin();
  delay(100);

  WiFi.softAP(ssid, password);
  Serial.println("Wi-Fi started");
  Serial.print("IP Address: ");
  Serial.println(WiFi.softAPIP());

  server.on("/", handleRoot);
  server.on("/update", HTTP_POST, handleUpdate);
  server.begin();
}

// Main loop
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
