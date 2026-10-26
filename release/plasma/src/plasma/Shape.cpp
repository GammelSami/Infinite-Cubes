// Registers shapes with the engine and tracks their users.
#include "plasma/Shape.h"
#include "plasma/Engine.h"
#include "plasma/EditFilter.h"

namespace plasma {

Shape::Shape(Engine *engine, const std::wstring &name)
    : Keyable(engine, name),
      m_type(SHAPE_UNTYPED),
      m_unused90(true),
      m_unused91(false)
{
    m_engineEntry = engine->addShape(this);
}

Shape::~Shape() {}

}
