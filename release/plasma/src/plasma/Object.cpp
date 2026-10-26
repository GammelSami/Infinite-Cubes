#include "plasma/Object.h"
#include "plasma/Engine.h"
#include "plasma/EditFilter.h"

namespace plasma {

Object::Object(Engine *engine)
    : m_engine(engine)
{
    m_managerEntry = engine->addObject(this);
}

Object::~Object()
{
    // While the engine deletes all its objects itself, it also empties the list; otherwise this object leaves it.
    Engine *engine = m_engine;
    if (!engine->m_destroying)
        engine->m_objects.erase(m_managerEntry);
}

}
