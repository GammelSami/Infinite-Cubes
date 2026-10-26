// Constructors for named engine objects.
#include "plasma/NamedObject.h"
#include "plasma/EditFilter.h"

namespace plasma {

NamedObject::NamedObject(Engine *engine, const std::wstring &name)
    : Object(engine),
      m_name(name)
{}

NamedObject::NamedObject(const NamedObject &other)
    : Object(other.m_engine),
      m_name(other.m_name)
{}

}
