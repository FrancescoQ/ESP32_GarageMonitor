/**
 * @file DoorController.h
 * @brief Relay-based door actuation (close, open, stop)
 *
 * Manages three relay GPIOs for a garage door motor. All relays
 * are forced OFF at begin() (fail-safe boot — see docs/safety.md).
 * Actuation uses a blocking pulse: RELAY_ON for RELAY_PULSE_MS,
 * then RELAY_OFF.
 */

#pragma once

#include <Arduino.h>

class DoorController {
public:
  DoorController();

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

  /**
   * @brief Update relay timing parameters (from ConfigManager)
   * @param pulseMs Relay pulse duration in ms (50-2000)
   * @param sequenceDelayMs Pause between STOP and action in ms (100-5000)
   */
  void setTiming(uint32_t pulseMs, uint32_t sequenceDelayMs);

private:
  uint32_t m_pulseMs;
  uint32_t m_sequenceDelayMs;

  /**
   * @brief Momentary pulse on a relay pin
   * @param pin GPIO pin to pulse
   * @param label Human-readable name for logging
   */
  void pulse(int pin, const char* label);
};
