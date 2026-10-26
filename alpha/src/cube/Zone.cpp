// Zone owns terrain columns and the generated contents of a zone.
#include "cube/Zone.h"

#include "cube/House.h"
#include "cube/Spawn.h"

namespace cube {

Zone::Zone()
    : m_position(0, 0),
      m_needsServerUpdate(true),
      m_modified(false),
      m_objectsChanged(false),
      m_baseSpawnCount(0),
      m_populationDays(0)
{
    InitializeCriticalSectionAndSpinCount(&m_lock, 0x80000400);
    m_fields = new Field[0x10000];
    m_cells = new std::list<CellRecord>[0x400];
}

Zone::~Zone()
{
    delete[] m_fields;
    delete[] m_cells;
    for (std::vector<Spawn *>::iterator it = m_spawns.begin(); it != m_spawns.end(); ++it)
        delete *it;
    for (std::vector<Airship *>::iterator it = m_airships.begin(); it != m_airships.end(); ++it)
        delete *it;
    for (std::vector<House *>::iterator it = m_houses.begin(); it != m_houses.end(); ++it)
        delete *it;
    for (std::vector<Record94 *>::iterator it = unused94.begin(); it != unused94.end(); ++it)
        delete *it;
    DeleteCriticalSection(&m_lock);
}

}
