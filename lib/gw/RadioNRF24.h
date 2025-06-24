#ifndef RADIONRF24_H
#define RADIONRF24_H

#include <SPI.h>
#include <RF24.h>
#include "RadioInterface.h"
#include "stats.h"

class RadioNRF24 : public RadioInterface
{
private:
    RF24 radio;
    uint8_t cePin;
    uint8_t csnPin;
    uint8_t currentChannel;

    // Endereços de comunicação (baseado no código funcional)
    uint8_t address[6][5] = {{0x78, 0x78, 0x78, 0x78, 0x78},
                             {0xF1, 0xB6, 0xB5, 0xB4, 0xB3},
                             {0xCD, 0xB6, 0xB5, 0xB4, 0xB3},
                             {0xA3, 0xB6, 0xB5, 0xB4, 0xB3},
                             {0x0F, 0xB6, 0xB5, 0xB4, 0xB3},
                             {0x05, 0xB6, 0xB5, 0xB4, 0xB3}};

public:
    RadioNRF24(uint8_t cePin = NRF24_CE, uint8_t csnPin = NRF24_CSN)
        : radio(cePin, csnPin), cePin(cePin), csnPin(csnPin) {}

    String getIdent() override
    {
        return "NRF24 (CE:" + String(cePin) + ", CSN:" + String(csnPin) + ")";
    }

    void loop() override
    {
        if (state == RadioStateRX)
        {
            if (receiveMessage())
                delay(100);
            if (!txQueue.isEmpty())
                setState(RadioStateTX);
        }
        else
        {
            if (sendNextMessage())
                delay(100);
            setState(RadioStateRX);
        }
    }
    bool begin(uint8_t nodeId, long channel = 76, bool promisc = true) override
    {
        this->terminalId = nodeId;
        this->currentChannel = channel;

        if (!radio.begin())
        {
#ifdef DEBUG_ON
            Serial.println("NRF24 Hardware não respondendo!");
#endif
            return false;
        }

        // Configuração básica do rádio
        radio.setChannel(currentChannel);
        radio.setPALevel(RF24_PA_LOW);
        radio.setDataRate(RF24_1MBPS);
        radio.setRetries(5, 15); // 750us entre retries, até 15 tentativas
        radio.setPayloadSize(sizeof(MessageRec));
        connected = radio.isChipConnected();
        return true;
    }

    void setMode(bool bReceiver)
    {
        if (bReceiver)
        {
            for (uint8_t i = 0; i < 6; ++i)
                radio.openReadingPipe(i, address[i]);
            radio.startListening(); // put radio in RX mode
        }
        else
        {
            radio.stopListening(address[bReceiver]);          // put radio in TX mode
            radio.setRetries(((bReceiver * 3) % 12) + 3, 15); // maximum value is 15 for both args
        }
    }

    void modeRx() override
    {
        setMode(true);
    }

    void modeTx() override
    {
        setMode(false);
    }

    bool sendMessage(MessageRec &rec) override
    {

        // Converte MessageRec para RF24Payload
        rec.updateCRC();
        stats.txCount++;
        // char msg[MESSAGE_MAX_LEN] = {0};
        // size_t len = rec.encode(msg, MESSAGE_MAX_LEN);
        rec.print();
        // Serial.println(msg + 5);
        // if (len == 0)
        //     return false;
        bool success = radio.write(&rec, sizeof(rec));

        if (success)
        {
            stats.txSuccess++;
            log(true, rec);
        }
        else
        {
            stats.txErrors++;
            if (isConnected())
                Logger::error("Erro ao transmitir, radio inativo %s|%s", rec.event, rec.value);
        }

        return success;
    }

    bool receiveMessage() override
    {
        MessageRec rec;
        uint8_t pipe;
        if (radio.available(&pipe))
        {                                           // is there a payload? get the pipe number that received it
            uint8_t bytes = radio.getPayloadSize(); // get the size of the payload
            // char msg[MESSAGE_MAX_LEN] = {0};
            radio.read(&rec, bytes);

            // if (!rec.decode(msg))
            if (rec.len < 3 || (rec.to == rec.from) || rec.hop > 32)
            {
                if (isConnected())
                    Logger::error("Mensagem mal formada: %d: %s %s", rec.from, rec.event, rec.value);
                return false;
            }
            rec.print();
            // #endif
            if (rec.from == terminalId)
                return false;
            if (rec.to == terminalId || rec.to == 0xFF)
            {
                stats.rxCount++;
                log(false, rec);

                return rxQueue.pushItem(rec);
            }
            if (rec.to != terminalId)
                meshMessage(rec);
            return true;
        }
        return false;
    }

    // Função adicional para mudança de canal dinâmica
    void setChannel(uint8_t channel)
    {
        currentChannel = channel;
        radio.setChannel(channel);
    }

    // Função para diagnóstico
};

#endif // RADIONRF24_H