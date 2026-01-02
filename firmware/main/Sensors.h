#pragma once
#include <Arduino.h>

namespace Sensors {
  void begin();
  
  // DHT Sensor (Temperature/Humidity)
  float readTemperature();
  float readHumidity();
  
  // ENS160-AHT21 Air Quality Sensor (I2C)
  float readCO2();                // eCO2 - Equivalent CO2 (ppm)
  float readTVOC();               // Total Volatile Organic Compounds (ppb)
  uint8_t readAQI();              // Air Quality Index (1-5)
  
  // Soil Moisture Sensor (Analog)
  float readMoisture();           // Analog reading (0-100%)
  bool readMoistureDigital();     // Digital reading (threshold triggered)
  
  // CWT-Soil-THCPH-S Sensor (RS485 Modbus)
  // Only pH is sent to Firebase, other values available for local use
  float readPH();                 // Soil pH (3-9 range)
  float readSoilTemperature();    // Soil temperature (°C) - optional
  float readSoilHumidity();       // Soil humidity from RS485 (%) - optional
  float readConductivity();       // Electrical conductivity (uS/cm) - optional
}
