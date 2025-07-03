#include "texts.h"

namespace Texts
{

    String left(String original, int length, char fillChar)
    {
        // Verifica se precisa truncar
        if (original.length() >= length)
        {
            return original.substring(0, length);
        }

        String result;
        result.reserve(length); // Pré-aloca memória

        // Adiciona preenchimento
        for (int i = 0; i < length - original.length(); ++i)
        {
            result += fillChar;
        }

        // Adiciona texto original
        result += original;

        return result;
    }

    String center(String text, int width, char fillChar)
    {
        const int textLength = text.length();

        // Verifica se precisa truncar
        if (textLength >= width)
        {
            return text.substring(0, width);
        }

        String centered;
        centered.reserve(width); // Pré-aloca memória

        const int totalPadding = width - textLength;
        const int leftPadding = totalPadding / 2;
        const int rightPadding = totalPadding - leftPadding;

        // Preenchimento esquerdo
        for (int i = 0; i < leftPadding; ++i)
        {
            centered += fillChar;
        }

        // Texto central
        centered += text;

        // Preenchimento direito
        for (int i = 0; i < rightPadding; ++i)
        {
            centered += fillChar;
        }

        return centered;
    }
    String format(const char *format, ...)
    {
        char formattedMsg[255];
        va_list args;

        va_start(args, format);                                      // Inicializa va_list com o último parâmetro fixo
        vsnprintf(formattedMsg, sizeof(formattedMsg), format, args); // Formata a string
        va_end(args);                                                // Finaliza va_list

        return String(formattedMsg); // Retorna a String formatada
    }
    String humanizedNumber(unsigned long num)
    {
        char buffer[10];

        if (num >= 1000000)
        {
            snprintf(buffer, sizeof(buffer), "%.1fM", num / 1000000.0);
        }
        else if (num >= 1000)
        {
            snprintf(buffer, sizeof(buffer), "%.1fk", num / 1000.0);
        }
        else
        {
            snprintf(buffer, sizeof(buffer), "%lu", num);
        }

        return String(buffer);
    }
    String humanizedUptime(int32_t detailBefore)
    {
        unsigned long now = millis();
        unsigned long seconds = now / 1000;
        unsigned long minutes = seconds / 60;
        unsigned long hours = minutes / 60;

        seconds %= 60;
        minutes %= 60;

        char buffer[20]; // Buffer suficiente para todas as combinações

        if (hours > 0)
        {
            if (minutes > 0)
            {
                snprintf(buffer, sizeof(buffer), "%luh%02lum", hours, minutes);
            }
            else
            {
                snprintf(buffer, sizeof(buffer), "%luh", hours);
            }
        }
        else if (minutes >= detailBefore)
        {
            snprintf(buffer, sizeof(buffer), "%lumin", minutes);
        }
        else
        {
            // Mostra minutos e segundos quando menos de 10 minutos
            if (minutes > 0)
            {
                snprintf(buffer, sizeof(buffer), "%lum%02lus", minutes, seconds);
            }
            else
            {
                snprintf(buffer, sizeof(buffer), "%lus", seconds);
            }
        }

        return String(buffer);
    }

} // namespace Texts