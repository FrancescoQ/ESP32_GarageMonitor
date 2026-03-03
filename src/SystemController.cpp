/**
 * @file SystemController.cpp
 * @brief Main system coordinator implementation
 *
 * @author Francesco
 * @date February 2026
 */

#include "SystemController.h"
#include "Config.h"

SystemController::SystemController()
  : m_lastSMSCheck(0) {
}

void SystemController::begin() {
  Serial.println(F("\n========================================"));
  Serial.println(F("  Garage Monitor - Starting"));
  Serial.println(F("========================================\n"));

  // Door subsystem (relays fail-safe OFF, then sensor init)
  m_door.begin();

  // Modem
  if (m_modem.begin()) {
    Serial.println(F("[SYS] Modem initialized successfully"));
  } else {
    Serial.println(F("[SYS] Modem initialization failed"));
  }

  Serial.println(F("[SYS] System ready"));
}

void SystemController::loop() {
  m_door.loop();
  m_modem.loop();

  unsigned long now = millis();

  // SMS polling: check for incoming messages
  if (m_modem.isNetworkConnected() && now - m_lastSMSCheck >= SMS_POLL_INTERVAL_MS) {
    m_lastSMSCheck = now;

    int count = m_modem.checkForSMS();

    // Safety net: if too many SMS accumulated, purge all and skip processing
    if (count >= SMS_PURGE_THRESHOLD) {
      Serial.print(F("[SYS] SMS count "));
      Serial.print(count);
      Serial.println(F(" >= threshold, purging all"));
      m_modem.deleteAllSMS();
      return;
    }

    for (int i = 0; i < count; i++) {
      ReceivedSMS sms;
      int idx = m_modem.getSMSIndex(i);
      if (m_modem.readSMS(idx, sms)) {
        Serial.print(F("[SYS] === SMS RECEIVED ==="));
        Serial.print(F("\n  From: "));
        Serial.print(sms.sender);
        Serial.print(F("\n  Time: "));
        Serial.print(sms.timestamp);
        Serial.print(F("\n  Body: '"));
        Serial.print(sms.message);
        Serial.println(F("'"));

        // Delete after reading to prevent SIM storage from filling up
        if (!m_modem.deleteSMS(idx)) {
          Serial.print(F("[SYS] Retrying delete for SMS #"));
          Serial.println(idx);
          m_modem.deleteSMS(idx);
        }
      }
    }
  }
}
