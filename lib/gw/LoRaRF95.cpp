#include <LoRaRF95.h>

#ifdef RF95

#ifdef ESP8266

RH_RF95<HardwareSerial> rf95(Serial);

#else
SoftwareSerial SSerial(RX_PIN, TX_PIN); // RX, TX
#define COMSerial SSerial
#define ShowSerial Serial
RH_RF95<SoftwareSerial> rf95(COMSerial);
#endif
#endif
