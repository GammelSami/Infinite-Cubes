#pragma once
#include <string>

// Shared narrow/wide conversion helpers used by the scene and font readers.
std::wstring widenString(const std::string &text);
std::string narrowString(const std::wstring &text);
