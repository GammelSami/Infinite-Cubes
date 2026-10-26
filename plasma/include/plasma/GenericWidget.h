// plasma::GenericWidget: a widget with a node of its own that shows a generic shape (e.g. the modules' "panel"),
// made by Engine::CreateGenericWidget.
#pragma once
#include <string>
#include "plasma/Widget.h"
#include "plasma/GenericShape.h"

namespace plasma {

class GenericWidget : public Widget {
public:
    GenericWidget(Engine *engine, const std::wstring &name, float x, float y, float width, float height, Node *parent,
                  const std::wstring &shape);
    GenericWidget(const GenericWidget &other, Node *node);
    virtual ~GenericWidget();

    virtual Widget *clone(Node *node);

private:
    // Makes the node (a child of parent) with a new generic shape of the given kind, and places the widget at position
    // with size.
    void create(const Vector<2, float> &position, const Vector<2, float> &size, Node *parent, const std::wstring &shape,
                const std::wstring &name);
};

}
