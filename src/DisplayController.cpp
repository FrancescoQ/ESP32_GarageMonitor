/**
 * @file DisplayController.cpp
 * @brief LCD 16x2 display driver with page navigation and auto-off
 *
 * Display is off by default. FUNC button turns it on (first press)
 * or cycles pages (subsequent presses). Auto-off after timeout.
 */

#include "DisplayController.h"
#include "Config.h"

DisplayController::DisplayController(const Door& door,
                                     const WaterSensor& water,
                                     ModemHandler& modem)
  : m_lcd(I2C_ADDR_LCD, LCD_COLS, LCD_ROWS),
    m_door(door),
    m_water(water),
    m_modem(modem),
    m_currentPage(0),
    m_lastRefresh(0),
    m_displayOn(false),
    m_displayOnTime(0),
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

  // Show splash, then turn off after timeout
  showMessage("Garage Monitor", "Starting...");
  Serial.println(F("[LCD] Display initialized"));
}

void DisplayController::loop() {
  checkFuncButton();

  // Auto-off after timeout
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

    // Act on falling edge (button just pressed)
    if (currentlyPressed) {
      if (!m_displayOn) {
        // First press: turn on, show page 0
        m_currentPage = 0;
        turnOn();
      } else {
        // Already on: cycle to next page, reset timeout
        m_currentPage = (m_currentPage + 1) % NUM_PAGES;
        m_displayOnTime = millis();
      }

      Serial.print(F("[LCD] Page "));
      Serial.println(m_currentPage);

      // Immediate refresh
      m_lastRefresh = millis();
      renderPage();
    }
  }
}

void DisplayController::renderPage() {
  m_lcd.clear();
  switch (m_currentPage) {
    case 0:
      renderMainStatus();
      break;
    case 1:
      renderModemInfo();
      break;
  }
}

void DisplayController::renderMainStatus() {
  // Line 1: "Door:CLOSED" or "Door:OPEN  45m"
  m_lcd.setCursor(0, 0);
  if (m_door.isOpen()) {
    unsigned long openMin = m_door.getOpenDurationMs() / 60000;
    char line[32];
    snprintf(line, sizeof(line), "Door:OPEN %3lum", openMin);
    line[LCD_COLS] = '\0';
    m_lcd.print(line);
  } else {
    m_lcd.print(F("Door:CLOSED"));
  }

  // Line 2: "Water:OK  CSQ:20" or "Water:ALERT!"
  m_lcd.setCursor(0, 1);
  if (m_water.isWaterDetected()) {
    m_lcd.print(F("Water:ALERT!"));
  } else {
    int16_t csq = m_modem.getSignalQuality();
    char line[32];
    snprintf(line, sizeof(line), "Water:OK  CSQ:%d", csq);
    line[LCD_COLS] = '\0';
    m_lcd.print(line);
  }
}

void DisplayController::renderModemInfo() {
  // Line 1: Network operator
  m_lcd.setCursor(0, 0);
  if (m_modem.isNetworkConnected()) {
    String op = m_modem.getOperator();
    if (op.length() > 0) {
      char line[32];
      snprintf(line, sizeof(line), "Net: %-11s", op.c_str());
      line[LCD_COLS] = '\0';
      m_lcd.print(line);
    } else {
      m_lcd.print(F("Net: connected"));
    }
  } else {
    m_lcd.print(F("Net: offline"));
  }

  // Line 2: Signal quality detail
  m_lcd.setCursor(0, 1);
  int16_t csq = m_modem.getSignalQuality();
  char line[32];
  snprintf(line, sizeof(line), "Signal: %d/31", csq);
  line[LCD_COLS] = '\0';
  m_lcd.print(line);
}
