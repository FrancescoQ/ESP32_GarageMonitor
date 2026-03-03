/**
 * @file WaterSensor.h
 * @brief XKC-Y25 capacitive water/flood detection with debounce
 *
 * Reads the XKC-Y25 sensor via INPUT_PULLUP GPIO and provides
 * debounced water-detected state and state-change detection.
 * Active LOW: sensor pulls pin LOW when water is present.
 */

#pragma once

#include <Arduino.h>

/**
 * @brief Debounced capacitive water sensor
 *
 * LOW = water detected, HIGH = dry (INPUT_PULLUP with XKC-Y25).
 * State changes are accepted only after the reading is stable for
 * WATER_DEBOUNCE_MS (defined in Config.h).
 */
class WaterSensor {
public:
  WaterSensor();

  /**
   * @brief Initialize GPIO pin and read initial state
   */
  void begin();

  /**
   * @brief Poll sensor and run debounce logic (call every loop iteration)
   */
  void loop();

  /**
   * @brief Current debounced water state
   * @return true if water is detected
   */
  bool isWaterDetected() const;

  /**
   * @brief Check if a state transition occurred since last call
   *
   * Returns true once per transition, then resets. Allows the caller
   * to react exactly once per detection/clearing event.
   *
   * @return true if state changed since last hasStateChanged() call
   */
  bool hasStateChanged();

private:
  bool m_waterDetected;             ///< Current debounced state
  bool m_lastRawState;              ///< Previous raw GPIO reading
  bool m_stateChanged;              ///< Flag: cleared by hasStateChanged()
  unsigned long m_lastDebounceTime; ///< When raw reading last changed
};
