#if defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#else
#include <WiFi.h>
#include <AsyncTCP.h>
#endif
#include <ESPAsyncWebServer.h>
#include <DNSServer.h>
#include <Preferences.h>

// Namespace para estilos web compartilhados
namespace WebStyles
{
  static String getModernStyle()
  {
    return (R"(
      body {
        font-family: 'Segoe UI', Arial, sans-serif;
        background-color: #f0f2f5;
        color: #333;
        margin: 0;
        padding: 20px;
        line-height: 1.6;
      }
      .container {
        max-width: 600px;
        margin: 0 auto;
        background: #fff;
        padding: 20px;
        border-radius: 10px;
        box-shadow: 0 4px 12px rgba(0, 0, 0, 0.1);
      }
      h1 {
        color: #1a73e8;
        text-align: center;
        margin-bottom: 20px;
      }
      .status {
        background: #f8f9fa;
        padding: 15px;
        border-radius: 8px;
        margin-bottom: 20px;
      }
      .status p {
        margin: 8px 0;
      }
      .form {
        display: flex;
        flex-direction: column;
        gap: 15px;
      }
      .form-group {
        display: flex;
        flex-direction: column;
        text-align: left;
      }
      .form-group label {
        font-weight: 500;
        margin-bottom: 5px;
        color: #555;
      }
      .form-group input {
        padding: 10px;
        border: 1px solid #ddd;
        border-radius: 5px;
        font-size: 16px;
        transition: border-color 0.3s;
      }
      .form-group input:focus {
        border-color: #1a73e8;
        outline: none;
      }
      .btn {
        background-color: #1a73e8;
        color: #fff;
        padding: 12px 20px;
        border: none;
        border-radius: 5px;
        font-size: 16px;
        cursor: pointer;
        transition: background-color 0.3s;
      }
      .btn:hover {
        background-color: #1557b0;
      }
      .btn-secondary {
        background-color: #6c757d;
      }
      .btn-secondary:hover {
        background-color: #5a6268;
      }
      h2 {
        color: #333;
        font-size: 20px;
        margin-top: 20px;
        margin-bottom: 10px;
      }
      h3 {
        color: #333;
        font-size: 18px;
        margin-top: 15px;
        margin-bottom: 10px;
      }
      @media (max-width: 600px) {
        .container {
          padding: 15px;
        }
        .form-group input {
          font-size: 14px;
        }
        .btn {
          font-size: 14px;
          padding: 10px;
        }
      }
    )");
  }
}

class AccessPointConfig
{
private:
  String ssid;
  String password;
  uint8_t channel;
  IPAddress localIP;
  IPAddress subnet;
  bool isAPStarted;
  bool isSTAEnabled;
  AsyncWebServer *server;
  DNSServer *dnsServer;
  uint16_t webServerPort;
  Preferences preferences;
  String staSSID[2];                       // Array para até 2 SSIDs de redes STA
  String staPassword[2];                   // Array para até 2 senhas de redes STA
  unsigned long connectTimeout;            // Tempo limite para conexão STA (ms)
  const String webUsername = ("admin");    // Usuário para autenticação
  const String webPassword = ("admin123"); // Senha para autenticação

  // Verificar autenticação
  bool isAuthenticated(AsyncWebServerRequest *request)
  {
    if (request->hasHeader(("Authorization")))
    {
      String authHeader = request->getHeader(("Authorization"))->value();
      if (authHeader.startsWith(("Basic ")))
      {
        String credentials = authHeader.substring(6);
        credentials.trim();
        char decoded[64];
        size_t len = base64_decode(decoded, credentials.c_str(), credentials.length());
        decoded[len] = '\0';
        String decodedStr = String(decoded);
        if (decodedStr == (webUsername + ":" + webPassword))
        {
          return true;
        }
      }
    }
    return false;
  }

