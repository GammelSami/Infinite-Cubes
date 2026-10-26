// ServerUpdates holds thirteen event lists queued for transmission.
#pragma once
#include "cube/ParticleIds.h"
#include <list>
#include "cube/AggroPacket.h"
#include "cube/HitPacket.h"
#include "cube/BuffPacket.h"
#include "cube/BulletPacket.h"
#include "cube/KillPacket.h"
#include "cube/Zone.h"
#include "cube/Item.h"
#include "cube/MissionState.h"

namespace cube {

struct ServerUpdates {
    // Each list owns packets of its declared element type.

    // A positional sound with playback speed and volume; both default to one.
    struct SoundPacket {
        // Full pitch and volume.
        SoundPacket()
            : pitch(1.0f),
              volume(1.0f)
        {}

        plasma::Vector<3, float> position;
        int sound;
        float pitch;
        float volume;
    };
    // A particle burst with a trailing padding word.
    struct ParticlePacket {
        // Type 0, spread 3.
        ParticlePacket()
            : type(ParticleLitCube),
              spread(3.0f)
        {}

        plasma::Vector<3, Fixed> position;
        plasma::Vector<3, float> acceleration;
        plasma::Vector<4, float> color;
        float size;
        int count;
        int type;
        float spread;
    };
    // Copies the zone's changed-block list.
    typedef Zone::CubeChange BlockPacket;
    // A zone object update contains zone coordinates, object index and shared state.
    struct ZoneObjectPacket {
        // The client begins with object defaults and leaves the key unset.
        ZoneObjectPacket() {}

        ZoneObjectPacket(const plasma::Vector<3, int> &key, const Zone::ObjectState &object)
            : key(key),
              object(object)
        {}

        plasma::Vector<3, int> key; // zone x, zone y, object index
        Zone::ObjectState object;
    };
    struct ItemPickupPacket {
        long long creatureId;
        Item item;
    };
    // A mission update contains the region-record coordinates and its mission state. Region requests return one for each of the region’s eight-by-eight records.
    struct MissionPacket {
        MissionPacket()
            : position(0, 0),
              creatureId(0),
              mission()
        {}

        plasma::Vector<2, int> position; // the record (zone * 8 + index)
        long long creatureId; // 0; the creature a started mission introduces (GameController::update talks to it)
        MissionState mission; // The region record mission state.
    };

    // These packets own a list of zone items or buildings.
    struct ItemListPacket {
        // Builds a packet from a zone list; the client starts from an empty list with unset zone coordinates.
        ItemListPacket() {}
        explicit ItemListPacket(const Zone &zone)
            : zone(zone.m_position),
              items(zone.m_items.begin(), zone.m_items.end())
        {}

        plasma::Vector<2, int> zone; // the zone's coordinates
        std::list<Zone::ZoneItem> items;
    };
    // The inner list carries Zone::Building records.
    struct BuildingListPacket {
        // Built from a zone: its position, then a list copied from the zone's m_buildings. World::update
        // sends one per zone of its second zone set. The client's receive thread reads one field by field.
        BuildingListPacket() {}
        explicit BuildingListPacket(const Zone &zone);
        plasma::Vector<2, int> zone; // the zone's coordinates
        std::list<Zone::Building> buildings;
    };

    ServerUpdates() {}
    ~ServerUpdates() {}

    std::list<HitPacket> hits;
    std::list<SoundPacket> sounds;
    std::list<ParticlePacket> particles;
    std::list<BlockPacket> blocks;
    std::list<BulletPacket> projectiles;
    std::list<ItemListPacket> items;
    std::list<ZoneObjectPacket> objects;
    std::list<BuildingListPacket> buildings;
    std::list<ItemPickupPacket> pickedUpItems;
    std::list<KillPacket> kills;
    std::list<AggroPacket> aggro;
    std::list<BuffPacket> buffs;
    std::list<MissionPacket> missions;
};

}
