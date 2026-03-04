/**
 * @file ModemHandler.cpp
 * @brief SIM7000G modem management implementation
 *
 * Extracted from main.cpp diagnostic sketch. Handles the full modem
 * lifecycle: PWRKEY power-on, SIM PIN unlock, TinyGSM initialization,
 * network registration, and periodic signal monitoring.
 */

#include "ModemHandler.h"
#include "Config.h"

#define SerialAT Serial2

static const unsigned long SIGNAL_CHECK_INTERVAL = 10000;

ModemHandler::ModemHandler()
  : m_modem(SerialAT),
    m_ready(false),
    m_networkConnected(false),
    m_lastSignalCheck(0),
    m_smsCount(0) {
  memset(m_smsIndices, 0, sizeof(m_smsIndices));
}

bool ModemHandler::begin() {
  // Setup PWRKEY pin
  pinMode(PIN_SIM_PWR, OUTPUT);
  digitalWrite(PIN_SIM_PWR, HIGH);

  // DTR LOW = keep modem awake (floating DTR can cause sleep!)
  pinMode(PIN_SIM_DTR, OUTPUT);
  digitalWrite(PIN_SIM_DTR, LOW);
  Serial.println(F("[MODEM] DTR set LOW (modem stay awake)"));

  delay(100);

  // Power on modem
  if (!powerOn()) {
    Serial.println(F("[MODEM] SIM7000G NOT responding!"));
    Serial.println(F("[MODEM] Check wiring: TX/RX, PWRKEY, GND"));
    return false;
  }

  // Disable echo
  sendCommand("ATE0");
  delay(100);
  flushSerial();

  // TinyGSM init (handles SIM PIN internally via getSimStatus/simUnlock)
  Serial.println(F("[MODEM] TinyGSM init..."));
  flushSerial();
  delay(500);

  if (!m_modem.init()) {
    Serial.println(F("[MODEM] modem.init() FAILED"));
    return false;
  }

  Serial.println(F("[MODEM] modem.init() SUCCESS"));
  m_ready = true;

  String modemName = m_modem.getModemName();
  Serial.print(F("[MODEM] Model: "));
  Serial.println(modemName);

  // Re-enter PIN if TinyGSM init reset the SIM state
  int simStatus = m_modem.getSimStatus();
  Serial.print(F("[MODEM] SIM status: "));
  Serial.println(simStatus);

  if (simStatus != 1 && simStatus != 3) {
    Serial.println(F("[MODEM] Unlocking SIM PIN via TinyGSM..."));
    m_modem.simUnlock(SIM_PIN);
    // SIM unlock can trigger modem internal restart, give it time to settle
    Serial.println(F("[MODEM] Waiting for SIM to settle..."));
    delay(7000);
  }

  // Wait for network registration
  Serial.println(F("[MODEM] Waiting for network (max 60s)..."));
  if (m_modem.waitForNetwork(60000L, true)) {
    m_networkConnected = true;
    Serial.println(F("[MODEM] Registered on network!"));

    String op = m_modem.getOperator();
    Serial.print(F("[MODEM] Operator: "));
    Serial.println(op);

    int16_t signal = m_modem.getSignalQuality();
    Serial.print(F("[MODEM] Signal (CSQ): "));
    Serial.print(signal);
    Serial.println(F("/31"));

    // Set SMS to text mode
    sendCommand("AT+CMGF=1", 3000);
    Serial.println(F("[MODEM] SMS text mode enabled"));
  } else {
    Serial.println(F("[MODEM] Network registration timeout (60s)"));
    Serial.println(F("[MODEM] Check antenna and signal strength"));
  }

  m_lastSignalCheck = millis();
  return m_ready;
}

void ModemHandler::loop() {
  unsigned long now = millis();
  if (now - m_lastSignalCheck < SIGNAL_CHECK_INTERVAL) {
    return;
  }
  m_lastSignalCheck = now;

  if (m_ready) {
    String op = m_modem.getOperator();
    int16_t signal = m_modem.getSignalQuality();
    Serial.print(F("[MODEM] Operator: "));
    Serial.print(op);
    Serial.print(F(" | CSQ: "));
    Serial.print(signal);
    Serial.println(F("/31"));

    if (signal == 99) {
      Serial.println(F("[MODEM] CSQ=99 means no signal"));
    }
  } else {
    sendCommand("AT+CSQ");
    sendCommand("AT+CREG?");
  }
}

bool ModemHandler::isReady() const {
  return m_ready;
}

bool ModemHandler::isNetworkConnected() const {
  return m_networkConnected;
}

int16_t ModemHandler::getSignalQuality() {
  return m_modem.getSignalQuality();
}

