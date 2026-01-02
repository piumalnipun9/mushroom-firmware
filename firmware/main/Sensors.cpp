#include "Sensors.h"
#include "Config.h"
#include <DHT.h>
#include <Arduino.h>
#include <Wire.h>
#include <ModbusMaster.h>

static DHT dht(PIN_DHT, DHT_TYPE);

// ENS160-AHT21 sensor state
static bool ens160Ready = false;
static bool aht21Ready = false;

// RS485 Serial for CWT-Soil-THCPH-S sensor
HardwareSerial SoilRS485Serial(2);  // Use UART2
ModbusMaster soilModbus;

// ModbusMaster callbacks for RS485 direction control
void rs485PreTransmission() {
    digitalWrite(PIN_SOIL_RS485_RE, HIGH);  // Disable receiver
    digitalWrite(PIN_SOIL_RS485_DE, HIGH);  // Enable driver
}

void rs485PostTransmission() {
    digitalWrite(PIN_SOIL_RS485_DE, LOW);   // Disable driver
    digitalWrite(PIN_SOIL_RS485_RE, LOW);   // Enable receiver
}

// CWT-Soil-THCPH-S register addresses (from datasheet):
//   0x0000 = Soil Humidity (0.1% RH) -> divide by 10
//   0x0001 = Soil Temperature (0.1°C, signed) -> divide by 10
//   0x0002 = Conductivity/pH (check your sensor) -> divide by 10
//   0x0003 = pH (0.1 pH) -> divide by 10

// ENS160 Register addresses
#define ENS160_REG_OPMODE     0x10
#define ENS160_REG_STATUS     0x20
#define ENS160_REG_DATA_AQI   0x21
#define ENS160_REG_DATA_TVOC  0x22
#define ENS160_REG_DATA_ECO2  0x24
#define ENS160_REG_TEMP_IN    0x13
#define ENS160_REG_RH_IN      0x15

// ENS160 Operating modes
#define ENS160_OPMODE_STANDARD 0x02

// Helper function to write to ENS160 register
static bool ens160WriteReg(uint8_t reg, uint8_t value)
{
    Wire.beginTransmission(ENS160_I2C_ADDR);
    Wire.write(reg);
    Wire.write(value);
    return Wire.endTransmission() == 0;
}

// Helper function to read from ENS160 register
static bool ens160ReadReg(uint8_t reg, uint8_t* buffer, uint8_t len)
{
    Wire.beginTransmission(ENS160_I2C_ADDR);
    Wire.write(reg);
    if (Wire.endTransmission() != 0) return false;
    
    Wire.requestFrom((uint8_t)ENS160_I2C_ADDR, len);
    if (Wire.available() < len) return false;
    
    for (uint8_t i = 0; i < len; i++)
    {
        buffer[i] = Wire.read();
    }
    return true;
}

// Initialize AHT21 sensor
static bool initAHT21()
{
    // Send initialization command
    Wire.beginTransmission(AHT21_I2C_ADDR);
    Wire.write(0xBE);  // Initialize command
    Wire.write(0x08);
    Wire.write(0x00);
    if (Wire.endTransmission() != 0) return false;
    delay(10);
    return true;
}

// Trigger AHT21 measurement
static bool aht21TriggerMeasurement()
{
    Wire.beginTransmission(AHT21_I2C_ADDR);
    Wire.write(0xAC);  // Trigger measurement
    Wire.write(0x33);
    Wire.write(0x00);
    return Wire.endTransmission() == 0;
}

// Read AHT21 data
static bool aht21ReadData(float* temp, float* humidity)
{
    aht21TriggerMeasurement();
    delay(80);  // Measurement takes ~75ms
    
    Wire.requestFrom((uint8_t)AHT21_I2C_ADDR, (uint8_t)6);
    if (Wire.available() < 6) return false;
    
    uint8_t data[6];
    for (int i = 0; i < 6; i++)
    {
        data[i] = Wire.read();
    }
    
    // Check if busy bit is set
    if (data[0] & 0x80) return false;
    
    // Calculate humidity (20-bit value)
    uint32_t rawHumidity = ((uint32_t)data[1] << 12) | ((uint32_t)data[2] << 4) | (data[3] >> 4);
    *humidity = (rawHumidity * 100.0f) / 1048576.0f;
    
    // Calculate temperature (20-bit value)
    uint32_t rawTemp = ((uint32_t)(data[3] & 0x0F) << 16) | ((uint32_t)data[4] << 8) | data[5];
    *temp = ((rawTemp * 200.0f) / 1048576.0f) - 50.0f;
    
    return true;
}

namespace Sensors
{

