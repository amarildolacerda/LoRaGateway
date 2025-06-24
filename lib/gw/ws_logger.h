#ifndef WSLOGGER_H
#define WSLOGGER_H

#ifdef WIFI

#include "Arduino.h"
#include "logger.h"
#include "config.h"
#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>

// Declarações externas
extern AsyncWebSocket wsService;
void handleWebSocketMessage(void *arg, uint8_t *data, size_t len);
void logCallbackFn(const String &message);

namespace WSLogger
{
    extern AsyncWebServer *wserver;

#ifdef WIFI
    void initWs(AsyncWebServer &server);
#endif
}

#endif

#endif