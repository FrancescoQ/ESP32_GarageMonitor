/**
 * Garage Monitoring System
 */

#include <Arduino.h>
#include "SystemController.h"

// Global system controller
SystemController *systemController = nullptr;

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Create and initialize system controller
  systemController = new SystemController();
  systemController->begin();

  Serial.println(F("\nSystem initialization complete"));
  Serial.println(F("Entering main loop...\n"));
}

void loop() {
  if (systemController) {
    systemController->loop();
  }

  // Small delay to prevent tight loop
  delay(10);
}
