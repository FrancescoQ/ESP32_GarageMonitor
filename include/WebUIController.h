/**
 * @file WebUIController.h
 * @brief WiFi AP + web server for setup mode configuration
 *
 * Creates a WiFi access point and serves a web UI for managing
 * authorized users, system settings, and viewing live diagnostics.
 * Only active in setup mode — never runs alongside the modem.
 *
 * Web UI files (HTML/CSS/JS) are served from LittleFS filesystem.
 * Upload with: pio run -t uploadfs
 */

#pragma once

#include <Arduino.h>
#include <WebServer.h>

// Forward declarations
class ConfigManager;
class Door;
class WaterSensor;
class EnvironmentalSensor;

/**
 * @brief WiFi AP and web server for configuration
 *
 * Manages the WiFi access point lifecycle and HTTP request routing.
 * All endpoints use JSON for request/response bodies.
 */
class WebUIController {
public:
  WebUIController();

  /**
   * @brief Start WiFi AP and web server
   * @param config ConfigManager reference for user/settings CRUD
   * @param door Door reference for diagnostics
   * @param water WaterSensor reference for diagnostics
   * @param env EnvironmentalSensor reference for diagnostics
   */
  void begin(ConfigManager& config, const Door& door,
             const WaterSensor& water, const EnvironmentalSensor& env);

  /**
   * @brief Process incoming HTTP requests (call from loop)
   */
  void loop();

  /**
   * @brief Get the IP address of the AP
   * @return IP address as string (e.g. "192.168.4.1")
   */
  String getIPAddress() const;

private:
  WebServer m_server;
  ConfigManager* m_config;
  const Door* m_door;
  const WaterSensor* m_water;
  const EnvironmentalSensor* m_env;

  // Route handlers
  void handleRoot();
  void handleGetUsers();
  void handlePostUser();
  void handleDeleteUser();
  void handleGetSettings();
  void handlePostSettings();
  void handleGetDiagnostics();
  void handleReboot();
  void handleNotFound();
};
