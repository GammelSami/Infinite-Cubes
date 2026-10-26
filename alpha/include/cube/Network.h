// The entity-data wire format: a 64-bit mask of the members that follow, then each flagged
// member's bytes, in member order (decodeEntityData/encodeEntityData, Creature/Network.cpp); both
// directions are defined in Network.cpp.
#pragma once
#include "cube/BytesIO.h"
#include "cube/Creature.h"

namespace cube {

// Reads the field mask and selected fields from a decompressed entity update, returning the packet
// size.
int readEntityData(BytesIO &data, Creature::EntityData &entityData);

// Appends an entity-data update with the fields that differ from the previous state, or all of
// them; returns data's new size.
int writeEntityData(BytesIO &data, const Creature::EntityData &previous, const Creature::EntityData &current,
                    bool full);

}
