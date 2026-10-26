#include "plasma/StaticMeshShape.h"
#include "plasma/Drawing.h"
#include "plasma/Engine.h"

namespace plasma {

StaticMeshShape::StaticMeshShape(Engine *engine, const std::wstring &name, Drawing *fill, Drawing *stroke,
                                 Drawing *extrusion)
    : Shape(engine, name),
      m_fillDrawing(fill),
      m_strokeDrawing(stroke),
      m_extrusionDrawing(extrusion)
{
    // The code leaves the bounds unset until UpdateDrawing.
}

StaticMeshShape::~StaticMeshShape()
{
    if (m_fillDrawing)
        Engine::deleteDrawing(m_fillDrawing);
    if (m_strokeDrawing)
        Engine::deleteDrawing(m_strokeDrawing);
    if (m_extrusionDrawing)
        Engine::deleteDrawing(m_extrusionDrawing);
}

Vector<2, float> StaticMeshShape::getMinimum()
{
    return m_minimum;
}

Vector<2, float> StaticMeshShape::getMaximum()
{
    return m_maximum;
}

bool StaticMeshShape::contains(const Vector<2, float> &point)
{
    if (m_fillDrawing && m_fillDrawing->contains(point))
        return true;
    if (m_strokeDrawing && m_strokeDrawing->contains(point))
        return true;
    if (m_extrusionDrawing && m_extrusionDrawing->contains(point))
        return true;
    return false;
}

}
