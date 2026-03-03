/**
 * @file Door.cpp
 * @brief Unified door facade implementation
 *
 * @author Francesco
 * @date March 2026
 */

#include "Door.h"

void Door::begin() {
  // Relays first — fail-safe before any sensor reads
  m_controller.begin();
  m_sensor.begin();
}

void Door::loop() {
  m_sensor.loop();
}

// -- Sensing delegates --

bool Door::isOpen() const {
  return m_sensor.isDoorOpen();
}

bool Door::hasStateChanged() {
  return m_sensor.hasStateChanged();
}

unsigned long Door::getOpenDurationMs() const {
  return m_sensor.getOpenDurationMs();
}

void Door::onStateChange(DoorStateCallback cb) {
  m_sensor.onStateChange(cb);
}

// -- Actuation delegates --

void Door::close() {
  m_controller.close();
}

void Door::open() {
  m_controller.open();
}

void Door::stop() {
  m_controller.stop();
}
