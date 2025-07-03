
#ifndef CONFIG_H
#define CONFIG_H

// Compiler: The project is compiled using the ESP32 toolchain.
// Platform: The project is designed to run on the ESP32 platform.
// Libraries: The project uses the ESP32 Arduino core libraries and other related libraries for LoRa, OLED display, and Wi-Fi functionalities.
#include <Arduino.h>

// #define TERMINAL_ID VTERMINAL_ID
// #define TERMINAL_NAME VTERMINAL_NAME

// ========== Configurações do Sistema ==========
namespace Config
{
    constexpr const char *TIMEZONE = "BRT3BRST";
    constexpr const char *ALEXA_TERMOSTATO = "Termostato";
#ifndef ALEXA_MAX_CLIENTS
    constexpr const uint8_t ALEXA_MAX_CLIENTS = 10;
#endif

    // Hardware - Pinos LoRa (TTGO LoRa32 v1)

#ifdef TTGO
    constexpr const uint8_t LORA_CS_PIN = 18;    // GPIO18 - Chip Select
    constexpr const uint8_t LORA_RESET_PIN = 14; // GPIO14 - Reset
    constexpr const uint8_t LORA_IRQ_PIN = 26;   // GPIO26 - Interrupção
#endif
#ifdef HELTEC
    constexpr const uint8_t LORA_CS_PIN = 18;    // GPIO18 - Chip Select
    constexpr const uint8_t LORA_RESET_PIN = 14; // GPIO14 - Reset
    constexpr const uint8_t LORA_IRQ_PIN = 26;   // GPIO26 - Interrupção
#endif

#if defined(TTGO) || defined(HELTEC)
    constexpr const uint32_t LORA_BAND = 868E6; // usado esp32 TTGO LoRa32 v1
#elif RF95
    constexpr const uint32_t LORA_BAND = 868.0; // usado RF95
#elif defined(NRF24)
    constexpr const int LORA_BAND = 76;
#else
    constexpr const uint32_t LORA_BAND = 0; // nao usado
#endif

    constexpr const uint16_t LORA_SYNC_WORD = 0x12;

    constexpr int MIN_RSSI_THRESHOLD = -120; // ajuste conforme necessário
    constexpr float MIN_SNR_THRESHOLD = -20.0;
    constexpr int ACK_TIMEOUT_MS = 100;

    // Display OLED
    constexpr const uint8_t SCREEN_WIDTH = 128;
    constexpr const uint8_t SCREEN_HEIGHT = 64;
    constexpr const uint8_t OLED_ADDRESS = 0x3C;
    constexpr const uint32_t DISPLAY_UPDATE_INTERVAL = 1000; // Atualização a cada 1s

    // Rede
    constexpr const uint16_t WEBSERVER_PORT = 80;
    constexpr const uint16_t WIFI_TIMEOUT_S = 180;
    constexpr const char *WIFI_AP_NAME = "transmitter";
    constexpr const char *NTP_SERVER = "pool.ntp.org";
    constexpr long GMT_OFFSET_SEC = -10800; // UTC-3 (Brasília)
    constexpr const int DAYLIGHT_OFFSET_SEC = 0;

    // Sistema
    constexpr const uint32_t SERIAL_BAUD = 115200;
    constexpr const uint32_t PING_TIMEOUT_MS = 30000;
    constexpr const uint32_t STATE_CHECK_INTERVAL = 30000;
    constexpr const uint32_t PRESENTATION_INTERVAL = 60000;
    constexpr const uint32_t COMMAND_TIMEOUT = 3000;

}

#endif