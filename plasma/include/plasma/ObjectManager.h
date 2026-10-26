// plasma::ObjectManager: owns a list of engine objects and deletes them when it is destroyed (the base of
// plasma::Engine).
#pragma once
#include <list>

namespace plasma {

class Object;

class ObjectManager {
public:
    ObjectManager()
        : m_destroying(false)
    {}
    virtual ~ObjectManager();

    bool isDestroying() const { return m_destroying; }

protected:
    friend class Object; // Object::~Object unregisters itself unless m_destroying.
    bool m_destroying;   // Set while the destructor deletes the objects.
    std::list<Object *> m_objects;
};

}
