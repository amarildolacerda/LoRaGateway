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

// Debug print macro
#ifndef DEBUG_PRINT
#ifdef DEBUG_ON
#define DEBUG_PRINT(...) Serial.printf(__VA_ARGS__)
#else
#define DEBUG_PRINT(...)
#endif
#endif

class WiFiManager
{
private:
    // Forward declarations of the static HTML templates
    static const char MAIN_PAGE[] PROGMEM;
    static const char WIFI_CONFIG_PAGE[] PROGMEM;
    static const char SAVE_SUCCESS_PAGE[] PROGMEM;

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
    const char *apPassword = "12345678";
    std::function<void(bool)> connectCallback;
    unsigned long lastAttemptTime;
    bool attemptingToConnect;

    struct WiFiNetwork
    {
        String ssid;
        int32_t rssi;
        bool secure;
    };

    void loadCredentials()
    {
        DEBUG_PRINT("WM* loadCredentials()\n");
        if (!preferences.begin("wifi-config", true))
        {
            DEBUG_PRINT("WM* Failed to begin preferences\n");
            return;
        }
        ssid = preferences.getString("ssid", "");
        password = preferences.getString("password", "");
        preferences.end();
        DEBUG_PRINT("WM* Loaded SSID: %s, Password: %s\n", ssid.c_str(), password.c_str());
    }

    void saveCredentials()
    {
        DEBUG_PRINT("WM* saveCredentials() - SSID: %s, Password: %s\n", ssid.c_str(), password.c_str());
        if (!preferences.begin("wifi-config", false))
        {
            DEBUG_PRINT("WM* Failed to begin preferences for writing\n");
            return;
        }
        preferences.putString("ssid", ssid);
        preferences.putString("password", password);
        preferences.end();
        DEBUG_PRINT("WM* Credentials saved successfully\n");
    }

    void startAP()
    {
        DEBUG_PRINT("WM* Starting AP mode\n");
        WiFi.mode(WIFI_AP_STA); // Mantém ambos os modos ativos
        if (!WiFi.softAP(apSSID, apPassword))
        {
            DEBUG_PRINT("WM* Failed to start AP mode\n");
            return;
        }

        DEBUG_PRINT("\nWM* AP Mode Started\n");
        DEBUG_PRINT("WM* SSID: %s\n", apSSID);
        DEBUG_PRINT("WM* IP: %s\n", WiFi.softAPIP().toString().c_str());

        if (dns)
        {
            dns->setErrorReplyCode(DNSReplyCode::NoError);
            dns->start(53, "*", WiFi.softAPIP());
            DEBUG_PRINT("WM* DNS server started\n");
        }

        isInConfigurationMode = true;
        setupWebServer();
    }

    void stopAP()
    {
        DEBUG_PRINT("WM* Stopping AP mode\n");
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
            DEBUG_PRINT("WM* No SSID configured, starting AP mode\n");
            startAP();
            return;
        }

        DEBUG_PRINT("\nWM* Connecting to: %s\n", ssid.c_str());
        WiFi.disconnect(true);
        delay(100);
        WiFi.mode(WIFI_STA);
        WiFi.setAutoReconnect(true);
        WIFI_SET_SLEEP;

