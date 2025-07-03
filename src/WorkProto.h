
#include "RadioInterface.h"
#include "LoRaCom.h"
#include "DeviceInfo.h"
#include "DeviceInfo.h"
#include "DisplayManager.h"

#ifdef WIFI
#include "WifiConn.h"
#endif

#ifdef RF95
#include "LoRaRF95.h"
LoRaCom radioCom(new LoRaRF95());
#elif defined(LORA32) || defined(TTGO) || defined(HELTEC)
#include "LoRa32.h"
LoRaCom radioCom(new LoRa32());
#elif NRF24
#include "RadioNRF24.h"
LoRaCom radioCom(new RadioNRF24());
#elif RADIO_RF433
#include "RadioRF433.h"
LoRaCom radioCom(new RadioRF433());

#else
#include "LoRaDummy.h"
LoRaCom radioCom(new RadioDummy());

#endif

#if defined(WIFI) && defined(RADIO_UDP)
#include "RadioUDP.h"
LoRaCom radioUDP(new RadioUDP());
#endif

class WorkingProto
{
private:
    std::vector<LoRaCom *> radios;
    bool mudou = false;
    bool mudouEstado = false;

public:
    std::vector<LoRaCom *> getRadios()
    {
        return radios;
    }
    void begin()
    {
        if (systemState.isGateway && RELAY_PIN > 0)
        {
            deviceInfo.updateDevice(nullptr, 0xFE, TERMINAL_NAME, false, 0);
        }
        if (systemState.isGateway)
        {
            for (auto &lora : radios)
                lora->sendPresentation(0xFF);
        }
        else
        {
            for (auto &lora : radios)
                lora->sendPresentation(0);
        }
    }
    void setup()
    {
        radios.push_back(&radioCom);
#ifdef RADIO_UDP
        radios.push_back(&radioUDP);
#endif
    }
    void loop()
    {

#ifdef WS
        MessageRec rec;
        if (txExtraQueue.popItem(rec))
        {
            LoRaCom *lora = deviceInfo.radioOf(rec.to);
            if (lora)
            {
                if (rec.to == 0xFE && systemState.isGateway)
                    handleReceived(lora, rec);
                else
                    lora->send(rec.to, rec.event, rec.value);
            }
        }

#endif

        for (auto &lora : radios)
        {
            lora->loop();
        }

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

        for (auto &lora : radios)
        {
            MessageRec rec;
            if (lora->processIncoming(rec))
            {
#ifdef DEBUG_ON
                Logger::debug("Received a message: from %d, id %d, event %s, value %s", rec.from, rec.id, rec.event, rec.value);
#endif
                handleReceived(lora, rec);
            }
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
                    for (auto &lora : radios)
                        lora->sendPresentation(0xFF);
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
            displayManager.snr = radioCom.packetSnr();
            displayManager.startedISODateTime = systemState.startedISODateTime;
            displayManager.rssi = radioCom.packetRssi();
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
        for (auto &lora : radios)
            lora->send(0xFF, EVT_PING, TERMINAL_NAME);
    }
    void sendStatus()
    {
        if (RELAY_PIN > 0)
            for (auto &lora : radios)
            {
                lora->send(0, EVT_STATUS, digitalRead(RELAY_PIN) ? "on" : "off");
            }
    }
    void ackNak(LoRaCom *lora, uint8_t to, bool b, uint8_t seq)
    {
        lora->send(to, b ? EVT_ACK : EVT_NAK, TERMINAL_NAME, TERMINAL_ID, seq);
    }
    void executeStatus(LoRaCom *lora, const MessageRec rec)
    {
        // gerar historico
        // notificar alexa

        bool status = strcmp(rec.value, "on") == 0;
#ifdef GATEWAY
        deviceInfo.updateState(lora, rec.from, status);
#endif
#ifdef ALEXA
        if (rec.from != 0xFF && rec.from != 0xFE)
        {
            if (alexaCom.indexOf(rec.from) < 0)
                alexaCom.addDevice(rec.from, String(rec.from).c_str());
        }
        alexaCom.updateStateAlexa(rec.from, status);
#endif
    }

    //------------------------------------------------
    void handleReceived(LoRaCom *lora, MessageRec &rec)
    {
#ifdef DISPLAY_ENABLED
        displayManager.showEvent(String(rec.from) + " " + String(rec.event) + "  " + String(rec.value));
#endif
        stats.rxSuccess++;
        Logger::log(LogLevel::RECEIVE, "Handled from %d:%d, %s|%s", rec.from, rec.id, rec.event, rec.value);

#ifdef DEBUG_ON
        Logger::debug("Handling event %s from device %d", rec.event, rec.from);
#endif

        if (RELAY_PIN > 0)
        {
            if (strcmp(rec.event, EVT_GPIO) == 0)
            {
                if (strcmp(rec.value, GPIO_ON) == 0)
                {
                    digitalWrite(RELAY_PIN, HIGH);
                    Logger::warn("Mudou para ON");
                }
                else if (strcmp(rec.value, GPIO_OFF) == 0)
                {
                    digitalWrite(RELAY_PIN, LOW);
                    Logger::warn("Mudou para OFF");
                }
                else if (strcmp(rec.value, GPIO_TOGGLE) == 0)
                {
                    digitalWrite(RELAY_PIN, !digitalRead(RELAY_PIN));
                }
                else
                {
                    return;
                }
                ackNak(lora, rec.from, true, rec.id);
                mudouEstado = true; // antecipa a notificação de mudança
                return;
            }
        }

        if (strcmp(rec.event, EVT_PING) == 0)
        {
            lora->send(rec.from, EVT_PONG, TERMINAL_NAME);
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
            ackNak(lora, rec.from, true, rec.id);
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
                ackNak(lora, rec.from, true, rec.id); // na estacao avisa que pode ficar transquila
                executeStatus(lora, rec);
            }
            return;
        }
        else if (strcmp(rec.event, EVT_PRESENTATION) == 0)
        {

#ifdef GATEWAY
            ackNak(lora, rec.from, true, rec.id);
            deviceInfo.updateDevice(lora, rec.from, rec.value, false, lora->packetRssi());
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
        ackNak(lora, rec.from, true, rec.id);
    }

    int addRadio(RadioInterface *rd)
    {
        int pos = radios.size();
        radios.push_back(new LoRaCom(rd));
        return pos;
    }
};
