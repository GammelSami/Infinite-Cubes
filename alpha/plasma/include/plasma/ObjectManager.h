// ObjectManager assigns identifiers and tracks engine objects.
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
    friend class Object;
    bool m_destroying;
    std::list<Object *> m_objects;
};

}
