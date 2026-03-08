/**
 * @file PowerManager.h
 * @brief ESP32 light sleep power management
 *
 * Implements light sleep with GPIO wake sources for low-power operation.
 * When enabled, the ESP32 sleeps between sensor/SMS checks, waking on:
 * - Timer (periodic check interval)
 * - Modem UART RX activity (incoming SMS URC)
 * - Door sensor state change
 * - Water sensor detection
 * - Any button press
 *
 * Light sleep preserves all RAM and peripheral state — no warm/cold boot
 * handling needed. millis() continues ticking during sleep.
 */

#pragma once

#include <Arduino.h>

/**
 * @brief Reason the ESP32 woke from light sleep
 */
enum class WakeReason {
  TIMER,          ///< Periodic timer expired
  MODEM_UART,     ///< Modem sent data on UART RX (SMS URC)
  WATER_SENSOR,   ///< Water sensor triggered
  DOOR_SENSOR,    ///< Door reed switch state changed
  BUTTON,         ///< Physical button pressed
  UNKNOWN         ///< Could not determine wake source
};

/**
 * @brief Manages ESP32 light sleep for power saving
 *
 * Configures GPIO wake sources and timer, then enters light sleep.
 * On wake, detects which source triggered the wakeup for appropriate
 * handling (e.g., force SMS poll on modem UART wake).
 */
class PowerManager {
public:
  /**
   * @brief Initialize power manager (log readiness)
   */
  void begin();

  /**
   * @brief Enter light sleep with configured wake sources
   *
   * Configures GPIO wake sources based on current sensor state,
   * sets timer wakeup, flushes serial, and enters light sleep.
   * Returns after wakeup with the detected wake reason.
   *
   * @param doorOpen Current door state (determines reed switch wake polarity)
   * @param sleepIntervalMin Timer wake interval in minutes
   * @return WakeReason indicating what triggered the wakeup
   */
  WakeReason enterLightSleep(bool doorOpen, uint32_t sleepIntervalMin);

  /**
   * @brief Detect what caused the most recent wakeup
   * @return WakeReason based on esp_sleep_get_wakeup_cause() and GPIO state
   */
  WakeReason detectWakeReason();

private:
  /**
   * @brief Configure all GPIO wake sources before sleep
   * @param doorOpen Current door state for dynamic polarity
   */
  void configureGPIOWake(bool doorOpen);
};
