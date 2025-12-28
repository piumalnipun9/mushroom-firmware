#ifndef MUSHROOM_CONFIG_H
#define MUSHROOM_CONFIG_H

// Pin definitions (change here if needed)
#define PIN_DHT 4        // digital4 (GPIO4) on the ESP32 dev board
#define DHT_TYPE DHT22   // set to DHT11 if you have that sensor

// Light PWM output (ESP32 LEDC)
#define PIN_LIGHT 15
#define PWM_FREQ 5000
#define PWM_CHANNEL 0
#define PWM_RESOLUTION 8

#endif // MUSHROOM_CONFIG_H
