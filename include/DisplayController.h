/**
 * @file DisplayController.h
 * @brief LCD 16x2 status display with multi-page navigation
 *
 * Drives a 16x2 I2C LCD to show system status across multiple pages.
 * The display stays off by default and turns on when the FUNC button
 * (GPIO 15) is pressed. Subsequent presses cycle through pages.
 * The display turns off automatically after a timeout.
 *
 * SystemController can call showMessage() to display transient
 * messages (splash screen, init errors) that auto-expire.
 */

#pragma once

#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include "Door.h"
#include "WaterSensor.h"
#include "ModemHandler.h"

class DisplayController {
public:
  /**
   * @param door Read-only door state reference
   * @param water Read-only water sensor reference
   * @param modem Read-only modem handler reference
   */
  DisplayController(const Door& door, const WaterSensor& water,
                    ModemHandler& modem);

  /**
   * @brief Initialize LCD, FUNC button, and show splash screen
   */
  void begin();

  /**
   * @brief Poll FUNC button and refresh display periodically
   */
  void loop();

  /**
   * @brief Show a two-line message that auto-expires
   *
   * Turns the display on and shows the message for DISPLAY_ON_DURATION_MS.
   * Useful for startup splash and init error messages.
   *
   * @param line1 First line text (max 16 chars, nullptr to skip)
   * @param line2 Second line text (max 16 chars, nullptr to skip)
   */
  void showMessage(const char* line1, const char* line2 = nullptr);

private:
  static const int NUM_PAGES = 2;
  static const unsigned long REFRESH_INTERVAL_MS = 1000;
  static const unsigned long DISPLAY_ON_DURATION_MS = 30000;

  LiquidCrystal_I2C m_lcd;
  const Door& m_door;
  const WaterSensor& m_water;
  ModemHandler& m_modem;

  int m_currentPage;
  unsigned long m_lastRefresh;
  bool m_displayOn;
  unsigned long m_displayOnTime;  ///< When display was last turned on

  // FUNC button debounce
  bool m_funcLastRaw;
  bool m_funcPressed;
  unsigned long m_funcDebounceTime;

  void turnOn();
  void turnOff();
  void renderPage();
  void renderMainStatus();
  void renderModemInfo();
  void checkFuncButton();
};
