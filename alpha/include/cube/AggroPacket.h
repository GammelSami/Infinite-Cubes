#pragma once

namespace cube {

// Reports a creature's aggro towards another. World::onAttack queues it for the attacked creature and alerted allies. The final member is not set there.
struct AggroPacket {
    long long creatureId; // the creature whose aggro changed
    long long targetId;   // the creature it is directed at
    float aggro;          // the new value
    int unused14;         // never written or read (the packet's 8-byte alignment)
};

}
