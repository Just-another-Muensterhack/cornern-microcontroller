#ifndef CONFIG_H
#define CONFIG_H

// Set value to 0/1 to turn console debug messages off/on
#define CONFIG_DEBUG_MODE 1

// ICS-43434 Microphone Pin Connection
#define I2S_SCK SCL
#define I2S_WS D6
#define I2S_SD SDA

// Sound Level Meter
#define SoundSensorPin A3
#define VREF  3.3

// Leds
#define GREEN_LED A10
#define YELLOW_LED A9
#define RED_LED A8

#define LIMIT_LOW 70
#define LIMIT_HIGH 90

// WiFi Configuration
#define WIFI_SSID "WiFi-Network-Name"           // WiFi that the device should connect to
#define WIFI_PASSWORD "WiFi-Network-Password"   // WiFi Password

// API Configuration
#define API_URL "https://backend.com/update"    // POST address of the Backend
#define AUTH_BEARER "Bearer Secure-Secret-Key"  // Secure API Token

#endif // CONFIG_H
