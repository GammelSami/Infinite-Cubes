// plasma::Object: the root of the engine's object model. Every object registers itself with its engine on
// construction (Engine::AddObject keeps it in the object manager's list) and unregisters on destruction, unless
// the engine is tearing all objects down itself.
#pragma once
#include <list>
#include <string>
#include "plasma/ObjectManager.h"

namespace plasma {

class Engine;

class Object {
    friend class PLXStreamReader;
    friend class PLXStreamWriter;

public:
    explicit Object(Engine *engine);
    virtual ~Object();

    Engine *getEngine() const { return m_engine; }

protected:
    Engine *m_engine;
    std::list<Object *>::iterator m_managerEntry; // This object's node in the manager's list.
};

}

#include "plasma/Engine.h"
