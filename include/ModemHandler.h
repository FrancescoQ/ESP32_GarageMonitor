/**
 * @file ModemHandler.h
 * @brief SIM7000G modem management
 *
 * Encapsulates power-on sequence, SIM PIN unlock, TinyGSM initialization,
 * network registration, and periodic signal monitoring.
 *
 * @author Francesco
 * @date February 2026
 */

#pragma once

#include <Arduino.h>
#include <TinyGsmClient.h>

/**
 * @brief Manages SIM7000G modem lifecycle and communication
 *
 * Handles the full modem startup sequence (PWRKEY toggle, SIM PIN,
 * TinyGSM init, network registration) and provides signal monitoring.
 * Exposes the TinyGsm instance for use by future SMSHandler.
 */
class ModemHandler {
public:
  ModemHandler();

  /**
   * @brief Initialize modem: power on, PIN unlock, TinyGSM init, network
   * @return true if modem is initialized and registered on network
   */
  bool begin();

  /**
   * @brief Periodic modem tasks (signal quality monitoring)
   */
  void loop();

  /**
   * @brief Check if TinyGSM initialized successfully
   * @return true if modem.init() succeeded
   */
  bool isReady() const;

  /**
   * @brief Check if registered on cellular network
   * @return true if network registration succeeded
   */
  bool isNetworkConnected() const;

  /**
   * @brief Get current signal quality
   * @return CSQ value (0-31, 99=unknown)
   */
  int16_t getSignalQuality();

  /**
   * @brief Get current network operator name
   * @return Operator string (e.g. "Iliad")
   */
  String getOperator();

  /**
   * @brief Send an SMS message
   * @param number Destination phone number (format: +39xxxxxxxxxx)
   * @param message Message text (max 160 chars)
   * @return true if SMS sent successfully
   */
  bool sendSMS(const char* number, const char* message);

  /**
   * @brief Access the TinyGsm instance for SMS/data operations
   * @return Reference to internal TinyGsm modem object
   */
  TinyGsm& getModem();

private:
  TinyGsm m_modem;
  bool m_ready;
  bool m_networkConnected;
  unsigned long m_lastSignalCheck;

  /**
   * @brief Send raw AT command and return response
   * @param cmd AT command string
   * @param timeout Response timeout in ms
   * @return Response string from modem
   */
  String sendCommand(const char* cmd, unsigned long timeout = 2000);

  /**
   * @brief Test if modem responds to AT commands
   * @param attempts Number of AT attempts
   * @return true if modem responded with OK
   */
  bool testConnection(int attempts = 5);

  /**
   * @brief Toggle modem power via PWRKEY pin
   */
  void togglePower();

  /**
   * @brief Power on modem and establish UART communication
   * @return true if modem responds to AT commands
   */
  bool powerOn();

  /**
   * @brief Flush any pending data from modem serial
   */
  void flushSerial();
};
