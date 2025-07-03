#ifndef TEXTS_H
#define TEXTS_H

#include <Arduino.h>

namespace Texts
{

    /**
     * @brief Alinha texto à esquerda com preenchimento
     * @param original Texto original
     * @param length Comprimento total desejado
     * @param fillChar Caractere de preenchimento (padrão: espaço)
     * @return String formatada
     */
    String left(String original, int length, char fillChar = ' ');

    /**
     * @brief Centraliza texto com preenchimento
     * @param text Texto original
     * @param width Largura total desejada
     * @param fillChar Caractere de preenchimento (padrão: espaço)
     * @return String centralizada
     */
    String center(String text, int width, char fillChar = ' ');

    String format(const char *format, ...);
    String humanizedNumber(unsigned long num);
    String humanizedUptime(int32_t detailBefore = 60);

} // namespace Texts

#endif // TEXTS_H