// WorldInfo describes one saved world in the selection screen: name, seed, explored area and a preview sprite. Game
// owns these records and stores them in the worlds database.
#pragma once
#include <string>
#include "cube/Sprite.h"

namespace cube {

struct WorldInfo {
    WorldInfo()
        : m_preview(nullptr),
          m_exploredArea(0)
    {}
    virtual ~WorldInfo() { delete m_preview; }

    Sprite *m_preview;
    std::string m_name; // Converted text of Interface's worldName edit field.
    int m_seed;         // the worldSeed node's text parsed as int; passed to Game with the name
    int m_exploredArea; // Initially zero; saved after the seed and displayed by WorldPreviewWidget as explored area in square kilometres.
};

}
