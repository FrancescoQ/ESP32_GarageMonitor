/**
 * @file DisplayController.h
 * @brief LCD 16x2 status display with multi-page navigation
 *
 * Drives a 16x2 I2C LCD to show system status across multiple pages.
 * The display stays off by default and turns on when the FUNC button
 * (GPIO 15) is pressed. Subsequent presses cycle through pages.
 * The display turns off automatically after a timeout.
 *
 * Notifications (SMS events) temporarily override any page for 30s,
 * then restore the previous display state (on with pages, or off).
 */

#pragma once

#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include "Door.h"
#include "WaterSensor.h"
#include "ModemHandler.h"
#include "EnvironmentalSensor.h"

class DisplayController {
public:
  DisplayController(const Door& door, const WaterSensor& water,
                    ModemHandler& modem, const EnvironmentalSensor& env);

  /**
   * @brief Initialize LCD, FUNC button, and show splash screen
   */
  void begin();

  /**
   * @brief Poll FUNC button and refresh display periodically
   */
  void loop();

  /**
   * @brief Show a two-line message (splash, errors)
   *
   * Turns the display on. Auto-off after DISPLAY_ON_DURATION_MS.
   *
   * @param line1 First line text (max 16 chars, nullptr to skip)
   * @param line2 Second line text (max 16 chars, nullptr to skip)
   */
  void showMessage(const char* line1, const char* line2 = nullptr);

  /**
   * @brief Show a notification that temporarily overrides the display
   *
   * Turns the display on if off, shows the notification for
   * NOTIFICATION_DURATION_MS, then restores the previous state
   * (pages if display was on, off if it was off).
   *
   * @param line1 First line text (max 16 chars)
   * @param line2 Second line text (max 16 chars, nullptr to skip)
   */
  void showNotification(const char* line1, const char* line2 = nullptr);

private:
  static const int NUM_PAGES = 3;

  LiquidCrystal_I2C m_lcd;
  const Door& m_door;
  const WaterSensor& m_water;
  ModemHandler& m_modem;
  const EnvironmentalSensor& m_env;

  int m_currentPage;
  unsigned long m_lastRefresh;
  bool m_displayOn;
  unsigned long m_displayOnTime;

  // Notification overlay
  bool m_notificationActive;
  unsigned long m_notificationTime;
  bool m_wasOnBeforeNotification;

  // Setup mode — persistent display, no auto-off or page cycling
  bool m_inSetupMode;

  // FUNC button debounce + long-press reboot
  bool m_funcLastRaw;
  bool m_funcPressed;
  unsigned long m_funcDebounceTime;
  unsigned long m_funcPressStart;

  void turnOn();
  void turnOff();
  void renderPage();
  void renderNetworkStatus();
  void renderEnvironment();
  void renderSystem();
  void checkFuncButton();

public:
  /**
   * @brief Show setup mode screen with SSID and IP address
   * @param ssid WiFi AP SSID
   * @param ip IP address string (e.g. "192.168.4.1")
   */
  void showSetupMode(const char* ssid, const char* ip);
};
