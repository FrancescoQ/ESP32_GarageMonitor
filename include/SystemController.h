#pragma once

#include <Arduino.h>
#include "Door.h"
#include "ButtonController.h"
#include "WaterSensor.h"
#include "ModemHandler.h"
#include "MessageParser.h"
#include "DisplayController.h"
#include "EnvironmentalSensor.h"
#include "ConfigManager.h"
#include "WebUIController.h"

/**
 * @brief Main system coordinator
 *
 * Orchestrates all subsystems via event-driven loop.
 * Supports two modes determined at boot:
 * - Normal mode: cellular + SMS monitoring
 * - Setup mode: WiFi AP + web UI for configuration
 */
class SystemController {
public:
  SystemController();

  void begin();
  void loop();

  /**
   * @brief Get reference to ConfigManager (for WebUIController access)
   */
  ConfigManager& getConfig();

  /**
   * @brief Get reference to Door (for WebUIController diagnostics)
   */
  const Door& getDoor() const;

  /**
   * @brief Get reference to WaterSensor (for WebUIController diagnostics)
   */
  const WaterSensor& getWater() const;

  /**
   * @brief Get reference to EnvironmentalSensor (for WebUIController diagnostics)
   */
  const EnvironmentalSensor& getEnv() const;

  /**
   * @brief Check if system is in setup mode
   */
  bool isSetupMode() const;

private:
  Door m_door;
  WaterSensor m_water;
  ButtonController m_buttons;
  ModemHandler m_modem;
  MessageParser m_parser;
  DisplayController m_display;
  EnvironmentalSensor m_env;
  ConfigManager m_config;
  WebUIController m_webUI;
  bool m_setupMode;

  // Normal mode state
  unsigned long m_lastSMSCheck;
  bool m_alertSent;
  bool m_doorWasOpen;

  bool detectSetupMode();
  void beginNormalMode();
  void beginSetupMode();
  void loopNormalMode();
  void loopSetupMode();

  void handleSMS(const ReceivedSMS& sms);
  void notifyAdmins(const char* message);
  void notifyAllUsers(const char* message);
  String buildStatusReply();
};
