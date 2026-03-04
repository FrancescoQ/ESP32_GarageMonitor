/**
 * @file ConfigManager.cpp
 * @brief NVS-backed persistent storage implementation
 *
 * Uses ESP32 Preferences library for NVS access. Two namespaces:
 * - "users": Authorized user entries (phone, permissions, name)
 * - "settings": System configuration values
 *
 * On first boot (no "count" key in "users" namespace), seeds NVS
 * from the hardcoded AUTHORIZED_USERS array in Secrets.h.
 */

#include "ConfigManager.h"
#include <Preferences.h>

// NVS namespace names
static const char* NS_USERS = "users";
static const char* NS_SETTINGS = "settings";

// NVS key names for settings
static const char* KEY_DOOR_ALERT = "door_alert_min";
static const char* KEY_SMS_POLL = "sms_poll_ms";
static const char* KEY_DEEP_SLEEP = "deep_sleep_on";

ConfigManager::ConfigManager()
  : m_userCount(0),
    m_initialized(false) {
  m_settings.doorAlertMin = DEFAULT_DOOR_ALERT_MIN;
  m_settings.smsPollMs = DEFAULT_SMS_POLL_MS;
  m_settings.deepSleepEnabled = DEFAULT_DEEP_SLEEP_ENABLED;
}

void ConfigManager::begin(const AuthorizedUser* defaultUsers) {
  Serial.println(F("[CFG] ConfigManager initializing..."));

  // Check if NVS has been seeded (users namespace has "count" key)
  Preferences prefs;
  prefs.begin(NS_USERS, true);  // Read-only
  bool hasUsers = prefs.isKey("count");
  prefs.end();

  if (!hasUsers) {
    Serial.println(F("[CFG] First boot — seeding NVS from defaults"));
    seedUsers(defaultUsers);
  }

  loadUsers();
  loadSettings();

  m_initialized = true;

  Serial.print(F("[CFG] Loaded "));
  Serial.print(m_userCount);
  Serial.println(F(" user(s) from NVS"));
  Serial.print(F("[CFG] Settings: door_alert="));
  Serial.print(m_settings.doorAlertMin);
  Serial.print(F("min, sms_poll="));
  Serial.print(m_settings.smsPollMs);
  Serial.print(F("ms, deep_sleep="));
  Serial.println(m_settings.deepSleepEnabled ? "ON" : "OFF");
}

// =============================================================================
// User management
// =============================================================================

int ConfigManager::getUserCount() const {
  return m_userCount;
}

const NvsUser* ConfigManager::getUser(int index) const {
  if (index < 0 || index >= m_userCount) {
    return nullptr;
  }
  return &m_users[index];
}

const NvsUser* ConfigManager::findUserByPhone(const String& normalizedPhone) const {
  for (int i = 0; i < m_userCount; i++) {
    if (m_users[i].phone == normalizedPhone) {
      return &m_users[i];
    }
  }
  return nullptr;
}

bool ConfigManager::addUser(const String& phone, uint8_t permissions,
                            const String& name) {
  if (m_userCount >= MAX_NVS_USERS) {
    Serial.println(F("[CFG] Cannot add user — at maximum capacity"));
    return false;
  }

  // Check for duplicate phone number
  if (findUserByPhone(phone) != nullptr) {
    Serial.print(F("[CFG] Cannot add user — duplicate phone: "));
    Serial.println(phone);
    return false;
  }

  m_users[m_userCount].phone = phone;
  m_users[m_userCount].permissions = permissions;
  m_users[m_userCount].name = name;
  m_userCount++;

  saveUsers();

  Serial.print(F("[CFG] Added user: "));
  Serial.print(name);
  Serial.print(F(" ("));
  Serial.print(phone);
  Serial.println(F(")"));
  return true;
}

bool ConfigManager::removeUser(int index) {
  if (index < 0 || index >= m_userCount) {
    return false;
  }

  Serial.print(F("[CFG] Removing user: "));
  Serial.println(m_users[index].name);

  // Shift remaining users down
  for (int i = index; i < m_userCount - 1; i++) {
    m_users[i] = m_users[i + 1];
  }
  m_userCount--;

  // Clear the vacated slot
  m_users[m_userCount].phone = "";
  m_users[m_userCount].permissions = 0;
  m_users[m_userCount].name = "";

  saveUsers();
  return true;
}

bool ConfigManager::updateUserPermissions(int index, uint8_t permissions) {
  if (index < 0 || index >= m_userCount) {
    return false;
  }

  m_users[index].permissions = permissions;
  saveUsers();

  Serial.print(F("[CFG] Updated permissions for "));
  Serial.print(m_users[index].name);
  Serial.print(F(": 0x"));
  Serial.println(permissions, HEX);
  return true;
}