  // Enviar cabeçalho de autenticação
  void requestAuthentication(AsyncWebServerRequest *request)
  {
    AsyncWebServerResponse *response = request->beginResponse(401, ("text/html"), ("<h1>Unauthorized</h1><p>Please log in.</p>"));
    response->addHeader(("WWW-Authenticate"), ("Basic realm=\"ESP Config\""));
    request->send(response);
  }

  // Página HTML para configuração
  String getConfigHTML()
  {
    String html = ("<!DOCTYPE html><html><head>"
                   "<title>ESP AP Config</title>"
                   "<meta name='viewport' content='width=device-width, initial-scale=1'>"
                   "<style>");
    html += WebStyles::getModernStyle();
    html += ("</style></head><body>"
             "<div class='container'>"
             "<h1>Access Point Configuration</h1>"
             "<div class='status'>"
             "<p><strong>AP SSID:</strong> ");
    html += ssid;
    html += ("</p>"
             "<p><strong>AP IP Address:</strong> ");
    html += WiFi.softAPIP().toString();
    html += ("</p>"
             "<p><strong>Connected Clients:</strong> ");
    html += String(WiFi.softAPgetStationNum());
    html += ("</p>"
             "<p><strong>STA SSID 1 (Saved):</strong> ");
    html += (staSSID[0].isEmpty() ? ("None") : staSSID[0]);
    html += ("</p>"
             "<p><strong>STA SSID 2 (Saved):</strong> ");
    html += (staSSID[1].isEmpty() ? ("None") : staSSID[1]);
    html += ("</p>");
    if (isSTAEnabled)
    {
      html += ("<p><strong>STA Status:</strong> ");
      html += (WiFi.status() == WL_CONNECTED ? ("Connected") : ("Disconnected"));
      html += ("</p>"
               "<p><strong>STA IP:</strong> ");
      html += (WiFi.status() == WL_CONNECTED ? WiFi.localIP().toString() : ("N/A"));
      html += ("</p>");
    }
    html += ("</div>"
             "<form action='/config' method='POST' class='form'>"
             "<h2>Update AP Settings</h2>"
             "<div class='form-group'>"
             "<label for='ap_ssid'>AP SSID:</label>"
             "<input type='text' id='ap_ssid' name='ap_ssid' value='");
    html += ssid;
    html += ("' required>"
             "</div>"
             "<div class='form-group'>"
             "<label for='ap_password'>AP Password:</label>"
             "<input type='text' id='ap_password' name='ap_password' value='");
    html += password;
    html += ("' required>"
             "</div>"
             "<div class='form-group'>"
             "<label for='channel'>Channel:</label>"
             "<input type='number' id='channel' name='channel' min='1' max='13' value='");
    html += String(channel);
    html += ("' required>"
             "</div>"
             "<h2>WiFi Network (STA)</h2>"
             "<h3>Network 1</h3>"
             "<div class='form-group'>"
             "<label for='sta_ssid_1'>STA SSID 1:</label>"
             "<input type='text' id='sta_ssid_1' name='sta_ssid_1' value='");
    html += staSSID[0];
    html += ("'>"
             "</div>"
             "<div class='form-group'>"
             "<label for='sta_password_1'>STA Password 1:</label>"
             "<input type='text' id='sta_password_1' name='sta_password_1' value='");
    html += staPassword[0];
    html += ("'>"
             "</div>"
             "<h3>Network 2</h3>"
             "<div class='form-group'>"
             "<label for='sta_ssid_2'>STA SSID 2:</label>"
             "<input type='text' id='sta_ssid_2' name='sta_ssid_2' value='");
    html += staSSID[1];
    html += ("'>"
             "</div>"
             "<div class='form-group'>"
             "<label for='sta_password_2'>STA Password 2:</label>"
             "<input type='text' id='sta_password_2' name='sta_password_2' value='");
    html += staPassword[1];
    html += ("'>"
             "</div>"
             "<button type='submit' class='btn'>Apply Changes</button>"
             "</form>"
             "<form action='/clear_sta' method='POST'>"
             "<button type='submit' class='btn btn-secondary'>Clear STA Credentials</button>"
             "</form>"
             "<form action='/restart' method='POST'>"
             "<button type='submit' class='btn btn-secondary'>Restart Device</button>"
             "</form>"
             "</div></body></html>");
    return html;
  }