int ModemHandler::getSignalStars() {
  int16_t csq = getSignalQuality();
  if (csq == 99 || csq < 0)  return 0;
  if (csq < 10) return 1;
  if (csq < 15) return 2;
  if (csq < 20) return 3;
  return 4;
}

String ModemHandler::getOperator() {
  String op = m_modem.getOperator();
  op.trim();

  // Some carriers report doubled names (e.g. "Iliad Iliad",
  // "My Carrier My Carrier"). If the string is "X X", return "X".
  int len = op.length();
  if (len >= 3 && len % 2 == 1 && op.charAt(len / 2) == ' ') {
    String first = op.substring(0, len / 2);
    String second = op.substring(len / 2 + 1);
    if (first.equalsIgnoreCase(second)) {
      return first;
    }
  }

  return op;
}

bool ModemHandler::sendSMS(const char* number, const char* message) {
  if (!m_networkConnected) {
    Serial.println(F("[MODEM] Cannot send SMS: not connected to network"));
    return false;
  }

  Serial.print(F("[MODEM] Sending SMS to "));
  Serial.print(number);
  Serial.print(F(": "));
  Serial.println(message);

  bool success = m_modem.sendSMS(String(number), String(message));

  if (success) {
    Serial.println(F("[MODEM] SMS sent successfully!"));
  } else {
    Serial.println(F("[MODEM] SMS send FAILED"));
  }

  return success;
}

int ModemHandler::checkForSMS() {
  m_smsCount = 0;

  if (!m_networkConnected) {
    return 0;
  }

  String response = sendCommand("AT+CMGL=\"REC UNREAD\"", 5000);

  // Parse +CMGL: <index> entries from response
  int searchFrom = 0;
  while (searchFrom < (int)response.length() && m_smsCount < MAX_SMS_SLOTS) {
    int pos = response.indexOf("+CMGL: ", searchFrom);
    if (pos < 0) {
      break;
    }

    // Extract index number after "+CMGL: "
    pos += 7;
    int commaPos = response.indexOf(',', pos);
    if (commaPos < 0) {
      break;
    }

    String indexStr = response.substring(pos, commaPos);
    indexStr.trim();
    m_smsIndices[m_smsCount] = indexStr.toInt();
    m_smsCount++;

    searchFrom = commaPos + 1;
  }

  if (m_smsCount > 0) {
    Serial.print(F("[MODEM] Found "));
    Serial.print(m_smsCount);
    Serial.println(F(" SMS message(s)"));
  }

  return m_smsCount;
}

int ModemHandler::getSMSIndex(int position) const {
  if (position < 0 || position >= m_smsCount) {
    return -1;
  }
  return m_smsIndices[position];
}

bool ModemHandler::readSMS(int index, ReceivedSMS& sms) {
  if (!m_networkConnected) {
    return false;
  }

  char cmd[20];
  snprintf(cmd, sizeof(cmd), "AT+CMGR=%d", index);
  String response = sendCommand(cmd, 3000);

  // Find +CMGR: header
  int headerPos = response.indexOf("+CMGR:");
  if (headerPos < 0) {
    Serial.println(F("[MODEM] No +CMGR header in response"));
    return false;
  }

  sms.index = index;

  // Parse quoted fields: +CMGR: "status","sender","name","timestamp"
  // 1st quoted field: status
  int quoteStart = response.indexOf('"', headerPos);
  if (quoteStart < 0) return false;
  int quoteEnd = response.indexOf('"', quoteStart + 1);
  if (quoteEnd < 0) return false;

  // 2nd quoted field: sender number
  quoteStart = response.indexOf('"', quoteEnd + 1);
  if (quoteStart < 0) return false;
  quoteEnd = response.indexOf('"', quoteStart + 1);
  if (quoteEnd < 0) return false;
  sms.sender = response.substring(quoteStart + 1, quoteEnd);

  // 3rd quoted field: phonebook name or timestamp
  // When phonebook is empty the modem returns ",," (unquoted), so the next
  // quoted field is actually the timestamp, not the phonebook name.
  quoteStart = response.indexOf('"', quoteEnd + 1);
  if (quoteStart >= 0) {
    quoteEnd = response.indexOf('"', quoteStart + 1);
    if (quoteEnd >= 0) {
      String field = response.substring(quoteStart + 1, quoteEnd);
      if (field.indexOf('/') >= 0) {
        // Contains date separator — this is the timestamp (phonebook was empty)
        sms.timestamp = field;
      } else {
        // This was the phonebook name, next quoted field is timestamp
        quoteStart = response.indexOf('"', quoteEnd + 1);
        if (quoteStart >= 0) {
          quoteEnd = response.indexOf('"', quoteStart + 1);
          if (quoteEnd >= 0) {
            sms.timestamp = response.substring(quoteStart + 1, quoteEnd);
          }
        }
      }
    }
  }

  // Message body: line after +CMGR header, before OK
  int headerEnd = response.indexOf('\n', headerPos);
  if (headerEnd < 0) return false;

  int okPos = response.lastIndexOf("OK");
  if (okPos > headerEnd) {
    sms.message = response.substring(headerEnd + 1, okPos);
  } else {
    sms.message = response.substring(headerEnd + 1);
  }
  sms.message.trim();

  Serial.print(F("[MODEM] SMS #"));
  Serial.print(index);
  Serial.print(F(" from: "));
  Serial.print(sms.sender);
  Serial.print(F(" msg: '"));
  Serial.print(sms.message);
  Serial.println(F("'"));

  return true;
}

