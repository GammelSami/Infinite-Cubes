#pragma once
#include "cube/Creature.h"

namespace cube {
// A buff message containing two creature identifiers followed by the buff payload.
struct BuffPacket {
    long long sourceId;
    long long targetId;
    Creature::Buff buff;
};
}
