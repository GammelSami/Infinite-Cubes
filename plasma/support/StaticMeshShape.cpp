// Static-mesh support implementations.
#include "plasma/StaticMeshShape.h"
#include "plasma/Engine.h"
#include "plasma/Drawing.h"
#include <algorithm>

namespace plasma {

void StaticMeshShape::draw(Node *node)
{
    if (m_extrusionDrawing)
        m_extrusionDrawing->draw(node, Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f));
    if (m_fillDrawing)
        m_fillDrawing->draw(node, m_engine->getFillColor());
    if (m_strokeDrawing)
        m_strokeDrawing->draw(node, m_engine->getOutlineColor());
}

void StaticMeshShape::updateDrawing()
{
    m_minimum = Vector<2, float>(0.0f, 0.0f);
    m_maximum = Vector<2, float>(0.0f, 0.0f);
    bool first = true;
    if (m_fillDrawing) {
        first = false;
        m_minimum = m_fillDrawing->m_minimum;
        m_maximum = m_fillDrawing->m_maximum;
    }
    if (m_strokeDrawing) {
        if (first) {
            first = false;
            m_minimum = m_strokeDrawing->m_minimum;
            m_maximum = m_strokeDrawing->m_maximum;
        } else {
            for (int i = 0; i < 2; ++i) {
                m_minimum.m_elements[i] = (std::min)(m_minimum.m_elements[i], m_strokeDrawing->m_minimum.m_elements[i]);
            }
            for (int i = 0; i < 2; ++i) {
                m_maximum.m_elements[i] = (std::max)(m_maximum.m_elements[i], m_strokeDrawing->m_maximum.m_elements[i]);
            }
        }
    }
    if (m_extrusionDrawing) {
        if (first) {
            m_minimum = m_extrusionDrawing->m_minimum;
            m_maximum = m_extrusionDrawing->m_maximum;
        } else {
            for (int i = 0; i < 2; ++i) {
                m_minimum.m_elements[i] =
                    (std::min)(m_minimum.m_elements[i], m_extrusionDrawing->m_minimum.m_elements[i]);
            }
            for (int i = 0; i < 2; ++i) {
                m_maximum.m_elements[i] =
                    (std::max)(m_maximum.m_elements[i], m_extrusionDrawing->m_maximum.m_elements[i]);
            }
        }
    }
}

}
