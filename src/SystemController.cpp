/**
 * @file SystemController.cpp
 * @brief Main system coordinator implementation
 *
 * Supports two boot modes:
 * - Normal mode: cellular + SMS monitoring (default)
 * - Setup mode: WiFi AP + web UI (FUNC held at boot or SETUP_MODE_DEFAULT)
 *
 * Both modes share: Door, WaterSensor, EnvironmentalSensor, Buttons,
 * ConfigManager, Display. Only one of ModemHandler or WebUIController
 * is activated per boot.
 */

#include "SystemController.h"
#include "Config.h"
#include <Preferences.h>

SystemController::SystemController()
  : m_buttons(m_door),
    m_display(m_door, m_water, m_modem, m_env),
    m_setupMode(false),
    m_lastSMSCheck(0),
    m_alertSent(false),
    m_doorWasOpen(false) {
}

void SystemController::begin() {
  Serial.println(F("\n========================================"));
  Serial.println(F("  Garage Monitor - Starting"));
  Serial.println(F("========================================\n"));

  // 1. Door subsystem FIRST (relays fail-safe OFF)
  m_door.begin();
  m_doorWasOpen = m_door.isOpen();

  // 2. Detect setup mode (FUNC button held at boot)
  m_setupMode = detectSetupMode();

  if (m_setupMode) {
    Serial.println(F("[SYS] *** SETUP MODE ***"));
  } else {
    Serial.println(F("[SYS] Normal mode"));
  }

  // 3. I2C bus (shared by LCD and BME280)
  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
  Serial.print(F("[SYS] I2C initialized (SDA="));
  Serial.print(PIN_I2C_SDA);
  Serial.print(F(", SCL="));
  Serial.print(PIN_I2C_SCL);
  Serial.println(F(")"));

  // 4. Sensors and buttons (always, both modes)
  m_water.begin();

  if (!m_env.begin()) {
    Serial.println(F("[SYS] BME280 init failed — environmental data unavailable"));
  }

  m_buttons.begin();

  // 5. ConfigManager (NVS load — both modes need user data)
  m_config.begin(AUTHORIZED_USERS);

  // Wire MessageParser to use ConfigManager for user lookups
  m_parser.setConfigManager(&m_config);

  // 6. Display
  m_display.begin();

  // 7. Mode-specific initialization
  if (m_setupMode) {
    beginSetupMode();
  } else {
    beginNormalMode();
  }

  Serial.println(F("[SYS] System ready"));
}

void SystemController::loop() {
  // Common subsystems (always run)
  m_door.loop();
  m_water.loop();
  m_env.loop();
  m_buttons.loop();
  m_display.loop();

  if (m_setupMode) {
    loopSetupMode();
  } else {
    loopNormalMode();
  }
}

ConfigManager& SystemController::getConfig() {
  return m_config;
}

const Door& SystemController::getDoor() const {
  return m_door;
}

const WaterSensor& SystemController::getWater() const {
  return m_water;
}

const EnvironmentalSensor& SystemController::getEnv() const {
  return m_env;
}

bool SystemController::isSetupMode() const {
  return m_setupMode;
}

// =============================================================================
// Setup mode detection
// =============================================================================

bool SystemController::detectSetupMode() {
  // Read FUNC button raw (GPIO already set to INPUT_PULLUP by Door or will be
  // by DisplayController — set it here too for safety, idempotent)
  pinMode(PIN_BTN_FUNC, INPUT_PULLUP);
  delay(50);  // Let pullup settle

  bool funcHeld = (digitalRead(PIN_BTN_FUNC) == LOW);

  if (funcHeld) {
    Serial.println(F("[SYS] FUNC button held at boot — entering setup mode"));
  }

  return funcHeld || SETUP_MODE_DEFAULT;
}

// =============================================================================
// Normal mode (cellular + SMS)
// =============================================================================

