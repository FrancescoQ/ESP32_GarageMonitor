/**
 * @file WaterSensor.cpp
 * @brief XKC-Y25 capacitive water sensor implementation
 */

#include "WaterSensor.h"
#include "Config.h"

WaterSensor::WaterSensor()
  : m_waterDetected(false),
    m_lastRawState(false),
    m_stateChanged(false),
    m_lastDebounceTime(0) {
}

void WaterSensor::begin() {
  pinMode(PIN_WATER_SENSOR, INPUT_PULLUP);

  // Read initial state without debounce (LOW = water)
  bool raw = (digitalRead(PIN_WATER_SENSOR) == LOW);
  m_waterDetected = raw;
  m_lastRawState = raw;
  m_lastDebounceTime = millis();

  Serial.print(F("[WATER] Sensor initialized on GPIO "));
  Serial.print(PIN_WATER_SENSOR);
  Serial.print(F(" - water "));
  Serial.println(m_waterDetected ? F("DETECTED") : F("not detected"));
}

void WaterSensor::loop() {
  bool raw = (digitalRead(PIN_WATER_SENSOR) == LOW);

  // Reset debounce timer when raw reading changes
  if (raw != m_lastRawState) {
    m_lastDebounceTime = millis();
    m_lastRawState = raw;
  }

  // Accept new state only after stable for WATER_DEBOUNCE_MS
  if (raw != m_waterDetected && (millis() - m_lastDebounceTime >= WATER_DEBOUNCE_MS)) {
    m_waterDetected = raw;
    m_stateChanged = true;

    Serial.print(F("[WATER] Water "));
    Serial.println(m_waterDetected ? F("DETECTED") : F("CLEARED"));
  }
}

bool WaterSensor::isWaterDetected() const {
  return m_waterDetected;
}

bool WaterSensor::hasStateChanged() {
  if (m_stateChanged) {
    m_stateChanged = false;
    return true;
  }
  return false;
}
