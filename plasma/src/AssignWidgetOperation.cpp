#include "WidgetTool.h"
#include "plasma/Node.h"

AssignWidgetOperation::AssignWidgetOperation(DesignSheet *sheet, const std::string &name, plasma::Node *node,
                                             plasma::Widget *oldWidget, plasma::Widget *newWidget)
    : Operation(sheet, name, true),
      m_node(node),
      m_oldWidget(oldWidget),
      m_newWidget(newWidget)
{}

void AssignWidgetOperation::execute()
{
    m_node->setWidget(m_newWidget);
}

void AssignWidgetOperation::undo()
{
    m_node->setWidget(m_oldWidget);
}
