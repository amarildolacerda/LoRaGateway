#ifndef LORATTGO_H
#define LORATTGO_H

#ifdef TTGO

#include "LoRaInterface.h"
#include "config.h"
#include <LoRa.h>
#include "queue_message.h"
#include "logger.h"
#include "stats.h"

class LoRaAutoTuner
{
private:
    uint8_t currentSF;
    long currentBW;
    uint8_t currentCR;
    uint8_t currentPower;
    int targetRSSI = -75;
    float targetSNR = 5.0;

    float rssiSamples[5] = {0};
    float snrSamples[5] = {0};
    uint8_t sampleIndex = 0;

    uint16_t lostPackets = 0;

public:
    uint16_t totalPackets = 0;
    LoRaAutoTuner(uint8_t sf, long bw, uint8_t cr, uint8_t power)
        : currentSF(sf), currentBW(bw), currentCR(cr), currentPower(power) {}

    void updateMetrics(float rssi, float snr, bool packetLost)
    {
        rssiSamples[sampleIndex] = rssi;
        snrSamples[sampleIndex] = snr;
        sampleIndex = (sampleIndex + 1) % 5;

        totalPackets++;
        if (packetLost)
            lostPackets++;
    }

    float getPacketLossRate()
    {
        if (totalPackets == 0)
            return 0.0;
        return (lostPackets * 100.0) / totalPackets;
    }

    float getAverageRSSI()
    {
        float sum = 0;
        for (int i = 0; i < 5; i++)
            sum += rssiSamples[i];
        return sum / 5;
    }

    float getAverageSNR()
    {
        float sum = 0;
        for (int i = 0; i < 5; i++)
            sum += snrSamples[i];
        return sum / 5;
    }

    void adjustParameters()
    {
        float avgRSSI = getAverageRSSI();
        float avgSNR = getAverageSNR();
        float lossRate = getPacketLossRate();

        if (avgRSSI > -60 && currentPower > 10)
        {
            currentPower -= 2;
            LoRa.setTxPower(currentPower);
            Logger::log(LogLevel::INFO, "Reduzindo potência para %d dBm", currentPower);
            return;
        }

        if (avgRSSI < -90 || avgSNR < 0)
        {
            if (currentSF < 12 && lossRate > 10)
            {
                currentSF++;
                LoRa.setSpreadingFactor(currentSF);
                Logger::log(LogLevel::INFO, "Aumentando SF para %d", currentSF);
            }
            else if (currentPower < 20)
            {
                currentPower += 2;
                LoRa.setTxPower(currentPower);
                Logger::log(LogLevel::INFO, "Aumentando potência para %d dBm", currentPower);
            }
            return;
        }

        if (lossRate > 5 && currentBW < 250E3)
        {
            currentBW = (currentBW == 125E3) ? 250E3 : 500E3;
            LoRa.setSignalBandwidth(currentBW);
            Logger::log(LogLevel::INFO, "Aumentando BW para %ld Hz", currentBW);
            return;
        }

        if (avgRSSI > -80 && avgSNR > 7 && lossRate < 2)
        {
            if (currentSF > 7)
            {
                currentSF--;
                LoRa.setSpreadingFactor(currentSF);
                Logger::log(LogLevel::INFO, "Reduzindo SF para %d para melhorar throughput", currentSF);
            }
            else if (currentBW == 125E3 && currentSF <= 9)
            {
                currentBW = 250E3;
                LoRa.setSignalBandwidth(currentBW);
                Logger::log(LogLevel::INFO, "Aumentando BW para %ld Hz para melhorar throughput", currentBW);
            }
        }
    }

    void resetCounters()
    {
        totalPackets = 0;
        lostPackets = 0;
    }

    void printStatus()
    {
        Logger::log(LogLevel::INFO, "Config: SF%d BW%ld CR%d PWR%d",
                    currentSF, currentBW, currentCR, currentPower);
        Logger::log(LogLevel::INFO, "Qualidade: RSSI %.1f SNR %.1f Perda %.1f%%",
                    getAverageRSSI(), getAverageSNR(), getPacketLossRate());
    }

    uint8_t getCurrentSF() const { return currentSF; }
    long getCurrentBW() const { return currentBW; }
    uint8_t getCurrentCR() const { return currentCR; }
    uint8_t getCurrentPower() const { return currentPower; }
};

class LoRaTTGO : public LoRaInterface
{
private:
    long lastAvailable = 0;
    uint8_t headerTo = 0;
    uint8_t headerFrom = 0;
    uint8_t headerId = 0;
    uint8_t headerHope = 0;
    LoRaAutoTuner autoTuner;
    bool lastPacketLost = false;

public:
    LoRaTTGO() : autoTuner(LORA_SF, LORA_BW, LORA_CR, LORA_PW) {} // Valores iniciais

    void modeTx() override
    {
        // LoRa.idle();
    }

    void modeRx() override
    {
        LoRa.receive();
    }

