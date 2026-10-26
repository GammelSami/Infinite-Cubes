// KeyNames provides full keyboard key names and abbreviations by DirectInput scan code, plus names for mouse buttons
// and unassigned bindings.
#pragma once
#include <map>
#include <string>
#include "cube/Controls.h"

namespace cube {

class KeyNames {
public:
    KeyNames();
    // The full name of a binding's key: "Unassigned" for an empty binding (code 0, device 0), "Mouse N" for mouse
    // button N - 1 (device 1), "Unknown" for a key without a name.
    std::wstring getFullName(const Controls::Button &button);
    // The abbreviation of a binding's key: "?" for an empty binding or a key without a name, "MN" for mouse button N -
    // 1.
    std::wstring getShortName(const Controls::Button &button);

    std::map<int, std::wstring> m_fullNames;
    std::map<int, std::wstring> m_abbreviations;
};

}
