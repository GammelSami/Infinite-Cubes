#include "plasma/GenericWidget.h"
#include "plasma/Engine.h"
#include "plasma/Node.h"

namespace plasma {

GenericWidget::GenericWidget(Engine *engine, const std::wstring &name, float x, float y, float width, float height,
                             Node *parent, const std::wstring &shape)
    : Widget(engine, 0, name)
{
    m_kind = WIDGET_GENERIC;
    create(Vector<2, float>(x, y), Vector<2, float>(width, height), parent, shape, name);
}

void GenericWidget::create(const Vector<2, float> &position, const Vector<2, float> &size, Node *parent,
                           const std::wstring &shape, const std::wstring &name)
{
    GenericShape *genericShape = m_engine->createGenericShape(shape, name);
    genericShape->m_position = Vector<2, float>(0.0f, 0.0f);
    genericShape->m_size = size;
    setNode(m_engine->createNode(0, genericShape, 0, parent, name));
    Matrix<float> identity;
    identity.identity();
    setGeometry(Vector<2, float>(0.0f, 0.0f), size, Vector<2, float>(0.0f, 0.0f), size, identity);
    setViewPosition(position, true);
}

GenericWidget::GenericWidget(const GenericWidget &other, Node *node)
    : Widget(other, node)
{}

GenericWidget::~GenericWidget() {}

Widget *GenericWidget::clone(Node *node)
{
    return new GenericWidget(*this, node);
}

}
