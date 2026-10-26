// WorldInfo holds a saved-world name, seed, explored area and preview model for the selection
// screen. Destruction releases the preview and name.
#pragma once
#include <string>
#include "cube/Sprite.h"

namespace cube {

struct WorldInfo {
    WorldInfo()
        : m_preview(0),
          m_exploredArea(0)
    {}
    virtual ~WorldInfo() { delete m_preview; }

    Sprite *m_preview; // owned
    std::string m_name;
    int m_seed;         // not set by the constructor
    int m_exploredArea; // 0
};

}
