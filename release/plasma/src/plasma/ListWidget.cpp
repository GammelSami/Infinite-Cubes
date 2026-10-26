#include "plasma/ListWidget.h"
#include "plasma/EditFilter.h"

namespace plasma {

ListWidget::ListWidget(Engine *engine, Node *node, const std::wstring &name)
    : Widget(engine, node, name)
{
    m_kind = WIDGET_LIST;
}

ListWidget::ListWidget(const ListWidget &other, Node *node)
    : Widget(other, node)
{
    m_kind = WIDGET_LIST;
}

ListWidget::~ListWidget() {}

Widget *ListWidget::clone(Node *node)
{
    return new ListWidget(*this, node);
}

}
