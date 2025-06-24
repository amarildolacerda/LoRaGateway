#ifndef LORA32_H
#define LORA32_H

#include <SPI.h> // include libraries
#include <LoRa.h>
#include <RadioInterface.h>
#include <config.h>
#include <app_messages.h>
#include "logger.h"
#include "stats.h"
const int csPin = 18;    // LoRa radio chip select
const int resetPin = 14; // LoRa radio reset
const int irqPin = 26;   // change for your board; must be a hardware interrupt pin

class LoRa32 : public RadioInterface
{
public:
    bool sendMessage(MessageRec &rec) override
    {
        stats.txCount++;
        char message[MESSAGE_MAX_LEN] = {0};
        size_t result = rec.encode(message, MESSAGE_MAX_LEN);
        if (result > 0)
        {
            LoRa.beginPacket(); // start packet
            for (uint8_t i = 0; i < result; i++)
            {
                LoRa.write(message[i]); // add payload
            }
            if (LoRa.endPacket() > 0)
            {
#ifdef DEBUG_ON
                Logger::hex(LogLevel::VERBOSE, message, result);
#endif
                stats.txSuccess++;
                log(true, rec);
            }; // finish packet and send it
        }
        return result > 0;
    };

    bool receiveMessage() override
    {
        stats.rxCount++;

        int packetSize = LoRa.parsePacket();
        if (packetSize == 0)
            return false; // if there's no packet, return

        MessageRec rec;

        size_t len = 0;
        char message[MESSAGE_MAX_LEN] = {0};
        long updated = millis();
        while (len <= packetSize)
        {
            if (!LoRa.available() && (millis() - updated > 50))
                break;
            if (LoRa.available())
            {
                message[len++] = (char)LoRa.read();
                updated = millis();
            }
        }
        bool result = rec.decode(message, len);
#ifdef DEBUG_ON
        rec.print();
#endif
        if (!result)
            return false;

        if (rec.from == terminalId)
        {
            return false; // skip messages from myself
        }

        addRxMessage(rec);
        meshMessage(rec);

        return true;
    };
    void configParams()
    {
        // LoRa.setSpreadingFactor(LORA_SF); NAO USAR
        // #ifdef LORA_CRC
        LoRa.enableCrc();
        // #endif
    }
    bool begin(const uint8_t terminal_Id, long band, bool promisc = true) override
    {
        terminalId = terminal_Id;
        LoRa.setPins(csPin, resetPin, irqPin); // set CS, reset, IRQ pin

        if (!LoRa.begin(band))
        { // initialize ratio at 915 MHz
            Serial.println("LoRa init failed. Check your connections.");
            while (true)
                ; // if failed, do nothing
        }
        else
            connected = true;
        configParams();

        return isConnected();
    };
    int packetRssi() override
    {
        return LoRa.packetRssi(); // return the packet RSSI
    }
    int packetSnr() override
    {
        return LoRa.packetSnr();
    }
};

#endif
