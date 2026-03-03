/**
 * @file ButtonController.cpp
 * @brief Manual control button handling with debounce
 */

#include "ButtonController.h"
#include "Config.h"

ButtonController::ButtonController(Door& door) : m_door(door) {
}

void ButtonController::begin() {
  const int pins[NUM_BUTTONS] = {PIN_BTN_CLOSE, PIN_BTN_OPEN, PIN_BTN_STOP};
  const char* names[NUM_BUTTONS] = {"CLOSE", "OPEN", "STOP"};

  for (int i = 0; i < NUM_BUTTONS; i++) {
    pinMode(pins[i], INPUT_PULLUP);
    m_buttons[i].pin = pins[i];
    m_buttons[i].lastRawState = HIGH;     // Released (pullup)
    m_buttons[i].lastDebounceTime = millis();
    m_buttons[i].pressed = false;

    Serial.print(F("[BTN] Button "));
    Serial.print(names[i]);
    Serial.print(F(" on GPIO "));
    Serial.println(pins[i]);
  }
}

void ButtonController::loop() {
  for (int i = 0; i < NUM_BUTTONS; i++) {
    bool raw = digitalRead(m_buttons[i].pin);

    // Reset debounce timer when raw reading changes
    if (raw != m_buttons[i].lastRawState) {
      m_buttons[i].lastDebounceTime = millis();
      m_buttons[i].lastRawState = raw;
    }

    // Accept new state only after stable for BTN_DEBOUNCE_MS
    bool currentlyPressed = (raw == LOW);
    if (currentlyPressed != m_buttons[i].pressed
        && (millis() - m_buttons[i].lastDebounceTime >= BTN_DEBOUNCE_MS)) {
      m_buttons[i].pressed = currentlyPressed;

      // Act on falling edge only (button just pressed)
      if (currentlyPressed) {
        switch (i) {
          case 0:
            Serial.println(F("[BTN] CLOSE pressed"));
            m_door.close();
            break;
          case 1:
            Serial.println(F("[BTN] OPEN pressed"));
            m_door.open();
            break;
          case 2:
            Serial.println(F("[BTN] STOP pressed"));
            m_door.stop();
            break;
        }
      }
    }
  }
}
