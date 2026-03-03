/**
 * @file DoorController.cpp
 * @brief Relay-based door actuation implementation
 *
 * @author Francesco
 * @date March 2026
 */

#include "DoorController.h"
#include "Config.h"

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
  delay(RELAY_SEQUENCE_DELAY_MS);
  pulse(PIN_RELAY_CLOSE, "CLOSE");
}

void DoorController::open() {
  stop();
  delay(RELAY_SEQUENCE_DELAY_MS);
  pulse(PIN_RELAY_OPEN, "OPEN");
}

void DoorController::stop() {
  pulse(PIN_RELAY_STOP, "STOP");
}

void DoorController::pulse(int pin, const char* label) {
  Serial.print(F("[RELAY] Pulsing "));
  Serial.print(label);
  Serial.print(F(" relay ("));
  Serial.print(RELAY_PULSE_MS);
  Serial.println(F("ms)"));

  digitalWrite(pin, RELAY_ON);
  delay(RELAY_PULSE_MS);
  digitalWrite(pin, RELAY_OFF);

  Serial.print(F("[RELAY] "));
  Serial.print(label);
  Serial.println(F(" pulse complete"));
}
