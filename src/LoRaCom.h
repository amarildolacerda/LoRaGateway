#ifndef LORACOM_H
#define LORACOM_H

#include "config.h"
#include "SystemState.h"
#include "app_messages.h"
#ifdef WS
#include "ExtraQueue.h"
#endif

#include "RadioInterface.h"

class LoRaCom
{
private:
#ifdef BROADCAST
    BroadcastHandler *broadcast = nullptr;
#endif
public:
    RadioInterface *radio = nullptr;

    LoRaCom(RadioInterface *rd) : radio(rd)
    {
    }

    ~LoRaCom()
    {
    }
    String getIdent()
    {
        return radio->getIdent();
    }
    void send(const uint8_t tid, const String &event, const String &value, const uint8_t from = TERMINAL_ID, uint8_t seq = 0)
    {
        send(radio, tid, event, value, from, seq);
    }
    void send(RadioInterface *rd, const uint8_t tid, const String &event, const String &value, const uint8_t from = TERMINAL_ID, uint8_t seq = 0)
    {

        rd->send(tid, event.c_str(), value.c_str(), (TERMINAL_ID == tid) ? 0xFE : from, seq);
    }
    void receive(const uint8_t tid, const String &event, const String &value)
    {
        receive(radio, tid, event, value);
    }
    void receive(RadioInterface *rd, const uint8_t tid, const String &event, const String &value)
    {
        MessageRec rec;
        rec.clear();
        rec.to = tid;
        rec.from = TERMINAL_ID;
        rec.setEvent(event.c_str());
        rec.setValue(value.c_str());
        // snprintf(rec.event, sizeof(rec.event), "%s", event.c_str());
        // snprintf(rec.value, sizeof(rec.value), "%s", value.c_str());
        rec.hop = ALIVE_PACKET;
        rd->receive(rec);
    }
    void loop()
    {
        radio->loop();
    }
    void sendPresentation(const uint8_t tid)
    {
        send(tid, EVT_PRESENTATION, systemState.terminalName);
    }
    bool begin(const uint8_t tid, const long band, const bool promiscuos = true)
    {
#ifdef BROADCAST
        broadcast = new BroadcastHandler(tid == 0);
        broadcast->setup();
        broadcast->setCallback(broadcastCallbackFn);
#endif
        return radio->begin(tid, band, promiscuos);
    }
    int packetRssi()
    {
        return radio->packetRssi();
    }

    int packetSnr()
    {
        return radio->packetSnr();
    }
    bool processIncoming(MessageRec &rec)
    {
        return radio->processIncoming(rec);
    }
    bool isConnected()
    {
        return radio->isConnected();
    }
};

// extern LoRaCom loraCom;
#endif