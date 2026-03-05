/**
 * @file WebUIController.cpp
 * @brief WiFi AP + web server implementation
 *
 * Static files (HTML/CSS/JS) served from LittleFS (data/ directory).
 * Upload filesystem with: pio run -t uploadfs
 *
 * Endpoints:
 *   GET  /               → index.html from LittleFS
 *   GET  /style.css      → style.css from LittleFS
 *   GET  /app.js         → app.js from LittleFS
 *   GET  /api/users      → JSON array of users
 *   POST /api/users      → Add user {name, phone, permissions}
 *   DELETE /api/users?index=N → Remove user by index
 *   GET  /api/settings   → JSON settings object
 *   POST /api/settings   → Update settings
 *   GET  /api/diagnostics → Live sensor readings
 *   POST /api/reboot      → Reboot the ESP32
 */

#include "WebUIController.h"
#include "ConfigManager.h"
#include "Door.h"
#include "WaterSensor.h"
#include "EnvironmentalSensor.h"
#include "Config.h"
#include <WiFi.h>
#include <LittleFS.h>
#include <ArduinoJson.h>

WebUIController::WebUIController()
  : m_server(WEB_SERVER_PORT),
    m_config(nullptr),
    m_door(nullptr),
    m_water(nullptr),
    m_env(nullptr) {
}

void WebUIController::begin(ConfigManager& config, const Door& door,
                            const WaterSensor& water,
                            const EnvironmentalSensor& env) {
  m_config = &config;
  m_door = &door;
  m_water = &water;
  m_env = &env;

  // Mount LittleFS (auto-format on first use)
  if (!LittleFS.begin(true)) {
    Serial.println(F("[WEB] ERROR: LittleFS mount failed"));
    return;
  }
  Serial.println(F("[WEB] LittleFS mounted"));

  // Start WiFi AP
  Serial.print(F("[WEB] Starting WiFi AP: "));
  Serial.println(WIFI_AP_SSID);

  WiFi.mode(WIFI_AP);
  WiFi.softAP(WIFI_AP_SSID, WIFI_AP_PASSWORD);

  Serial.print(F("[WEB] AP IP: "));
  Serial.println(WiFi.softAPIP());

  // Register routes — static files from LittleFS
  m_server.on("/", HTTP_GET, [this]() { handleRoot(); });
  m_server.serveStatic("/app.js", LittleFS, "/app.js", "no-cache");
  m_server.serveStatic("/app.css", LittleFS, "/app.css", "no-cache");

  // API endpoints
  m_server.on("/api/users", HTTP_GET, [this]() { handleGetUsers(); });
  m_server.on("/api/users", HTTP_POST, [this]() { handlePostUser(); });
  m_server.on("/api/users", HTTP_DELETE, [this]() { handleDeleteUser(); });
  m_server.on("/api/settings", HTTP_GET, [this]() { handleGetSettings(); });
  m_server.on("/api/settings", HTTP_POST, [this]() { handlePostSettings(); });
  m_server.on("/api/diagnostics", HTTP_GET, [this]() { handleGetDiagnostics(); });
  m_server.on("/api/reboot", HTTP_POST, [this]() { handleReboot(); });
  m_server.onNotFound([this]() { handleNotFound(); });

  m_server.begin();
  Serial.println(F("[WEB] Web server started"));
}

void WebUIController::loop() {
  m_server.handleClient();
}

String WebUIController::getIPAddress() const {
  return WiFi.softAPIP().toString();
}

// =============================================================================
// Route handlers
// =============================================================================

void WebUIController::handleRoot() {
  File file = LittleFS.open("/index.html", "r");
  if (!file) {
    m_server.send(500, "text/plain", "index.html not found — upload filesystem with: pio run -t uploadfs");
    return;
  }
  m_server.streamFile(file, "text/html");
  file.close();
}

void WebUIController::handleGetUsers() {
  DynamicJsonDocument doc(1024);
  JsonArray arr = doc.createNestedArray("users");

  for (int i = 0; i < m_config->getUserCount(); i++) {
    const NvsUser* user = m_config->getUser(i);
    if (user != nullptr) {
      JsonObject obj = arr.createNestedObject();
      obj["name"] = user->name;
      obj["phone"] = user->phone;
      obj["permissions"] = user->permissions;
    }
  }

  String response;
  serializeJson(doc, response);
  m_server.send(200, "application/json", response);
}

void WebUIController::handlePostUser() {
  if (!m_server.hasArg("plain")) {
    m_server.send(400, "application/json", "{\"ok\":false,\"error\":\"No body\"}");
    return;
  }

  DynamicJsonDocument doc(256);
  DeserializationError err = deserializeJson(doc, m_server.arg("plain"));
  if (err) {
    m_server.send(400, "application/json", "{\"ok\":false,\"error\":\"Invalid JSON\"}");
    return;
  }

  const char* name = doc["name"];
  const char* phone = doc["phone"];
  uint8_t perms = doc["permissions"] | 0;

  if (name == nullptr || phone == nullptr || strlen(name) == 0 || strlen(phone) == 0) {
    m_server.send(400, "application/json",
                  "{\"ok\":false,\"error\":\"Name and phone required\"}");
    return;
  }

  // Normalize phone number before storing
  String normalizedPhone = MessageParser::normalizePhoneNumber(String(phone));

  if (m_config->addUser(normalizedPhone, perms, String(name))) {
    m_server.send(200, "application/json", "{\"ok\":true}");
  } else {
    m_server.send(400, "application/json",
                  "{\"ok\":false,\"error\":\"Full or duplicate\"}");
  }
}

