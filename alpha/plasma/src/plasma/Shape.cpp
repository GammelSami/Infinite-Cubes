// Shape defines drawing, bounds and hit testing for scene geometry.
#include "plasma/Shape.h"
#include "plasma/Engine.h"

namespace plasma {

Shape::Shape(Engine *engine, const std::wstring &name)
    : Keyable(engine, name),
      m_type(SHAPE_UNTYPED),
      m_unused58(true),
      m_unused59(false)
{
    m_engineEntry = engine->addShape(this);
}

Shape::~Shape() {}

}
