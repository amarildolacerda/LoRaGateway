

#include "logger.h"
#include "logger.h"
#ifdef DISPLAY_ENABLED
#include "DisplayManager.h"
#endif

#ifdef WIFI
#include "WiFiConn.h"
#endif

#include "stats.h"

#ifdef ALEXA
#include "alexaCom.h"
#endif

#include "LoRaCom.h"
#include "SystemState.h"

#ifdef GATEWAY
#include "DeviceInfo.h"
#endif

#include "app_messages.h"

#include "WorkProto.h"

WorkingProto proto;

/// LoRa -------------------------------------------------------------------------

#ifdef ALEXA
static void alexaDeviceCallback(uint8_t tid, const char *device_name, bool state, unsigned char value)
{
    for (auto &lora : proto.getRadios())
        // if (deviceInfo.hasTerminal(tid, lora))
        lora->send(tid, EVT_GPIO, state ? GPIO_ON : GPIO_OFF);
}
#endif

class App
{
private:
public:
    void initNet()
    {
#ifdef WIFI
        wifiConn.begin();

#ifdef ALEXA
        wifiConn.setCallback(alexaDeviceCallback);
#endif

#endif
    }
    void initPerif()
    {
#ifdef DISPLAY_ENABLED
        displayManager.initialize();
        displayManager.showMessage("Preparando...");
#endif
    }
    void setup()
    {
        Serial.begin(Config::SERIAL_BAUD); // initialize serial
        while (!Serial)
            ;
        Serial.println("\nIniciando");

        initPerif();
        initNet();

        systemState.terminalId = TERMINAL_ID;
        systemState.terminalName = String(TERMINAL_NAME);
#ifdef GATEWAY
        systemState.isGateway = true;
#else
        systemState.isGateway = (systemState.terminalId == 0);

#endif
        if (RELAY_PIN > 0)
        {
            pinMode(RELAY_PIN, OUTPUT);
#ifdef GATEWAY
            // deviceInfo.updateDevice(nullptr, TERMINAL_ID, TERMINAL_NAME, false, 0);
#endif
        }

        proto.setup();
        int8_t pos = 0;
        String ident = "Radio duplex ";
        for (auto &lora : proto.getRadios())
        {
            lora->begin(systemState.terminalId, Config::LORA_BAND, true); // initialize LoRa at 868 MHz

            if (pos++ == 0)
            {
                systemState.isInitialized = lora->isConnected();
                Serial.println("Radio started");
                ident += lora->getIdent();
                ident += " Term: %d %s";
            }
        }
        Logger::info(ident.c_str(), TERMINAL_ID, TERMINAL_NAME);

        systemState.setDiscovering(true, 30000);
        for (auto &lora : proto.getRadios())
            if (systemState.isGateway)
            {
                lora->sendPresentation(0xFF); // pede apresentação para os terminais.
            }
            else
            {
                lora->sendPresentation(0); // se apresenta ao gateway
            }
    }

    void loop()
    {
        systemState.handle();
        systemState.isRunning = true;
        systemState.previousMillis = millis();
#ifdef WIFI
        wifiConn.loop();
#endif

        proto.loop();
    }

    int addRadio(RadioInterface *rd)
    {
        return proto.addRadio(rd);
    }
};
static App app;