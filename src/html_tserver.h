#ifdef WS
#ifndef HTML_SERVER_H
#define HTML_SERVER_H

#include <ESPAsyncWebServer.h>
#include "queue_message.h"
#include <ArduinoJson.h>

class HtmlServer
{
private:
    static AsyncWebServer *espServer;

    // Métodos privados
    static String getCommonStyles();
    static String generateMenu();
    static void generateHomePage(AsyncWebServerRequest *request);
    static void generateDeviceDetailsPage(AsyncWebServerRequest *request, uint8_t tid);
    static void generateOTAPage(AsyncWebServerRequest *request);
    static void handleRootRequest(AsyncWebServerRequest *request);
    static void handleDeviceDetailsRequest(AsyncWebServerRequest *request);
    static void doOTAUpdate(AsyncWebServerRequest *request);
    static void handleBatchStatusRequest(AsyncWebServerRequest *request); // Novo método

public:
        // ========== Web Server Interface ==========
    static void waitTimeout(const bool ateQueDiferente, const int timeout = 3000);
    static void generateHtmlPage();
    static void generateDeviceListHtmlPage();
    static void handleToggleDevice();
    static void initWebServer(AsyncWebServer *ws = nullptr);
    static void process();
    static bool begin();
    static void respStatus(AsyncWebServerRequest *request, uint8_t tid, String status);
    static void handleToggleDevice(AsyncWebServerRequest *request);
};

extern HtmlServer htmlServer;
#endif
#endif // HTML_SERVER_H