#if defined(ASYNC_WS)
#ifdef ESP32
#include <WiFi.h>
#else
#include <ESP8266WiFi.h>
#endif
#include <ESPAsyncTCP.h>
#include <WiFiUdp.h>

#define BROADCAST_PORT 12346 // Porta UDP do broadcast
#define CLIENT_PORT 12345    // Porta do cliente para resposta

struct NetworkInfo
{
    IPAddress ip;
    int port;
};

typedef void (*BroadcastCallback)(const NetworkInfo *info); // Definição do callback

class BroadcastHandler
{
private:
    bool isGateway;

public:
    BroadcastHandler(bool gateway) : isGateway(gateway), callback(nullptr) {}

    void setup()
    {
        udp.begin(BROADCAST_PORT); // Inicia escuta do broadcast
    }

    void sendBroadcastRequest()
    {
        Serial.println("📢 Enviando pedido de broadcast...");
        String message = "ESP_DISCOVERY|ip=" + WiFi.localIP().toString() + "|port=" + String(CLIENT_PORT);

        udp.beginPacket("255.255.255.255", BROADCAST_PORT);
        udp.print(message);
        udp.endPacket();
        Serial.println("✅ Pedido de broadcast enviado: " + message);
    }

    void loop()
    {
        int packetSize = udp.parsePacket();
        if (packetSize)
        {
            char buffer[100];
            udp.read(buffer, sizeof(buffer));
            buffer[packetSize] = '\0';

            String receivedMessage = String(buffer);

            // **Filtrar mensagens apenas do nosso protocolo**
            if (receivedMessage.startsWith("ESP_DISCOVERY|"))
            {
                Serial.print("📡 Broadcast válido recebido: ");
                Serial.println(receivedMessage);

                // Extrai IP e porta do servidor
                String serverIP = receivedMessage.substring(receivedMessage.indexOf("ip=") + 3, receivedMessage.indexOf("|port="));
                int serverPort = receivedMessage.substring(receivedMessage.indexOf("|port=") + 6).toInt();

                if (callback && serverPort > 0)
                {
                    NetworkInfo info;
                    info.ip.fromString(serverIP.c_str());
                    info.port = serverPort;
                    callback(&info);

                    Serial.print("🌐 Servidor encontrado: ");
                    Serial.print(serverIP);
                    Serial.print(":");
                    Serial.println(serverPort);
                }
            }
            else
            {
                Serial.println("⚠ Mensagem ignorada! Não pertence ao nosso protocolo.");
            }
        }
    }
    void setCallback(BroadcastCallback cb)
    {
        callback = cb;
    }

private:
    WiFiUDP udp;
    BroadcastCallback callback;
};

#endif