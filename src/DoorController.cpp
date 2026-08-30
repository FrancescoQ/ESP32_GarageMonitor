/**
 * @file DoorController.cpp
 * @brief Relay-based door actuation implementation
 */

#include "DoorController.h"
#include "Config.h"

DoorController::DoorController()
  : m_pulseMs(RELAY_PULSE_MS),
    m_sequenceDelayMs(RELAY_SEQUENCE_DELAY_MS) {
}

void DoorController::begin() {
  // Fail-safe: all relays OFF before anything else (see docs/safety.md)
  pinMode(PIN_RELAY_CLOSE, OUTPUT);
  digitalWrite(PIN_RELAY_CLOSE, RELAY_OFF);
  pinMode(PIN_RELAY_STOP, OUTPUT);
  digitalWrite(PIN_RELAY_STOP, RELAY_OFF);
  pinMode(PIN_RELAY_OPEN, OUTPUT);
  digitalWrite(PIN_RELAY_OPEN, RELAY_OFF);

  Serial.println(F("[RELAY] All relays initialized OFF (fail-safe)"));
}

void DoorController::close() {
  stop();
  delay(m_sequenceDelayMs);
  pulse(PIN_RELAY_CLOSE, "CLOSE");
}

void DoorController::open() {
  stop();
  delay(m_sequenceDelayMs);
  pulse(PIN_RELAY_OPEN, "OPEN");
}

void DoorController::stop() {
  pulse(PIN_RELAY_STOP, "STOP");
}

void DoorController::setTiming(uint32_t pulseMs, uint32_t sequenceDelayMs) {
  m_pulseMs = pulseMs;
  m_sequenceDelayMs = sequenceDelayMs;
  Serial.print(F("[RELAY] Timing updated: pulse="));
  Serial.print(m_pulseMs);
  Serial.print(F("ms, seq_delay="));
  Serial.print(m_sequenceDelayMs);
  Serial.println(F("ms"));
}

void DoorController::pulse(int pin, const char* label) {
  Serial.print(F("[RELAY] Pulsing "));
  Serial.print(label);
  Serial.print(F(" relay ("));
  Serial.print(m_pulseMs);
  Serial.println(F("ms)"));

  digitalWrite(pin, RELAY_ON);
  delay(m_pulseMs);
  digitalWrite(pin, RELAY_OFF);

  Serial.print(F("[RELAY] "));
  Serial.print(label);
  Serial.println(F(" pulse complete"));
}