    bool begin(const uint8_t terminal_Id, long frequency, bool promiscuous = true) override
    {
        terminalId = terminal_Id;
        if (!LoRa.begin(frequency))
        {
            Logger::error("Failed to initialize LoRa");
            return false;
        }

        // Configuração inicial baseada no autoTuner
        LoRa.setSpreadingFactor(autoTuner.getCurrentSF());
        LoRa.setSignalBandwidth(autoTuner.getCurrentBW());
        LoRa.setCodingRate4(autoTuner.getCurrentCR());
        LoRa.setTxPower(autoTuner.getCurrentPower());

        LoRa.setSyncWord(Config::LORA_SYNC_WORD);
        LoRa.setPreambleLength(8);
        LoRa.enableCrc();
        _promiscuos = promiscuous;

        Logger::log(LogLevel::INFO, "LoRa inicializado com SF%d BW%ld CR%d PWR%d",
                    autoTuner.getCurrentSF(), autoTuner.getCurrentBW(),
                    autoTuner.getCurrentCR(), autoTuner.getCurrentPower());

        return true;
    }

    bool sendMessage(MessageRec &rec) override
    {
        stats.txCount++;
        char message[Config::MESSAGE_MAX_LEN] = {0};
        snprintf(message, sizeof(message), "{%s|%s}", rec.event, rec.value);
        uint8_t len = strlen(message);

        // LoRa.idle();
        LoRa.beginPacket();

        LoRa.write(rec.to);
        LoRa.write(rec.from);
        LoRa.write(rec.id);
        LoRa.write(rec.hope);
        LoRa.write(terminalId);

        int snd = LoRa.print(message);
        bool rt = LoRa.endPacket(true) > 0;
        if (rt)
        {
            stats.txSuccess++;
        }
        Logger::log(LogLevel::SEND, "(%d)[%d→%d:%d] L: %d B: %s",
                    terminalId, rec.from, rec.to, rec.id, len, message);

        // Atualiza métricas (assume que pacote foi enviado com sucesso)
        autoTuner.updateMetrics(LoRa.packetRssi(), LoRa.packetSnr(), false);
        checkAutoTune();

        return rt;
    }

    bool receiveMessage() override
    {

        stats.rxCount++;
        int packetSize = LoRa.parsePacket();
        if (packetSize < 5)
        {
            lastPacketLost = true;
            return false;
        }

        headerTo = LoRa.read();
        headerFrom = LoRa.read();
        headerId = LoRa.read();
        headerHope = LoRa.read();
        headerSender = LoRa.read();

        char buffer[Config::MESSAGE_MAX_LEN] = {0};
        uint8_t len = 0;
        long rcvDelay = millis();
        bool pipe = false;

        while (len < packetSize - 5)
        {
            if (!LoRa.available() && millis() - rcvDelay > 100)
            {
                lastPacketLost = true;
                break;
            }
            rcvDelay = millis();
            uint8_t r = LoRa.read();
            buffer[len++] = (char)r;
            if (r == '|')
                pipe = true;
            if (pipe && r == '}')
                break;
        }
        buffer[len] = '\0';

        if ((headerFrom == terminalId || headerSender == terminalId))
        {
            lastPacketLost = true;
            return false;
        }

        MessageRec rec;
        bool parsed = parseRecv(buffer, len, rec);
        stats.rxSuccess++;
        Logger::log(parsed ? LogLevel::RECEIVE : LogLevel::ERROR, "(%d)[%d→%d:%d](%d) L: %d B: %s",
                    headerSender, headerFrom, headerTo, headerId, headerHope,
                    len, buffer);

        if (!parsed)
        {
            lastPacketLost = true;
            return false;
        }

        // Atualiza métricas
        autoTuner.updateMetrics(LoRa.packetRssi(), LoRa.packetSnr(), lastPacketLost);
        checkAutoTune();
        lastPacketLost = false;

        bool result = (headerTo == 0xFF || headerTo == terminalId);
        if (result)
        {
            return rxQueue.pushItem(rec);
        }

        return result;
    }

    void checkAutoTune()
    {

        static uint32_t lastCheck = 0;
        if (millis() - lastCheck > 30000 || autoTuner.totalPackets >= 50)
        {
            if (LORA_AUTO)
                autoTuner.adjustParameters();
            autoTuner.printStatus();
            autoTuner.resetCounters();
            lastCheck = millis();
        }
    }

    bool parseRecv(char *buf, uint8_t len, MessageRec &rec)
    {
        memset(&rec, 0, sizeof(MessageRec));
        rec.to = headerTo;
        rec.from = headerFrom;
        rec.id = headerId;
        rec.hope = headerHope;

        if (buf == NULL || buf[0] != '{' || buf[len - 1] != '}')
            return false;

        char content[100];
        strncpy(content, buf + 1, len - 2);
        content[len - 2] = '\0';

        char *token = strtok(content, "|");
        if (token != nullptr)
        {
            int xe = snprintf(rec.event, sizeof(rec.event), token);
            rec.event[xe] = '\0';
        }

        token = strtok(nullptr, "|");
        if (token != nullptr)
        {
            int xv = snprintf(rec.value, sizeof(rec.value), token);
            rec.value[xv] = '\0';
        }
        else
        {
            sprintf(rec.value, "");
        }
        return true;
    }

    void setPins(const uint8_t cs, const uint8_t reset, const uint8_t irq) override
    {
        Logger::info("Configuração do LoRa com CS: %d   RESET: %d  IRQ: %d", cs, reset, irq);
        LoRa.setPins(cs, reset, irq);
    }

    int packetRssi() override
    {
        return LoRa.packetRssi();
    }

    int packetSnr() override
    {
        return LoRa.packetSnr();
    }
};

#endif // LORATTGO_H
#endif