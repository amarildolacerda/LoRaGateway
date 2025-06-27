#if !defined(RF433_INTERFACE_H) && defined(RF433)
#define RF433_INTERFACE_H

#include <RadioInterface.h>
#include <VirtualWire.h>
#include "logger.h"
#include "stats.h"

class RadioRF433 : public RadioInterface
{
private:
    uint8_t _txPin;
    uint8_t _rxPin;
    uint8_t _pttPin;
    uint16_t _speed;

public:
    RadioRF433(uint8_t rxPin = RX_PIN, uint8_t txPin = TX_PIN, uint8_t pttPin = -1, uint16_t speed = 2000) : _txPin(txPin), _rxPin(rxPin), _pttPin(pttPin), _speed(speed) {}

    bool sendMessage(MessageRec &rec) override
    {
        stats.txCount++;
        char message[MESSAGE_MAX_LEN] = {0};
        size_t result = rec.encode(message, MESSAGE_MAX_LEN);

        if (result > 0)
        {
            vw_send((uint8_t *)message, result);
            vw_wait_tx(); // Wait until the whole message is gone

            stats.txSuccess++;
            log(true, rec);
            return true;
        }
        return false;
    }

    bool receiveMessage() override
    {
        stats.rxCount++;
        uint8_t buf[MESSAGE_MAX_LEN];
        uint8_t buflen = MESSAGE_MAX_LEN;

        if (!vw_get_message(buf, &buflen))
            return false;

        MessageRec rec;
        bool result = rec.decode((char *)buf, buflen);

        if (!result)
            return false;

        if (rec.from == terminalId)
        {
            return false; // skip messages from myself
        }

        addRxMessage(rec);
        meshMessage(rec);

        return true;
    }

    void modeRx() override
    {
        vw_rx_start();
    }

    void modeTx() override
    {
        vw_rx_stop();
    }

    bool begin(const uint8_t terminal_Id, long band, bool promisc = true) override
    {
        terminalId = terminal_Id;

        // Initialize VirtualWire
        vw_set_tx_pin(_txPin);
        vw_set_rx_pin(_rxPin);
        if (_pttPin != -1)
        {
            vw_set_ptt_pin(_pttPin);
        }
        vw_set_ptt_inverted(true); // Required for some RF modules
        vw_setup(_speed);          // Bits per sec

        vw_rx_start(); // Start the receiver PLL running

        connected = true;
        return isConnected();
    }

    int packetRssi() override
    {
        // RF433 doesn't provide RSSI in VirtualWire
        return 0;
    }

    int packetSnr() override
    {
        // RF433 doesn't provide SNR in VirtualWire
        return 0;
    }

    virtual String getIdent() override
    {
        return "RF433";
    }
};

#endif // RF433_INTERFACE_H