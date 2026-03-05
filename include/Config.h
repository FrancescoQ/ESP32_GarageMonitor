#pragma once

/**
 * @file Config.h
 * @brief Hardware pin assignments and system configuration
 *
 * Central configuration file for the garage monitoring system.
 * All pin assignments and hardware constants are defined here.
 *
 * Sensitive data (SIM PIN, phone numbers) lives in Secrets.h,
 * which is gitignored. See Secrets.h.example for the template.
 */

// Sensitive configuration (SIM PIN, authorized numbers)
#include "Secrets.h"

// ============================================================================
// I2C Bus (shared: LCD + BME280)
// ============================================================================
const int PIN_I2C_SDA = 21;
const int PIN_I2C_SCL = 22;

// I2C device addresses
const uint8_t I2C_ADDR_LCD = 0x27;     // LCD 16x2 (alternative: 0x3F)
const uint8_t I2C_ADDR_BME280 = 0x76;  // BME280 (alternative: 0x77)

// LCD dimensions
const int LCD_COLS = 16;
const int LCD_ROWS = 2;

// ============================================================================
// UART - SIM7000G
// ============================================================================
const int PIN_SIM_TXD = 17;   // ESP32 TX -> SIM7000G RX
const int PIN_SIM_RXD = 16;   // ESP32 RX <- SIM7000G TX
const int PIN_SIM_PWR = 4;    // SIM7000G power key
const int PIN_SIM_DTR = 18;   // SIM7000G DTR (sleep control, Phase 4)

const long SIM_BAUD_RATE = 115200;

// ============================================================================
// Door Sensor (Reed Switch)
// ============================================================================
const int PIN_DOOR_SENSOR = 5;  // INPUT_PULLUP, no external resistor needed

// ============================================================================
// Water Sensor (XKC-Y25)
// ============================================================================
const int PIN_WATER_SENSOR = 32;

// ============================================================================
// Relay Outputs (Door Control)
// ============================================================================
const int PIN_RELAY_CLOSE = 25;  // Door close relay
const int PIN_RELAY_STOP = 26;   // Door stop relay
const int PIN_RELAY_OPEN = 27;   // Door open relay

// Relay logic: using HIGH trigger jumper (5V module with 3.3V ESP32)
const bool RELAY_ACTIVE_LOW = false;
const int RELAY_ON  = RELAY_ACTIVE_LOW ? LOW : HIGH;
const int RELAY_OFF = RELAY_ACTIVE_LOW ? HIGH : LOW;

// ============================================================================
// Manual Control Buttons
// ============================================================================
const int PIN_BTN_CLOSE = 33;  // Manual close button (INPUT_PULLUP)
const int PIN_BTN_OPEN  = 13;  // Manual open button (INPUT_PULLUP)
const int PIN_BTN_STOP  = 14;  // Manual stop button (INPUT_PULLUP)
const int PIN_BTN_FUNC  = 15;  // Multipurpose button (INPUT_PULLUP): display page, WiFi setup, reset

const unsigned long BTN_DEBOUNCE_MS = 50;  // Button debounce time

// ============================================================================
// Timing Constants
// ============================================================================
const unsigned long RELAY_PULSE_MS = 200;                 // Momentary relay activation
const unsigned long RELAY_SEQUENCE_DELAY_MS = 500;        // Pause between STOP and OPEN/CLOSE
const unsigned long DOOR_DEBOUNCE_MS = 50;                // Reed switch debounce
const unsigned long WATER_DEBOUNCE_MS = 50;               // Water sensor debounce
const int SMS_PURGE_THRESHOLD = 10;                       // Purge all SMS if count reaches this
const unsigned long REBOOT_CHECK_INTERVAL_MS = 900000UL;    // How often to check clock (15 min)

// ============================================================================
// Display Timing
// ============================================================================
const unsigned long DISPLAY_ON_DURATION_MS = 30000;     // Auto-off after this (page mode)
const unsigned long NOTIFICATION_DURATION_MS = 5000;    // Notification overlay duration
const unsigned long DISPLAY_REFRESH_INTERVAL_MS = 1000;  // Page content refresh rate
const unsigned long FUNC_REBOOT_HOLD_MS = 5000;          // Hold FUNC this long to reboot

// ============================================================================
// Setup Mode (WiFi AP for configuration)
// ============================================================================
// Set to true to force setup mode without holding FUNC button at boot.
// Useful during development. Set to false for production.
const bool SETUP_MODE_DEFAULT = false;

const char* const WIFI_AP_SSID = "GarageSetup";
const int WEB_SERVER_PORT = 80;

// ============================================================================
// System
// ============================================================================
const long SERIAL_BAUD_RATE = 115200;
