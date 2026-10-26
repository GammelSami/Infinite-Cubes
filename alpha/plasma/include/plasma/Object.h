// Object is the base of engine-owned scene objects.
#pragma once
#include <list>
#include <string>
#include "plasma/ObjectManager.h"

namespace plasma {

class Engine;

// PLA/PLX references map this id to null; transient textures retain it.
enum ObjectId {
    NO_OBJECT_ID = -1
};

class Object {
    friend class PLXStreamReader;
    friend class PLXStreamWriter;

public:
    explicit Object(Engine *engine);
    virtual ~Object();

    Engine *getEngine() const { return m_engine; }

protected:
    Engine *m_engine;
    std::list<Object *>::iterator m_managerEntry;
};

}
