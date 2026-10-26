#include "plasma/GenericShape.h"
#include "plasma/Engine.h"
#include "plasma/Drawing.h"

namespace plasma {

GenericShape::GenericShape(Engine *engine, const std::wstring &source, const std::wstring &name)
    : Shape(engine, name),
      m_position(0.0f, 0.0f),
      m_size(100.0f, 100.0f),
      m_source(source)
{
    m_instanceNode = 0;
    m_type = SHAPE_GENERIC;
    m_drawing = engine->createDrawing();
    attributesChanged(ATTRIBUTES_CHANGED_ALL);
}

GenericShape::~GenericShape()
{
    Engine::deleteDrawing(m_drawing);
}

void GenericShape::attributesChanged(int reason)
{
    m_changed = true;
}

Vector<2, float> GenericShape::getMinimum()
{
    return m_position;
}

Vector<2, float> GenericShape::getMaximum()
{
    return m_position + m_size;
}

bool GenericShape::contains(const Vector<2, float> &point)
{
    // Always returns false.
    return false;
}

Shape *GenericShape::clone()
{
    GenericShape *shape = m_engine->createGenericShape(m_source, m_name);
    shape->m_position = m_position;
    shape->m_size = m_size;
    shape->attributesChanged(ATTRIBUTES_CHANGED_ALL);
    return shape;
}

}
