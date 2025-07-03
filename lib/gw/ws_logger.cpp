#include "ws_logger.h"
#ifdef WIFI

#include <ESPAsyncWebServer.h>
#ifdef ESP8266
#include <ESPAsyncTCP.h>
#else
#include <AsyncTCP.h>
#endif

// Definições das variáveis
AsyncWebSocket wsService("/ws");
AsyncWebServer *WSLogger::wserver = nullptr;

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len)
{
    AwsFrameInfo *info = (AwsFrameInfo *)arg;
    if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT)
    {
        data[len] = 0;

        DynamicJsonDocument doc(256);
        deserializeJson(doc, data);

        const char *cmd = doc["cmd"];
        if (strcmp(cmd, "display") == 0)
        {
            // Processamento do comando display
        }
    }
}

void logCallbackFn(const String &message)
{
    if (wsService.count())
    {
        wsService.textAll(message);
    }
}

#ifdef WIFI
void WSLogger::initWs(AsyncWebServer &server)
{
    wserver = &server;
    Logger::setLogCallback(logCallbackFn);

    wsService.onEvent([](AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
                         void *arg, uint8_t *data, size_t len)
                      {
        if (type == WS_EVT_CONNECT) {
            Logger::info("Cliente WebSocket #%u conectado", client->id());
        } else if (type == WS_EVT_DISCONNECT) {
            Logger::info("Cliente WebSocket #%u desconectado", client->id());
        } else if (type == WS_EVT_DATA) {
            handleWebSocketMessage(arg, data, len);
        } });

    wserver->addHandler(&wsService);

    wserver->on("/test", HTTP_GET, [](AsyncWebServerRequest *request)
                { request->send(200, "text/plain", "Servidor Web ativo"); });

    wserver->on("/ws", HTTP_GET, [](AsyncWebServerRequest *request)
                { request->send(200, "text/html", R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>Gateway WebSocket</title>
    <script>
        var ws = new WebSocket('ws://' + window.location.hostname + '/ws');
        ws.onopen = function() {
            console.log('WebSocket conectado');
        };
        ws.onmessage = function(e) {
            console.log('Mensagem:', e.data);
            var logs = document.getElementById('logs');
            logs.value += e.data + '\n';
            logs.scrollTop = logs.scrollHeight;
        };
        ws.onerror = function(e) {
            console.error('Erro no WebSocket:', e);
        };
        function sendMessage() {
            var msg = document.getElementById('message').value;
            ws.send(msg);
            document.getElementById('message').value = '';
        }
    </script>
</head>
<body>
    <h1>Logs Gateway</h1>
    <textarea id="logs" rows="10" cols="50" readonly></textarea><br>
    <input type="text" id="message" placeholder="Digite um comando">
    <button onclick="sendMessage()">Enviar</button>
</body>
</html>
)rawliteral"); });
}
#endif

#endif