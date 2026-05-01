/**
 * @file MessageParser.cpp
 * @brief SMS command parsing and authorization implementation
 */

#include "MessageParser.h"
#include "ConfigManager.h"
#include "Config.h"

MessageParser::MessageParser()
  : m_config(nullptr) {
}

void MessageParser::setConfigManager(ConfigManager* config) {
  m_config = config;
}

ParseResult MessageParser::parse(const String& sender, const String& message) const {
  String normalized = normalizePhoneNumber(sender);
  SMSCommand cmd = parseCommand(message);

  const char* userName = nullptr;
  uint8_t userPerms = 0;

  if (!findUser(normalized, userName, userPerms)) {
    return {cmd, false, false, nullptr, 0};
  }

  uint8_t required = requiredPermission(cmd);
  bool permitted = (required != 0) && ((userPerms & required) != 0);

  return {cmd, true, permitted, userName, userPerms};
}

String MessageParser::normalizePhoneNumber(const String& number) {
  String normalized = number;

  // Strip spaces and dashes
  normalized.replace(" ", "");
  normalized.replace("-", "");

  // Standardize "00XX..." international prefix to "+XX..."
  if (normalized.startsWith("00")) {
    normalized = "+" + normalized.substring(2);
  }

  return normalized;
}

bool MessageParser::findUser(const String& normalizedNumber,
                             const char*& outName, uint8_t& outPerms) const {
  if (m_config != nullptr) {
    // Delegate to ConfigManager (NVS-backed)
    const NvsUser* user = m_config->findUserByPhone(normalizedNumber);
    if (user != nullptr) {
      outName = user->name.c_str();
      outPerms = user->permissions;
      return true;
    }
    return false;
  }

  // Fallback: hardcoded AUTHORIZED_USERS (before ConfigManager is set)
  for (int i = 0; AUTHORIZED_USERS[i].number != nullptr; i++) {
    if (normalizedNumber.equals(AUTHORIZED_USERS[i].number)) {
      outName = AUTHORIZED_USERS[i].name;
      outPerms = AUTHORIZED_USERS[i].permissions;
      return true;
    }
  }
  return false;
}

SMSCommand MessageParser::parseCommand(const String& message) {
  String trimmed = message;
  trimmed.trim();
  trimmed.toUpperCase();

  if (trimmed == "STATUS" || trimmed == "STATO") return SMSCommand::STATUS;
  if (trimmed == "CLOSE" || trimmed == "CHIUDI") return SMSCommand::CLOSE;
  if (trimmed == "OPEN" || trimmed == "APRI") return SMSCommand::OPEN;
  if (trimmed == "CREDIT" || trimmed == "CREDITO") return SMSCommand::CREDIT;
  if (trimmed == "REBOOT" || trimmed == "RIAVVIO") return SMSCommand::REBOOT;
  if (trimmed == "HELP" || trimmed == "AIUTO") return SMSCommand::HELP;

  return SMSCommand::UNKNOWN;
}

uint8_t MessageParser::requiredPermission(SMSCommand cmd) {
  switch (cmd) {
    case SMSCommand::STATUS: return PERM_STATUS;
    case SMSCommand::CLOSE:  return PERM_CLOSE;
    case SMSCommand::OPEN:   return PERM_OPEN;
    case SMSCommand::CREDIT:  return PERM_CONFIG;
    case SMSCommand::REBOOT:  return PERM_CONFIG;
    case SMSCommand::HELP:    return PERM_STATUS;
    case SMSCommand::UNKNOWN: return 0;
  }
  return 0;
}
