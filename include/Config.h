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
 *
 * @author Francesco
 * @date February 2026
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
// Timing Constants
// ============================================================================
const unsigned long RELAY_PULSE_MS = 200;             // Momentary relay activation
const unsigned long DOOR_DEBOUNCE_MS = 50;          // Reed switch debounce
const unsigned long DOOR_ALERT_DELAY_MS = 300000;   // 5 min before door-open alert
const unsigned long SENSOR_READ_INTERVAL_MS = 60000; // Read sensors every 60s
const unsigned long SMS_REPORT_INTERVAL_MS = 21600000; // Periodic report every 6h
const unsigned long SMS_POLL_INTERVAL_MS = 5000;    // Poll for incoming SMS every 5s
const int SMS_PURGE_THRESHOLD = 10;                 // Purge all SMS if count reaches this

// ============================================================================
// System
// ============================================================================
const long SERIAL_BAUD_RATE = 115200;
