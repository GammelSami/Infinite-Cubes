#pragma once
#include "cube/Fixed.h"
#include "plasma/Vector.h"

namespace cube {

// 1 gives the target a quarter of its mana; 3, 4 and 5 have
enum HitType : unsigned char {
    HitTypeDamage = 0,
    HitTypeBlock = 1,
    HitTypeMiss = 3,
    HitTypeDodge = 4,
    HitTypeMana = 5,
    HitTypeTameness = 6 // sent when a pet food is fed, no damage
};

// Hit message exchanged between clients and server. Its producer supplies damage and creature identifiers.
struct HitPacket {
    HitPacket()
        : critical(0),
          stunTime(0),
          actionLockTime(0)
    {
        position[0].raw = 0;
        position[1].raw = 0;
        position[2].raw = 0;
        direction[0] = 0.0f;
        direction[1] = 0.0f;
        direction[2] = 0.0f;
        powered = 0;
        hitType = HitTypeDamage;
        supportRayLight = 0;
    }

    long long attackerId;
    long long targetId;
    float damage;
    unsigned char critical;
    int stunTime;
    int actionLockTime;
    plasma::Vector<3, Fixed> position;
    plasma::Vector<3, float> direction;
    unsigned char powered;
    HitType hitType;
    unsigned char supportRayLight;
};
}
