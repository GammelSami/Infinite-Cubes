#pragma once
#include "cube/Fixed.h"
#include "plasma/Vector.h"

namespace cube {

// World::update moves them by type: arrows fall and stop at the first target, magic bursts (hitting
// all within five blocks; the magical flag of attackCreature), the boomerang returns to its owner
// after 0.8 s, a cloud lingers (a water spell turns into one). ThrowRock produces type 4; render
// uses the iron-deposit model for the rock.
enum ProjectileType {
    ProjectileArrow = 0,
    ProjectileMagic = 1,
    ProjectileBoomerang = 2,
    ProjectileCloud = 3,
    ProjectileRock = 4
};
// BulletPacket:element: water heals the owner's allies instead of hurting (and sounds of a water
// splash); a fire spell may give buff 9.
enum ProjectileElement {
    ElementFire = 1,
    ElementWater = 2
};

// A projectile sent between the server and clients.
struct BulletPacket {
    BulletPacket()
        : ownerId(-1),
          home(-1, -1, 0),
          unused30(0),
          unused34(0),
          unused38(0),
          sizeInBlocks(0.5f),
          visualSize(1.0f),
          knockback(1.0f),
          strength(0),
          powered(0),
          projectileType(ProjectileArrow),
          element(0),
          currentTime(0),
          slowedTime(0)
    {}

    long long ownerId;
    // The shooter's spawn slot: zone coordinates and spawn index, initially (-1, -1, 0).
    plasma::Vector<3, int> home;
    plasma::Vector<3, Fixed> position;
    // Unused words, initially zero and copied with the packet.
    int unused30;
    int unused34;
    int unused38;
    plasma::Vector<3, float> velocity;
    float damage;
    float sizeInBlocks;
    float visualSize;
    float knockback;
    float strength;
    unsigned char powered;
    ProjectileType projectileType;
    unsigned char element;
    int currentTime; // milliseconds since it was fired (World::update)
    int slowedTime;
};
}
