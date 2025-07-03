
#pragma once
#include "logger.h"
class SystemState
{
private:
    long discoveryUpdate = 0;

public:
    uint8_t terminalId = TERMINAL_ID;    // ID do terminal, 0 para gateway
    String terminalName = TERMINAL_NAME; // Nome do terminal, usado para identificação
    bool isGateway = true;               // tipos de estacao
    long previousMillis = 0;             // used to store the last time the system was updated
    String startedISODateTime = "";      // used to store the system start date
    bool isRunning = false;              // inLoop
    bool isConnected = false;            // wifi ok
    bool isInitialized = false;          // lora ok.
    bool isRelayOn = false;              // on/off
    bool isDiscovering = false;          // used to indicate if the system is in discovery mode
    long discoveryDuration = 30000;
    bool waitingACK = false;

    bool setDiscovering(bool value, const long ms = 60000)
    {
        discoveryDuration = ms;
        isDiscovering = value;
        discoveryUpdate = millis();
        return isDiscovering;
    }
    void handle()
    {
        if (isDiscovering && millis() - discoveryUpdate > discoveryDuration)
        {
            isDiscovering = false; // stop discovery after timeout
            Logger::log(LogLevel::INFO, "Discovery timeout reached, stopping discovery.");
        }
    }
};

extern SystemState systemState;