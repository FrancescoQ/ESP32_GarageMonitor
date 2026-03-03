/**
 * @file DoorSensor.cpp
 * @brief Reed switch door monitoring implementation
 */

#include "DoorSensor.h"
#include "Config.h"

DoorSensor::DoorSensor()
  : m_doorOpen(false),
    m_lastRawState(false),
    m_stateChanged(false),
    m_lastDebounceTime(0),
    m_doorOpenedAt(0),
    m_callback(nullptr) {
}

void DoorSensor::begin() {
  pinMode(PIN_DOOR_SENSOR, INPUT_PULLUP);

  // Read initial state without debounce
  bool raw = (digitalRead(PIN_DOOR_SENSOR) == HIGH);
  m_doorOpen = raw;
  m_lastRawState = raw;
  m_lastDebounceTime = millis();

  if (m_doorOpen) {
    m_doorOpenedAt = millis();
  }

  Serial.print(F("[DOOR] Sensor initialized on GPIO "));
  Serial.print(PIN_DOOR_SENSOR);
  Serial.print(F(" - door is "));
  Serial.println(m_doorOpen ? F("OPEN") : F("CLOSED"));
}

void DoorSensor::loop() {
  bool raw = (digitalRead(PIN_DOOR_SENSOR) == HIGH);

  // Reset debounce timer when raw reading changes
  if (raw != m_lastRawState) {
    m_lastDebounceTime = millis();
    m_lastRawState = raw;
  }

  // Accept new state only after stable for DOOR_DEBOUNCE_MS
  if (raw != m_doorOpen && (millis() - m_lastDebounceTime >= DOOR_DEBOUNCE_MS)) {
    m_doorOpen = raw;
    m_stateChanged = true;

    // Track when the door opened
    if (m_doorOpen) {
      m_doorOpenedAt = millis();
    } else {
      m_doorOpenedAt = 0;
    }

    if (m_callback) {
      m_callback(m_doorOpen);
    }
  }
}

bool DoorSensor::isDoorOpen() const {
  return m_doorOpen;
}

bool DoorSensor::hasStateChanged() {
  if (m_stateChanged) {
    m_stateChanged = false;
    return true;
  }
  return false;
}

unsigned long DoorSensor::getOpenDurationMs() const {
  if (!m_doorOpen || m_doorOpenedAt == 0) {
    return 0;
  }
  return millis() - m_doorOpenedAt;
}

void DoorSensor::onStateChange(DoorStateCallback cb) {
  m_callback = cb;
}
