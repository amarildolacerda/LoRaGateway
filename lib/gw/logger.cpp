#include "logger.h"

#ifdef WIFI
// Definição da variável estática
std::function<void(const String &)> Logger::logCallback = nullptr;

void Logger::setLogCallback(std::function<void(const String &)> callback)
{
    logCallback = callback;
}
#endif

// Implementações dos outros métodos
void Logger::info(const char *msg, ...)
{
    va_list args;
    va_start(args, msg);
    vlog(LogLevel::INFO, msg, args);
    va_end(args);
}

void Logger::error(const char *msg, ...)
{
    va_list args;
    va_start(args, msg);
    vlog(LogLevel::ERROR, msg, args);
    va_end(args);
}
void Logger::debug(const char *msg, ...)
{
    va_list args;
    va_start(args, msg);
    vlog(LogLevel::DEBUG, msg, args);
    va_end(args);
}
void Logger::warn(const char *msg, ...)
{
    va_list args;
    va_start(args, msg);
    vlog(LogLevel::WARNING, msg, args);
    va_end(args);
}
bool Logger::log(const LogLevel level, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    bool result = vlog(level, format, args);
    va_end(args);
    return result;
}

// Implemente todos os outros métodos aqui...
bool Logger::vlog(const LogLevel level, const char *format, va_list args)
{
    char formattedMsg[MAX_LOG_LENGTH];
    vsnprintf(formattedMsg, MAX_LOG_LENGTH, format, args);

    static const char levelStrings[][7] PROGMEM = {
        "[ERRO]", "[WARN]", "[RECV]", "[SEND]", "[INFO]",
        "[DBUG]",
        "[VERB]"};

    static const char colorCodes[][8] PROGMEM = {
        "\033[31m", // ERRO - Red
        "\033[34m", // WARN - Blue
        "\033[35m", // RECEIVE - Magenta
        "\033[36m", // SEND - Cyan
        "\033[32m", // INFO - Green
        "\033[33m", // DBUG - Yellow
        "\033[37m"  // VERB - White
    };

    int idx = static_cast<int>(level);
    char levelBuffer[7];
    char colorBuffer[8];
    strcpy_P(levelBuffer, levelStrings[idx]);
    strcpy_P(colorBuffer, colorCodes[idx]);

#if defined(ESP8266) && defined(RF95)
// nao pode usar a Serial, é usada para o RF95
#else
    Serial.print(colorBuffer);
    Serial.print(levelBuffer);
#ifdef TIMESTAMP
    Serial.print(F(" "));
    Serial.print(getISOHour());
#endif
    Serial.print(F(" "));
    Serial.println(formattedMsg);
    Serial.print(F("\033[0m"));
#endif

#ifdef WIFI
    if (logCallback)
    {
        char msg[255] = {0};

        sprintf(msg, "%s %s", levelBuffer, formattedMsg);
        logCallback(msg);
    }
#endif

    return true;
}

void Logger::hex(LogLevel level, const char *texto, const size_t len)
{
    String resultado = "";
    for (int i = 0; i < len; i++)
    {
        char c = texto[i];
        if (c < 16)
            resultado += "0"; // Garante dois dígitos
        resultado += String(c, HEX);
        resultado += " ";
    }
    resultado.trim();
    Logger::log(level, resultado.c_str()); // Remove espaço no final
}
