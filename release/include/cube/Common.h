// Formats world coordinates for display.
#pragma once
#include <string>

namespace cube {

// A world coordinate in blocks... the value divided by 20 as decimal text with a '.' between groups of three digits
// ("0" below 20, a leading '-' for negative values).
std::wstring formatCoordinate(int value);
// Two coordinates as "x | y".
std::wstring formatCoordinate(int x, int y);

}
