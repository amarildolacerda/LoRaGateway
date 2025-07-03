#ifndef STATS_H
#define STATS_H
#include "logger.h"

class Stats
{
private:
    long resetTime = 0;

public:
    int rxCount = 0;
    int txCount = 0;
    int rxSuccess = 0;
    int txSuccess = 0;
    int txErrors = 0;
    int rxErrors = 0;

    void print()
    {

        Logger::log(LogLevel::INFO, "Rx: %d, ok: %d, Tx: %d, ok: %d", rxCount, rxSuccess, txCount, txSuccess);
        /* Serial.print("RxCount: ");
         Serial.print(rxCount);
         Serial.print(":");
         Serial.print(rxSuccess);
         Serial.print(" Rxs: ");
         Serial.print(ps());
         Serial.print(" TxCount: ");
         Serial.print(txCount);
         Serial.print(":");
         Serial.println(txSuccess);
 */
        if (millis() - resetTime > 60000)
        {
            resetTime = millis();
            txCount = 0;
            txSuccess = 0;
            rxCount = 0;
            rxSuccess = 0;
        }
    }
    long ps()
    {
        return (rxCount / ((((millis() - resetTime)) / 1000) + 1)); // +1 para garantir >0
        // return rxSuccess;
    }
};

extern Stats stats;
#endif