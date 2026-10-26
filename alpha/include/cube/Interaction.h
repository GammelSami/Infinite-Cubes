// Queued interaction for a creature.
#pragma once
#include "cube/Item.h"

namespace cube {

// World::update handles them.
enum InteractionType : unsigned char {
    InteractionUseConsumable = 1,
    InteractionTalk = 2,
    InteractionUseObject = 3,
    InteractionPickUp = 5,
    InteractionDrop = 6,
    InteractionBuild = 7,
    InteractionCallCompanion = 8
};

// The producer supplies the interaction type byte.
struct Interaction {
    Interaction()
        : position(-1, -1, -1),
          unused124(0),
          unused12a(1)
    {}

    Item item;
    plasma::Vector<3, int> position; // zone x/y and object index
    int unused124;                   // zero in constructor; nothing reads it
    InteractionType type;
    short unused12a; // 1 in constructor; nothing reads it
};

}
