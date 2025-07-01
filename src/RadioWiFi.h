#if !defined(RADIO_WIFI_ASYNC_H) && defined(RADIO_WIFI)
#define RADIO_WIFI_ASYNC_H

#include "Arduino.h"
#include "RadioInterface.h"
#ifdef ESP8266
#include <ESPAsyncTCP.h>
#else
#include <AsyncTCP.h>
#endif
#include <vector>
#include "EEPROM.h"

struct Device
{
    uint8_t termId;
    AsyncClient *client;
    // Device() : id(0),
    //  client(nullptr){}
};

class RadioWiFi : public RadioInterface
{
private:
    struct PersistentData
    {
        char gatewayIP[20];
        uint16_t port;
    };
    AsyncServer *tcpServer;
    AsyncClient *tcpClient;
    std::vector<Device> clients; // Para modo gateway
    uint16_t tcpPort;
    unsigned long lastConnectionAttempt = 0;
    const unsigned long connectionInterval = 5000; // 5 segundos
    String pendingData;                            // Buffer para dados recebidos parcialmente
    bool isGateway;
    String gatewayIP = "";

public:
    RadioWiFi(uint16_t port = 12345, bool gatewayMode = false)
        : tcpPort(port), isGateway(gatewayMode), tcpServer(nullptr), tcpClient(nullptr) {}

    virtual ~RadioWiFi()
    {
        if (tcpServer)
        {
            tcpServer->end();
            delete tcpServer;
        }
        if (tcpClient)
        {
            tcpClient->close();
            delete tcpClient;
        }
        // bool removed = false;
        for (auto &dev : clients)
        {
            if (dev.client)
            {
                dev.client->close();
                delete dev.client;
                dev.client = nullptr;
                // removed = true;
                break;
            }
        }
    }

    virtual String getIdent() override
    {
        return "WiFiAsync";
    }

    virtual bool begin(const uint8_t terminal_Id, long band, bool promisc = true) override
    {
        terminalId = terminal_Id;
        _promiscuos = promisc;

        if (isGateway)
        {
            tcpServer = new AsyncServer(tcpPort);
            tcpServer->onClient([this](void *arg, AsyncClient *client)
                                { this->handleNewClient(client); }, nullptr);
            tcpServer->begin();
            connected = true;
            Serial.println("Modo Gateway WiFi (Async) iniciado");
        }
        else
        {
            // No modo cliente, a conexão será estabelecida no loop
            connected = false;
            loadPersistentData();
        }

        return true;
    }

