#pragma once

#include <Arduino.h>
#include "Door.h"
#include "ModemHandler.h"
#include "MessageParser.h"

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
  ModemHandler m_modem;
  MessageParser m_parser;
  unsigned long m_lastSMSCheck;
  bool m_alertSent;
  bool m_doorWasOpen;

  void handleSMS(const ReceivedSMS& sms);
  void notifyAdmins(const char* message);
  String buildStatusReply();
};
