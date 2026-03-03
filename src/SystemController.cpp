/**
 * @file SystemController.cpp
 * @brief Main system coordinator implementation
 */

#include "SystemController.h"
#include "Config.h"

SystemController::SystemController()
  : m_buttons(m_door), m_lastSMSCheck(0), m_alertSent(false), m_doorWasOpen(false) {
}

void SystemController::begin() {
  Serial.println(F("\n========================================"));
  Serial.println(F("  Garage Monitor - Starting"));
  Serial.println(F("========================================\n"));

  // Door subsystem (relays fail-safe OFF, then sensor init)
  m_door.begin();
  m_doorWasOpen = m_door.isOpen();

  // Water sensor
  m_water.begin();

  // Manual control buttons
  m_buttons.begin();

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
  m_water.loop();
  m_modem.loop();
  m_buttons.loop();

  // Door state change detection
  bool doorOpen = m_door.isOpen();
  if (doorOpen != m_doorWasOpen) {
    m_doorWasOpen = doorOpen;
    if (doorOpen) {
      Serial.println(F("[SYS] Door opened."));
      m_alertSent = false;
      // Don't notify on every state change, it's a waste of SMS.
      // notifyAdmins("Garage door OPEN");
    } else {
      Serial.println(F("[SYS] Door closed."));
      // Don't notify on every state change, it's a waste of SMS.
      // notifyAdmins("Garage door CLOSED");
    }
  }

  // Door open too long alert (single alert per open event)
  if (doorOpen && !m_alertSent && m_door.getOpenDurationMs() >= DOOR_ALERT_DELAY_MS) {
    Serial.print(F("[SYS] Door open >"));
    Serial.print(DOOR_ALERT_DELAY_MIN);
    Serial.println(F("min — sending alert"));
    m_alertSent = true;
    char alertMsg[64];
    snprintf(alertMsg, sizeof(alertMsg), "ALERT: Garage door still open after %lu minutes", DOOR_ALERT_DELAY_MIN);
    notifyAdmins(alertMsg);
  }

  // Water detection — immediate alert to all users
  if (m_water.hasStateChanged()) {
    if (m_water.isWaterDetected()) {
      Serial.println(F("[SYS] Water detected — alerting all users"));
      notifyAllUsers("ALERT: Water detected in garage!");
    } else {
      Serial.println(F("[SYS] Water cleared — notifying all users"));
      notifyAllUsers("Water no longer detected.");
    }
  }

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

    int processed = 0;
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

        handleSMS(sms);
        processed++;
      }
    }

    // Batch-delete all read SMS in one AT command instead of per-message
    if (processed > 0) {
      m_modem.deleteReadSMS();
    }
  }
}

void SystemController::handleSMS(const ReceivedSMS& sms) {
  ParseResult result = m_parser.parse(sms.sender, sms.message);

  if (!result.isAuthorized) {
    Serial.print(F("[SYS] Unauthorized sender: "));
    Serial.println(sms.sender);
    return;
  }

  Serial.print(F("[SYS] Authorized user: "));
  Serial.println(result.userName);

  // Ignore unrecognized commands before permission check
  if (result.command == SMSCommand::UNKNOWN) {
    Serial.println(F("[SYS] Unknown command, ignoring"));
    return;
  }

  if (!result.hasPermission) {
    Serial.println(F("[SYS] Permission denied for command"));
    m_modem.sendSMS(sms.sender.c_str(), "Permission denied.");
    return;
  }

  switch (result.command) {
    case SMSCommand::STATUS: {
      String reply = buildStatusReply();
      Serial.print(F("[SYS] STATUS reply: "));
      Serial.println(reply);
      m_modem.sendSMS(sms.sender.c_str(), reply.c_str());
      break;
    }
    case SMSCommand::CLOSE:
      Serial.println(F("[SYS] Executing CLOSE command"));
      m_door.close();
      m_modem.sendSMS(sms.sender.c_str(), "Closing garage door.");
      break;

    case SMSCommand::OPEN:
      Serial.println(F("[SYS] Executing OPEN command"));
      m_door.open();
      m_modem.sendSMS(sms.sender.c_str(), "Opening garage door.");
      break;

    case SMSCommand::UNKNOWN:
      // Unreachable — handled above, but keeps compiler happy
      break;
  }
}

void SystemController::notifyAdmins(const char* message) {
  for (int i = 0; AUTHORIZED_USERS[i].number != nullptr; i++) {
    if (AUTHORIZED_USERS[i].permissions & PERM_CONFIG) {
      Serial.print(F("[SYS] Notifying "));
      Serial.print(AUTHORIZED_USERS[i].name);
      Serial.print(F(": "));
      Serial.println(message);
      m_modem.sendSMS(AUTHORIZED_USERS[i].number, message);
    }
  }
}

void SystemController::notifyAllUsers(const char* message) {
  for (int i = 0; AUTHORIZED_USERS[i].number != nullptr; i++) {
    Serial.print(F("[SYS] Notifying "));
    Serial.print(AUTHORIZED_USERS[i].name);
    Serial.print(F(": "));
    Serial.println(message);
    m_modem.sendSMS(AUTHORIZED_USERS[i].number, message);
  }
}

String SystemController::buildStatusReply() {
  String reply;

  // Door state
  if (m_door.isOpen()) {
    unsigned long openMs = m_door.getOpenDurationMs();
    unsigned long openMin = openMs / 60000;
    reply = "Door: OPEN (" + String(openMin) + "min)";
  } else {
    reply = "Door: CLOSED";
  }

  // Water state
  reply += m_water.isWaterDetected() ? " | Water: ALERT" : " | Water: OK";

  // Signal quality
  int16_t csq = m_modem.getSignalQuality();
  reply += " | CSQ: " + String(csq) + "/31";

  return reply;
}
