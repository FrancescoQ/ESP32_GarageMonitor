/**
 * @file SystemController.cpp
 * @brief Main system coordinator implementation
 *
 * @author Francesco
 * @date February 2026
 */

#include "SystemController.h"

SystemController::SystemController() {
}

void SystemController::begin() {
  Serial.println(F("\n========================================"));
  Serial.println(F("  Garage Monitor - Starting"));
  Serial.println(F("========================================\n"));

  if (m_modem.begin()) {
    Serial.println(F("[SYS] Modem initialized successfully"));
  } else {
    Serial.println(F("[SYS] Modem initialization failed"));
  }

  Serial.println(F("[SYS] System ready"));
}

void SystemController::loop() {
  m_modem.loop();
}