        WiFi.begin(ssid.c_str(), password.c_str());
        lastAttemptTime = millis();
        attemptingToConnect = true;
    }

    void checkConnection()
    {
        if (!attemptingToConnect)
            return;

        if (WiFi.status() == WL_CONNECTED)
        {
            DEBUG_PRINT("\nWM* Connected successfully!\n");
            DEBUG_PRINT("WM* IP Address: %s\n", WiFi.localIP().toString().c_str());
            if (connectCallback)
                connectCallback(true);
            if (isInConfigurationMode)
                stopAP();
            attemptingToConnect = false;
        }
        else if (millis() - lastAttemptTime > 20000) // Timeout após 20 segundos
        {
            DEBUG_PRINT("\nWM* Connection failed\n");
            if (connectCallback)
                connectCallback(false);
            startAP();
            attemptingToConnect = false;
        }
    }

    String getStatusString()
    {
        if (isInConfigurationMode)
        {
            return "Modo de configuração ativo";
        }
        else if (WiFi.status() == WL_CONNECTED)
        {
            return String("Conectado à rede: ") + WiFi.SSID();
        }
        else
        {
            return "Desconectado";
        }
    }

    void setupWebServer()
    {
        DEBUG_PRINT("WM* Setting up web server\n");

        // Configuração das rotas do servidor web
        server->on("/", HTTP_GET, [this](AsyncWebServerRequest *request)
                   { 
            DEBUG_PRINT("WM* Handling / request\n");
            String html = FPSTR(MAIN_PAGE);
            html.replace("%STATUS%", getStatusString());
            request->send(200, "text/html", html); });

        server->on("/wifi", HTTP_GET, [this](AsyncWebServerRequest *request)
                   {
            DEBUG_PRINT("WM* Handling /wifi request\n");
            request->send_P(200, "text/html", WIFI_CONFIG_PAGE); });

        server->on("/save", HTTP_POST, [this](AsyncWebServerRequest *request)
                   {
            DEBUG_PRINT("WM* Handling /save request\n");
            if (request->hasParam("ssid", true) && request->hasParam("password", true)) {
                ssid = request->getParam("ssid", true)->value();
                password = request->getParam("password", true)->value();
                DEBUG_PRINT("WM* New credentials received - SSID: %s, Password: %s\n", ssid.c_str(), password.c_str());
                saveCredentials();
                request->send_P(200, "text/html", SAVE_SUCCESS_PAGE);
                delay(1000);
                ESP.restart();
            } else {
                DEBUG_PRINT("WM* Invalid data received in /save request\n");
                request->send(400, "text/plain", "Dados inválidos");
            } });
    }

public:
    WiFiManager(AsyncWebServer *existingServer, DNSServer *dnsServer) : server(existingServer),
                                                                        dns(dnsServer),
                                                                        isInConfigurationMode(false),
                                                                        autoReconnect(true),
                                                                        maxConnectionAttempts(5),
                                                                        connectionAttempts(0),
                                                                        attemptingToConnect(false)
    {
        DEBUG_PRINT("WM* WiFiManager constructor\n");
    }

    ~WiFiManager()
    {
        DEBUG_PRINT("WM* WiFiManager destructor\n");
    }

    void resetSettings()
    {
        DEBUG_PRINT("WM* Resetting WiFi settings\n");
        preferences.begin("wifi-config", false);
        preferences.clear();
        preferences.end();
        ssid = "";
        password = "";
    }

    void autoConnect(const char *defaultSSID = "", const char *defaultPassword = "")
    {
        DEBUG_PRINT("WM* autoConnect called\n");
#if defined(ESP32)
        nvs_flash_init();
#endif
        loadCredentials();

        if (strlen(defaultSSID) > 0 && ssid.isEmpty())
        {
            DEBUG_PRINT("WM* Using default SSID: %s\n", defaultSSID);
            ssid = defaultSSID;
            password = defaultPassword;
        }

        tryConnect();
        server->begin();
        DEBUG_PRINT("WM* Web server started\n");
    }

    void setConnectCallback(std::function<void(bool)> callback)
    {
        DEBUG_PRINT("WM* Connect callback set\n");
        connectCallback = callback;
    }

    void loop()
    {
        if (attemptingToConnect)
        {
            checkConnection();
        }

        if (dns && isInConfigurationMode)
        {
            dns->processNextRequest();
        }
    }

    bool isConnected() const
    {
        bool connected = WiFi.status() == WL_CONNECTED;
        DEBUG_PRINT("WM* isConnected: %d\n", connected);
        return connected;
    }

    String getSSID() const
    {
        DEBUG_PRINT("WM* getSSID: %s\n", ssid.c_str());
        return ssid;
    }

    IPAddress getLocalIP() const
    {
        IPAddress ip = WiFi.localIP();
        DEBUG_PRINT("WM* getLocalIP: %s\n", ip.toString().c_str());
        return ip;
    }
};

#endif // WIFI_MANAGER_H