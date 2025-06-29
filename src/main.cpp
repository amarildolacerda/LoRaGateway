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

#define RESET_NVS // Comente esta linha para desativar o reset do NVS

#ifdef ESP32
#include <nvs_flash.h>
#endif

void setup()
{
  Serial.begin(115200);
  delay(1000); // Espera inicialização estável

  Serial.println("\n--- Inicialização do Sistema ---");

  // Verificação crítica de memória
  Serial.print("Free Heap Inicial: ");
  Serial.print(ESP.getFreeHeap());
  Serial.println(" bytes");

  if (ESP.getFreeHeap() == 0)
  {
    Serial.println("ERRO CRÍTICO: Heap zerado!");
    Serial.println("Possível corrupção de memória. Reiniciando...");
    delay(1000);
    ESP.restart();
  }

#ifdef ESP32
#ifdef RESET_NVS
  Serial.println("\nIniciando procedimento de reset do NVS...");

  esp_err_t ret = nvs_flash_erase();
  if (ret != ESP_OK)
  {
    Serial.printf("Falha ao apagar NVS (0x%x). Reiniciando...\n", ret);
    delay(1000);
    ESP.restart();
  }

  ret = nvs_flash_init();
  if (ret != ESP_OK)
  {
    Serial.printf("Falha ao inicializar NVS (0x%x). Reiniciando...\n", ret);
    delay(1000);
    ESP.restart();
  }

  Serial.println("NVS resetado com sucesso!");
#endif
#endif

  Serial.println("\n--- Sistema Inicializado ---");
  Serial.print("Free Heap Disponível: ");
  Serial.print(ESP.getFreeHeap());
  Serial.println(" bytes");
}

void loop()
{
  static unsigned int counter = 0;

  Serial.printf("\n[%d] Status do Sistema:\n", ++counter);
  Serial.print("Free Heap: ");
  Serial.print(ESP.getFreeHeap());
  Serial.println(" bytes");

#ifdef ESP32
  Serial.print("Menor Heap Livre: ");
  Serial.print(ESP.getMinFreeHeap());
  Serial.println(" bytes");

  Serial.print("Tamanho do Heap: ");
  Serial.print(ESP.getHeapSize());
  Serial.println(" bytes");
#endif

  delay(5000); // Verifica a cada 5 segundos
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
void setup()
{
  app.setup();
}
void loop()
{
  app.loop();
}

#endif