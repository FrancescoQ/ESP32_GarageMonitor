/**
 * @file ConfigManager.h
 * @brief NVS-backed persistent storage for users and system settings
 *
 * Manages authorized users and system settings in ESP32 NVS
 * (non-volatile storage). On first boot, seeds NVS from hardcoded
 * defaults in Secrets.h. Subsequent boots load from NVS only.
 *
 * Used in both normal mode (SMS auth) and setup mode (web UI editing).
 */

#pragma once

#include <Arduino.h>
#include "MessageParser.h"

// Maximum number of users stored in NVS
const int MAX_NVS_USERS = 10;

/**
 * @brief Runtime user entry (RAM copy of NVS data)
 *
 * Unlike AuthorizedUser (which uses const char* into flash),
 * NvsUser owns its strings as Arduino Strings for mutability.
 */
struct NvsUser {
  String phone;
  uint8_t permissions;
  String name;
};

/**
 * @brief System settings stored in NVS
 */
struct SystemSettings {
  uint32_t doorAlertMin;    // Minutes before door-open alert
  uint32_t smsPollMs;       // SMS polling interval in ms
  bool deepSleepEnabled;    // Deep sleep on/off (Phase 5)
};

// Default settings values
const uint32_t DEFAULT_DOOR_ALERT_MIN = 30;
const uint32_t DEFAULT_SMS_POLL_MS = 5000;
const bool DEFAULT_DEEP_SLEEP_ENABLED = false;

/**
 * @brief NVS-backed configuration manager
 *
 * Provides CRUD operations for authorized users and system settings.
 * Data is persisted in ESP32 NVS and loaded into RAM at boot.
 */
class ConfigManager {
public:
  ConfigManager();

  /**
   * @brief Load users and settings from NVS, seed defaults on first boot
   * @param defaultUsers Null-terminated array of default users (from Secrets.h)
   */
  void begin(const AuthorizedUser* defaultUsers);

  // =========================================================================
  // User management
  // =========================================================================

  /**
   * @brief Get number of configured users
   * @return User count (0 to MAX_NVS_USERS)
   */
  int getUserCount() const;

  /**
   * @brief Get user by index
   * @param index User index (0 to getUserCount()-1)
   * @return Pointer to NvsUser, or nullptr if index out of range
   */
  const NvsUser* getUser(int index) const;

  /**
   * @brief Find user by normalized phone number
   * @param normalizedPhone Phone number in +CC format
   * @return Pointer to NvsUser, or nullptr if not found
   */
  const NvsUser* findUserByPhone(const String& normalizedPhone) const;

  /**
   * @brief Add a new user
   * @param phone Phone number (+CC format)
   * @param permissions Permission bitmask
   * @param name Display name
   * @return true if added, false if full or duplicate phone
   */
  bool addUser(const String& phone, uint8_t permissions, const String& name);

  /**
   * @brief Remove user by index
   * @param index User index to remove
   * @return true if removed, false if index out of range
   */
  bool removeUser(int index);

  /**
   * @brief Update user permissions by index
   * @param index User index
   * @param permissions New permission bitmask
   * @return true if updated, false if index out of range
   */
  bool updateUserPermissions(int index, uint8_t permissions);

  // =========================================================================
  // Settings
  // =========================================================================

  /**
   * @brief Get current system settings (RAM copy)
   * @return Reference to settings struct
   */
  const SystemSettings& getSettings() const;

  /**
   * @brief Update and persist system settings
   * @param settings New settings to save
   */
  void setSettings(const SystemSettings& settings);

private:
  NvsUser m_users[MAX_NVS_USERS];
  int m_userCount;
  SystemSettings m_settings;
  bool m_initialized;

  void loadUsers();
  void saveUsers();
  void loadSettings();
  void saveSettings();
  void seedUsers(const AuthorizedUser* defaultUsers);
};
