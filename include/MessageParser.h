/**
 * @file MessageParser.h
 * @brief SMS command parsing and authorization
 *
 * Parses incoming SMS messages, verifies sender against an
 * allowlist, and checks granular permissions before allowing
 * command execution.
 *
 * User lookup delegates to ConfigManager when set, falling back
 * to the hardcoded AUTHORIZED_USERS array otherwise.
 */

#pragma once

#include <Arduino.h>

// Forward declaration — avoids circular include
class ConfigManager;

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
const uint8_t PERM_ADMIN = PERM_STATUS | PERM_CLOSE | PERM_OPEN | PERM_CONFIG;
const uint8_t PERM_CONTROL_FULL = PERM_STATUS | PERM_CLOSE | PERM_OPEN;
const uint8_t PERM_CONTROL_MIN = PERM_STATUS | PERM_CLOSE;
const uint8_t PERM_MONITOR = PERM_STATUS;

// ============================================================================
// Authorized user entry (populated in Secrets.h for defaults)
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
   * @brief Set ConfigManager for dynamic user lookup
   *
   * When set, findUser() delegates to ConfigManager instead of
   * scanning the hardcoded AUTHORIZED_USERS array.
   *
   * @param config Pointer to ConfigManager (must outlive MessageParser)
   */
  void setConfigManager(ConfigManager* config);

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
  ConfigManager* m_config;

  /**
   * @brief Find user by normalized number (delegates to ConfigManager or hardcoded list)
   * @param normalizedNumber Phone number in +CC format
   * @param outName Set to user's display name if found
   * @param outPerms Set to user's permissions if found
   * @return true if user found
   */
  bool findUser(const String& normalizedNumber,
                const char*& outName, uint8_t& outPerms) const;

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
