/**
 * @file SystemController.cpp
 * @brief Main system coordinator implementation
 *
 * @author Francesco
 * @date February 2026
 */

#include "SystemController.h"
#include "Config.h"

SystemController::SystemController() {
}

void SystemController::begin() {
  Serial.println(F("\n========================================"));
  Serial.println(F("  Garage Monitor - Starting"));
  Serial.println(F("========================================\n"));

  if (m_modem.begin()) {
    Serial.println(F("[SYS] Modem initialized successfully"));

    // Test SMS
    // m_modem.sendSMS("+393494263651", "Test!");
  } else {
    Serial.println(F("[SYS] Modem initialization failed"));
  }

  pinMode(PIN_DOOR_SENSOR, INPUT_PULLUP);
  Serial.println(F("[SYS] Door sensor initialized on GPIO 5"));

  // Relay setup: active-low on GPIO 25
  pinMode(PIN_RELAY_CLOSE, OUTPUT);
  digitalWrite(PIN_RELAY_CLOSE, RELAY_OFF);
  Serial.println(F("[RELAY] Close relay initialized on GPIO 25 (active-low)"));
  Serial.println(F("[RELAY] Reed switch controls relay: OPEN -> relay ON, CLOSED -> relay OFF"));

  Serial.println(F("[SYS] System ready"));
}

void SystemController::loop() {
  m_modem.loop();

  // Reed switch test: print door status and drive relay
  static unsigned long lastPrint = 0;
  static int lastState = -1;
  unsigned long now = millis();

  int state = digitalRead(PIN_DOOR_SENSOR);

  // React immediately on state change
  if (state != lastState) {
    lastState = state;
    bool doorOpen = (state == HIGH);
    digitalWrite(PIN_RELAY_CLOSE, doorOpen ? RELAY_ON : RELAY_OFF);
    Serial.print(F("[DOOR] "));
    Serial.print(doorOpen ? "OPEN" : "CLOSED");
    Serial.print(F(" -> relay "));
    Serial.println(doorOpen ? "ON" : "OFF");
  }

  // Periodic status print
  if (now - lastPrint >= 2000) {
    lastPrint = now;
    Serial.print(F("[DOOR] "));
    Serial.print(state == LOW ? "CLOSED" : "OPEN");
    Serial.print(F(" | relay "));
    Serial.println(state == HIGH ? "ON" : "OFF");
  }
}