bool ModemHandler::deleteSMS(int index) {
  char cmd[20];
  snprintf(cmd, sizeof(cmd), "AT+CMGD=%d", index);
  String response = sendCommand(cmd, 3000);
  bool success = response.indexOf("OK") >= 0;

  if (success) {
    Serial.print(F("[MODEM] Deleted SMS #"));
    Serial.println(index);
  } else {
    Serial.print(F("[MODEM] Failed to delete SMS #"));
    Serial.println(index);
  }

  return success;
}

bool ModemHandler::deleteReadSMS() {
  String response = sendCommand("AT+CMGD=1,1", 5000);
  bool success = response.indexOf("OK") >= 0;

  if (success) {
    Serial.println(F("[MODEM] Deleted all read SMS"));
  } else {
    Serial.println(F("[MODEM] Failed to delete read SMS"));
  }

  return success;
}

bool ModemHandler::deleteAllSMS() {
  String response = sendCommand("AT+CMGD=1,4", 5000);
  bool success = response.indexOf("OK") >= 0;

  if (success) {
    Serial.println(F("[MODEM] Deleted all SMS messages"));
    m_smsCount = 0;
  } else {
    Serial.println(F("[MODEM] Failed to delete all SMS"));
  }

  return success;
}

TinyGsm& ModemHandler::getModem() {
  return m_modem;
}

// -- Private methods --

String ModemHandler::sendCommand(const char* cmd, unsigned long timeout) {
  delay(100);
  flushSerial();

  Serial.print(F("[AT] Sending: "));
  Serial.println(cmd);

  SerialAT.println(cmd);

  unsigned long start = millis();
  String response = "";
  while (millis() - start < timeout) {
    while (SerialAT.available()) {
      char c = SerialAT.read();
      response += c;
    }
    if (response.indexOf("OK") >= 0 || response.indexOf("ERROR") >= 0) {
      break;
    }
    delay(10);
  }

  Serial.print(F("[AT] Response: '"));
  Serial.print(response);
  Serial.println(F("'"));
  return response;
}

bool ModemHandler::testConnection(int attempts) {
  for (int i = 0; i < attempts; i++) {
    SerialAT.println("AT");
    delay(500);
    if (SerialAT.available()) {
      String r = SerialAT.readString();
      Serial.print(F("  Attempt "));
      Serial.print(i + 1);
      Serial.print(F(": '"));
      Serial.print(r);
      Serial.println(F("'"));
      if (r.indexOf("OK") >= 0) {
        return true;
      }
    } else {
      Serial.print(F("  Attempt "));
      Serial.print(i + 1);
      Serial.println(F(": no response"));
    }
  }
  return false;
}

void ModemHandler::togglePower() {
  Serial.println(F("[MODEM] Toggling PWRKEY (LOW for 1.5s)..."));
  digitalWrite(PIN_SIM_PWR, LOW);
  delay(1500);
  digitalWrite(PIN_SIM_PWR, HIGH);
  Serial.println(F("[MODEM] PWRKEY released, waiting 5s for modem boot..."));
  delay(5000);
}

bool ModemHandler::powerOn() {
  SerialAT.begin(SIM_BAUD_RATE, SERIAL_8N1, PIN_SIM_RXD, PIN_SIM_TXD);
  delay(500);
  flushSerial();

  // Check if modem is already on
  Serial.println(F("[MODEM] Checking if modem is already on..."));
  if (testConnection(3)) {
    Serial.println(F("[MODEM] Modem already on!"));
    return true;
  }

  // First PWRKEY toggle
  Serial.println(F("[MODEM] Toggling PWRKEY to turn on..."));
  togglePower();
  if (testConnection(5)) {
    Serial.println(F("[MODEM] Modem powered on!"));
    return true;
  }

  // Second toggle (first may have turned modem OFF)
  Serial.println(F("[MODEM] Toggling PWRKEY again (first toggle may have turned modem OFF)..."));
  togglePower();
  if (testConnection(5)) {
    Serial.println(F("[MODEM] Modem powered on!"));
    return true;
  }

  return false;
}

void ModemHandler::flushSerial() {
  while (SerialAT.available()) {
    SerialAT.read();
  }
}