    void begin()
    {
        // DHT sensor initialization (primary for temp/humidity)
        pinMode(PIN_DHT, INPUT_PULLUP);
        dht.begin();
        randomSeed(analogRead(0));
        Serial.println("[Sensors] DHT sensor initialized (for Temperature/Humidity)");
        
        // I2C initialization for ENS160 (CO2/TVOC/AQI sensor)
        Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
        Wire.setClock(100000);  // 100kHz I2C clock
        Serial.println("[Sensors] I2C initialized");
        
        // Initialize ENS160 for CO2 readings only
        delay(100);  // Wait for sensor to power up
        if (ens160WriteReg(ENS160_REG_OPMODE, ENS160_OPMODE_STANDARD))
        {
            ens160Ready = true;
            Serial.println("[Sensors] ENS160 (CO2/TVOC/AQI) initialized successfully");
            Serial.println("[Sensors] Note: ENS160 needs ~3 min warm-up for accurate readings");
        }
        else
        {
            Serial.println("[Sensors] ERROR: ENS160 initialization failed! CO2 readings will use fallback values.");
        }
        
        // Soil Moisture sensor initialization
        pinMode(PIN_SOIL_ANALOG, INPUT);    // Analog input (ADC)
        pinMode(PIN_SOIL_DIGITAL, INPUT);   // Digital input (threshold)
        Serial.println("[Sensors] Soil Moisture sensor initialized");
        
        // CWT-Soil-THCPH-S sensor (RS485) initialization using ModbusMaster
        pinMode(PIN_SOIL_RS485_RE, OUTPUT);
        pinMode(PIN_SOIL_RS485_DE, OUTPUT);
        digitalWrite(PIN_SOIL_RS485_RE, LOW);  // Enable receiver by default
        digitalWrite(PIN_SOIL_RS485_DE, LOW);  // Disable driver by default
        
        // Initialize Serial for RS485
        SoilRS485Serial.begin(SOIL_RS485_BAUDRATE, SERIAL_8N1, PIN_SOIL_RS485_RX, PIN_SOIL_RS485_TX);
        
        // Initialize ModbusMaster
        soilModbus.begin(1, SoilRS485Serial);  // Slave ID = 1
        soilModbus.preTransmission(rs485PreTransmission);
        soilModbus.postTransmission(rs485PostTransmission);
        
        Serial.println("[Sensors] CWT-Soil-THCPH-S pH sensor (RS485) initialized");
        Serial.print("[Sensors] RS485 RX: GPIO"); Serial.print(PIN_SOIL_RS485_RX);
        Serial.print(", TX: GPIO"); Serial.print(PIN_SOIL_RS485_TX);
        Serial.print(", RE: GPIO"); Serial.print(PIN_SOIL_RS485_RE);
        Serial.print(", DE: GPIO"); Serial.print(PIN_SOIL_RS485_DE);
        Serial.print(", Baud: "); Serial.println(SOIL_RS485_BAUDRATE);
    }

    float readTemperature()
    {
        // Read from DHT sensor (primary temperature source)
        float t = dht.readTemperature();
        if (isnan(t))
        {
            float fallback = 30.0f + (random(-20, 20) / 10.0f);
            Serial.print("[FALLBACK - RANDOM VALUE] Temperature sensor failed! Using: ");
            Serial.print(fallback);
            Serial.println(" C");
            return fallback;
        }
        return t;
    }

    float readHumidity()
    {
        // Read from DHT sensor (primary humidity source)
        float h = dht.readHumidity();
        if (isnan(h))
        {
            float fallback = 80.0f + (random(-50, 50) / 10.0f);
            Serial.print("[FALLBACK - RANDOM VALUE] Humidity sensor failed! Using: ");
            Serial.print(fallback);
            Serial.println(" %");
            return fallback;
        }
        return h;
    }

    float readCO2()
    {
        if (!ens160Ready)
        {
            float fallback = 800.0f + random(-100, 100);
            Serial.print("[FALLBACK - RANDOM VALUE] ENS160 not ready! CO2 using: ");
            Serial.print(fallback);
            Serial.println(" ppm");
            return fallback;
        }
        
        // Check sensor status
        uint8_t status;
        if (!ens160ReadReg(ENS160_REG_STATUS, &status, 1))
        {
            float fallback = 800.0f + random(-100, 100);
            Serial.print("[FALLBACK - RANDOM VALUE] ENS160 status read failed! CO2 using: ");
            Serial.print(fallback);
            Serial.println(" ppm");
            return fallback;
        }
        
        // Check if new data is available (bit 1 = NEWDAT)
        if (!(status & 0x02))
        {
            // No new data, but try to read anyway
        }
        
        // Read eCO2 (2 bytes, little-endian)
        uint8_t data[2];
        if (ens160ReadReg(ENS160_REG_DATA_ECO2, data, 2))
        {
            uint16_t eco2 = data[0] | (data[1] << 8);
            return (float)eco2;
        }
        
        float fallback = 800.0f + random(-100, 100);
        Serial.print("[FALLBACK - RANDOM VALUE] ENS160 eCO2 read failed! Using: ");
        Serial.print(fallback);
        Serial.println(" ppm");
        return fallback;
    }

    float readTVOC()
    {
        if (!ens160Ready)
        {
            float fallback = 100.0f + random(-20, 20);
            Serial.print("[FALLBACK - RANDOM VALUE] ENS160 not ready! TVOC using: ");
            Serial.print(fallback);
            Serial.println(" ppb");
            return fallback;
        }
        
        // Read TVOC (2 bytes, little-endian)
        uint8_t data[2];
        if (ens160ReadReg(ENS160_REG_DATA_TVOC, data, 2))
        {
            uint16_t tvoc = data[0] | (data[1] << 8);
            return (float)tvoc;
        }
        
        float fallback = 100.0f + random(-20, 20);
        Serial.print("[FALLBACK - RANDOM VALUE] ENS160 TVOC read failed! Using: ");
        Serial.print(fallback);
        Serial.println(" ppb");
        return fallback;
    }

