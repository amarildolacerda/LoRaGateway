

#include "logger.h"
#include "logger.h"
#ifdef DISPLAY_ENABLED
#include "DisplayManager.h"
#endif

#ifdef WIFI
#include "WiFiConn.h"
#endif

#include "stats.h"

#ifdef ALEXA
#include "alexaCom.h"
#endif

#include "LoRaCom.h"
#include "SystemState.h"

#ifdef GATEWAY
#include "DeviceInfo.h"
#endif

#include "app_messages.h"

/// LoRa -------------------------------------------------------------------------

#ifdef ALEXA
static void alexaDeviceCallback(uint8_t tid, const char *device_name, bool state, unsigned char value)
{
    loraCom.send(tid, EVT_GPIO, state ? GPIO_ON : GPIO_OFF);
}
#endif

class App
{
private:
public:
    void initNet()
    {
#ifdef WIFI
#ifdef ALEXA
        wifiConn.setCallback(alexaDeviceCallback);
#endif

        wifiConn.begin();

#endif
    }
    void initPerif()
    {
#ifdef DISPLAY_ENABLED
        displayManager.initialize();
        displayManager.showMessage("Preparando...");
#endif
    }
    void setup()
    {
        Serial.begin(Config::SERIAL_BAUD); // initialize serial
        while (!Serial)
            ;
        Serial.println("\nIniciando");

        initPerif();
        initNet();

        systemState.terminalId = TERMINAL_ID;
        systemState.terminalName = String(TERMINAL_NAME);
#ifdef GATEWAY
        systemState.isGateway = true;
#else
        systemState.isGateway = (systemState.terminalId == 0);

#endif
        if (RELAY_PIN > 0)
        {
            pinMode(RELAY_PIN, OUTPUT);
#ifdef GATEWAY
            deviceInfo.updateDevice(TERMINAL_ID, TERMINAL_NAME, false, 0);
#endif
        }

        loraCom.begin(systemState.terminalId, Config::LORA_BAND, true); // initialize LoRa at 868 MHz

        systemState.isInitialized = loraCom.isConnected();
        Serial.println("Radio started");
        String ident = "Radio duplex " + loraCom.getIdent();
        ident += " Term: %d %s";
        Logger::info(ident.c_str(), TERMINAL_ID, TERMINAL_NAME);

        systemState.setDiscovering(true, 30000);
        if (systemState.isGateway)
        {
            loraCom.sendPresentation(0xFF); // pede apresentação para os terminais.
        }
        else
        {
            loraCom.sendPresentation(0); // se apresenta ao gateway
        }
    }

    // Removed unused variable discoveryUpdate
    bool mudouEstado = true;
    // ... existing code ...

    void loop()
    {
        systemState.handle();
        systemState.isRunning = true;
        systemState.previousMillis = millis();
#ifdef WIFI
        wifiConn.loop();
#endif

        // Lora ------------------------------------------------------------
        loraCom.loop();

        static long lastSendTime = 0; // last send time
        int timeUpdate = Config::PING_TIMEOUT_MS;

        // if (!systemState.isGateway)
        timeUpdate = ((systemState.waitingACK || mudouEstado) ? 5000 : 30000);

        if (mudouEstado || millis() - lastSendTime > timeUpdate)
        {
#ifdef DEBUG_ON
            Logger::debug("State changed or time update condition met. Sending ping/status. Time since last send: %ld ms", millis() - lastSendTime);
#endif
            if (systemState.isGateway)
            {
                sendPing();
            }
            if (RELAY_PIN > 0)
            {
                sendStatus();
                systemState.waitingACK = true;
            }

            lastSendTime = millis(); // timestamp the message
            mudouEstado = false;
        }

        MessageRec rec;
        if (loraCom.processIncoming(rec))
        {
#ifdef DEBUG_ON
            Logger::debug("Received a message: from %d, id %d, event %s, value %s", rec.from, rec.id, rec.event, rec.value);
#endif
            handleReceived(rec);
        }

        if (systemState.isGateway)
        {
            if (systemState.isDiscovering)
            {
                static long discUpdate = 0;
                if (millis() - discUpdate > 30000)
                {
#ifdef DEBUG_ON
                    Logger::debug("Discovering devices. Sending presentation.");
#endif
                    loraCom.sendPresentation(0xFF);
                    discUpdate = millis();
                }
            }
        }

        updateDisplay();
        systemState.isRunning = false;

#if defined(ESP32) || defined(ESP8266)
        static long freeUpdated = 0;
        if (millis() - freeUpdated > 30000)
        {
            Logger::info("Memoria Livre: %d", ESP.getFreeHeap());
            freeUpdated = millis();
        }
#endif
    }

