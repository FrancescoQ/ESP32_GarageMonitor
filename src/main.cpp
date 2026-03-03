/**
 * @file main.cpp
 * @brief Garage monitoring system entry point
 *
 * Minimal main file - all logic delegated to SystemController.
 */

#include <Arduino.h>
#include "SystemController.h"

SystemController systemController;

void setup() {
  Serial.begin(115200);
  delay(1000);
  systemController.begin();
}

void loop() {
  systemController.loop();
  delay(10);
}
