// StaticMeshShape stores baked fill, stroke and extrusion drawings.
#include "plasma/StaticMeshShape.h"
#include "plasma/Drawing.h"
#include "plasma/Engine.h"
#include <algorithm>

namespace plasma {

StaticMeshShape::StaticMeshShape(Engine *engine, Drawing *fill, Drawing *stroke, Drawing *extrusion,
                                 const std::wstring &name)
    : Shape(engine, name),
      m_fillDrawing(fill),
      m_strokeDrawing(stroke),
      m_extrusionDrawing(extrusion)
{
    // Leaves the bounds unset until updateDrawing.
}

StaticMeshShape::~StaticMeshShape()
{
    if (m_fillDrawing)
        m_engine->deleteDrawing(m_fillDrawing);
    if (m_strokeDrawing)
        m_engine->deleteDrawing(m_strokeDrawing);
    if (m_extrusionDrawing)
        m_engine->deleteDrawing(m_extrusionDrawing);
}

Vector<2, float> StaticMeshShape::getMaximum()
{
    return m_maximum;
}

Vector<2, float> StaticMeshShape::getMinimum()
{
    return m_minimum;
}

void StaticMeshShape::draw(Node *node)
{
    if (m_extrusionDrawing)
        m_extrusionDrawing->draw(node, Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f));
    if (m_fillDrawing)
        m_fillDrawing->draw(node, m_engine->getFillColor());
    if (m_strokeDrawing)
        m_strokeDrawing->draw(node, m_engine->getOutlineColor());
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

void StaticMeshShape::updateDrawing()
{
    m_minimum = Vector<2, float>(0.0f, 0.0f);
    m_maximum = Vector<2, float>(0.0f, 0.0f);
    bool first = true;
    if (m_fillDrawing) {
        m_minimum = m_fillDrawing->getMinimum();
        m_maximum = m_fillDrawing->getMaximum();
        first = false;
    }
    if (m_strokeDrawing) {
        if (first) {
            m_minimum = m_strokeDrawing->getMinimum();
            m_maximum = m_strokeDrawing->getMaximum();
            first = false;
        } else {
            m_minimum[0] = (std::min)(m_minimum[0], m_strokeDrawing->getMinimum()[0]);
            m_minimum[1] = (std::min)(m_minimum[1], m_strokeDrawing->getMinimum()[1]);
            m_maximum[0] = (std::max)(m_maximum[0], m_strokeDrawing->getMaximum()[0]);
            m_maximum[1] = (std::max)(m_maximum[1], m_strokeDrawing->getMaximum()[1]);
        }
    }
    if (m_extrusionDrawing) {
        if (first) {
            m_minimum = m_extrusionDrawing->getMinimum();
            m_maximum = m_extrusionDrawing->getMaximum();
            first = false;
        } else {
            m_minimum[0] = (std::min)(m_minimum[0], m_extrusionDrawing->getMinimum()[0]);
            m_minimum[1] = (std::min)(m_minimum[1], m_extrusionDrawing->getMinimum()[1]);
            m_maximum[0] = (std::max)(m_maximum[0], m_extrusionDrawing->getMaximum()[0]);
            m_maximum[1] = (std::max)(m_maximum[1], m_extrusionDrawing->getMaximum()[1]);
        }
    }
}

}
