/**
 * @file DoorController.h
 * @brief Relay-based door actuation (close, open, stop)
 *
 * Manages three relay GPIOs for a garage door motor. All relays
 * are forced OFF at begin() (fail-safe boot — see docs/safety.md).
 * Actuation uses a blocking pulse: RELAY_ON for RELAY_PULSE_MS,
 * then RELAY_OFF.
 *
 * @author Francesco
 * @date March 2026
 */

#pragma once

#include <Arduino.h>

class DoorController {
public:
  /**
   * @brief Initialize relay GPIOs and set all to RELAY_OFF
   *
   * This MUST be the first GPIO action after pin mode setup.
   * See docs/safety.md for the rationale.
   */
  void begin();

  /**
   * @brief Pulse the CLOSE relay to close the door
   */
  void close();

  /**
   * @brief Pulse the OPEN relay to open the door
   */
  void open();

  /**
   * @brief Pulse the STOP relay to stop the door
   */
  void stop();

private:
  /**
   * @brief Momentary pulse on a relay pin
   * @param pin GPIO pin to pulse
   * @param label Human-readable name for logging
   */
  void pulse(int pin, const char* label);
};
