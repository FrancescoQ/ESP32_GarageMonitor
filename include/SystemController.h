#pragma once

#include <Arduino.h>
#include "DoorSensor.h"
#include "ModemHandler.h"

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
  DoorSensor m_doorSensor;
  ModemHandler m_modem;
  unsigned long m_lastSMSCheck;
};
