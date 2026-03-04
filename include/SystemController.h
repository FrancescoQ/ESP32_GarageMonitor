#pragma once

#include <Arduino.h>
#include "Door.h"
#include "ButtonController.h"
#include "WaterSensor.h"
#include "ModemHandler.h"
#include "MessageParser.h"
#include "DisplayController.h"
#include "EnvironmentalSensor.h"

/**
 * @brief System state enumeration
 */
enum class SystemState {
  IDLE,
  MONITORING,
  ALERT,
  SLEEP,
  SETUP,
  COMMAND_PROCESSING
};

/**
 * @brief Main system coordinator
 * Orchestrates all subsystems and manages state transitions
 */
class SystemController {
public:
  SystemController();

  void begin();
  void loop();

private:
  Door m_door;
  WaterSensor m_water;
  ButtonController m_buttons;
  ModemHandler m_modem;
  MessageParser m_parser;
  DisplayController m_display;
  EnvironmentalSensor m_env;
  unsigned long m_lastSMSCheck;
  bool m_alertSent;
  bool m_doorWasOpen;

  void handleSMS(const ReceivedSMS& sms);
  void notifyAdmins(const char* message);
  void notifyAllUsers(const char* message);
  String buildStatusReply();
};
