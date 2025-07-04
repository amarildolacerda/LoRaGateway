#ifndef QUEUEMESSAGE_H
#define QUEUEMESSAGE_H

#include "Arduino.h"
#include "logger.h"

#include "Arduino.h"
#include "logger.h"

#define MAX_EVENT_LEN 8
#define MAX_VALUE_LEN 24
#if defined(ESP32) || defined(ESP8266)
#define MAX_ITEMS 5
#else
#define MAX_ITEMS 2
#endif

#pragma pack(push, 1)

struct MessageRec
{
    uint8_t to;
    uint8_t from;
    uint8_t id;
    uint8_t len;
    uint8_t hop;
    char event[MAX_EVENT_LEN];
    char value[MAX_VALUE_LEN];
    uint8_t crc;
    MessageRec() : to(0), from(0), id(0), len(0), hop(3) {}

    MessageRec clone()
    {
        MessageRec novo;
        novo.to = to;
        novo.from = from;
        novo.hop = hop;
        novo.id = hop;
        novo.len = len;
        novo.crc = crc;
        return novo;
    }
    uint8_t headerCount()
    {
        return 5;
    }
    // Calcula CRC-8 usando polinômio 0x07
    void updateCRC()
    {
        len = strlen(event) + strlen(value) + 3;
        crc = calculateCRC();
    }

    uint8_t sprint(char *data)
    {
        return sprintf(data, reinterpret_cast<const char *>(this));
    }
    uint8_t calculateCRC() const
    {
        uint8_t calculated = 0;
        char data[255];
        size_t x = sprintf(data, "%c%c%c%c%c{%s|%s}", to, from, id, len, hop, event, value);
        for (size_t i = 0; i < x; i++)
        {
            calculated ^= data[i];
            for (uint8_t j = 0; j < 8; j++)
            {
                if (calculated & 0x80)
                    calculated = (calculated << 1) ^ 0x07;
                else
                    calculated <<= 1;
            }
        }
        return calculated;
    }
    // Verifica a integridade dos dados
    bool verifyCRC() const
    {
        uint8_t calc = calculateCRC();
#ifdef DEBUG_ON
        if (crc != calc)
        {
            const uint8_t *data = reinterpret_cast<const uint8_t *>(this);
            char acrc[3];
            char ccrc[3];
            sprintf(acrc, "%02X", crc);
            sprintf(ccrc, "%02X", calc);
            // Logger::warn("CRC: %s Calc:%s Data:%s", acrc, ccrc, data + 5);
            // Logger::hex(LogLevel::ERROR, (char *)data, sizeof(MessageRec));
        }
#endif
        return crc == calc;
    }

#ifdef DEBUG_ON
    void print()
    {
        char msg[100] = {0};
        snprintf(msg, sizeof(msg), "[%d-%d:%d](%d:%d) {%s|%s}", from, to, id, hop, len, event, value);
        // Serial.println(msg);
    }
#endif
    // Adicionar dentro da struct
    bool setEvent(const char *str)
    {
        strncpy(event, str, MAX_EVENT_LEN - 1);
        event[MAX_EVENT_LEN - 1] = '\0';
        return strlen(str) < MAX_EVENT_LEN;
    }

    bool setValue(const char *str)
    {
        strncpy(value, str, MAX_VALUE_LEN - 1);
        value[MAX_VALUE_LEN - 1] = '\0';
        return strlen(str) < MAX_VALUE_LEN;
    }

    void clear()
    {
        memset(this, 0, sizeof(MessageRec));
    }
    bool isEvent(const char *ev) const
    {
        return strcmp(event, ev) == 0;
    }
    bool decode(const char *msg, const size_t plen)
    {
        clear();
        if (plen < 5)
            return false;
        to = msg[0];
        from = msg[1];
        id = msg[2];
        len = msg[3];
        hop = msg[4];
        bool b = parseCmd(String(msg + 5));

        return b;
    }
    size_t encode(char *buffer, const size_t plen)
    {
        updateCRC();
        int result = snprintf(buffer, plen, "%c%c%c%c%c{%s|%s}", to, from, id, len, hop, event, value);
        return result;
        //     // buffer[3] = result - 5;
        //     //  rec.print();
        //   updateCRC();
        //   return snprintf(msg, len, "%c%c%c%c%c{%s|%s}", to, from, id, len, hop, event, value);
    }