  // Configurar rotas do servidor web
  void setupWebServer()
  {
    if (!server)
      return;

    // Rota para a página principal
    server->on("/", HTTP_GET, [this](AsyncWebServerRequest *request)
               {
      if (!isAuthenticated(request)) {
        requestAuthentication(request);
        return;
      }
      request->send(200, ("text/html"), getConfigHTML()); });

    // Redirecionar todas as outras requisições para a página principal
    server->onNotFound([this](AsyncWebServerRequest *request)
                       { request->redirect("http://" + WiFi.softAPIP().toString() + "/"); });

    // Rota para processar configurações
    server->on("/config", HTTP_POST, [this](AsyncWebServerRequest *request)
               {
      if (!isAuthenticated(request)) {
        requestAuthentication(request);
        return;
      }


      bool restartAP = false;
      bool updateSTA = false;


      // Configurações do AP
      if (request->hasParam(("ap_ssid"), true) && request->hasParam(("ap_password"), true) && request->hasParam(("channel"), true)) {
        String newAPSSID = request->getParam(("ap_ssid"), true)->value();
        String newAPPassword = request->getParam(("ap_password"), true)->value();
        String channelStr = request->getParam(("channel"), true)->value();
        uint8_t newChannel = channelStr.toInt();


        if (newAPPassword.length() >= 8 && newChannel >= 1 && newChannel <= 13) {
          if (newAPSSID != ssid || newAPPassword != password || newChannel != channel) {
            setSSID(newAPSSID);
            setPassword(newAPPassword);
            setChannel(newChannel);
            saveAPConfig();
            restartAP = true;
          }
        } else {
          request->send(400, ("text/html"), ("<h1>Error</h1><p>Invalid AP password (min 8 chars) or channel (1-13).</p><a href='/'>Back</a>"));
          return;
        }
      }


      // Configurações do STA
      String newSTASSID[2];
      String newSTAPassword[2];
      bool staChanged = false;
      for (int i = 0; i < 2; i++) {
        String ssidKey = String(("sta_ssid_")) + String(i + 1);
        String passKey = String(("sta_password_")) + String(i + 1);
        if (request->hasParam(ssidKey, true) && request->hasParam(passKey, true)) {
          newSTASSID[i] = request->getParam(ssidKey, true)->value();
          newSTAPassword[i] = request->getParam(passKey, true)->value();
          if (newSTASSID[i] != staSSID[i] || newSTAPassword[i] != staPassword[i]) {
            staSSID[i] = newSTASSID[i];
            staPassword[i] = newSTAPassword[i];
            staChanged = true;
          }
        }
      }
      if (staChanged) {
        saveSTAConfig();
        updateSTA = true;
      }


      if (restartAP || updateSTA) {
        if (restartAP) {
          endAP();
          if (!beginAP()) {
            request->send(500, ("text/html"), ("<h1>Error</h1><p>Failed to restart AP.</p><a href='/'>Back</a>"));
            return;
          }
        }
        request->send(200, ("text/html"), ("<h1>Settings Updated</h1><p>Settings saved. Restart the device to try connecting to WiFi.</p><a href='/'>Back</a>"));
      } else {
        request->send(200, ("text/html"), ("<h1>No Changes</h1><p>No settings were modified.</p><a href='/'>Back</a>"));
      } });

    // Rota para limpar credenciais STA
    server->on("/clear_sta", HTTP_POST, [this](AsyncWebServerRequest *request)
               {
      if (!isAuthenticated(request)) {
        requestAuthentication(request);
        return;
      }
      clearSTAConfig();
      request->send(200, ("text/html"), ("<h1>STA Credentials Cleared</h1><p>STA credentials removed. Restart the device to apply.</p><a href='/'>Back</a>")); });

    // Rota para reiniciar o dispositivo
    server->on("/restart", HTTP_POST, [this](AsyncWebServerRequest *request)
               {
      if (!isAuthenticated(request)) {
        requestAuthentication(request);
        return;
      }
      request->send(200, ("text/html"), ("<h1>Restarting</h1><p>Device is restarting...</p>"));
      delay(1000);
      ESP.restart(); });

    // Iniciar o servidor apenas se ainda não estiver ativo
    // if (!server->hasBegun())
    {
      server->begin();
    }
  }

