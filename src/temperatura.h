#pragma once

#ifdef ESP32
#ifdef __cplusplus
extern "C"
{
#endif
    float temperatureRead();

#ifdef __cplusplus
}
#endif
// #define BATTERY_PIN 34 // Pino onde a tensão da bateria está conectada

#endif
