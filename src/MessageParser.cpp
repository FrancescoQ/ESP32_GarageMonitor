/**
 * @file MessageParser.cpp
 * @brief SMS command parsing and authorization implementation
 */

#include "MessageParser.h"
#include "Config.h"

MessageParser::MessageParser() {
}

ParseResult MessageParser::parse(const String& sender, const String& message) const {
  String normalized = normalizePhoneNumber(sender);
  SMSCommand cmd = parseCommand(message);

  const AuthorizedUser* user = findUser(normalized);
  if (user == nullptr) {
    return {cmd, false, false, nullptr};
  }

  uint8_t required = requiredPermission(cmd);
  bool permitted = (required != 0) && ((user->permissions & required) != 0);

  return {cmd, true, permitted, user->name};
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

const AuthorizedUser* MessageParser::findUser(const String& normalizedNumber) const {
  for (int i = 0; AUTHORIZED_USERS[i].number != nullptr; i++) {
    if (normalizedNumber.equals(AUTHORIZED_USERS[i].number)) {
      return &AUTHORIZED_USERS[i];
    }
  }
  return nullptr;
}

SMSCommand MessageParser::parseCommand(const String& message) {
  String trimmed = message;
  trimmed.trim();
  trimmed.toUpperCase();

  if (trimmed == "STATUS") return SMSCommand::STATUS;
  if (trimmed == "CLOSE") return SMSCommand::CLOSE;
  if (trimmed == "OPEN") return SMSCommand::OPEN;

  return SMSCommand::UNKNOWN;
}

uint8_t MessageParser::requiredPermission(SMSCommand cmd) {
  switch (cmd) {
    case SMSCommand::STATUS: return PERM_STATUS;
    case SMSCommand::CLOSE:  return PERM_CLOSE;
    case SMSCommand::OPEN:   return PERM_OPEN;
    case SMSCommand::UNKNOWN: return 0;
  }
  return 0;
}
