#pragma once

#ifdef WIFI

#include "Arduino.h"

#ifdef ESP32
#include <nvs_flash.h>
#endif

#include "config.h"
#include "SystemState.h"
#include "ws_logger.h"

// Conditional includes
#ifdef GATEWAY
#include "DeviceInfo.h"
#endif

#ifdef ALEXA
#include "AlexaCom.h"
#include "temperatura.h"
#endif

// WiFi related includes
#ifdef WIFIMANAGER
#include <LocalWiFiManager.h>
#else
#include "ESPAsyncWiFiManager.h"
#endif

#include "ESPAsyncWebServer.h"
#ifdef ESP8266
#include <ESP8266WiFi.h>
#else
#include "WiFi.h"
#endif
#include <ezTime.h>

#ifdef WS
#include "html_tserver.h"
#endif

class WiFiConn
{
private:
    Timezone myTZ;

    AsyncWebServer *server;
    DNSServer *dns;

#ifdef WIFIMANAGER
    WiFiManager *wifiManager;
#else
    AsyncWiFiManager *wifiManager;
#endif
#ifdef ALEXA
    AlexaCallbackType alexaCallbackFn;
#endif

    void initWiFi()
    {
#ifdef WIFIMANAGER
        wifiManager->autoConnect(); //("kcasa", "3938373635");
        systemState.isConnected = WiFi.isConnected();
#else
        WiFi.mode(WIFI_AP_STA);
        wifiManager->setConnectTimeout(20);       // Tempo de tentativa de conexão
        wifiManager->setConfigPortalTimeout(180); // 3 minutos no modo AP

        wifiManager->setAPCallback([](AsyncWiFiManager *wifiMgr)
                                   {
    Serial.println("Modo Configuração Ativado");
    Serial.print("IP do AP: ");
    Serial.println(WiFi.softAPIP()); });

        wifiManager->setSaveConfigCallback([]()
                                           {
    Serial.println("Configuração salva. Reiniciando...");
    ESP.restart(); });

#if defined(WIFI_SSID)
        WiFi.begin(String(WIFI_SSID), String(WIFI_PASSWORD));
        if (WiFi.status() != WL_CONNECTED)
            wifiManager->autoConnect("gateway-config");
#else
        // Tenta conectar em background
        wifiManager->autoConnect("gateway-config", "");
#endif

#endif
    }

    void initNTP()
    {
        waitForSync();
        myTZ.setLocation("America/Sao_Paulo");
        systemState.startedISODateTime = getISOTime();
    }

#ifdef ALEXA
    void handleAlexaCallback(unsigned char device_id, const char *device_name, bool state, unsigned char value)
    {
        if (alexaCallbackFn)
        {
            alexaCallbackFn(device_id, device_name, state, value);
        }
        Logger::log(LogLevel::INFO,
                    "Alexa Device Callback: ID: %d, Name: %s, State: %d, Value: %d",
                    device_id, device_name, state, value);
    }
#endif

public:
#ifdef ALEXA
    void setCallback(AlexaCallbackType cb = nullptr)
    {
        alexaCallbackFn = cb;
    }
#endif

    WiFiConn(AsyncWebServer *srv, DNSServer *dnsServer) : server(srv), dns(dnsServer)
    {
#ifdef WIFIMANAGER
        wifiManager = new WiFiManager(srv, dns);
#else
        wifiManager = new AsyncWiFiManager(server, dns);
#endif
#ifdef ESP32
        esp_err_t ret = nvs_flash_init();
        if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
        {
            Serial.println("Reiniciando NVS");
            ESP_ERROR_CHECK(nvs_flash_erase());
            ret = nvs_flash_init();
        }
        else
        {
            Serial.println("NVS OK");
        }
        ESP_ERROR_CHECK(ret);
#endif
    }

    ~WiFiConn()
    {

        delete wifiManager;
    }

    void changeNetwork()
    {
        WiFi.disconnect();
        wifiManager->startConfigPortal("ESP32-Config");
    }
    bool begin()
    {
        initWiFi();

        // Verificar conexão a cada 5 segundos
        // reconnectTicker.attach(5, []()
        //                       {
        //                           const bool connected = WiFi.isConnected();
        //                           if (systemState.isConnected != connected)
        //                          {
        //                              systemState.isConnected = connected;
        //                          } });

        // if (systemState.isConnected)
        //{
        initNTP();
        // Logger::log(LogLevel::INFO, "WiFi connected: %s", WiFi.SSID().c_str());
        systemState.isRunning = true;
        // systemState.isConnected = true;
        systemState.isInitialized = true;
        //}
        // else
        //{
        //    Logger::log(LogLevel::ERROR, "Failed to connect to WiFi");
        //    systemState.isRunning = false;
        //    systemState.isConnected = false;
        //    systemState.isInitialized = false;
        //}

        initAlexa();
        delay(500); // Stabilization after connection

#ifdef WS
        htmlServer.initWebServer(server);
        // wifiManager->addRoots(server);
        htmlServer.begin();
#else
        server->begin();
#endif

        WSLogger::initWs(*server);

        return systemState.isConnected;
    }
#define ESPTemperature "ESPTemperature"
    void initAlexa()
    {
#ifdef ALEXA
        alexaCom.setup(server, [this](unsigned char device_id, const char *device_name, bool state, unsigned char value)
                       { this->handleAlexaCallback(device_id, device_name, state, value); });
#if defined(ESP32) && defined(GATEWAY)
        // alexaCom.addTermostat("termostato");
        alexaCom.addTemperature(ESPTemperature);
        alexaCom.setTemperature(ESPTemperature, temperatureRead());

#endif
#endif
    }

    void resetWifi()
    {
        wifiManager->resetSettings();
    }
    void loop()
    {

#ifdef WIFIMANAGER
        wifiManager->process();
#endif

#ifdef ALEXA
        alexaCom.loop();
#if defined(ESP32) && defined(GATEWAY)
        static long temperaturaUpdate = 0;
        if (millis() - temperaturaUpdate > 30000)
        {
            temperaturaUpdate = millis();
            alexaCom.setTemperature(ESPTemperature, temperatureRead());
        }
#endif
#endif

#ifdef WS
        htmlServer.process();
#endif

        static long wifiCheck = 0;
        if (millis() - wifiCheck > 30000)
        {
            wifiCheck = millis();
            if (!WiFi.isConnected())
            {
                WiFi.disconnect();
                WiFi.reconnect();
            }
        }
    }

    String getISOTime(String format = "")
    {
        const char *baseTime = "1970-01-01T00:00:00Z";

        if (WiFi.isConnected())
        {
            if (format.isEmpty())
            {
                return myTZ.dateTime(ISO8601);
            }
            return myTZ.dateTime(format);
        }
        return String(baseTime);
    }
};

// Add this near the end of WiFiConn.h, before the #endif
extern WiFiConn wifiConn;
#endif // WIFI