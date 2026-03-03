/**
 * @file Door.h
 * @brief Unified door facade (sensing + actuation)
 *
 * Composes DoorSensor (reed switch monitoring) and DoorController
 * (relay actuation) behind a single interface so SystemController
 * can call m_door.isOpen(), m_door.close(), etc.
 *
 * @author Francesco
 * @date March 2026
 */

#pragma once

#include "DoorSensor.h"
#include "DoorController.h"

class Door {
public:
  /**
   * @brief Initialize relays (fail-safe OFF) then door sensor
   *
   * Controller begins first so relays are safe before any
   * sensor reads or callbacks fire.
   */
  void begin();

  /**
   * @brief Poll door sensor debounce logic (call every loop iteration)
   */
  void loop();

  // -- Sensing (delegates to DoorSensor) --

  /**
   * @brief Current debounced door state
   * @return true if door is open
   */
  bool isOpen() const;

  /**
   * @brief Check if a state transition occurred since last call
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

  // -- Actuation (delegates to DoorController) --

  /**
   * @brief Pulse CLOSE relay to close the door
   */
  void close();

  /**
   * @brief Pulse OPEN relay to open the door
   */
  void open();

  /**
   * @brief Pulse STOP relay to stop the door
   */
  void stop();

private:
  DoorSensor m_sensor;
  DoorController m_controller;
};
