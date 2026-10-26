// ServerUpdates groups the world events of one tick into lists of hits, sounds, particles, block changes, projectiles,
// zone contents, pickups, buffs and chat lines. World accumulates generated events, Game keeps received events and
// each Connection queues events for its client.
#pragma once
#include "cube/ServerUpdateIds.h"
#include <string.h>
#include <list>
#include <string>
#include "plasma/Vector.h"
#include "cube/Block.h"
#include "cube/Fixed.h"
#include "cube/Creature.h"
#include "cube/Item.h"
#include "cube/Zone.h"

namespace cube {

// The protocol version, sent with an invitation and compared by the receiver.
static const int PROTOCOL_VERSION = 100001;

struct ChatMessage {
    long long creatureId;
    std::wstring text;
};

class ServerUpdates {
public:
    // Element types.

    struct HitPacket {
        HitPacket()
            : damage(0),
              critical(0),
              stunTime(0),
              actionLockTime(0),
              position(0, 0, 0),
              direction(0.0f, 0.0f, 0.0f),
              powered(0),
              hitType(HitTypeDamage),
              supportRayLight(0)
        {}

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
        // Presentation flag; does not describe damage semantics.
        unsigned char supportRayLight; // is padding (not copied)
    };

    // A sound played at a position.
    struct SoundPacket {
        SoundPacket()
            : speed(1.0f),
              volume(1.0f),
              sourceId(0)
        {}

        plasma::Vector<3, Fixed> position;
        int sound;
        float speed;
        float volume;
        long long sourceId;
    };

    // A particle burst.
    struct ParticlePacket {
        ParticlePacket()
            : type(ParticleLitCube),
              spread(3.0f)
        {}

        plasma::Vector<3, Fixed> position;
        plasma::Vector<3, float> acceleration;
        plasma::Vector<4, float> color;
        float size;
        int count;
        ParticleType type;
        float spread;
    };

    // A block changed in the world: cube::CubeChange (cube/Zone.h), the element of a zone's own change list too.
    typedef cube::CubeChange CubeChange;

    // Projectile event record.
    struct BulletPacket {
        BulletPacket()
            : ownerId(-1),
              targetPosition(0, 0, 0),
              unused38(0),
              unused3c(0),
              unused40(0),
              sizeInBlocks(0.5f),
              visualSize(1.0f),
              knockback(1.0f),
              strength(0),
              powered(0),
              projectileType(ProjectileArrow),
              element(ElementPhysical),
              currentTime(0),
              timeToLive(5000),
              slowedTime(0)
        {}

        long long ownerId;
        plasma::Vector<3, Fixed> position;
        plasma::Vector<3, Fixed> targetPosition;
        int unused38;
        int unused3c;
        int unused40;
        plasma::Vector<3, float> velocity;
        float damage;
        float sizeInBlocks;
        float visualSize;
        float knockback;
        float
            strength; // the shot's strength (World::performAbility: the strike strength or the mana it cost); 0 for spells and orbs; no reader found
        bool powered;
        ProjectileType projectileType;
        unsigned char element; // a DamageElement (not HitPacket's HitType)
        float currentTime;
        int timeToLive;
        int slowedTime; // is padding (not copied)
    };

    // A zone's coordinates and its items.
    struct ItemListPacket {
        ItemListPacket() {}
        explicit ItemListPacket(Zone &zone)
            : zone(zone.m_position),
              items(zone.m_items.begin(), zone.m_items.end())
        {}

        plasma::Vector<2, int> zone;
        std::list<ZoneItem> items;
    };

    // A zone's coordinates and its static entities.
    struct Packet6 {
        Packet6() {}
        explicit Packet6(Zone &zone)
            : zone(zone.m_position),
              objects(zone.m_staticEntities.begin(), zone.m_staticEntities.end())
        {}

        plasma::Vector<2, int> zone;
        std::list<StaticEntity> objects;
    };

    // An item picked up.
    struct ItemPickupPacket {
        ItemPickupPacket()
            : quantity(1),
              flags(0)
        {}

        long long creatureId;
        Item item;
        unsigned short quantity;
        plasma::Vector<3, Fixed> position; // Block coordinates obtained from fixed-point positions.
        int flags;
    };

    struct KillPacket {
        long long killerId;
        long long targetId;
    };

    // These members are never read.
    struct PassivePacket {
        long long creatureId;
        unsigned char unused08;
        unsigned char unused09;
        // Two values whose meaning and numeric representation are unknown.
        plasma::Vector<2, int> unused0c;
        long long unused18;
        int unused20;
        int unused24;
    };

    struct AggroPacket {
        long long attackerId;
        long long creatureId;
        float aggro; // Floating-point map value stored with the event.
    };

    // A buff given by one creature to another.
    struct BuffPacket {
        BuffPacket()
            : replace(0)
        {}

        long long sourceId;
        long long targetId;
        // The buff as Creature::setBuff takes it.
        Creature::Buff buff;
        unsigned char replace;
        int unused2c;
    };

    std::list<HitPacket> m_hitPacketList;
    std::list<SoundPacket> m_soundPacketList;
    std::list<ParticlePacket> m_particlePacketList;
    std::list<CubeChange> m_cubeChangeList;
    std::list<BulletPacket> m_bulletPacketList;
    std::list<ItemListPacket> m_itemListPacketList;
    // The static entities of zones whose entities changed, one Packet6 per zone.
    std::list<Packet6> m_staticEntityPacketList;
    // The zones whose contents this update delivers: Host::addServerUpdates appends each zone a player asked for after
    // its changes, items and entities, and Game::update clears the zone's m_awaitingData.
    std::list<plasma::Vector<2, int>> m_deliveredZoneList;
    std::list<ItemPickupPacket> m_itemPickupPacketList;
    std::list<KillPacket> m_killPacketList;
    std::list<PassivePacket> m_passivePacketList;
    std::list<AggroPacket> m_aggroPacketList;
    std::list<BuffPacket> m_buffPacketList;
    std::list<int> m_worldEventList;
    std::list<long long> m_talkingCreatureList;
    // Neither Host nor Client sends this list.
    std::list<std::wstring> m_systemMessageList;
};

}
