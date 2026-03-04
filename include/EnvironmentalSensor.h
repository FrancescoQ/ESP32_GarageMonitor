/**
 * @file EnvironmentalSensor.h
 * @brief BME280 temperature, humidity, and pressure readings
 *
 * Wraps the Adafruit BME280 library to provide periodic environmental
 * readings over the shared I2C bus. Readings are cached and refreshed
 * at a configurable interval to avoid hammering the sensor.
 */

#pragma once

#include <Arduino.h>
#include <Adafruit_BME280.h>

class EnvironmentalSensor {
public:
  EnvironmentalSensor();

  /**
   * @brief Initialize BME280 on I2C bus
   * @return true if sensor found and initialized
   */
  bool begin();

  /**
   * @brief Refresh readings if interval has elapsed
   */
  void loop();

  /**
   * @brief Check if sensor initialized successfully
   */
  bool isReady() const;

  /**
   * @brief Temperature in Celsius
   */
  float getTemperature() const;

  /**
   * @brief Relative humidity in percent
   */
  float getHumidity() const;

  /**
   * @brief Atmospheric pressure in hPa
   */
  float getPressure() const;

private:
  static const unsigned long READ_INTERVAL_MS = 10000;  // Read every 10s

  Adafruit_BME280 m_bme;
  bool m_ready;
  float m_temperature;
  float m_humidity;
  float m_pressure;
  unsigned long m_lastRead;

  void readSensor();
};
