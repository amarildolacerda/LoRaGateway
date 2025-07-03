#ifndef LORA_DUMMY_H
#define LORA_DUMMY_H
#include "RadioInterface.h"
#include "stats.h"
#include "app_messages.h"

class LoRaDummy : public RadioInterface
{

public:
    bool sendMessage(MessageRec &rec)
    {
        stats.txCount++;
        Logger::log(LogLevel::SEND, "Envio: %s", rec.event);

#ifdef DEBUG_ON
        rec.print();
#endif
        return true;
    };
    bool receiveMessage()
    {
        stats.rxCount++;
        // simula um terminal enviando um status
        static long simulaStatus = 0;
        if (millis() - simulaStatus > 5000)
        {
            simulaStatus = millis();

            MessageRec rec;
            rec.from = 200;
            rec.to = 0; // Broadcast
            rec.hop = ALIVE_PACKET;
            rec.id = ++nHeaderId;
            snprintf(rec.event, sizeof(rec.event), EVT_STATUS);
            if (nHeaderId % 2 == 0)
                snprintf(rec.value, sizeof(rec.value), "on");
            else
                snprintf(rec.value, sizeof(rec.value), "off");
            Logger::log(LogLevel::RECEIVE, "Simulando status: %s", rec.value);
            rxQueue.pushItem(rec);
        }
        return !rxQueue.isEmpty();
    };
    void modeRx() {};
    void modeTx() {

    };
    bool begin(const uint8_t terminal_Id, long band, bool promisc = true)
    {
        connected = true;
        return true;
    };
    void loop() override
    {
        RadioInterface::loop();
        static long lastReceive = 0;
        if (millis() - lastReceive > 1000)
        {
            lastReceive = millis();
            MessageRec rec;
            rec.from = terminalId;
            rec.to = 0xFF; // Broadcast
            rec.hop = ALIVE_PACKET;
            rec.id = ++nHeaderId;
            snprintf(rec.event, sizeof(rec.event), EVT_PING);
            snprintf(rec.value, sizeof(rec.value), "dummy");
            txQueue.pushItem(rec);
            // Serial.print(txQueue.size());
            // Serial.println(" ------------------------------TX");
        }
    }
};
#endif