// Serializer reads and writes character saves and world state through a BytesIO buffer. Character sections depend on
// the loaded format version; world state has its own version.
#pragma once
#include "cube/BytesIO.h"
#include "cube/Item.h"
#include "cube/World.h"

namespace cube {

class Serializer {
public:
    Serializer(BytesIO *stream);

    // Reads and writes a character and a world's state.
    void loadCreature(Creature *creature);
    bool readItem(Item &item);
    void saveCreature(Creature *creature);
    void loadWorldState(World::WorldState &state);
    void saveWorldState(const World::WorldState &state);
    void writeItem(const Item &item);

    BytesIO *m_stream;
    int m_version;       // Character format version, currently 11.
    int m_loadedVersion; // the format version of the character being read; sections depend on it
    int m_stateVersion;  // World-state format version, currently 44; written and checked at the start of the stream.
    int m_unused14;      // 0 (constructor); no other access
    int m_unused18;      // 0 (constructor); no other access
};

}
