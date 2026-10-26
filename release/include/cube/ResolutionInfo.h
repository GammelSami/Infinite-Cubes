// cube::ResolutionInfo: a display resolution and the refresh rates the display offers for it.
#pragma once
#include <array>
#include <set>
#include <utility>

namespace cube {

typedef std::pair<std::array<int, 2>, std::set<int>> ResolutionInfo;

}
