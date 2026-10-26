// See cube/Common.h.
#include <stdlib.h>
#include <sstream>
#include "cube/Common.h"

namespace cube {

// Shared noise gradient tables.
unsigned char perm[512];
unsigned char permMod12[512];

std::wstring formatCoordinate(int value)
{
    bool negative = value < 0;
    int rest = abs(value) / 20;
    if (!rest)
        return L"0";
    std::wstring text;
    bool first = true;
    do {
        if (!first)
            text = L"." + text;
        first = false;
        std::wstringstream stream;
        if (rest / 1000) {
            stream.width(3);
            stream.fill(L'0');
        }
        stream << rest % 1000;
        text = stream.str() + text;
        rest /= 1000;
    } while (rest);
    if (negative)
        return L"-" + text;
    return text;
}

std::wstring formatCoordinate(int x, int y)
{
    return formatCoordinate(x) + L" | " + formatCoordinate(y);
}

}
