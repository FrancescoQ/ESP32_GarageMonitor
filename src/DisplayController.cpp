/**
 * @file DisplayController.cpp
 * @brief LCD 16x2 display driver with page navigation, auto-off,
 *        and notification overlay
 */

#include "DisplayController.h"
#include "Config.h"

DisplayController::DisplayController(const Door& door,
                                     const WaterSensor& water,
                                     ModemHandler& modem,
                                     const EnvironmentalSensor& env)
  : m_lcd(I2C_ADDR_LCD, LCD_COLS, LCD_ROWS),
    m_door(door),
    m_water(water),
    m_modem(modem),
    m_env(env),
    m_currentPage(0),
    m_lastRefresh(0),
    m_displayOn(false),
    m_displayOnTime(0),
    m_notificationActive(false),
    m_notificationTime(0),
    m_wasOnBeforeNotification(false),
    m_funcLastRaw(HIGH),
    m_funcPressed(false),
    m_funcDebounceTime(0) {
}

void DisplayController::begin() {
  // FUNC button
  pinMode(PIN_BTN_FUNC, INPUT_PULLUP);
  m_funcLastRaw = digitalRead(PIN_BTN_FUNC);
  Serial.print(F("[LCD] FUNC button on GPIO "));
  Serial.println(PIN_BTN_FUNC);

  // LCD init
  m_lcd.init();

  // Show splash, then auto-off after timeout
  showMessage("Garage Monitor", "Starting...");
  Serial.println(F("[LCD] Display initialized"));
}

void DisplayController::loop() {
  checkFuncButton();

  // Notification auto-expire
  if (m_notificationActive
      && millis() - m_notificationTime >= NOTIFICATION_DURATION_MS) {
    m_notificationActive = false;
    Serial.println(F("[LCD] Notification expired"));

    if (m_wasOnBeforeNotification) {
      // Restore pages — reset auto-off timer and render
      m_displayOnTime = millis();
      m_lastRefresh = millis();
      renderPage();
    } else {
      turnOff();
    }
    return;
  }

  // Don't refresh pages while notification is showing
  if (m_notificationActive) return;

  // Auto-off after timeout (pages mode)
  if (m_displayOn && millis() - m_displayOnTime >= DISPLAY_ON_DURATION_MS) {
    turnOff();
    return;
  }

  // Refresh content while display is on
  if (m_displayOn) {
    unsigned long now = millis();
    if (now - m_lastRefresh >= REFRESH_INTERVAL_MS) {
      m_lastRefresh = now;
      renderPage();
    }
  }
}

void DisplayController::showMessage(const char* line1, const char* line2) {
  turnOn();
  m_lcd.clear();
  if (line1 != nullptr) {
    m_lcd.setCursor(0, 0);
    m_lcd.print(line1);
  }
  if (line2 != nullptr) {
    m_lcd.setCursor(0, 1);
    m_lcd.print(line2);
  }
}

void DisplayController::showNotification(const char* line1, const char* line2) {
  // Remember previous state so we can restore after expiry
  if (!m_notificationActive) {
    m_wasOnBeforeNotification = m_displayOn;
  }

  m_notificationActive = true;
  m_notificationTime = millis();

  turnOn();
  m_lcd.clear();
  if (line1 != nullptr) {
    m_lcd.setCursor(0, 0);
    m_lcd.print(line1);
  }
  if (line2 != nullptr) {
    m_lcd.setCursor(0, 1);
    m_lcd.print(line2);
  }

  Serial.print(F("[LCD] Notification: "));
  if (line1 != nullptr) Serial.print(line1);
  if (line2 != nullptr) {
    Serial.print(F(" | "));
    Serial.print(line2);
  }
  Serial.println();
}

void DisplayController::turnOn() {
  if (!m_displayOn) {
    m_lcd.backlight();
    m_lcd.display();
    m_displayOn = true;
    Serial.println(F("[LCD] Display on"));
  }
  m_displayOnTime = millis();
}

void DisplayController::turnOff() {
  m_lcd.noBacklight();
  m_lcd.noDisplay();
  m_lcd.clear();
  m_displayOn = false;
  m_currentPage = 0;
  m_notificationActive = false;
  Serial.println(F("[LCD] Display off"));
}

