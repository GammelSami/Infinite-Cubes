#pragma once
#include "cube/BytesIO.h"
#include "cube/MissionState.h"
#include "cube/MonsterState.h"

namespace cube {
class Zone;
class World;
class Creature;

// Versioned save data: a stream and the format version written (1) and read. The methods are
// ordinary functions of Serializer.cpp, one file of both projects (see there).
class Serializer {
public:
    // Stores the stream, current version one and initial loaded version zero.
    explicit Serializer(BytesIO *stream);
    // Short reads keep the old field (BytesIO::read). load/save as loadCreature/saveCreature: in both
    // executables these sort before BytesIO::read and setBlock as only loadsave* names do (readwrite*
    // cannot).
    void loadMission(MissionState &mission);
    void loadMonster(MonsterState &monster);
    void saveMission(const MissionState &mission);
    void saveMonster(const MonsterState &monster);

    void loadCreature(Creature *creature);
    void saveCreature(Creature *creature);

    void loadZone(World *world, Zone *zone);

    // Writes the zone's saved changes.
    void saveZone(const Zone *zone);

    BytesIO *m_stream;
    int m_version;
    int m_loadedVersion;
};
}
