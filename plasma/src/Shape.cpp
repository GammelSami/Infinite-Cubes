#include "plasma/Shape.h"

namespace plasma {

Shape::Shape(Engine *engine, const std::wstring &name)
    : Keyable(engine, name),
      m_type(SHAPE_UNTYPED),
      m_unused6C(true),
      m_unused6D(false)
{
    m_engineEntry = engine->addShape(this);
}

Shape::~Shape() {}

int Shape::countShownNodes()
{
    int count = 0;
    for (std::set<Node *>::iterator it = m_nodes.begin(); it != m_nodes.end(); ++it) {
        if ((*it)->isShown())
            count++;
    }
    return count;
}

}