// =============================================================================
// Settings
// =============================================================================

const SystemSettings& ConfigManager::getSettings() const {
  return m_settings;
}

void ConfigManager::setSettings(const SystemSettings& settings) {
  m_settings = settings;
  saveSettings();

  Serial.print(F("[CFG] Settings saved: door_alert="));
  Serial.print(m_settings.doorAlertMin);
  Serial.print(F("min, sms_poll="));
  Serial.print(m_settings.smsPollMs);
  Serial.print(F("ms, deep_sleep="));
  Serial.println(m_settings.deepSleepEnabled ? "ON" : "OFF");
}

// =============================================================================
// NVS persistence (private)
// =============================================================================

void ConfigManager::loadUsers() {
  Preferences prefs;
  prefs.begin(NS_USERS, true);  // Read-only

  m_userCount = prefs.getUChar("count", 0);
  if (m_userCount > MAX_NVS_USERS) {
    m_userCount = MAX_NVS_USERS;
  }

  for (int i = 0; i < m_userCount; i++) {
    char phoneKey[12], permsKey[12], nameKey[12];
    snprintf(phoneKey, sizeof(phoneKey), "phone_%d", i);
    snprintf(permsKey, sizeof(permsKey), "perms_%d", i);
    snprintf(nameKey, sizeof(nameKey), "name_%d", i);

    m_users[i].phone = prefs.getString(phoneKey, "");
    m_users[i].permissions = prefs.getUChar(permsKey, 0);
    m_users[i].name = prefs.getString(nameKey, "");
  }

  prefs.end();
}

void ConfigManager::saveUsers() {
  Preferences prefs;
  prefs.begin(NS_USERS, false);  // Read-write

  prefs.putUChar("count", m_userCount);

  for (int i = 0; i < m_userCount; i++) {
    char phoneKey[12], permsKey[12], nameKey[12];
    snprintf(phoneKey, sizeof(phoneKey), "phone_%d", i);
    snprintf(permsKey, sizeof(permsKey), "perms_%d", i);
    snprintf(nameKey, sizeof(nameKey), "name_%d", i);

    prefs.putString(phoneKey, m_users[i].phone);
    prefs.putUChar(permsKey, m_users[i].permissions);
    prefs.putString(nameKey, m_users[i].name);
  }

  // Clean up any stale entries beyond current count
  for (int i = m_userCount; i < MAX_NVS_USERS; i++) {
    char phoneKey[12], permsKey[12], nameKey[12];
    snprintf(phoneKey, sizeof(phoneKey), "phone_%d", i);
    snprintf(permsKey, sizeof(permsKey), "perms_%d", i);
    snprintf(nameKey, sizeof(nameKey), "name_%d", i);

    prefs.remove(phoneKey);
    prefs.remove(permsKey);
    prefs.remove(nameKey);
  }

  prefs.end();
}

void ConfigManager::loadSettings() {
  Preferences prefs;
  prefs.begin(NS_SETTINGS, true);  // Read-only

  m_settings.doorAlertMin = prefs.getUInt(KEY_DOOR_ALERT, DEFAULT_DOOR_ALERT_MIN);
  m_settings.smsPollMs = prefs.getUInt(KEY_SMS_POLL, DEFAULT_SMS_POLL_MS);
  m_settings.deepSleepEnabled = prefs.getBool(KEY_DEEP_SLEEP, DEFAULT_DEEP_SLEEP_ENABLED);

  prefs.end();
}

void ConfigManager::saveSettings() {
  Preferences prefs;
  prefs.begin(NS_SETTINGS, false);  // Read-write

  prefs.putUInt(KEY_DOOR_ALERT, m_settings.doorAlertMin);
  prefs.putUInt(KEY_SMS_POLL, m_settings.smsPollMs);
  prefs.putBool(KEY_DEEP_SLEEP, m_settings.deepSleepEnabled);

  prefs.end();
}

void ConfigManager::seedUsers(const AuthorizedUser* defaultUsers) {
  m_userCount = 0;

  for (int i = 0; defaultUsers[i].number != nullptr && m_userCount < MAX_NVS_USERS; i++) {
    m_users[m_userCount].phone = defaultUsers[i].number;
    m_users[m_userCount].permissions = defaultUsers[i].permissions;
    m_users[m_userCount].name = defaultUsers[i].name ? defaultUsers[i].name : "";
    m_userCount++;
  }

  saveUsers();

  Serial.print(F("[CFG] Seeded "));
  Serial.print(m_userCount);
  Serial.println(F(" user(s) from defaults"));
}