void SystemController::beginNormalMode() {
  // Modem (display shows splash during this slow init)
  if (m_modem.begin()) {
    Serial.println(F("[SYS] Modem initialized successfully"));

    // After a scheduled auto-reboot, confirm system is back online
    Preferences rebootFlag;
    rebootFlag.begin("system", false);
    bool pending = rebootFlag.getBool("reboot", false);
    if (pending) {
      rebootFlag.putBool("reboot", false);
      Serial.println(F("[SYS] Back online after scheduled reboot"));
      if (m_config.getSettings().notifyReboot) {
        notifyAdmins("System back online after scheduled reboot.");
      }
    }
    rebootFlag.end();

    purgeBootMessages();
  } else {
    Serial.println(F("[SYS] Modem initialization failed"));
    m_display.showMessage("Modem init", "FAILED");
    // Clear reboot flag even on modem failure
    Preferences rebootFlag;
    rebootFlag.begin("system", false);
    rebootFlag.putBool("reboot", false);
    rebootFlag.end();
  }
}

void SystemController::purgeBootMessages() {
  int count = m_modem.checkForSMS();
  if (count <= 0) {
    return;
  }

  Serial.print(F("[SYS] Boot: found "));
  Serial.print(count);
  Serial.println(F(" queued SMS — purging for safety"));

  m_modem.deleteAllSMS();

  // Notify admins
  char msg[100];
  snprintf(msg, sizeof(msg),
    "System restarted. %d queued SMS discarded for safety. "
    "Resend commands if needed.", count);
  notifyAdmins(msg);
}

