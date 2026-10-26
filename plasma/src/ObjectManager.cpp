#include "plasma/ObjectManager.h"
#include "plasma/Object.h"

namespace plasma {

ObjectManager::~ObjectManager()
{
    m_destroying = true;
    for (std::list<Object *>::iterator it = m_objects.begin(); it != m_objects.end(); ++it)
        delete *it;
}

}
