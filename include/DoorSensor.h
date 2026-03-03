/**
 * @file DoorSensor.h
 * @brief Reed switch door monitoring with debounce
 *
 * Reads a reed switch via INPUT_PULLUP GPIO and provides
 * debounced open/closed state, state-change detection,
 * open-duration tracking, and an optional callback.
 *
 * @author Francesco
 * @date February 2026
 */

#pragma once

#include <Arduino.h>

/// Callback type fired on each confirmed state change
using DoorStateCallback = void (*)(bool doorOpen);

/**
 * @brief Debounced reed switch door sensor
 *
 * HIGH = door open, LOW = door closed (INPUT_PULLUP with NO reed switch).
 * State changes are accepted only after the reading is stable for
 * DOOR_DEBOUNCE_MS (defined in Config.h).
 */
class DoorSensor {
public:
  DoorSensor();

  /**
   * @brief Initialize GPIO pin and read initial state
   */
  void begin();

  /**
   * @brief Poll sensor and run debounce logic (call every loop iteration)
   */
  void loop();

  /**
   * @brief Current debounced door state
   * @return true if door is open
   */
  bool isDoorOpen() const;

  /**
   * @brief Check if a state transition occurred since last call
   *
   * Returns true once per transition, then resets. Allows the caller
   * to react exactly once per open/close event.
   *
   * @return true if state changed since last hasStateChanged() call
   */
  bool hasStateChanged();

  /**
   * @brief Time the door has been continuously open
   * @return Milliseconds since door opened, or 0 if closed
   */
  unsigned long getOpenDurationMs() const;

  /**
   * @brief Register a callback for confirmed state changes
   * @param cb Function called with (true = open, false = closed)
   */
  void onStateChange(DoorStateCallback cb);

private:
  bool m_doorOpen;                ///< Current debounced state
  bool m_lastRawState;            ///< Previous raw GPIO reading
  bool m_stateChanged;            ///< Flag: cleared by hasStateChanged()
  unsigned long m_lastDebounceTime; ///< When raw reading last changed
  unsigned long m_doorOpenedAt;   ///< millis() when door opened (0 if closed)
  DoorStateCallback m_callback;   ///< Optional state-change callback
};
