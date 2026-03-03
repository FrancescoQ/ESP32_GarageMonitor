/**
 * @file MessageParser.h
 * @brief SMS command parsing and authorization
 *
 * Parses incoming SMS messages, verifies sender against an
 * allowlist, and checks granular permissions before allowing
 * command execution.
 */

#pragma once

#include <Arduino.h>

// ============================================================================
// Permission flags (bitmask)
// ============================================================================

enum Permission {
  PERM_STATUS = 0x01,  // Can request status
  PERM_CLOSE  = 0x02,  // Can close door
  PERM_OPEN   = 0x04,  // Can open door
  PERM_CONFIG = 0x08   // Can change settings
};

// Permission presets
const uint8_t PERM_ADMIN   = PERM_STATUS | PERM_CLOSE | PERM_OPEN | PERM_CONFIG;
const uint8_t PERM_CONTROL = PERM_STATUS | PERM_CLOSE;
const uint8_t PERM_MONITOR = PERM_STATUS;

// ============================================================================
// Authorized user entry (populated in Secrets.h)
// ============================================================================

struct AuthorizedUser {
  const char* number;      // Normalized +39 format
  uint8_t permissions;
  const char* name;        // For logging
};

// ============================================================================
// Parsed command types
// ============================================================================

enum class SMSCommand {
  STATUS,
  CLOSE,
  OPEN,
  UNKNOWN
};

// ============================================================================
// Result of parsing + authorization
// ============================================================================

struct ParseResult {
  SMSCommand command;
  bool isAuthorized;     // Sender found in allowlist
  bool hasPermission;    // Sender has permission for this command
  const char* userName;  // Name from allowlist (nullptr if unknown sender)
};

// ============================================================================
// MessageParser class
// ============================================================================

/**
 * @brief Parses SMS commands and verifies sender authorization
 *
 * Takes a sender number and message body, normalizes the number,
 * looks up the sender in the authorized users list, parses the
 * command, and checks permissions. Returns a ParseResult with
 * all authorization details.
 */
class MessageParser {
public:
  MessageParser();

  /**
   * @brief Parse an incoming SMS and check authorization
   * @param sender Phone number of the SMS sender
   * @param message SMS body text
   * @return ParseResult with command, authorization, and permission info
   */
  ParseResult parse(const String& sender, const String& message) const;

  /**
   * @brief Normalize phone number to +CCXXXXXXXXXX format
   *
   * Strips spaces/dashes, converts 00CC→+CC (any country code).
   *
   * @param number Raw phone number string
   * @return Normalized number string
   */
  static String normalizePhoneNumber(const String& number);

private:
  /**
   * @brief Find user in AUTHORIZED_USERS by normalized number
   * @param normalizedNumber Phone number in +39 format
   * @return Pointer to AuthorizedUser entry, or nullptr if not found
   */
  const AuthorizedUser* findUser(const String& normalizedNumber) const;

  /**
   * @brief Parse command string to enum (case-insensitive, trimmed)
   * @param message SMS body text
   * @return SMSCommand enum value
   */
  static SMSCommand parseCommand(const String& message);

  /**
   * @brief Get required permission flag for a command
   * @param cmd Command to check
   * @return Permission bitmask required, or 0 for UNKNOWN
   */
  static uint8_t requiredPermission(SMSCommand cmd);
};