    bool parseCmd(const String msg)
    {

        uint8_t len = msg.length();
        if (msg.endsWith("\n"))
        {
            len--;
        }
        String content = msg.substring(0, len); // remove { and }
        if (!content.startsWith("{") || !content.endsWith("}"))
        {
            // Logger::error("Mensagem mal formatada: %s", msg);
            return false;
        }

        content = content.substring(1, content.length() - 1); // remove { and }
        int sepIndex = content.indexOf('|');
        int x = sprintf(event, "%s", content.substring(0, sepIndex).c_str());
        event[x] = '\0';
        if (sepIndex != -1)
        {
            x = sprintf(value, "%s", content.substring(sepIndex + 1).c_str());
            value[x] = '\0';
        }
        else
        {
            value[0] = '\0'; // no value provided
        }

        return true;
    }
};
#pragma pack(pop)

#ifndef __AVR__
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

class MessageQueue
{
private:
    QueueHandle_t xQueue;
    bool checkDup = false;

public:
    // Construtor - cria a fila FreeRTOS
    MessageQueue(int size = MAX_ITEMS)
    {
        xQueue = xQueueCreate(size, sizeof(MessageRec));
        if (xQueue == NULL)
        {
            // Logger::error("Falha ao criar fila FreeRTOS");
        }
    }

    // Destrutor - libera a fila
    ~MessageQueue()
    {
        if (xQueue != NULL)
        {
            vQueueDelete(xQueue);
        }
    }

    // Método push modificado
    bool pushItem(const MessageRec &item)
    {
        if (xQueue == NULL)
            return false;
        if (checkDup && contains(item))
            return false;

        // Envia para a fila com timeout zero (não bloqueante)
        return xQueueSend(xQueue, &item, 0) == pdPASS;
    }

    // Método pop modificado
    bool popItem(MessageRec &item)
    {
        if (xQueue == NULL)
            return false;
        return xQueueReceive(xQueue, &item, 0) == pdPASS;
    }

    // Verificação de duplicados (opcional)
    bool contains(const MessageRec &item) const
    {
        if (xQueue == NULL)
            return false;

        // Cria uma cópia temporária da fila para verificação
        QueueHandle_t xTempQueue = xQueueCreate(uxQueueMessagesWaiting(xQueue), sizeof(MessageRec));
        if (xTempQueue == NULL)
            return false;

        bool found = false;
        MessageRec tempItem;

        // Verifica todos os itens na fila
        while (xQueueReceive(xQueue, &tempItem, 0) == pdPASS)
        {
            if (tempItem.crc == item.crc)
            {
                found = true;
            }
            xQueueSend(xTempQueue, &tempItem, 0);
        }

        // Restaura a fila original
        while (xQueueReceive(xTempQueue, &tempItem, 0) == pdPASS)
        {
            xQueueSend(xQueue, &tempItem, 0);
        }

        vQueueDelete(xTempQueue);
        return found;
    }

    // Métodos auxiliares
    bool isEmpty() const
    {
        return xQueue == NULL || uxQueueMessagesWaiting(xQueue) == 0;
    }

    bool isFull() const
    {
        return xQueue != NULL && uxQueueSpacesAvailable(xQueue) == 0;
    }

    int size() const
    {
        return xQueue != NULL ? uxQueueMessagesWaiting(xQueue) : 0;
    }

