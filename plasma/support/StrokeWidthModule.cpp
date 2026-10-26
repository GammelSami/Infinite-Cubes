// Widens byte strings character by character for text modules and Windows system helpers.
#include "SpecialColorModule.h"
#include <stdlib.h>
#include <string.h>
#include "svg/SVGReader.h"

std::wstring widenString(const std::string &text)
{
    return std::wstring(text.begin(), text.end());
}
