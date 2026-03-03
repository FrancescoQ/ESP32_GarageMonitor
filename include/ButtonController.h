#pragma once

#include <Arduino.h>
#include "Door.h"

/**
 * @brief Debounce state for a single push button
 */
struct ButtonState {
  int pin;
  bool lastRawState;              // Last raw reading (HIGH = released)
  unsigned long lastDebounceTime; // When raw state last changed
  bool pressed;                   // Debounced state (true = pressed/LOW)
};

/**
 * @brief Manual control buttons (CLOSE/OPEN/STOP)
 *
 * Debounces physical push buttons and triggers door actions
 * on falling edge (press). Uses the same Door methods as SMS
 * commands, so relay safety behaviour is identical.
 */
class ButtonController {
public:
  /**
   * @param door Reference to Door instance for close/open/stop actions
   */
  ButtonController(Door& door);

  void begin();
  void loop();

private:
  static const int NUM_BUTTONS = 3;
  Door& m_door;
  ButtonState m_buttons[NUM_BUTTONS];
};
