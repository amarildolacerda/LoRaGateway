// implementa transmissor ou reception com base em -DRECEPTION ou -DTRANSMISSOR
#ifdef RESET
#include "ESPAsyncWiFiManager.h"
static AsyncWebServer server(80);
static DNSServer dns;
AsyncWiFiManager wf(&server, &dns);

void setup()
{
    Serial.begin(115200);
    wf.resetSettings();
    Serial.println("mudar direteirva RESET para recompilas");
}
void loop()
{
}
#elif TEST
#include "Arduino.h"
#include "ESPAsyncWiFiManager.h"
#include "ESPAsyncWebServer.h"
#include "DNSServer.h"

static AsyncWebServer server(80);
static DNSServer dns;
AsyncWiFiManager wf(&server, &dns);
void setup()
{
    Serial.begin(115200);
    delay(1000); // Espera inicialização estável

    wf.resetSettings();
    Serial.println("\n--- Teste de Memória ---");
    Serial.print("Free Heap: ");
    Serial.println(ESP.getFreeHeap()); // Deve ser > 0

    if (ESP.getFreeHeap() == 0)
    {
        Serial.println("ERRO: Heap zerado! Reiniciando...");
        ESP.restart();
    }
}

void loop()
{
    Serial.print("Free Heap (loop): ");
    Serial.println(ESP.getFreeHeap());
    delay(2000);
}

#elif TEST_RF95
#include <SoftwareSerial.h>
#include <RH_RF95.h>
#include "config.h"
static SoftwareSerial SSerial(Config::LORA_RX_PIN, Config::LORA_TX_PIN);
// #define LoRaSerial SSerial

static RH_RF95<SoftwareSerial> rf(SSerial);

void setup()
{
    Serial.begin(115200);
    Serial.println(F("Testando transmissor"));
    rf.init();
    rf.setPromiscuous(true);
    rf.setFrequency(868.0);
    rf.setHeaderTo(0x00);
    rf.setHeaderFrom(100);
    rf.setTxPower(14);
}

void loop()
{

    if (rf.available())
    {

        Serial.println("Recebendo mensagem");
        char buf[RH_RF95_MAX_MESSAGE_LEN];
        uint8_t len = sizeof(buf);
        if (rf.recv((uint8_t *)buf, &len))
        {
            char info[128];
            snprintf(info, sizeof(info), "Received from: %d to %d id %d", rf.headerFrom(), rf.headerTo(), rf.headerId());
            Serial.println(info);
            Serial.println((char *)buf);
        }
    }
    else
    {
        Serial.println("Enviando mensagem");
        uint8_t msg[] = "Hello World!\0";
        int len = sizeof(msg);
        rf.send(msg, len);
        if (!rf.waitPacketSent())
        {
            Serial.println("Falha ao enviar mensagem");
        }
        else
        {
            Serial.println("Mensagem enviada com sucesso");
        }
        delay(3000);
    }
}

#else
#include "transmissor.h"

#ifdef ALEXA
#include "Espalexa.h"
#include "AlexaCom.h"
#include "ESP8266WebServer.h

ESP8266WebServer server(80);
Espalexa alexa;
#endif
void setup()
{
    alexaCom.setServer(server);

#ifdef ALEXA
    alexaCom.setAlexaCmp(&alexa);
#endif
    app.setup();
}
void loop()
{
    app.loop();
#ifdef ALEXA
    alexa.loop();
#endif
}

#endif