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
 * @brief Parsed data from a received SMS message
 */
struct ReceivedSMS {
  int index;          ///< SIM storage index
  String sender;      ///< Sender phone number
  String message;     ///< Message body text
  String timestamp;   ///< Delivery timestamp from network
};

/// Maximum number of SMS the polling buffer can track
static const int MAX_SMS_SLOTS = 30;

/**
 * @brief Manages SIM7000G modem lifecycle and communication
 *
 * Handles the full modem startup sequence (PWRKEY toggle, SIM PIN,
 * TinyGSM init, network registration) and provides signal monitoring.
 * Includes raw AT command support for SMS receive/read/delete
 * (TinyGSM only supports send).
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
   * @brief Poll SIM storage for SMS messages
   *
   * Sends AT+CMGL="REC UNREAD" and parses response to find unread SMS indices.
   * Indices are stored internally and accessible via getSMSIndex().
   *
   * @return Number of SMS messages found (0 if none or error)
   */
  int checkForSMS();

  /**
   * @brief Get SIM storage index for a found SMS
   * @param position Position in the found SMS list (0-based)
   * @return SIM storage index, or -1 if position is out of range
   */
  int getSMSIndex(int position) const;

  /**
   * @brief Read a single SMS from SIM storage
   * @param index SIM storage index (from getSMSIndex or checkForSMS)
   * @param sms Output struct filled with sender, message, timestamp
   * @return true if SMS was read and parsed successfully
   */
  bool readSMS(int index, ReceivedSMS& sms);

  /**
   * @brief Delete a single SMS from SIM storage
   * @param index SIM storage index to delete
   * @return true if deletion succeeded
   */
  bool deleteSMS(int index);

  /**
   * @brief Delete all read SMS from SIM storage
   *
   * Uses AT+CMGD=1,1 to delete all "received read" messages in one
   * command. Unread messages are preserved. Call after processing a
   * batch of SMS to keep SIM storage clean without per-message deletes.
   *
   * @return true if deletion succeeded
   */
  bool deleteReadSMS();

  /**
   * @brief Delete all SMS from SIM storage
   *
   * Uses AT+CMGD=1,4 to purge all messages. Use as a safety net
   * when SIM storage is critically full (silently drops new SMS).
   *
   * @return true if deletion succeeded
   */
  bool deleteAllSMS();

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
  int m_smsIndices[MAX_SMS_SLOTS];  ///< SIM storage indices from last poll
  int m_smsCount;                   ///< Number of SMS found in last poll

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