void SystemController::loopNormalMode() {
  // Auto-reboot for long-term reliability (runtime-configurable)
  const SystemSettings& rebootSettings = m_config.getSettings();
  if (rebootSettings.autoRebootEnabled) {
    unsigned long rebootIntervalMs = (unsigned long)rebootSettings.autoRebootDays * 86400000UL;
    if (millis() >= rebootIntervalMs) {
      if (rebootSettings.autoRebootHour < 0) {
        // No time-of-day constraint — reboot now
        Serial.println(F("[SYS] Scheduled reboot (interval elapsed)"));
        if (rebootSettings.notifyReboot) {
          notifyAdmins("System rebooting (scheduled restart).");
          delay(5000);  // Allow SMS to transmit before restarting
        }
        {
          Preferences rebootFlag;
          rebootFlag.begin("system", false);
          rebootFlag.putBool("reboot", true);
          rebootFlag.end();
        }
        ESP.restart();
      }
      static unsigned long lastRebootCheck = 0;
      unsigned long rebootNow = millis();
      if (rebootNow - lastRebootCheck >= REBOOT_CHECK_INTERVAL_MS) {
        lastRebootCheck = rebootNow;
        int hour = m_modem.getHour();
        bool targetHour = (hour == rebootSettings.autoRebootHour);
        // Safety net: if time is unavailable for 24h past threshold, reboot anyway
        bool forceReboot = (rebootNow >= rebootIntervalMs + 86400000UL);
        if (targetHour || forceReboot) {
          Serial.print(F("[SYS] Scheduled reboot (hour="));
          Serial.print(hour);
          Serial.println(F(")"));
          if (rebootSettings.notifyReboot) {
            notifyAdmins("System rebooting (scheduled restart).");
            delay(5000);  // Allow SMS to transmit before restarting
          }
          {
            Preferences rebootFlag;
            rebootFlag.begin("system", false);
            rebootFlag.putBool("reboot", true);
            rebootFlag.end();
          }
          ESP.restart();
        }
      }
    }
  }

  m_modem.loop();

  // Door state change detection
  bool doorOpen = m_door.isOpen();
  if (doorOpen != m_doorWasOpen) {
    m_doorWasOpen = doorOpen;
    if (doorOpen) {
      Serial.println(F("[SYS] Door opened."));
      m_alertSent = false;
      m_display.showNotification("Door:", "OPENED");
    } else {
      Serial.println(F("[SYS] Door closed."));
      m_display.showNotification("Door:", "CLOSED");
    }
  }

  // Door open too long alert (single alert per open event)
  uint32_t alertDelayMs = (uint32_t)m_config.getSettings().doorAlertMin * 60 * 1000;
  if (doorOpen && !m_alertSent && m_door.getOpenDurationMs() >= alertDelayMs) {
    Serial.print(F("[SYS] Door open >"));
    Serial.print(m_config.getSettings().doorAlertMin);
    Serial.println(F("min — sending alert"));
    m_alertSent = true;
    char alertMsg[64];
    snprintf(alertMsg, sizeof(alertMsg),
             "ALERT: Garage door still open after %lu minutes",
             (unsigned long)m_config.getSettings().doorAlertMin);
    notifyAdmins(alertMsg);
    m_display.showNotification("SMS sent:", "Door open alert");
  }

  // Water detection — immediate alert to all users
  if (m_water.hasStateChanged()) {
    if (m_water.isWaterDetected()) {
      Serial.println(F("[SYS] Water detected — alerting all users"));
      notifyAllUsers("ALERT: Water detected in garage!");
      m_display.showNotification("SMS sent:", "Water ALERT!");
    } else {
      Serial.println(F("[SYS] Water cleared — notifying all users"));
      notifyAllUsers("Water no longer detected.");
      m_display.showNotification("SMS sent:", "Water cleared");
    }
  }

  unsigned long now = millis();
  uint32_t pollInterval = m_config.getSettings().smsPollMs;

  // SMS polling: check for incoming messages
  if (m_modem.isNetworkConnected() && now - m_lastSMSCheck >= pollInterval) {
    m_lastSMSCheck = now;

    int count = m_modem.checkForSMS();

    // Safety net: if too many SMS accumulated, purge all and skip processing
    if (count >= SMS_PURGE_THRESHOLD) {
      Serial.print(F("[SYS] SMS count "));
      Serial.print(count);
      Serial.println(F(" >= threshold, purging all"));
      m_modem.deleteAllSMS();
      return;
    }

    int processed = 0;
    for (int i = 0; i < count; i++) {
      ReceivedSMS sms;
      int idx = m_modem.getSMSIndex(i);
      if (m_modem.readSMS(idx, sms)) {
        Serial.print(F("[SYS] === SMS RECEIVED ==="));
        Serial.print(F("\n  From: "));
        Serial.print(sms.sender);
        Serial.print(F("\n  Time: "));
        Serial.print(sms.timestamp);
        Serial.print(F("\n  Body: '"));
        Serial.print(sms.message);
        Serial.println(F("'"));

        handleSMS(sms);
        processed++;
      }
    }

    // Batch-delete all read SMS in one AT command instead of per-message
    if (processed > 0) {
      m_modem.deleteReadSMS();
    }
  }
}

// =============================================================================
// Setup mode (WiFi AP + web UI)
// =============================================================================

void SystemController::beginSetupMode() {
  m_webUI.begin(m_config, m_door, m_water, m_env);

  // Show SSID and IP on LCD
  String ip = m_webUI.getIPAddress();
  m_display.showSetupMode(WIFI_AP_SSID, ip.c_str());

  Serial.print(F("[SYS] Setup mode ready — connect to "));
  Serial.print(WIFI_AP_SSID);
  Serial.print(F(" and browse http://"));
  Serial.println(ip);
}

void SystemController::loopSetupMode() {
  m_webUI.loop();
}

// =============================================================================
// SMS command handling
// =============================================================================

