/**
 * @file EnvironmentalSensor.cpp
 * @brief BME280 sensor driver implementation
 */

#include "EnvironmentalSensor.h"
#include "Config.h"

EnvironmentalSensor::EnvironmentalSensor()
  : m_ready(false),
    m_temperature(0.0f),
    m_humidity(0.0f),
    m_pressure(0.0f),
    m_lastRead(0) {
}

bool EnvironmentalSensor::begin() {
  if (m_bme.begin(I2C_ADDR_BME280)) {
    m_ready = true;
    readSensor();
    Serial.print(F("[ENV] BME280 initialized at 0x"));
    Serial.println(I2C_ADDR_BME280, HEX);
    Serial.print(F("[ENV] Temp: "));
    Serial.print(m_temperature, 1);
    Serial.print(F("C  Hum: "));
    Serial.print(m_humidity, 1);
    Serial.print(F("%  Press: "));
    Serial.print(m_pressure, 1);
    Serial.println(F("hPa"));
    return true;
  }

  Serial.println(F("[ENV] BME280 not found!"));
  return false;
}

void EnvironmentalSensor::loop() {
  if (!m_ready) return;

  unsigned long now = millis();
  if (now - m_lastRead >= READ_INTERVAL_MS) {
    m_lastRead = now;
    readSensor();
  }
}

bool EnvironmentalSensor::isReady() const {
  return m_ready;
}

float EnvironmentalSensor::getTemperature() const {
  return m_temperature;
}

float EnvironmentalSensor::getHumidity() const {
  return m_humidity;
}

float EnvironmentalSensor::getPressure() const {
  return m_pressure;
}

void EnvironmentalSensor::readSensor() {
  m_temperature = m_bme.readTemperature();
  m_humidity = m_bme.readHumidity();
  m_pressure = m_bme.readPressure() / 100.0f;  // Pa -> hPa
}
