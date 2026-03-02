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

  Serial.println(F("[SYS] System ready"));
}

void SystemController::loop() {
  m_modem.loop();

  // Reed switch test: print door status every 2 seconds
  static unsigned long lastPrint = 0;
  unsigned long now = millis();
  if (now - lastPrint >= 2000) {
    lastPrint = now;
    int state = digitalRead(PIN_DOOR_SENSOR);
    Serial.print(F("[DOOR] "));
    Serial.println(state == LOW ? "CLOSED" : "OPEN");
  }
}
