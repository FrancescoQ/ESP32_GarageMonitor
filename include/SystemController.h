#pragma once

#include <Arduino.h>

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
  ~SystemController();

  void begin();
  void loop();
};
