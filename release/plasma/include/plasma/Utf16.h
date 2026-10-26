// Text stored as UTF-16 code units: Plasma files and the chat packets. On Windows wchar_t is UTF-16 and these only copy;
// with a 32-bit wchar_t (Linux), surrogate pairs become one character and are split again.
#pragma once
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace plasma {

// The characters of count UTF-16 units, zero units included.
inline std::wstring fromUtf16(const uint16_t *units, size_t count)
{
    std::wstring text;
    text.reserve(count);
    for (size_t i = 0; i < count; i++) {
        uint32_t unit = units[i];
        if (sizeof(wchar_t) == 4 && unit >= 0xD800 && unit < 0xDC00 && i + 1 < count && units[i + 1] >= 0xDC00 &&
            units[i + 1] < 0xE000) {
            unit = 0x10000 + ((unit - 0xD800) << 10) + (units[i + 1] - 0xDC00);
            i++;
        }
        text += (wchar_t)unit;
    }
    return text;
}

// The UTF-16 units of text.
inline std::vector<uint16_t> toUtf16(const std::wstring &text)
{
    std::vector<uint16_t> units;
    units.reserve(text.size());
    for (wchar_t character : text) {
        uint32_t code = (uint32_t)character;
        if (sizeof(wchar_t) == 4 && code >= 0x10000) {
            code -= 0x10000;
            units.push_back((uint16_t)(0xD800 + (code >> 10)));
            units.push_back((uint16_t)(0xDC00 + (code & 0x3FF)));
        } else {
            units.push_back((uint16_t)code);
        }
    }
    return units;
}

}
