#if !defined(UDP_INTERFACE_H) && defined(RADIO_UDP)
#define UDP_INTERFACE_H

#include <RadioInterface.h>
#include <WiFiUdp.h>
#include "logger.h"
#include "stats.h"

class RadioUDP : public RadioInterface
{
private:
    WiFiUDP udp;
    uint16_t _localPort;
    IPAddress _broadcastIP;
    bool _isServer;
    bool _initialized;

public:
    RadioUDP(uint16_t localPort = 1234, bool isServer = false) : _localPort(localPort), _isServer(isServer), _initialized(false)
    {
        _broadcastIP = IPAddress(255, 255, 255, 255);
    }

    void broadcastTo(char *message, size_t size, int16_t _localPort)
    {
        udp.beginPacket(_broadcastIP, _localPort);
        udp.write(message,size);
        udp.endPacket();
    }
    bool sendMessage(MessageRec &rec) override
    {
        if (!_initialized)
            return false;

        stats.txCount++;
        char message[MESSAGE_MAX_LEN] = {0};
        size_t result = rec.encode(message, MESSAGE_MAX_LEN);

        if (result > 0)
        {
            if (_isServer)
            {
                // Servidor envia para broadcast
                broadcastTo(message, result, _localPort);
            }
            else
            {
                // Cliente envia para servidor
                broadcastTo(message, result, _localPort);
            }

            stats.txSuccess++;
            log(true, rec);
            return true;
        }
        return false;
    }

    bool receiveMessage() override
    {
        size_t size = udp.parsePacket();
        if (size == 0)
            return false;

        char incomingPacket[255];
        int len = udp.read(incomingPacket, 255);
        if (len > 0)
        {
            incomingPacket[len] = '\0';
            MessageRec rec;
            bool result = rec.decode(incomingPacket, len);

            if (!result && rec.from == terminalId)
            {
#ifdef DEBUG_ON
                Logger::error("Pacote UDP mal formado");
#endif
                return false;
            }

            if (rec.from == terminalId)
            {
                return false; // skip messages from myself
            }

            addRxMessage(rec);
            meshMessage(rec);
        }

        return false;
    }

    void modeRx() override
    {
        // Não há ação específica para modo RX no UDP
    }

    void modeTx() override
    {
        // Não há ação específica para modo TX no UDP
    }

    bool begin(const uint8_t terminal_id, uint16_t port)
    {
        _localPort = port;
        return begin(terminal_id, 0, true);
    }
    bool begin(const uint8_t terminal_Id, long band, bool promisc = true) override
    {
        terminalId = terminal_Id;
        udp.begin(_localPort);

        connected = true;
        _initialized = true;

#ifdef DEBUG_ON
        Logger::info("UDP Interface iniciada na porta %d", _localPort);
#endif
        return true;
    }

    void setBroadcastIP(IPAddress ip)
    {
        _broadcastIP = ip;
    }

    int packetRssi() override
    {
        // UDP não tem RSSI
        return 0;
    }

    int packetSnr() override
    {
        // UDP não tem SNR
        return 0;
    }

    virtual String getIdent() override
    {
        return "UDP";
    }
};

#endif // UDP_INTERFACE_H