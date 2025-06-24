#ifndef DISPLAY_INTERFACE_H
#define DISPLAY_INTERFACE_H

#include <Arduino.h>

class DisplayInterface
{
public:
    virtual ~DisplayInterface() {}

    // Métodos básicos
    virtual bool initialize() = 0;
    virtual void setPos(const uint8_t row, const uint8_t col) = 0;
    virtual void print(const String msg) = 0;
    virtual void println(const String msg) = 0;
    virtual void clearDisplay() = 0;
    virtual void show() = 0;
    virtual void setTextColor(uint8_t color, uint8_t fundo) = 0;
    virtual void setTextSize(uint8_t tam) = 0;
    virtual void fillRect(int x1, int y1, int x2, int y2) {}
    virtual void setColor(uint8_t color) {};
};

#endif // DISPLAY_INTERFACE_H