  // Configurar o servidor DNS para o portal cativo
  void setupDNSServer()
  {
    if (!dnsServer)
      return;
    dnsServer->setErrorReplyCode(DNSReplyCode::NoError);
    dnsServer->start(53, ("*"), localIP);
  }

  // Salvar configurações do AP na memória não volátil
  void saveAPConfig()
  {
    preferences.begin(("ap_config"), false);
    preferences.putString(("ssid"), ssid);
    preferences.putString(("password"), password);
    preferences.putUInt(("channel"), channel);
    preferences.end();
  }

  // Carregar configurações do AP da memória não volátil
  void loadAPConfig()
  {
    preferences.begin(("ap_config"), true);
    ssid = preferences.getString(("ssid"), ssid);
    password = preferences.getString(("password"), password);
    channel = preferences.getUInt(("channel"), channel);
    preferences.end();
  }

  // Salvar configurações do STA na memória não volátil
  void saveSTAConfig()
  {
    preferences.begin(("sta_config"), false);
    for (int i = 0; i < 2; i++)
    {
      String ssidKey = String(("sta_ssid_")) + String(i + 1);
      String passKey = String(("sta_password_")) + String(i + 1);
      preferences.putString(ssidKey.c_str(), staSSID[i]);
      preferences.putString(passKey.c_str(), staPassword[i]);
    }
    preferences.end();
  }

  // Carregar configurações do STA da memória não volátil
  void loadSTAConfig()
  {
    preferences.begin(("sta_config"), true);
    for (int i = 0; i < 2; i++)
    {
      String ssidKey = String(("sta_ssid_")) + String(i + 1);
      String passKey = String(("sta_password_")) + String(i + 1);
      staSSID[i] = preferences.getString(ssidKey.c_str(), "");
      staPassword[i] = preferences.getString(passKey.c_str(), "");
    }
    preferences.end();
  }

  // Limpar configurações do STA
  void clearSTAConfig()
  {
    preferences.begin(("sta_config"), false);
    preferences.clear();
    for (int i = 0; i < 2; i++)
    {
      staSSID[i] = "";
      staPassword[i] = "";
    }
    preferences.end();
  }

public:
  // Construtor com servidores externos
  AccessPointConfig(AsyncWebServer *webServer,
                    DNSServer *dnsServer,
                    const String &ssid = ("ESP-AP"),
                    const String &password = ("12345678"),
                    uint8_t channel = 1,
                    const IPAddress &localIP = IPAddress(192, 168, 4, 1),
                    const IPAddress &subnet = IPAddress(255, 255, 255, 0),
                    uint16_t port = 80,
                    unsigned long connectTimeout = 10000)
      : ssid(ssid), password(password), channel(channel),
        localIP(localIP), subnet(subnet), isAPStarted(false),
        isSTAEnabled(false), webServerPort(port), server(webServer),
        dnsServer(dnsServer), connectTimeout(connectTimeout)
  {
    loadAPConfig();
    loadSTAConfig();
  }

  // Destrutor não libera servidores, pois são externos
  ~AccessPointConfig()
  {
    // Não deletar server ou dnsServer, pois são gerenciados externamente
  }