    String toString(IPAddress ip)
    {
        char ipChar[16]; // Buffer para o IP (máximo "XXX.XXX.XXX.XXX")
        sprintf(ipChar, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
        return String(ipChar);
    }
    void setClientToGateway(IPAddress ip, int port)
    {
        if (!isGateway)
        {
            String newIP = toString(ip);
            if (IPAddress::isValid(newIP))
            {
                Logger::info(String("Broadcast: " + newIP + ":" + String(port)).c_str());
                if (!isGateway && (newIP.compareTo(gatewayIP) != 0 || port != tcpPort))
                {

                    if (tcpClient)
                    {
                        tcpClient->close();
                        connected = false;
                    }
                    gatewayIP = String("" + newIP);
                    tcpPort = port;

                    savePersistentData();
                }
            }
        }
    }

    void loadPersistentData()
    {
        PersistentData data;
        EEPROM.begin(sizeof(PersistentData));
        EEPROM.get(0, data);
        EEPROM.end();

        if (data.gatewayIP[0] != '\0')
        {

            if (IPAddress::isValid(data.gatewayIP))
            {
                gatewayIP = String(data.gatewayIP);
                // gatewayIP = "192.168.15.96";
                tcpPort = data.port;
                Serial.println("Dados persistentes carregados: " + gatewayIP + ":" + String(tcpPort));
            }
        }
    }

    void savePersistentData()
    {
        PersistentData data;
        strncpy(data.gatewayIP, gatewayIP.c_str(), sizeof(data.gatewayIP));
        data.port = tcpPort;

        EEPROM.begin(sizeof(PersistentData));
        EEPROM.put(0, data);
        EEPROM.commit();
        EEPROM.end();
        Serial.println("Dados persistentes salvos: " + gatewayIP + ":" + String(tcpPort));
    }

    virtual void loop() override
    {
        // O AsyncTCP lida com os eventos em callbacks, então não precisamos fazer muito aqui
        if (!isGateway && !connected && millis() - lastConnectionAttempt > connectionInterval)
        {
            connectToGateway();
            lastConnectionAttempt = millis();
        }
        else
        {
        }

        MessageRec txRec;
        if (txQueue.popItem(txRec))
        {
            sendMessage(txRec);
        }
    }

    virtual bool sendMessage(MessageRec &rec) override
    {
        if (isGateway)
        {
            // Serial.println("Enviando ");
            // rec.print();
            //  Envia para todos os clientes conectados
            bool sent = false;
            for (auto dev : clients)
            {
                if (dev.termId == rec.to || rec.to == 0xFF)
                {
                    if (dev.client && dev.client->connected())
                    {
                        sent |= sendToClient(dev.client, rec);
                    }
                }
            }
            return sent;
        }
        else
        {
            // Envia apenas para o gateway
            if (tcpClient && tcpClient->connected())
            {
                return sendToClient(tcpClient, rec);
            }
            return false;
        }
    }

    virtual bool receiveMessage() override
    {
        // As mensagens são processadas nos callbacks de recebimento
        return false;
    }

    virtual void modeRx() override
    {
        // Nada específico necessário para WiFi assíncrono
    }

    virtual void modeTx() override
    {
        // Nada específico necessário para WiFi assíncrono
    }

private:
    void handleNewClient(AsyncClient *client)
    {
        Serial.printf("Novo cliente conectado: %s\n", client->remoteIP().toString().c_str());

        // Configura callbacks para o novo cliente
        client->onData([this](void *arg, AsyncClient *c, void *data, size_t len)
                       { this->handleClientData(c, (char *)data, len); }, nullptr);

        client->onDisconnect([this](void *arg, AsyncClient *c)
                             { this->handleClientDisconnect(c); }, nullptr);

        client->onError([this](void *arg, AsyncClient *c, int8_t error)
                        { Serial.printf("Erro de conexão: %d\n", error); }, nullptr);

        client->onTimeout([this](void *arg, AsyncClient *c, uint32_t time)
                          { Serial.println("Timeout de conexão"); }, nullptr);

        Device newDev;
        newDev.client = client;
        newDev.termId = 0xFF;

        clients.push_back(newDev);
        /*
                // Envia identificação para o novo cliente
                MessageRec identMsg;
                identMsg.to = 0xFF; // Broadcast para o cliente
                identMsg.from = terminalId;
                identMsg.setEvent(EVT_PRESENTATION);
                identMsg.setValue(terminalName);
                identMsg.updateCRC();
                sendToClient(client, identMsg);
                */
    }

    void handleClientData(AsyncClient *client, char *data, size_t len)
    {
        pendingData.concat(data, len);

        // Serial.println(pendingData.substring(6, pendingData.indexOf('\n')));
        // Serial.println(pendingData);
        // Serial.println(pendingData.indexOf('\n', 6)); // Processa todas as mensagens completas no buffer
        int endPos;
        while ((endPos = pendingData.indexOf('\n', 6)) != -1)
        {
            String message = pendingData.substring(0, endPos);
            // Serial.println(message);
            pendingData.remove(0, endPos + 1);

            MessageRec msg;
            if (msg.decode(message.c_str(), message.length()))
            {
                if (addRxMessage(msg))
                {
                    for (auto &dev : clients)
                    {
                        if (dev.client == client)
                        {
                            dev.termId = msg.from;
                        }
                    }
                }
                // else
                //     log(false, msg);
            }
            else
            {
                // msg.print();
            }
        }
    }

    void handleClientDisconnect(AsyncClient *client)
    {
        Serial.println("Cliente desconectado");
        for (auto it = clients.begin(); it != clients.end(); ++it)
        {
            if (it->client == client)
            {
                clients.erase(it);
                delete client;
                break;
            }
        }
    }
    int indexOf(const uint8_t termId)
    {
        for (size_t i = 0; i < clients.size(); i++)
            if (clients[i].termId == termId)
            {
                return i;
            }

        return -1;
    }
    bool sendToClient(AsyncClient *client, MessageRec &rec)
    {
        if (!client->connected())
        {
            return false;
        }
        // Serial.println();
        // Serial.print("sendToClient ");
        // Serial.print(rec.to);
        // Serial.print(" ");
        // Serial.println(client->localIP().toString());
        // rec.print();

        char buffer[MESSAGE_MAX_LEN];
        size_t len = rec.encode(buffer, sizeof(buffer));
        // Serial.print(len);
        // Serial.print(":");
        // Serial.println(buffer + 5);
        if (len == 0)
        {
            return false;
        }

        // Adiciona delimitador de fim de mensagem
        buffer[len++] = '\n';

        bool sent = client->write(buffer, len);
        if (sent)
        {
            log(true, rec);
            // Serial.print("To: ");
            // Serial.println(client->remoteIP());
        }
        return sent;
    }

    void connectToGateway()
    {
        if (gatewayIP.isEmpty())
        {
            return;
        }
        if (tcpClient)
        {
            tcpClient->close();
            delete tcpClient;
        }

        tcpClient = new AsyncClient();

        // Configura callbacks para o cliente
        tcpClient->onConnect([this](void *arg, AsyncClient *c)
                             {
            Serial.println("Conectado ao gateway WiFi");
            connected = true;
            
            // Envia mensagem de identificação
            MessageRec identMsg;
            identMsg.to = 0xFF; // Broadcast para o gateway
            identMsg.from = terminalId;
            identMsg.setEvent("pub");
            identMsg.setValue(terminalName);
            identMsg.updateCRC();
            sendToClient(c, identMsg); }, nullptr);

        tcpClient->onData([this](void *arg, AsyncClient *c, void *data, size_t len)
                          { this->handleClientData(c, (char *)data, len); }, nullptr);

        tcpClient->onDisconnect([this](void *arg, AsyncClient *c)
                                {
            Serial.println("Desconectado do gateway");
            connected = false;
            delete c;
            tcpClient = nullptr; }, nullptr);

        tcpClient->onError([this](void *arg, AsyncClient *c, int8_t error)
                           {
            Serial.printf("Erro de conexão: %d\n", error);
            connected = false; }, nullptr);

        // Conecta ao gateway (substitua pelo IP do seu gateway)
        // IPAddress gatewayIP(192, 168, 1, 1);
        // tcpClient->connect(gatewayIP, tcpPort);
        IPAddress ip;
        ip.fromString(gatewayIP);
        tcpClient->connect(ip, tcpPort);

        if (tcpClient->connected())
            Logger::info(String("Conectou em: " + String(gatewayIP) + ":" + String(tcpPort)).c_str());
        // Serial.print(ip);
        // Serial.print(" ");
        // Serial.println(gatewayIP);
    }
};

#endif // RADIO_WIFI_ASYNC_H