void WebUIController::handleDeleteUser() {
  if (!m_server.hasArg("index")) {
    m_server.send(400, "application/json", "{\"ok\":false,\"error\":\"Missing index\"}");
    return;
  }

  int index = m_server.arg("index").toInt();

  if (m_config->removeUser(index)) {
    m_server.send(200, "application/json", "{\"ok\":true}");
  } else {
    m_server.send(400, "application/json",
                  "{\"ok\":false,\"error\":\"Invalid index\"}");
  }
}

void WebUIController::handleGetSettings() {
  const SystemSettings& s = m_config->getSettings();

  DynamicJsonDocument doc(256);
  doc["door_alert_min"] = s.doorAlertMin;
  doc["sms_poll_ms"] = s.smsPollMs;
  doc["deep_sleep"] = s.deepSleepEnabled;
  doc["fwd_unknown"] = s.forwardUnknownSms;
  doc["notify_reboot"] = s.notifyReboot;
  doc["auto_reboot"] = s.autoRebootEnabled;
  doc["reboot_days"] = s.autoRebootDays;
  doc["reboot_hour"] = s.autoRebootHour;

  String response;
  serializeJson(doc, response);
  m_server.send(200, "application/json", response);
}

void WebUIController::handlePostSettings() {
  if (!m_server.hasArg("plain")) {
    m_server.send(400, "application/json", "{\"ok\":false,\"error\":\"No body\"}");
    return;
  }

  DynamicJsonDocument doc(256);
  DeserializationError err = deserializeJson(doc, m_server.arg("plain"));
  if (err) {
    m_server.send(400, "application/json", "{\"ok\":false,\"error\":\"Invalid JSON\"}");
    return;
  }

  SystemSettings s = m_config->getSettings();

  if (doc.containsKey("door_alert_min")) {
    uint32_t val = doc["door_alert_min"];
    if (val >= 1 && val <= 1440) {
      s.doorAlertMin = val;
    }
  }
  if (doc.containsKey("sms_poll_ms")) {
    uint32_t val = doc["sms_poll_ms"];
    if (val >= 1000 && val <= 60000) {
      s.smsPollMs = val;
    }
  }
  if (doc.containsKey("deep_sleep")) {
    s.deepSleepEnabled = doc["deep_sleep"];
  }
  if (doc.containsKey("fwd_unknown")) {
    s.forwardUnknownSms = doc["fwd_unknown"];
  }
  if (doc.containsKey("notify_reboot")) {
    s.notifyReboot = doc["notify_reboot"];
  }
  if (doc.containsKey("auto_reboot")) {
    s.autoRebootEnabled = doc["auto_reboot"];
  }
  if (doc.containsKey("reboot_days")) {
    uint32_t val = doc["reboot_days"];
    if (val >= 1 && val <= 30) {
      s.autoRebootDays = val;
    }
  }
  if (doc.containsKey("reboot_hour")) {
    int val = doc["reboot_hour"];
    if (val >= -1 && val <= 23) {
      s.autoRebootHour = (int8_t)val;
    }
  }

  m_config->setSettings(s);
  m_server.send(200, "application/json", "{\"ok\":true}");
}

void WebUIController::handleGetDiagnostics() {
  DynamicJsonDocument doc(256);

  // Door
  if (m_door->isOpen()) {
    unsigned long openMin = m_door->getOpenDurationMs() / 60000;
    doc["door"] = "OPEN (" + String(openMin) + " min)";
  } else {
    doc["door"] = "CLOSED";
  }

  // Environmental
  if (m_env->isReady()) {
    doc["temperature"] = String(m_env->getTemperature(), 1) + " C";
    doc["humidity"] = String(m_env->getHumidity(), 1) + " %";
    doc["pressure"] = String(m_env->getPressure(), 0) + " hPa";
  } else {
    doc["temperature"] = "N/A";
    doc["humidity"] = "N/A";
    doc["pressure"] = "N/A";
  }

  // Water
  doc["water"] = m_water->isWaterDetected() ? "ALERT" : "OK";

  // Uptime
  unsigned long totalMin = millis() / 60000;
  unsigned long days = totalMin / 1440;
  unsigned long hours = (totalMin % 1440) / 60;
  unsigned long mins = totalMin % 60;
  char uptime[32];
  if (days > 0) {
    snprintf(uptime, sizeof(uptime), "%lud %luh %lum", days, hours, mins);
  } else {
    snprintf(uptime, sizeof(uptime), "%luh %lum", hours, mins);
  }
  doc["uptime"] = uptime;

  String response;
  serializeJson(doc, response);
  m_server.send(200, "application/json", response);
}

void WebUIController::handleReboot() {
  Serial.println(F("[WEB] Reboot requested via web UI"));
  m_server.send(200, "application/json", "{\"ok\":true}");
  delay(500);
  ESP.restart();
}

void WebUIController::handleNotFound() {
  m_server.send(404, "text/plain", "Not found");
}
