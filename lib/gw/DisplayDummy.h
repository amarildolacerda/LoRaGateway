#ifndef TTGO_DISPLAY_H
#define TTGO_DISPLAY_H

#ifdef DISPLAYNONE
#include "DisplayInterface.h"

class DisplayDummy : public DisplayInterface
{
private:
    void showFooter()
    {
    }

public:
    bool initialize() override
    {
        return true;
    }
    void setPos(const uint8_t row, const uint8_t col) override {}
    void print(const String msg) override {}
    void println(const String msg) override {}
    void clearDisplay() override {}
    void display() override {}
    void setTextColor(uint8_t color, uint8_t fundo) override {}
    void setTextSize(uint8_t tam) override {}
};

#endif
#endif // TTGO_DISPLAY_H