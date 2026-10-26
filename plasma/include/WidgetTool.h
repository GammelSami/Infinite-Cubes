// WidgetTool is the node tool variant for widgets (type 12, "tool:widget"). It overrides the context menu and
// keeps NodeTool's other behavior.
#pragma once
#include "NodeTool.h"
#include "Operation.h"

class WidgetTool : public NodeTool {
public:
    virtual void fillContextMenu();
    void noWidget();
    void assignWidget();
    void assignButton();
    void assignEdit();
    void assignScrollButton();
    void assignScrollSlider();

    WidgetTool();
};

// AssignWidgetOperation records assigning a widget to a node.
class AssignWidgetOperation : public Operation {
public:
    AssignWidgetOperation(DesignSheet *sheet, const std::string &name, plasma::Node *node, plasma::Widget *oldWidget,
                          plasma::Widget *newWidget);
    virtual void execute();
    virtual void undo();

protected:
    plasma::Node *m_node;
    plasma::Widget *m_oldWidget;
    plasma::Widget *m_newWidget;
};
