# bombaIO
/*


# **Documentação do Projeto: Transmissor/Terminal**
## **Objetivo**
Este projeto implementa um transmissor usando a placa **TTGO LoRa32**, capaz de:
1. Enviar comandos via LoRa para um receptor (ex.: LIGAR, DESLIGAR, STATUS, REVERTER).
2. Solicitar automaticamente o status do receptor a cada minuto.
3. Integrar com a plataforma **Tuya IoT** para controle remoto e pareamento do dispositivo.
4. Exibir o status da última transmissão em um **display OLED** embutido.

---

## **Componentes do Sistema**
### **Hardware**
- **TTGO LoRa32**: Placa principal com WiFi, LoRa e display OLED integrado.
- **Botões físicos**:
  - `BUTTON_LIGAR` (GPIO0): Envia comando "LIGAR".
  - `BUTTON_STATUS` (GPIO35): Solicita o status atual do receptor.
  - `BUTTON_REVERTER` (GPIO34): Envia comando "REVERTER".
  - `BUTTON_CONFIG` (GPIO13): Inicia o emparelhamento com Tuya IoT.
  Os botões fisicos propostos não serão implementados na versão inicial do projeto, mas estão disponíveis para futuras implementações.
- **Display OLED SSD1306**: Exibe mensagens de status e informações do sistema.

### **Software**
- **Linguagem**: C++ (com framework Arduino) com platformio.
- **Bibliotecas**:
  - `SPI` e `LoRa`: Comunicação LoRa.
  - `WiFiManager`: Configuração WiFi.
  - `Adafruit_GFX` e `Adafruit_SSD1306`: Controle do display OLED.
  - `time.h`: Sincronização de horário via NTP.
  - `Tuya_WiFi_MCU_SDK`: Integração com Tuya IoT.
  - `ArduinoJson`: Criação e processamento de mensagens JSON.

---

## **Características Funcionais**
1. **Transmissão de Comandos**
   - Comandos suportados:
     - **"LIGAR"**: Aciona o relé do receptor.
     - **"DESLIGAR"**: Desliga o relé do receptor.
     - **"STATUS"**: Solicita o estado atual do relé.
     - **"REVERTER"**: Alterna o estado atual do relé.
   - Mensagens enviadas no formato JSON via LoRa.

2. **Solicitação Automática de Status**
   - A cada 5 minuto, o transmissor solicita automaticamente o status ao receptor.
   - O receptor deve responder com o estado atual do relé sempre que for alterado ou solicitado.

3. **Emparelhamento Tuya IoT**
   - Com `BUTTON_CONFIG` pressionado, o dispositivo entra no modo de emparelhamento e pode ser reconhecido pela plataforma Tuya.

4. **Exibição no Display OLED**
   - Informações como inicialização, erros, comandos enviados e status são exibidas no display.

5. **Sincronização Dinâmica de Hora**
   - Sincronização com servidor NTP para gerar timestamps dinâmicos no formato **ISO 8601**.

---

## **Configuração do Sistema**
### **Estrutura do Projeto**
```
Projeto/
├── src/
│   ├── main.cpp          # Código principal
├── platformio.ini        # Configuração do PlatformIO
```

### **Configuração do `platformio.ini`**
```ini
[env:TRANS]
platform = espressif32
board = ttgo-lora32-v1

monitor_speed = 115200
framework = arduino
; change microcontroller
board_build.mcu = esp32

; change MCU frequency
board_build.f_cpu = 240000000L
build_flags = -D TRANSMISSOR
lib_deps =
    sandeepmistry/LoRa @ ^0.8.0
    bblanchon/ArduinoJson @ ^6.21.2
    tzapu/WiFiManager@^2.0.17
    https://github.com/tuya/tuya-wifi-mcu-sdk-arduino-library.git
    adafruit/Adafruit GFX Library@^1.12.1
    adafruit/Adafruit SSD1306@^2.5.13


[env:RECP]
platform = espressif8266
board = nodemcu
framework = arduino
build_flags = -D RECEPTOR
lib_deps =
    sandeepmistry/LoRa @ ^0.8.0
    bblanchon/ArduinoJson @ ^6.21.2


    ```

---

## **Fluxo de Inicialização**
1. **Display OLED**:
   - Mostra mensagens de status como "Inicializando..." e "WiFi conectado".
2. **Configuração WiFi**:
   - Usa `WiFiManager` para permitir configuração fácil da rede WiFi.
3. **Sincronização com NTP**:
   - Sincroniza o relógio interno com um servidor NTP para gerar timestamps precisos.
4. **Inicialização LoRa**:
   - Configura os pinos e a frequência do módulo LoRa.
5. **Pareamento Tuya (Opcional)**:
   - Quando o botão `BUTTON_CONFIG` é pressionado durante a inicialização, o dispositivo entra no modo de pareamento.

---

## **Formato das Mensagens**
As mensagens enviadas via LoRa seguem o formato JSON:
```json
{
  "device_id": "transmitter_01",
  "command": "STATUS",
  "timestamp": "2025-04-28T22:15:00Z"
}
```

- **`device_id`**: Identificação única do transmissor.
- **`command`**: Comando enviado (ex.: LIGAR, DESLIGAR, STATUS, REVERTER).
- **`timestamp`**: Carimbo de data e hora gerado dinamicamente via NTP.

---

## **Funções Principais**
### **1. `inicializarDisplay()`**
Inicializa o display OLED e exibe uma mensagem inicial.
### **2. `inicializarWiFi()`**
Configura a rede WiFi usando `WiFiManager`. Exibe mensagem no OLED se conectado com sucesso.
### **3. `inicializarNTP()`**
Sincroniza a hora do dispositivo com um servidor NTP.
### **4. `inicializarLoRa()`**
Configura o módulo LoRa com os pinos GPIO e a frequência especificada.
### **5. `verificarConfiguracaoTuya()`**
Verifica se o botão de configuração está pressionado e entra no modo de pareamento Tuya.
### **6. `enviarComando()`**
Envia mensagens no formato JSON via LoRa e exibe o status no display.
### **7. `getTimestamp()`**
Gera o timestamp atual em formato **ISO 8601** usando a biblioteca `time.h`.

---

## **Testes Recomendados**
### **Testar o Display OLED**
- Confirme se as mensagens aparecem corretamente no OLED.
- Simule comandos para verificar as mensagens exibidas.
### **Testar LoRa**
- Envie comandos como "LIGAR" e "STATUS" e verifique a resposta do receptor.
### **Testar Emparelhamento Tuya**
- Pressione o botão `BUTTON_CONFIG` na inicialização e emparelhe o dispositivo na plataforma Tuya IoT.
### **Testar Sincronização de Hora**
- Verifique no serial monitor se a hora é sincronizada corretamente com o servidor NTP.

---

## **Observações**
1. **Gerenciamento de Energia**:
   - Certifique-se de que a alimentação para o TTGO LoRa32 seja estável, especialmente ao usar o display e o LoRa simultaneamente.
2. **Segurança**:
   - Use identificadores únicos no `device_id` para evitar interferências de transmissores não autorizados.
3. **Customização**:
   - Ajuste os pinos de GPIO e a frequência LoRa conforme necessário para o seu hardware.

---

Com este guia e o código, você pode configurar e testar o transmissor TTGO LoRa32 de forma eficaz. Se precisar de mais detalhes ou ajustes, é só me avisar!*/
