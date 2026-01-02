ESP32 Pinout & Wiring Recommendations

Overview

This document lists recommended ESP32 pin connections for the mushroom monitoring system: sensors, actuators, motor/robot-arm control, camera notes, and LED strip PWM wiring. These are suggestions — adapt pins to your exact board and peripherals.

General notes

- Power: sensors and ESP32 use 3.3V. LED strips (analog PWM-driven DRIVERS) or addressable LEDs (WS2812/Neopixel) usually require 5V. Use a common ground between 5V and 3.3V supplies.
- Analog readings: prefer ADC1 pins for stable analog measurements while Wi-Fi is active. ADC1 pins: GPIO32, GPIO33, GPIO34, GPIO35, GPIO36 (VP), GPIO37 (VN), GPIO38, GPIO39. Pins 34-39 are input-only (no PWM).
- PWM (LEDC): most GPIOs support PWM (LED control) except input-only pins (34–39) and flash pins (6–11). Use LEDC channels for dimming.
- Avoid using GPIO6–GPIO11 (flash/SPI) for peripherals.
- DHT/one-wire sensors require a pull-up (4.7K recommended).
- I2C default pins: SDA = GPIO21, SCL = GPIO22.

Recommended pin mapping (example)

Sensors

- Temperature & Humidity (DHT22 or SHT3x):
  - DHT22 (single-wire): DHT_PIN = GPIO4 (needs 4.7K pull-up to 3.3V)
  - SHT3x (I2C): SDA = GPIO21, SCL = GPIO22, VCC = 3.3V, GND = GND

- ENS160-AHT21 Air Quality Sensor (I2C) - ONLY using ENS160 for CO2:
  - Uses standard I2C bus (shared with other I2C devices)
  - SDA: GPIO21 (ESP32 default)
  - SCL: GPIO22 (ESP32 default)
  - **Power Options:**
    - VCC/VIN: 3.3V-5V (most modules have onboard voltage regulator)
    - If your module has VIN pin: You can use 5V safely
    - If your module only has VCC: Check if it has a regulator, otherwise use 3.3V
  - GND: Common GND
  - ENS160 (Air Quality) - **Primary CO2 source**:
    - I2C Address: 0x53 (default) or 0x52 (if ADDR pin is LOW)
    - Provides: eCO2 (400-65000 ppm), TVOC (0-65000 ppb), AQI (1-5)
    - Warm-up time: ~1 hour for accurate readings
    - Initial start-up: ~3 minutes before valid readings
  - AHT21 (Temperature & Humidity) - **NOT USED** (DHT sensor is primary):
    - I2C Address: 0x38 (fixed)
    - Available if you want to use it later

- Soil Moisture Sensor (Analog + Digital Interface):
  - Analog Output (AO) -> ESP32 GPIO34 (ADC1_CH6, input-only)
  - Digital Output (DO) -> ESP32 GPIO23
  - VCC: 3.3V-5V
  - GND: Common GND
  - Potentiometer on module adjusts digital threshold
  - Analog: 0-4095 (12-bit ADC), inversely proportional to moisture
  - Digital: LOW = wet (above threshold), HIGH = dry (below threshold)

- NPK Sensor (RS485 with TTL Converter):
  - RS485 TTL Converter connections:
    - VCC: 3.3V-5V (match your module)
    - GND: Common GND
    - RO (Receiver Output) -> ESP32 GPIO25 (Software Serial RX)
    - DI (Driver Input) <- ESP32 GPIO26 (Software Serial TX)
    - DE (Driver Enable) + RE (Receiver Enable) -> ESP32 GPIO27
  - NPK Sensor to RS485 Converter:
    - A (Yellow/+): RS485 A/+ terminal
    - B (Blue/-): RS485 B/- terminal
    - VCC: 12V or 24V (check sensor specs)
    - GND: Common GND
  - Protocol: Modbus RTU
  - Baudrate: 9600
  - Sensor Address: 0x01 (default)
  - Registers:
    - Nitrogen: 0x001E
    - Phosphorus: 0x001F
    - Potassium: 0x0020
  - Output unit: mg/kg

- Substrate moisture (capacitive / analog):
  - MOISTURE_PIN = GPIO34 (ADC1_CH6) — input-only ADC pin; do not use for PWM

- pH sensor (analog probe via amplifier):
  - PH_PIN = GPIO35 (ADC1_CH7) — input-only ADC pin; connect via recommended signal conditioning circuit and common ground

- Light intensity (lux) sensor (e.g., BH1750 I2C):
  - SDA = GPIO21, SCL = GPIO22

