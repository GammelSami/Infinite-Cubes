// Baked mesh drawing lifecycle and bounds updates.
#include <algorithm>
#include "plasma/StaticMeshShape.h"
#include "plasma/Drawing.h"
#include "plasma/Engine.h"
#include "plasma/EditFilter.h"

namespace plasma {

StaticMeshShape::StaticMeshShape(Engine *engine, Drawing *fill, Drawing *stroke, Drawing *extrusion,
                                 const std::wstring &name)
    : Shape(engine, name),
      m_fillDrawing(fill),
      m_strokeDrawing(stroke),
      m_extrusionDrawing(extrusion)
{
    // The bounds stay unset until updateDrawing.
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
        m_fillDrawing->draw(node, m_engine->m_fillColor);
    if (m_strokeDrawing)
        m_strokeDrawing->draw(node, m_engine->m_outlineColor);
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
        if (first) {
            first = false;
            m_minimum = m_fillDrawing->m_minimum;
            m_maximum = m_fillDrawing->m_maximum;
        } else {
            m_minimum.m_elements[0] = (std::min)(m_minimum.m_elements[0], m_fillDrawing->m_minimum.m_elements[0]);
            m_minimum.m_elements[1] = (std::min)(m_minimum.m_elements[1], m_fillDrawing->m_minimum.m_elements[1]);
            m_maximum.m_elements[0] = (std::max)(m_maximum.m_elements[0], m_fillDrawing->m_maximum.m_elements[0]);
            m_maximum.m_elements[1] = (std::max)(m_maximum.m_elements[1], m_fillDrawing->m_maximum.m_elements[1]);
        }
    }
    if (m_strokeDrawing) {
        if (first) {
            first = false;
            m_minimum = m_strokeDrawing->m_minimum;
            m_maximum = m_strokeDrawing->m_maximum;
        } else {
            m_minimum.m_elements[0] = (std::min)(m_minimum.m_elements[0], m_strokeDrawing->m_minimum.m_elements[0]);
            m_minimum.m_elements[1] = (std::min)(m_minimum.m_elements[1], m_strokeDrawing->m_minimum.m_elements[1]);
            m_maximum.m_elements[0] = (std::max)(m_maximum.m_elements[0], m_strokeDrawing->m_maximum.m_elements[0]);
            m_maximum.m_elements[1] = (std::max)(m_maximum.m_elements[1], m_strokeDrawing->m_maximum.m_elements[1]);
        }
    }
    if (m_extrusionDrawing) {
        if (first) {
            first = false;
            m_minimum = m_extrusionDrawing->m_minimum;
            m_maximum = m_extrusionDrawing->m_maximum;
        } else {
            m_minimum.m_elements[0] = (std::min)(m_minimum.m_elements[0], m_extrusionDrawing->m_minimum.m_elements[0]);
            m_minimum.m_elements[1] = (std::min)(m_minimum.m_elements[1], m_extrusionDrawing->m_minimum.m_elements[1]);
            m_maximum.m_elements[0] = (std::max)(m_maximum.m_elements[0], m_extrusionDrawing->m_maximum.m_elements[0]);
            m_maximum.m_elements[1] = (std::max)(m_maximum.m_elements[1], m_extrusionDrawing->m_maximum.m_elements[1]);
        }
    }
}

}
