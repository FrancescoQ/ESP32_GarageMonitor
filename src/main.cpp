/**
 * @file main.cpp
 * @brief SIM7000G modem diagnostic sketch
 *
 * Focused on establishing UART communication with SIM7000G.
 * Handles PWRKEY power-on, SIM PIN unlock, and network registration.
 *
 * @author Francesco
 * @date February 2026
 */

#define TINY_GSM_MODEM_SIM7000
#define TINY_GSM_DEBUG Serial

#include <Arduino.h>
#include <TinyGsmClient.h>
#include "Config.h"

#define SerialAT Serial2

TinyGsm modem(SerialAT);

unsigned long lastSignalCheck = 0;
const unsigned long SIGNAL_INTERVAL = 10000;
bool modemReady = false;

/**
 * @brief Send raw AT command and return response
 * @param cmd Full AT command string (e.g. "AT", "AT+CSQ")
 * @param timeout Timeout in ms
 * @return Response string from modem
 */
String sendAT(const char* cmd, unsigned long timeout = 2000) {
  delay(100);  // Small gap between commands

  while (SerialAT.available()) {
    SerialAT.read();
  }

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
    // Return early once we have a complete response
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

/**
 * @brief Try to get an "OK" response from the modem
 * @param attempts Number of AT attempts
 * @return true if modem responded with OK
 */
bool testModemAT(int attempts = 5) {
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

/**
 * @brief Toggle SIM7000G power via PWRKEY pin
 *
 * The SIM7000G requires a LOW pulse on PWRKEY for >1s to toggle power.
 */
void toggleModemPower() {
  Serial.println(F("[INFO] Toggling PWRKEY (LOW for 1.5s)..."));
  digitalWrite(PIN_SIM_PWR, LOW);
  delay(1500);
  digitalWrite(PIN_SIM_PWR, HIGH);
  Serial.println(F("[INFO] PWRKEY released, waiting 5s for modem boot..."));
  delay(5000);
}

/**
 * @brief Power on modem and establish UART communication
 * @return true if modem responds to AT commands
 */
bool powerOnModem() {
  SerialAT.begin(SIM_BAUD_RATE, SERIAL_8N1, PIN_SIM_RXD, PIN_SIM_TXD);
  delay(500);
  while (SerialAT.available()) {
    SerialAT.read();
  }

  Serial.println(F("\n--- Step 1: Check if modem is already on ---"));
  if (testModemAT(3)) {
    Serial.println(F("[INFO] Modem already on!"));
    return true;
  }

  Serial.println(F("\n--- Step 2: Toggling PWRKEY to turn on ---"));
  toggleModemPower();
  if (testModemAT(5)) {
    Serial.println(F("[INFO] Modem powered on!"));
    return true;
  }

  Serial.println(F("\n--- Step 3: Toggling PWRKEY again ---"));
  Serial.println(F("[INFO] (First toggle may have turned modem OFF)"));
  toggleModemPower();
  if (testModemAT(5)) {
    Serial.println(F("[INFO] Modem powered on!"));
    return true;
  }

  return false;
}

void setup() {
  Serial.begin(SERIAL_BAUD_RATE);
  delay(1000);
  Serial.println(F("\n========================================"));
  Serial.println(F("  Garage Monitor - SIM7000G Diagnostic"));
  Serial.println(F("========================================\n"));

  Serial.print(F("[INFO] UART pins: RX=GPIO"));
  Serial.print(PIN_SIM_RXD);
  Serial.print(F(", TX=GPIO"));
  Serial.println(PIN_SIM_TXD);
  Serial.print(F("[INFO] PWRKEY pin: GPIO"));
  Serial.println(PIN_SIM_PWR);
  Serial.print(F("[INFO] Baud rate: "));
  Serial.println(SIM_BAUD_RATE);

  // Setup PWRKEY pin
  pinMode(PIN_SIM_PWR, OUTPUT);
  digitalWrite(PIN_SIM_PWR, HIGH);

  // DTR LOW = keep modem awake (floating DTR can cause sleep!)
  pinMode(PIN_SIM_DTR, OUTPUT);
  digitalWrite(PIN_SIM_DTR, LOW);
  Serial.println(F("[INFO] DTR set LOW (modem stay awake)"));

  delay(100);

  // =============================================
  // Step 1-3: Power on modem
  // =============================================
  if (!powerOnModem()) {
    Serial.println(F("\n[ERROR] SIM7000G NOT responding!"));
    Serial.println(F("[ERROR] Check wiring: TX/RX, PWRKEY, GND"));
    Serial.println(F("\n[INFO] Starting main loop\n"));
    lastSignalCheck = millis();
    return;
  }

  // =============================================
  // Disable echo, show modem info
  // =============================================
  sendAT("ATE0");
  delay(100);
  while (SerialAT.available()) {
    SerialAT.read();
  }

  Serial.println(F("\n--- Modem info ---"));
  sendAT("AT+GMM");
  sendAT("AT+GSV");

  // =============================================
  // Handle SIM PIN
  // =============================================
  Serial.println(F("\n--- SIM PIN ---"));

  // Verify modem is still alive before SIM access
  String aliveCheck = sendAT("AT");
  if (aliveCheck.indexOf("OK") < 0) {
    Serial.println(F("[ERROR] Modem stopped responding before CPIN!"));
    Serial.println(F("[ERROR] Possible causes: DTR sleep, power brownout"));
    Serial.println(F("[INFO] Starting main loop\n"));
    lastSignalCheck = millis();
    return;
  }

  delay(500);
  String cpinResponse = sendAT("AT+CPIN?", 10000);

  if (cpinResponse.indexOf("SIM PIN") >= 0) {
    Serial.print(F("[INFO] SIM requires PIN, sending: "));
    Serial.println(SIM_PIN);

    String cmd = "AT+CPIN=\"";
    cmd += SIM_PIN;
    cmd += "\"";
    String pinResponse = sendAT(cmd.c_str(), 10000);

    if (pinResponse.indexOf("OK") >= 0 || pinResponse.indexOf("READY") >= 0) {
      Serial.println(F("[INFO] SIM PIN accepted!"));
      Serial.println(F("[INFO] Waiting 5s for SIM initialization..."));
      delay(5000);
    } else {
      Serial.println(F("[ERROR] SIM PIN failed! Check PIN in Config.h"));
      Serial.println(F("[ERROR] WARNING: Too many wrong PINs will lock the SIM!"));
    }
  } else if (cpinResponse.indexOf("READY") >= 0) {
    Serial.println(F("[INFO] SIM ready (no PIN needed)"));
  } else if (cpinResponse.indexOf("SIM PUK") >= 0) {
    Serial.println(F("[ERROR] SIM PUK locked! Use a phone to unlock."));
  } else {
    Serial.println(F("[WARN] Unexpected CPIN response"));
  }

  // Verify SIM is now ready
  String verifyResponse = sendAT("AT+CPIN?", 5000);
  if (verifyResponse.indexOf("READY") >= 0) {
    Serial.println(F("[INFO] SIM confirmed READY"));
  } else {
    Serial.println(F("[WARN] SIM may not be ready yet"));
  }

  // =============================================
  // Check signal and network (raw AT first)
  // =============================================
  Serial.println(F("\n--- Network status (raw AT) ---"));
  sendAT("AT+CSQ");
  sendAT("AT+CREG?", 5000);
  sendAT("AT+COPS?", 10000);

  // =============================================
  // TinyGSM init (uses the already-running modem)
  // =============================================
  Serial.println(F("\n--- TinyGSM init ---"));

  // Clean flush before TinyGSM takes over
  while (SerialAT.available()) {
    SerialAT.read();
  }
  delay(500);

  if (modem.init()) {
    Serial.println(F("[INFO] modem.init() SUCCESS!"));
    modemReady = true;

    String modemName = modem.getModemName();
    Serial.print(F("[INFO] Modem: "));
    Serial.println(modemName);

    // TinyGSM init may have reset echo/config but NOT the modem power
    // Re-enter PIN if SIM went back to locked state
    int simStatus = modem.getSimStatus();
    Serial.print(F("[INFO] SIM status: "));
    Serial.println(simStatus);

    if (simStatus != 1 && simStatus != 3) {
      Serial.println(F("[INFO] Re-entering SIM PIN via TinyGSM..."));
      modem.simUnlock(SIM_PIN);
      delay(5000);
    }

    Serial.println(F("[INFO] Waiting for network (max 60s)..."));
    if (modem.waitForNetwork(60000L, true)) {
      Serial.println(F("[INFO] Registered on network!"));

      String op = modem.getOperator();
      Serial.print(F("[INFO] Operator: "));
      Serial.println(op);

      int16_t signal = modem.getSignalQuality();
      Serial.print(F("[INFO] Signal (CSQ): "));
      Serial.print(signal);
      Serial.println(F("/31"));

      // Set SMS to text mode
      sendAT("AT+CMGF=1", 3000);
      Serial.println(F("[INFO] SMS text mode enabled"));
    } else {
      Serial.println(F("[WARN] Network registration timeout (60s)"));
      Serial.println(F("[WARN] Check antenna and signal strength"));
    }
  } else {
    Serial.println(F("[WARN] modem.init() FAILED - will use raw AT"));
    Serial.println(F("[INFO] This is OK for diagnostics"));
  }

  // =============================================
  // Start loop
  // =============================================
  Serial.println(F("\n[INFO] Starting main loop (signal check every 10s)\n"));
  lastSignalCheck = millis();
}

void loop() {
  unsigned long now = millis();
  if (now - lastSignalCheck >= SIGNAL_INTERVAL) {
    lastSignalCheck = now;

    // Use raw AT if TinyGSM init failed
    if (modemReady) {
      String op = modem.getOperator();
      int16_t signal = modem.getSignalQuality();
      Serial.print(F("[INFO] Operator: "));
      Serial.print(op);
      Serial.print(F(" | CSQ: "));
      Serial.print(signal);
      Serial.println(F("/31"));

      if (signal == 99) {
        Serial.println(F("[WARN] CSQ=99 means no signal"));
      }
    } else {
      sendAT("AT+CSQ");
      sendAT("AT+CREG?");
    }
  }
  delay(10);
}
