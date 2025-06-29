#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <functional>
#include <vector>
#include <logger.h>
#if defined(ESP8266)
#include <ESP8266WiFi.h>
#define WIFI_SET_SLEEP WiFi.setSleepMode(WIFI_NONE_SLEEP)
#include <ESPAsyncTCP.h>
#include <ESP8266mDNS.h>
#define WIFI_AUTH_OPEN ENC_TYPE_NONE
#elif defined(ESP32)
#include <WiFi.h>
#define WIFI_SET_SLEEP WiFi.setSleep(false)
#include <AsyncTCP.h>
#include <ESPmDNS.h>
#include <nvs_flash.h>
#endif

#include <ESPAsyncWebServer.h>
#include <DNSServer.h>
#include <Preferences.h>

class WiFiManager
{
private:
    AsyncWebServer *server;
    DNSServer *dns;
    bool isInConfigurationMode;
    String ssid;
    String password;
    Preferences preferences;
    unsigned long connectionAttempts;
    bool autoReconnect;
    int maxConnectionAttempts;
    const char *apSSID = "ESP_Config";
    const char *apPassword = "12345678"; // Pode deixar vazio para rede aberta
    std::function<void(bool)> connectCallback;

    // Estrutura para armazenar redes encontradas
    struct WiFiNetwork
    {
        String ssid;
        int32_t rssi;
        bool secure;
    };

    void loadCredentials()
    {
        DEBUG_PRINT("WM* loadCredentials()");
        if (!preferences.begin("wifi-config", true))
        {
            return;
        }
        ssid = preferences.getString("ssid", "");
        password = preferences.getString("password", "");
        preferences.end();
    }

    void saveCredentials()
    {
        DEBUG_PRINT("WM* saveCredentials()");
        if (!preferences.begin("wifi-config", false))
        {
            return;
        }
        preferences.putString("ssid", ssid);
        preferences.putString("password", password);
        preferences.end();
    }

    void startAP()
    {
        WiFi.mode(WIFI_AP);
        if (!WiFi.softAP(apSSID, apPassword))
        {
            Serial.println("Falha ao iniciar modo AP");
            return;
        }

        Serial.println("\nModo AP Iniciado");
        Serial.printf("SSID: %s\n", apSSID);
        Serial.print("IP: ");
        Serial.println(WiFi.softAPIP());

        if (dns)
        {
            dns->setErrorReplyCode(DNSReplyCode::NoError);
            dns->start(53, "*", WiFi.softAPIP());
        }
        DEBUG_PRINT("WM* startAP(%s,%s) ", apSSID, apPassword);

        isInConfigurationMode = true;
    }

    void stopAP()
    {
        if (dns)
        {
            dns->stop();
        }
        WiFi.softAPdisconnect(true);
        isInConfigurationMode = false;
    }

    void tryConnect()
    {
        if (ssid.isEmpty())
        {
            startAP();
            return;
        }

        Serial.printf("\nConectando a: %s\n", ssid.c_str());
        WiFi.disconnect(true);
        delay(100);
        WiFi.mode(WIFI_STA);
        WiFi.setAutoReconnect(true);
#if defined(ESP8266)
        WiFi.setSleepMode(WIFI_NONE_SLEEP);
#elif defined(ESP32)
        WiFi.setSleep(false);
#endif

        WiFi.begin(ssid.c_str(), password.c_str());

        unsigned long startTime = millis();
        while (millis() - startTime < 20000 && WiFi.status() != WL_CONNECTED)
        {
            delay(500);
            Serial.print(".");
        }

        if (WiFi.status() == WL_CONNECTED)
        {
            Serial.println("\nConectado!");
            if (connectCallback)
                connectCallback(true);
            if (isInConfigurationMode)
                stopAP();
        }
        else
        {
            Serial.println("\nFalha na conexão");
            if (connectCallback)
                connectCallback(false);
            startAP();
        }
    }

    void setupWebServer()
    {
        // Configuração das rotas do servidor web
        server->on("/", HTTP_GET, [](AsyncWebServerRequest *request)
                   { request->send(200, "text/html",
                                   "<html><body>"
                                   "<h1>Configuração WiFi</h1>"
                                   "<p><a href='/wifi'>Configurar WiFi</a></p>"
                                   "</body></html>"); });

        server->on("/wifi", HTTP_GET, [this](AsyncWebServerRequest *request)
                   {
            String html = "<html><body><h1>Configurar WiFi</h1>"
                         "<form method='post' action='/save'>"
                         "SSID: <input type='text' name='ssid'><br>"
                         "Senha: <input type='password' name='password'><br>"
                         "<input type='submit' value='Salvar'>"
                         "</form></body></html>";
            request->send(200, "text/html", html); });

        server->on("/save", HTTP_POST, [this](AsyncWebServerRequest *request)
                   {
            if (request->hasParam("ssid", true) && request->hasParam("password", true)) {
                ssid = request->getParam("ssid", true)->value();
                password = request->getParam("password", true)->value();
                saveCredentials();
                request->send(200, "text/html", 
                    "<html><body>"
                    "<h1>Configuração salva!</h1>"
                    "<p>Reiniciando para conectar...</p>"
                    "</body></html>");
                delay(1000);
                ESP.restart();
            } else {
                request->send(400, "text/plain", "Dados inválidos");
            } });
    }

public:
    WiFiManager(AsyncWebServer *existingServer, DNSServer *dnsServer) : server(existingServer), dns(dnsServer), isInConfigurationMode(false),
                                                                        autoReconnect(true), maxConnectionAttempts(5), connectionAttempts(0)
    {
    }

    ~WiFiManager()
    {
    }

    void resetSettings()
    {
        DEBUG_PRINT("WM* resetSettings()");
    }

    void autoConnect(const char *defaultSSID = "", const char *defaultPassword = "")
    {
#if defined(ESP32)
        nvs_flash_init();
#endif
        loadCredentials();

        if (strlen(defaultSSID) > 0 && ssid.isEmpty())
        {
            ssid = defaultSSID;
            password = defaultPassword;
        }

        tryConnect();
        setupWebServer();
        server->begin();
    }

    void setConnectCallback(std::function<void(bool)> callback)
    {
        connectCallback = callback;
    }

    void loop()
    {
        if (dns && isInConfigurationMode)
        {
            dns->processNextRequest();
        }
    }

    bool isConnected() const
    {
        return WiFi.status() == WL_CONNECTED;
    }

    String getSSID() const
    {
        return ssid;
    }

    IPAddress getLocalIP() const
    {
        return WiFi.localIP();
    }
};

#endif // WIFI_MANAGER_H