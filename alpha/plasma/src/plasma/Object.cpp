// Object is the base of engine-owned scene objects.
#include "plasma/Object.h"
#include "plasma/Engine.h"

namespace plasma {

Object::Object(Engine *engine)
    : m_engine(engine)
{
    m_managerEntry = engine->addObject(this);
}

Object::~Object()
{
    Engine *engine = m_engine;
    if (!engine->m_destroying)
        engine->m_objects.erase(m_managerEntry);
}

}