Actuators

- Humidifier relay / MOSFET control:
  - HUMIDIFIER_CTRL = GPIO25 (use transistor or opto-isolated relay; active LOW or HIGH depending on relay)
- Exhaust fan relay / MOSFET:
  - FAN_CTRL = GPIO26
- Water sprayer / pump control:
  - SPRAYER_CTRL = GPIO27
- LED grow strip (high-power, analog PWM via MOSFET):
  - Use 12V/24V supply for strip, MOSFET gate driven from ESP32 PWM pin (through gate resistor and level-shifting if needed).
  - PWM_PIN_LIGHTS = GPIO18 (LEDC channel e.g., channel 0, 12-bit resolution, up to 40 kHz)
  - Use a logic-level N-channel MOSFET (e.g., IRLZ44N or better), common ground, and a flyback diode for inductive loads where necessary.

LED Addressable (WS2812/NeoPixel)

- DATA_PIN_WS2812 = GPIO17 or GPIO21 or GPIO18 (choose a pin with RMT or reliable timing). Use 5V power for LEDs and common ground.
- Add a 330Ω resistor between ESP32 data pin and LED data line and a 1000 µF bulk capacitor across LED power rails.

Robot arm & sliding rail (motors + endstops)

- Stepper driver Step/Dir pins (example using two stepper motors):
  - X-axis step = GPIO14, X-axis dir = GPIO12
  - Y/arm step = GPIO13, Y/arm dir = GPIO15
  - Enable pins (optional) = GPIO2
  - Limit switches / endstops: HOME_LEFT = GPIO32 (input), HOME_RIGHT = GPIO33 (input)
  - Use proper stepper drivers (A4988, DRV8825, TMC2209); driver step pins accept digital pulses from ESP32 PWM-capable pins.
- Servo for camera tilt/arm orientation (if using hobby servos):
  - SERVO_BASE = GPIO19 (use LEDC for stable PWM frequency, or Servo library configured for ESP32)
  - SERVO_JOINT = GPIO5

Camera

- If using an `esp32-camera` module (AI-Thinker ESP32-CAM), use the camera board directly (it has fixed pins). Do not remap pins; follow the module wiring.
- If using a separate camera + USB/OV7670 or similar, follow the camera module docs for pin assignments and required power.

Power & Grounding

- ESP32 VIN / 5V: supply board VIN with stable 5V if using 5V source; use onboard 3.3V regulator for ESP32 core.
- LED strips: use dedicated 12V/24V supply sized for current draw; DO NOT power strips from the ESP32 5V regulator.
- Always common-ground the ESP32 and external supplies (MOSFET gates reference ground).
- Use decoupling capacitors on sensor power rails and large cap near LED strip input (1000 µF recommended).

Pin usage cautions

- Avoid GPIO6-GPIO11 (connected to SPI flash). Using them may corrupt flash.
- Pins 34-39 are input-only; they cannot be used for PWM, I2C, or driving devices.
- Prefer ADC1 pins (32–39) for analog sensors when Wi‑Fi is used; ADC2 pins can be unreliable when Wi‑Fi is active.
- If a sensor requires 5V logic, use level shifting before connecting to ESP32 GPIOs.

Example minimal mapping (quick reference)

- `GPIO4` — DHT22 data (backup temp/humidity)
- `GPIO21` — I2C SDA (ENS160-AHT21, BH1750, SHT3x)
- `GPIO22` — I2C SCL (shared I2C bus)
- `GPIO34` — Soil Moisture Analog (ADC1_CH6, input-only)
- `GPIO23` — Soil Moisture Digital (threshold output)
- `GPIO25` — NPK Sensor RX (Software Serial RX via RS485)
- `GPIO26` — NPK Sensor TX (Software Serial TX via RS485)
- `GPIO27` — NPK RS485 DE/RE (direction control)
- `GPIO35` — pH (ADC)
- `GPIO32` — Humidifier relay/MOSFET
- `GPIO33` — Exhaust fan relay/MOSFET
- `GPIO18` — PWM for grow lights (PWM via LEDC)
- `GPIO16/17` — Free (previously used for MH-Z19C UART)
- `GPIO14/12/13/15` — Stepper step/dir examples

Final notes

- This pinout is a starting point; adapt to the number of sensors and actuators you actually have.
- Update `main.ino` and the modules in `firmware/` to match these pin choices before wiring.
- I can generate a printable wiring diagram (SVG) or a PlatformIO `board` sample if you want — tell me which board variant you will use (e.g., ESP32 DevKitC, ESP32-CAM, M5Stack).
