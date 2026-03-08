/**
 * @file PowerManager.cpp
 * @brief ESP32 light sleep implementation
 *
 * Uses gpio_wakeup_enable() for all wake sources. The modem UART RX
 * (GPIO 16) wakes on LOW level — when the modem sends a URC (+CMTI),
 * the UART start bit pulls the line LOW, waking the ESP32 instantly.
 *
 * Door sensor polarity is dynamic: wake on the opposite of current state
 * so any state change triggers a wakeup.
 */

#include "PowerManager.h"
#include "Config.h"
#include <esp_sleep.h>
#include <driver/gpio.h>

void PowerManager::begin() {
  Serial.println(F("[PWR] PowerManager initialized"));
}

WakeReason PowerManager::enterLightSleep(bool doorOpen,
                                         uint32_t sleepIntervalMin) {
  // Configure wake sources
  configureGPIOWake(doorOpen);

  // Timer wake — convert minutes to microseconds
  uint64_t intervalUs = (uint64_t)sleepIntervalMin * 60ULL * 1000000ULL;
  esp_sleep_enable_timer_wakeup(intervalUs);

  // Flush serial output before sleeping to avoid garbled text
  Serial.print(F("[PWR] Entering light sleep ("));
  Serial.print(sleepIntervalMin);
  Serial.println(F("min timer)"));
  Serial.flush();

  // Enter light sleep — execution blocks here until wake
  esp_light_sleep_start();

  // Woke up — detect reason
  WakeReason reason = detectWakeReason();
  return reason;
}

WakeReason PowerManager::detectWakeReason() {
  esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();

  if (cause == ESP_SLEEP_WAKEUP_TIMER) {
    return WakeReason::TIMER;
  }

  if (cause == ESP_SLEEP_WAKEUP_GPIO) {
    // Check which GPIO triggered the wake by reading current pin state.
    // GPIO wake fires on level, so the pin should still be in the
    // triggering state when we check immediately after waking.

    // Modem UART RX — LOW means data incoming
    if (digitalRead(PIN_SIM_RXD) == LOW) {
      return WakeReason::MODEM_UART;
    }

    // Water sensor — LOW means water detected
    if (digitalRead(PIN_WATER_SENSOR) == LOW) {
      return WakeReason::WATER_SENSOR;
    }

    // Buttons — any LOW means pressed (INPUT_PULLUP)
    if (digitalRead(PIN_BTN_CLOSE) == LOW ||
        digitalRead(PIN_BTN_OPEN) == LOW ||
        digitalRead(PIN_BTN_STOP) == LOW ||
        digitalRead(PIN_BTN_FUNC) == LOW) {
      return WakeReason::BUTTON;
    }

    // Door sensor — state changed (could be either HIGH or LOW)
    // If none of the above matched, assume it was the door
    return WakeReason::DOOR_SENSOR;
  }

  return WakeReason::UNKNOWN;
}

void PowerManager::configureGPIOWake(bool doorOpen) {
  // Modem UART RX (GPIO 16): wake on LOW (UART start bit)
  gpio_wakeup_enable(GPIO_NUM_16, GPIO_INTR_LOW_LEVEL);

  // Water sensor (GPIO 32): wake on LOW (water detected)
  gpio_wakeup_enable(GPIO_NUM_32, GPIO_INTR_LOW_LEVEL);

  // Door sensor (GPIO 5): wake on state change
  // INPUT_PULLUP: door open = HIGH, door closed = LOW
  // Wake on the opposite of current state to detect any change
  if (doorOpen) {
    gpio_wakeup_enable(GPIO_NUM_5, GPIO_INTR_LOW_LEVEL);
  } else {
    gpio_wakeup_enable(GPIO_NUM_5, GPIO_INTR_HIGH_LEVEL);
  }

  // Buttons (all INPUT_PULLUP): wake on LOW (pressed)
  gpio_wakeup_enable(GPIO_NUM_33, GPIO_INTR_LOW_LEVEL);  // BTN_CLOSE
  gpio_wakeup_enable(GPIO_NUM_13, GPIO_INTR_LOW_LEVEL);  // BTN_OPEN
  gpio_wakeup_enable(GPIO_NUM_14, GPIO_INTR_LOW_LEVEL);  // BTN_STOP
  gpio_wakeup_enable(GPIO_NUM_15, GPIO_INTR_LOW_LEVEL);  // BTN_FUNC

  // Enable GPIO as wake source
  esp_sleep_enable_gpio_wakeup();
}
