/**
 * @file ModemHandler.cpp
 * @brief SIM7000G modem management implementation
 *
 * Extracted from main.cpp diagnostic sketch. Handles the full modem
 * lifecycle: PWRKEY power-on, SIM PIN unlock, TinyGSM initialization,
 * network registration, and periodic signal monitoring.
 *
 * @author Francesco
 * @date February 2026
 */

#include "ModemHandler.h"
#include "Config.h"

#define SerialAT Serial2

static const unsigned long SIGNAL_CHECK_INTERVAL = 10000;

ModemHandler::ModemHandler()
  : m_modem(SerialAT),
    m_ready(false),
    m_networkConnected(false),
    m_lastSignalCheck(0) {
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

String ModemHandler::getOperator() {
  return m_modem.getOperator();
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