  // Método para tentar conectar ao WiFi (STA) e manter o AP ativo
  bool autoConnect()
  {
    loadSTAConfig();

    // Iniciar o AP com limite de 2 clientes
    if (!beginAP())
    {
      return false;
    }

    // Tentar conectar às redes STA salvas
    for (int i = 0; i < 2; i++)
    {
      if (!staSSID[i].isEmpty())
      {
        WiFi.mode(WIFI_AP_STA);
        WiFi.begin(staSSID[i].c_str(), staPassword[i].c_str());
        isSTAEnabled = true;

        unsigned long startTime = millis();
        while (WiFi.status() != WL_CONNECTED && millis() - startTime < connectTimeout)
        {
          delay(500);
        }

        if (WiFi.status() == WL_CONNECTED)
        {
          return true; // Conectado ao STA, AP ativo
        }
      }
    }

    return true; // AP está ativo, mesmo que STA não conecte
  }

  // Método para iniciar o Access Point, servidor web e DNS
  bool beginAP()
  {
    if (!isAPStarted)
    {
      WiFi.mode(isSTAEnabled ? WIFI_AP_STA : WIFI_AP);
      if (!WiFi.softAPConfig(localIP, localIP, subnet))
      {
        return false;
      }
      bool success = WiFi.softAP(ssid.c_str(), password.c_str(), channel, 0, 2); // Limite de 2 clientes
      if (success)
      {
        isAPStarted = true;
        setupWebServer();
        setupDNSServer();
        return true;
      }
      return false;
    }
    return true;
  }

  // Método para processar requisições DNS
  void loop()
  {
    if (dnsServer && isAPStarted)
    {
      dnsServer->processNextRequest();
    }
  }

  // Método para parar o Access Point
  void endAP()
  {
    if (isAPStarted)
    {
      WiFi.softAPdisconnect(true);
      isAPStarted = false;
    }
    // Não parar o server ou dnsServer, pois são compartilhados
  }

  // Obter o SSID do AP
  String getSSID() const
  {
    return ssid;
  }

  // Obter o IP do AP ou STA
  IPAddress getIP() const
  {
    return isAPStarted ? WiFi.softAPIP() : WiFi.localIP();
  }

  // Verificar se o AP está ativo
  bool isRunning() const
  {
    return isAPStarted;
  }

  // Verificar se está conectado ao WiFi (STA)
  bool isSTAConnected() const
  {
    return WiFi.status() == WL_CONNECTED && isSTAEnabled;
  }

  // Obter número de clientes conectados (AP)
  uint8_t getConnectedClients() const
  {
    return isAPStarted ? WiFi.softAPgetStationNum() : 0;
  }

  // Definir novo SSID do AP
  void setSSID(const String &newSSID)
  {
    ssid = newSSID;
  }

  // Definir nova senha do AP
  void setPassword(const String &newPassword)
  {
    password = newPassword;
  }

  // Definir novo canal
  void setChannel(uint8_t newChannel)
  {
    if (newChannel >= 1 && newChannel <= 13)
    {
      channel = newChannel;
    }
  }

  // Definir configurações do STA
  void setSTAConfig(const String &newSTASSID, const String &newSTAPassword, int index)
  {
    if (index >= 0 && index < 2)
    {
      staSSID[index] = newSTASSID;
      staPassword[index] = newSTAPassword;
      saveSTAConfig();
    }
  }

private:
  // Função para decodificação Base64
  size_t base64_decode(char *output, const char *input, size_t inputLen)
  {
    static const char *dec = ("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/");
    size_t outputLen = 0;
    for (size_t i = 0; i < inputLen; i += 4)
    {
      char a = strchr(dec, input[i]) - dec;
      char b = strchr(dec, input[i + 1]) - dec;
      char c = strchr(dec, input[i + 2]) - dec;
      char d = strchr(dec, input[i + 3]) - dec;
      output[outputLen++] = (a << 2) | (b >> 4);
      if (input[i + 2] != '=')
        output[outputLen++] = ((b & 15) << 4) | (c >> 2);
      if (input[i + 3] != '=')
        output[outputLen++] = ((c & 3) << 6) | d;
    }
    return outputLen;
  }
};