    void updateDisplay()
    {
// Display -----------------------------------------------------------
#ifdef DISPLAY_ENABLED
        static long displayUpdate = 0;
        if (millis() - displayUpdate > 1000)
        {
#ifdef GATEWAY
            displayManager.termAtivos = deviceInfo.running();
            displayManager.termTotal = deviceInfo.size();
#endif
#ifdef WIFI
            displayManager.isoDateTime = wifiConn.getISOTime();
#endif
            displayManager.isDiscovering = systemState.isDiscovering;
            displayManager.snr = loraCom.packetSnr();
            displayManager.startedISODateTime = systemState.startedISODateTime;
            displayManager.rssi = loraCom.packetRssi();
            displayManager.ps = stats.ps();
            displayManager.loraConnected = systemState.isInitialized;
            displayManager.handle();
            displayUpdate = millis();
        }
#endif
    }

    //---------------------------
    // Ping
    // Envia um ping para o clientes
    // O clientes deve responder com um pong

    void sendPing()
    {
        loraCom.send(0xFF, EVT_PING, TERMINAL_NAME);
    }
    void sendStatus()
    {
        if (RELAY_PIN > 0)
            loraCom.send(0, EVT_STATUS, digitalRead(RELAY_PIN) ? "on" : "off");
    }
    void ackNak(uint8_t to, bool b, uint8_t seq)
    {
        loraCom.send(to, b ? EVT_ACK : EVT_NAK, TERMINAL_NAME, TERMINAL_ID, seq);
    }
    void executeStatus(const MessageRec rec)
    {
        // gerar historico
        // notificar alexa

        bool status = strcmp(rec.value, "on") == 0;
#ifdef GATEWAY
        deviceInfo.updateState(rec.from, status);
#endif
#ifdef ALEXA
        if (rec.from != 0xFF && rec.from != 0xFE)
        {
            if (alexaCom.indexOf(rec.from) < 0)
                alexaCom.addDevice(rec.from, String(rec.from).c_str());
            alexaCom.updateStateAlexa(rec.from, status);
        }
#endif
    }

    //------------------------------------------------
    void handleReceived(MessageRec &rec)
    {
#ifdef DISPLAY_ENABLED
        displayManager.showEvent(String(rec.from) + " " + String(rec.event) + "  " + String(rec.value));
#endif
        stats.rxSuccess++;
        Logger::log(LogLevel::RECEIVE, "Handled from %d:%d, %s|%s", rec.from, rec.id, rec.event, rec.value);

#ifdef DEBUG_ON
        Logger::debug("Handling event %s from device %d", rec.event, rec.from);
#endif

        if (systemState.isGateway)
        {
            if (strcmp(rec.event, EVT_GPIO) == 0)
            {
                if (strcmp(rec.value, GPIO_ON) == 0)
                {
                    if (RELAY_PIN > 0)
                        digitalWrite(RELAY_PIN, HIGH);
                    Logger::warn("Mudou para ON");
                }
                else if (strcmp(rec.value, GPIO_OFF) == 0)
                {
                    if (RELAY_PIN > 0)
                        digitalWrite(RELAY_PIN, LOW);
                    Logger::warn("Mudou para OFF");
                }
                else if (strcmp(rec.value, GPIO_TOGGLE) == 0)
                {
                    if (RELAY_PIN > 0)
                        digitalWrite(RELAY_PIN, !digitalRead(RELAY_PIN));
                }
                else
                {
                    return;
                }
                ackNak(rec.from, true, rec.id);
                mudouEstado = true; // antecipa a notificação de mudança
                return;
            }
        }

        if (strcmp(rec.event, EVT_PING) == 0)
        {
            loraCom.send(rec.from, EVT_PONG, TERMINAL_NAME);
        }
        else if (strcmp(rec.event, EVT_ACK) == 0)
        {
            systemState.waitingACK = false;
#ifdef GATEWAY
            if (strlen(rec.value) > 0)
            {
                deviceInfo.updateDeviceName(rec.from, rec.value);
            }
#else

#endif
            return;
            // Nada a fazer
        }
        else if (strcmp(rec.event, EVT_NAK) == 0)
        {
            // Nada a fazer
            return;
        }
        else if (strcmp(rec.event, EVT_PONG) == 0)
        {
            ackNak(rec.from, true, rec.id);
#ifdef GATEWAY
            if (strlen(rec.value) > 0)
            {
                deviceInfo.updateDeviceName(rec.from, rec.value);
            }
#endif
            return;
        }
        else if (strcmp(rec.event, EVT_STATUS) == 0)
        {
            if (strcmp(rec.value, "get") == 0)
            {
                mudouEstado = true;
            }
            else
            {
                ackNak(rec.from, true, rec.id); // na estacao avisa que pode ficar transquila
                executeStatus(rec);
            }
            return;
        }
        else if (strcmp(rec.event, EVT_PRESENTATION) == 0)
        {

#ifdef GATEWAY
            ackNak(rec.from, true, rec.id);
            deviceInfo.updateDevice(rec.from, rec.value, false, loraCom.packetRssi());
#ifdef ALEXA
            if (rec.to != 0xFF && rec.to != 0xFE)
                alexaCom.addDevice(rec.from, String(rec.value).c_str());
#endif
#else
            // loraCom.send(rec.from, EVT_PRESENTATION, systemState.terminalName);
            loraCom.sendPresentation(rec.from);
#endif
            return;
        }
        ackNak(rec.from, true, rec.id);
    }

    // ... existing code ...
};
static App app;