void SystemController::handleSMS(const ReceivedSMS& sms) {
  ParseResult result = m_parser.parse(sms.sender, sms.message);

  if (!result.isAuthorized) {
    Serial.print(F("[SYS] Unauthorized sender: "));
    Serial.println(sms.sender);
    if (m_config.getSettings().forwardUnknownSms) {
      String fwd = "FWD from " + sms.sender + ":\n" + sms.message;
      Serial.println(F("[SYS] Forwarding unknown SMS to admins"));
      notifyAdmins(fwd.c_str());
    }
    return;
  }

  Serial.print(F("[SYS] Authorized user: "));
  Serial.println(result.userName);

  // Ignore unrecognized commands before permission check
  if (result.command == SMSCommand::UNKNOWN) {
    Serial.println(F("[SYS] Unknown command, ignoring"));
    return;
  }

  if (!result.hasPermission) {
    Serial.println(F("[SYS] Permission denied for command"));
    m_modem.sendSMS(sms.sender.c_str(), "Permission denied.");
    return;
  }

  // Build notification header: "SMS: <name>"
  char notifyLine1[LCD_COLS + 1];
  snprintf(notifyLine1, sizeof(notifyLine1), "SMS: %s", result.userName);

  switch (result.command) {
    case SMSCommand::STATUS: {
      String reply = buildStatusReply();
      Serial.print(F("[SYS] STATUS reply: "));
      Serial.println(reply);
      m_modem.sendSMS(sms.sender.c_str(), reply.c_str());
      m_display.showNotification(notifyLine1, "CMD: STATUS");
      break;
    }
    case SMSCommand::CLOSE:
      Serial.println(F("[SYS] Executing CLOSE command"));
      m_door.close();
      m_modem.sendSMS(sms.sender.c_str(), "Closing garage door.");
      m_display.showNotification(notifyLine1, "CMD: CLOSE");
      break;

    case SMSCommand::OPEN:
      Serial.println(F("[SYS] Executing OPEN command"));
      m_door.open();
      m_modem.sendSMS(sms.sender.c_str(), "Opening garage door.");
      m_display.showNotification(notifyLine1, "CMD: OPEN");
      break;

    case SMSCommand::UNKNOWN:
      // Unreachable — handled above, but keeps compiler happy
      break;
  }
}

void SystemController::notifyAdmins(const char* message) {
  for (int i = 0; i < m_config.getUserCount(); i++) {
    const NvsUser* user = m_config.getUser(i);
    if (user != nullptr && (user->permissions & PERM_CONFIG)) {
      Serial.print(F("[SYS] Notifying "));
      Serial.print(user->name);
      Serial.print(F(": "));
      Serial.println(message);
      m_modem.sendSMS(user->phone.c_str(), message);
    }
  }
}

void SystemController::notifyAllUsers(const char* message) {
  for (int i = 0; i < m_config.getUserCount(); i++) {
    const NvsUser* user = m_config.getUser(i);
    if (user != nullptr) {
      Serial.print(F("[SYS] Notifying "));
      Serial.print(user->name);
      Serial.print(F(": "));
      Serial.println(message);
      m_modem.sendSMS(user->phone.c_str(), message);
    }
  }
}

String SystemController::buildStatusReply() {
  String reply;

  // Door state
  if (m_door.isOpen()) {
    unsigned long openMs = m_door.getOpenDurationMs();
    unsigned long openMin = openMs / 60000;
    reply = "Door: OPEN (" + String(openMin) + "min)";
  } else {
    reply = "Door: CLOSED";
  }

  // Environmental data
  if (m_env.isReady()) {
    reply += "\nTemp: " + String(m_env.getTemperature(), 1) + "C";
    reply += " | Hum: " + String(m_env.getHumidity(), 1) + "%";
  }

  // Water state
  reply += "\nWater: ";
  reply += m_water.isWaterDetected() ? "ALERT" : "OK";

  // Signal quality
  int stars = m_modem.getSignalStars();
  char sig[5] = "----";
  for (int i = 0; i < stars && i < 4; i++) sig[i] = '*';
  reply += " | Sig: ";
  reply += sig;

  return reply;
}
