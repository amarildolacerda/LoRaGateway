#ifndef LOGGER_H
#define LOGGER_H

#include "Arduino.h"
#include <stdarg.h>

enum class LogLevel
{
    ERROR,
    WARNING,
    RECEIVE,
    SEND,
    INFO,
    DEBUG,
    VERBOSE
};

class Logger
{
private:
    static const size_t MAX_LOG_LENGTH = 125;
#ifdef WIFI
    static std::function<void(const String &)> logCallback; // Apenas declaração
#endif
public:
#ifdef WIFI
    static void setLogCallback(std::function<void(const String &)> callback);
#endif
    static void info(const char *msg, ...);
    static void error(const char *msg, ...);
    static void debug(const char *msg, ...);
    static bool log(LogLevel level, const char *format, ...);
    static void warn(const char *msg, ...);
    static void hex(LogLevel level, const char *texto, const size_t len);

private:
    static bool vlog(const LogLevel level, const char *format, va_list args);
};

#endif