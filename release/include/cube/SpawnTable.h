// SpawnTable selects groups of creature races that appear together in the wild according to terrain. World owns the
// table and supplies its generation context.
#pragma once
#include <vector>

namespace cube {

class World;

class SpawnTable {
public:
    struct Group {
        std::vector<int> creatureTypes;
        int sizeFlags;
        int terrainFlags;
        int locationFlags;
    };

    explicit SpawnTable(World *world);
    bool chooseGroup(int x, int y, bool special, Group &group);

    World *m_world;
    // Creature power class; its use is not otherwise known.
    std::vector<int> m_powers;
    std::vector<Group> m_groups; // the constructor's 120 groups
    // Nothing fills or reads it.
    std::vector<Group> m_unusedGroups;
};

}