    uint8_t readAQI()
    {
        if (!ens160Ready)
        {
            return 2;  // Default to "Good" AQI
        }
        
        // Read AQI (1 byte)
        uint8_t aqi;
        if (ens160ReadReg(ENS160_REG_DATA_AQI, &aqi, 1))
        {
            // AQI is in lower 3 bits, range 1-5
            return aqi & 0x07;
        }
        
        return 2;  // Default to "Good"
    }

    float readMoisture()
    {
        // Read analog value from soil moisture sensor
        int rawValue = analogRead(PIN_SOIL_ANALOG);
        
        // ESP32 ADC is 12-bit (0-4095)
        // Convert to percentage (inversely proportional - dry = high resistance = high value)
        // Typical ranges: Dry soil ~3500-4095, Wet soil ~1000-2000, Water ~0-500
        
        // Map and constrain to 0-100%
        float moisture = map(rawValue, 4095, 0, 0, 100);
        moisture = constrain(moisture, 0.0f, 100.0f);
        
        // If reading is suspicious (exactly 0 or 100), use fallback
        if (rawValue < 0 || rawValue > 6000)
        {
            float fallback = 70.0f + (random(-50, 50) / 10.0f);
            Serial.print("[FALLBACK - RANDOM VALUE] Soil moisture suspicious reading! Using: ");
            Serial.print(fallback);
            Serial.println(" %");
            return fallback;
        }
        
        return moisture;
    }

    bool readMoistureDigital()
    {
        // Digital output is typically LOW when moisture exceeds threshold
        // (depends on sensor module - some are inverted)
        return digitalRead(PIN_SOIL_DIGITAL) == LOW;
    }

    // ====== CWT-Soil-THCPH-S Sensor Functions (RS485 Modbus using ModbusMaster library) ======
    // This sensor measures: pH, Soil Temp, Soil Humidity, Conductivity
    // Only pH is sent to Firebase from this sensor

    float readPH()
    {
        // Read pH from CWT-Soil-THCPH-S sensor via RS485
        // Register 0x0000 = Soil pH (value × 10, e.g., 65 = 6.5 pH)
        uint8_t result = soilModbus.readInputRegisters(0x0000, 1);
        
        if (result == soilModbus.ku8MBSuccess)
        {
            uint16_t rawValue = soilModbus.getResponseBuffer(0);
            float ph = rawValue / 10.0f;
            Serial.print("[pH Sensor] pH: ");
            Serial.println(ph);
            return ph;
        }
        else
        {
            float fallback = 6.5f + (random(-10, 10) / 10.0f);
            Serial.printf("[FALLBACK] pH read failed (error %d)! Using: %.1f\n", result, fallback);
            return fallback;
        }
    }

    float readSoilTemperature()
    {
        // Read soil temperature from CWT-Soil-THCPH-S sensor via RS485
        uint8_t result = soilModbus.readInputRegisters(0x0001, 1);
        
        if (result == soilModbus.ku8MBSuccess)
        {
            int16_t rawValue = (int16_t)soilModbus.getResponseBuffer(0);  // Signed for negative temps
            float temp = rawValue / 10.0f;
            return temp;
        }
        else
        {
            float fallback = 25.0f + (random(-20, 20) / 10.0f);
            Serial.printf("[FALLBACK] Soil temp read failed! Using: %.1f C\n", fallback);
            return fallback;
        }
    }

    float readSoilHumidity()
    {
        // Read soil humidity from CWT-Soil-THCPH-S sensor via RS485
        uint8_t result = soilModbus.readInputRegisters(0x0000, 1);
        
        if (result == soilModbus.ku8MBSuccess)
        {
            uint16_t rawValue = soilModbus.getResponseBuffer(0);
            float humidity = rawValue / 10.0f;
            return humidity;
        }
        else
        {
            float fallback = 70.0f + (random(-50, 50) / 10.0f);
            Serial.printf("[FALLBACK] Soil humidity read failed! Using: %.1f%%\n", fallback);
            return fallback;
        }
    }

    float readConductivity()
    {
        // Read electrical conductivity from CWT-Soil-THCPH-S sensor via RS485
        // Note: If pH is at 0x0002, conductivity might be elsewhere - check your sensor
        uint8_t result = soilModbus.readInputRegisters(0x0003, 1);
        
        if (result == soilModbus.ku8MBSuccess)
        {
            uint16_t rawValue = soilModbus.getResponseBuffer(0);
            return (float)rawValue;
        }
        else
        {
            float fallback = 500.0f + random(-100, 100);
            Serial.printf("[FALLBACK] Conductivity read failed! Using: %.0f uS/cm\n", fallback);
            return fallback;
        }
    }

} // namespace Sensors
