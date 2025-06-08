#ifndef LORA_INTERFACE_H
#define LORA_INTERFACE_H

#include <Arduino.h>
#include <queue_message.h>

//[stable]
enum LoRaStates
{
    LoRaRX,
    LoRaTX
};

enum LoRaConfig
{
    LORA_SLOW,
    LORA_FAST,
    LORA_MED
};

// Constantes para controle de mesh
#define ALIVE_PACKET 3
#define MESSAGE_TIMEOUT_MS 100

class LoRaInterface
{
protected:
    uint8_t terminalId = 0;
    char terminalName[10] = {0};
    FifoList txQueue;
    FifoList rxQueue;
    unsigned long lastStateChange = millis();

    uint8_t nHeaderId = 0;
    uint8_t headerSender = 0;
    bool _promiscuos = false;

    const char bEOF = '}';
    const char bBOF = '{';
    const char bSEP = '|';

private:
    LoRaStates state = LoRaRX;
    void setState(LoRaStates st)
    {
        state = st;
        switch (state)
        {
        case LoRaRX:
            modeRx();
            /* code */
            break;
        case LoRaTX:
            modeTx();
            break;
        default:

            break;
        }
    }

public:
    LoRaConfig config = LORA_MED;
    bool isValidMessage(const char *msg, uint8_t len)
    {
        return (len >= 4) && (msg[0] == '{') && (msg[len - 1] == '}');
    }
    bool connected = false;
    virtual void setTerminalName(const char name[10])
    {
        snprintf(terminalName, sizeof(terminalName), name);
    };
    virtual void setTerminalId(uint8_t tid)
    {
        terminalId = tid;
    };
    virtual void setPins(const uint8_t cs, const uint8_t reset, const uint8_t irq) {};
    virtual void endSetup()
    {
    }
    virtual int packetRssi()
    {
        return 0;
    };
    virtual int packetSnr()
    {
        return 0;
    }
    virtual bool available()
    {
        return !rxQueue.isEmpty();
    }

    virtual ~LoRaInterface() {}
    virtual bool send(uint8_t tid, const char *event, const char *value, const uint8_t terminalId)
    {
        MessageRec rec;
        memset(&rec, 0, sizeof(MessageRec));
        rec.to = tid;
        rec.from = terminalId;
        snprintf(rec.event, sizeof(rec.event), event);
        snprintf(rec.value, sizeof(rec.value), value);
        rec.hope = ALIVE_PACKET;
        rec.id = ++nHeaderId;
        return txQueue.pushItem(rec);
    }
    virtual bool processIncoming(MessageRec &rec)
    {
        return rxQueue.popItem(rec);
    }
    bool sendNextMessage()
    {
        MessageRec rec;
        if (!txQueue.popItem(rec))
        {
            // Serial.println("No message to send");
            return false;
        }
        return sendMessage(rec);
    }

    // Modificar o loop para tratamento específico do Heltec
    virtual void loop()
    {
        switch (state)
        {
        case LoRaRX:
            if (receiveMessage())
                lastStateChange = millis();

            // Reduzido de 100ms para 50ms para melhorar throughput
            if (txQueue.size() > 0 && millis() - lastStateChange > MESSAGE_TIMEOUT_MS)
            {
                setState(LoRaTX);
                lastStateChange = millis();
            }
            break;

        case LoRaTX:
            if (sendNextMessage())
            {
                lastStateChange = millis();
            }

            // Reduzido de 100ms para 50ms
            if (millis() - lastStateChange > MESSAGE_TIMEOUT_MS)
            {
                setState(LoRaRX);
                lastStateChange = millis();
            }

            break;
        }
    }
    virtual bool sendMessage(MessageRec &rec) = 0;
    virtual bool receiveMessage() = 0;
    virtual void modeRx() = 0;
    virtual void modeTx() = 0;
    virtual bool begin(const uint8_t terminal_Id, long band, bool promisc = true) = 0;
};

#endif // LORA_INTERFACE_H
