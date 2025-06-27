#ifdef WIFI
#include "WiFiConn.h"

#ifdef ALEXA
// AlexaCallbackType WiFiConn::alexaCallbackFn = nullptr;
#endif

#ifdef ASYNC_WS
AsyncWebServer server(Config::WEBSERVER_PORT);
DNSServer dns;
WiFiConn wifiConn(&server, &dns);
#else
WiFiConn wifiConn;
#endif

#endif