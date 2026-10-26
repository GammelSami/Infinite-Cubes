// The alpha retains only the construction and cloning of this widget kind.
#pragma once
#include "plasma/Widget.h"
namespace plasma {
class ListWidget : public Widget {
public:
    ListWidget(Engine *engine, Node *node, const std::wstring &name);
    ListWidget(const ListWidget &other, Node *node);
    virtual ~ListWidget();
    virtual Widget *clone(Node *node);
};
}
