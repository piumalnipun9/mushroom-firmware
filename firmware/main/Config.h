#ifndef MUSHROOM_CONFIG_H
#define MUSHROOM_CONFIG_H

// Pin definitions (change here if needed)
#define PIN_DHT 4        // digital4 (GPIO4) on the ESP32 dev board
#define DHT_TYPE DHT11   // set to DHT11 if you have that sensor

// Light PWM output (ESP32 LEDC)
#define PIN_LIGHT 15
#define PWM_FREQ 5000
#define PWM_CHANNEL 0
#define PWM_RESOLUTION 8

// ============================================
// ENS160-AHT21 Air Quality + Temp/Humidity Sensor (I2C)
// ============================================
// Uses standard I2C pins - shared with other I2C devices
#define PIN_I2C_SDA 21       // ESP32 default I2C SDA
#define PIN_I2C_SCL 22       // ESP32 default I2C SCL
#define ENS160_I2C_ADDR 0x53 // ENS160 default I2C address (can be 0x52 if ADDR pin is LOW)
#define AHT21_I2C_ADDR 0x38  // AHT21 default I2C address
// ENS160 provides: eCO2 (equivalent CO2), TVOC, AQI
// AHT21 provides: Temperature, Humidity

// ============================================
// Soil Moisture Sensor (Analog + Digital)
// ============================================
#define PIN_SOIL_ANALOG 34   // ADC1_CH6 (input-only) - Analog output from sensor
#define PIN_SOIL_DIGITAL 23  // Digital output from sensor (threshold triggered)
// Note: GPIO34 is input-only, suitable for ADC
// Sensor VCC: 3.3V-5V, GND: GND

// ============================================
// CWT-Soil-THCPH-S Sensor (RS485 Modbus) - Uses Hardware Serial2
// This sensor reads: pH, Temperature, Humidity, Conductivity
// Only pH is sent to Firebase
// ============================================
#define PIN_SOIL_RS485_RX 16     // Hardware Serial2 RX (GPIO16) <- MAX485 RO
#define PIN_SOIL_RS485_TX 17     // Hardware Serial2 TX (GPIO17) -> MAX485 DI
#define PIN_SOIL_RS485_RE 33     // Receiver Enable (LOW = receive enabled)
#define PIN_SOIL_RS485_DE 25     // Driver Enable (HIGH = transmit enabled)
#define SOIL_RS485_BAUDRATE 4800 // CWT sensor default baud rate
// Wiring:
// - Brown wire: Power + (DC 5-30V)
// - Black wire: Power - (GND) - MUST share with ESP32 GND!
// - Yellow wire: RS485 A+ -> MAX485 A
// - Blue wire: RS485 B- -> MAX485 B
// MAX485 -> ESP32:
// - VCC: 3.3V
// - GND: GND (shared with sensor)
// - RO (Receiver Out) -> GPIO16
// - DI (Driver In) <- GPIO17
// - RE (Receiver Enable) <- GPIO33
// - DE (Driver Enable) <- GPIO25

#endif // MUSHROOM_CONFIG_H