    // Versão para uso em ISRs (Interrupt Service Routines)
    bool pushFromISR(const MessageRec &item, BaseType_t *pxHigherPriorityTaskWoken = NULL)
    {
        if (xQueue == NULL)
            return false;
        return xQueueSendFromISR(xQueue, &item, pxHigherPriorityTaskWoken) == pdPASS;
    }

    bool popFromISR(MessageRec &item, BaseType_t *pxHigherPriorityTaskWoken = NULL)
    {
        if (xQueue == NULL)
            return false;
        return xQueueReceiveFromISR(xQueue, &item, pxHigherPriorityTaskWoken) == pdPASS;
    }

    bool push(const uint8_t to, const char *event, const char *value,
              const uint8_t from, const uint8_t hop, const uint8_t id = 0)
    {
        MessageRec msg;
        msg.clear();
        msg.to = to;
        msg.from = from;
        msg.hop = hop;
        msg.id = id;
        msg.setEvent(event);
        msg.setValue(value);
        msg.updateCRC();
        return pushItem(msg);
    }
};
#else

#if defined(__AVR__)
#include <util/atomic.h>
#define CRITICAL_SECTION ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
#elif defined(ESP32)
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

static portMUX_TYPE queueMutex = portMUX_INITIALIZER_UNLOCKED;
#define CRITICAL_SECTION             \
    portENTER_CRITICAL(&queueMutex); \
    portEXIT_CRITICAL(&queueMutex);
#else
#define CRITICAL_SECTION
#endif

class MessageQueue
{
private:
    MessageRec *items; // Ponteiro para o array dinâmico
    volatile int head = 0;
    volatile int tail = 0;
    volatile int count = 0;
    volatile int maxItems; // Tamanho máximo da fila

public:
    bool checkDup = false;

    // Construtor que recebe o tamanho da fila
    MessageQueue(int size = MAX_ITEMS) : maxItems(size)
    {
        items = new MessageRec[maxItems];
    }

    // Destrutor para liberar memória
    ~MessageQueue()
    {
        delete[] items;
    }

    bool pushItem(const MessageRec &item)
    {
        bool result = false;

        if (checkDup && contains(item))
            return false;

        CRITICAL_SECTION
        {

            if (count < maxItems)
            {
                items[tail] = item;
                tail = (tail + 1) % maxItems;
                count++;
                result = true;
#ifdef DEBUG_ON
                Serial.print("Push item. Count: ");
                Serial.println(count);
#endif
            }
        }
        return result;
    }

    bool contains(const MessageRec &item) const
    {
#ifdef __AVR__
        return false;
#else
        bool result = false;
        CRITICAL_SECTION
        {
            for (int i = 0; i < count; i++)
            {
                int index = (head + i) % maxItems;
                if (items[index].crc == item.calculateCRC())
                {
                    result = true;
                    break;
                }
            }
        }
        return result;
#endif
    }

    bool popItem(MessageRec &item)
    {
        bool result = false;
        CRITICAL_SECTION
        {

            if (count > 0)
            {
                item = items[head];
                head = (head + 1) % maxItems;
                count--;
                result = true;
#ifdef DEBUG_ON
                Serial.print("Pop item. Count: ");
                Serial.println(count);
#endif
            }
        }
        return result;
    }

    bool push(const uint8_t to, const char *event, const char *value,
              const uint8_t from, const uint8_t hope, const uint8_t id = 0)
    {
        MessageRec msg;
        msg.clear();
        msg.to = to;
        msg.from = from;
        msg.hop = hope;
        msg.id = id;
        msg.setEvent(event);
        msg.setValue(value);
        msg.calculateCRC();
        return pushItem(msg);
    }

    bool isEmpty()
    {
        bool result;
        CRITICAL_SECTION
        {
            result = (count == 0);
        }
        return result;
    }

    bool isFull()
    {
        bool result;
        CRITICAL_SECTION
        {
            result = (count == maxItems);
        }
        return result;
    }

    int size()
    {
        int result;
        CRITICAL_SECTION
        {
            result = count;
        }
        return result;
    }
};

#endif

#endif