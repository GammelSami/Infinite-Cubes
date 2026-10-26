// ListWidget is a Plasma widget for list content.
#pragma once
#include <string>
#include "plasma/Widget.h"

namespace plasma {

class Engine;
class Node;

class ListWidget : public Widget {
public:
    ListWidget(Engine *engine, Node *node, const std::wstring &name);
    ListWidget(const ListWidget &other, Node *node); // Copied by clone.
    virtual ~ListWidget();

    virtual Widget *clone(Node *node);
};

}