void DisplayController::checkFuncButton() {
  bool raw = digitalRead(PIN_BTN_FUNC);

  if (raw != m_funcLastRaw) {
    m_funcDebounceTime = millis();
    m_funcLastRaw = raw;
  }

  bool currentlyPressed = (raw == LOW);
  if (currentlyPressed != m_funcPressed
      && (millis() - m_funcDebounceTime >= BTN_DEBOUNCE_MS)) {
    m_funcPressed = currentlyPressed;

    if (currentlyPressed) {
      // Dismiss notification on button press
      if (m_notificationActive) {
        m_notificationActive = false;
        m_currentPage = 0;
        Serial.println(F("[LCD] Notification dismissed"));
      } else if (!m_displayOn) {
        // First press: turn on, show page 0
        m_currentPage = 0;
      } else {
        // Already on: cycle to next page
        m_currentPage = (m_currentPage + 1) % NUM_PAGES;
      }

      turnOn();

      Serial.print(F("[LCD] Page "));
      Serial.println(m_currentPage);

      m_lastRefresh = millis();
      renderPage();
    }
  }
}

void DisplayController::renderPage() {
  m_lcd.clear();
  switch (m_currentPage) {
    case 0:
      renderNetworkStatus();
      break;
    case 1:
      renderEnvironment();
      break;
    case 2:
      renderSystem();
      break;
  }
}

void DisplayController::renderNetworkStatus() {
  // Line 1: Operator + signal stars
  // "Iliad    Sig:***-"
  m_lcd.setCursor(0, 0);
  if (m_modem.isNetworkConnected()) {
    String op = m_modem.getOperator();
    int stars = m_modem.getSignalStars();
    char sig[5] = "----";
    for (int i = 0; i < stars && i < 4; i++) sig[i] = '*';

    char line[32];
    if (op.length() > 0) {
      snprintf(line, sizeof(line), "%-12.12s%4s", op.c_str(), sig);
    } else {
      snprintf(line, sizeof(line), "Connected   %4s", sig);
    }
    m_lcd.print(line);
  } else {
    m_lcd.print(F("Net: offline"));
  }

  // Line 2: Temperature + humidity (compact)
  // "22.3C      65.2%"
  m_lcd.setCursor(0, 1);
  if (m_env.isReady()) {
    char temp[8], hum[8];
    snprintf(temp, sizeof(temp), "%.1fC", m_env.getTemperature());
    snprintf(hum, sizeof(hum), "%.1f%%", m_env.getHumidity());
    char line[32];
    snprintf(line, sizeof(line), "%-8s%8s", temp, hum);
    m_lcd.print(line);
  } else {
    m_lcd.print(F("BME280: N/A"));
  }
}

void DisplayController::renderEnvironment() {
  // Line 1: Temperature + humidity + pressure
  // "22.3C 65% 1013mb"
  m_lcd.setCursor(0, 0);
  if (m_env.isReady()) {
    char line[32];
    snprintf(line, sizeof(line), "%.1fC %.0f%% %.0fmb",
             m_env.getTemperature(),
             m_env.getHumidity(),
             m_env.getPressure());
    m_lcd.print(line);
  } else {
    m_lcd.print(F("BME280: N/A"));
  }

  // Line 2: Water sensor status
  // "Water: OK" or "Water: ALERT!"
  m_lcd.setCursor(0, 1);
  if (m_water.isWaterDetected()) {
    m_lcd.print(F("Water: ALERT!"));
  } else {
    m_lcd.print(F("Water: OK"));
  }
}

void DisplayController::renderSystem() {
  // Line 1: Door state with duration
  // "Door:Open    45m" or "Door:Closed"
  m_lcd.setCursor(0, 0);
  if (m_door.isOpen()) {
    unsigned long openMin = m_door.getOpenDurationMs() / 60000;
    char line[32];
    snprintf(line, sizeof(line), "Door:Open %4lum", openMin);
    m_lcd.print(line);
  } else {
    m_lcd.print(F("Door:Closed"));
  }

  // Line 2: Uptime
  // "Up: 3d 14h 22m"
  m_lcd.setCursor(0, 1);
  unsigned long totalMin = millis() / 60000;
  unsigned long days = totalMin / 1440;
  unsigned long hours = (totalMin % 1440) / 60;
  unsigned long mins = totalMin % 60;
  char line[32];
  if (days > 0) {
    snprintf(line, sizeof(line), "Up: %lud %luh %lum", days, hours, mins);
  } else {
    snprintf(line, sizeof(line), "Up: %luh %lum", hours, mins);
  }
  m_lcd.print(line);
}
