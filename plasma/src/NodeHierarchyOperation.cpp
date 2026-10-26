#include "NodeTool.h"
#include "plasma/Node.h"

NodeHierarchyOperation::NodeHierarchyOperation(DesignSheet *sheet, const std::string &name, plasma::Node *node,
                                               plasma::Node *oldParent, plasma::Node *oldNextSibling)
    : Operation(sheet, name, true),
      m_node(node),
      m_oldParent(oldParent),
      m_oldNextSibling(oldNextSibling),
      m_newParent(node->m_parent),
      m_newNextSibling(node->getNextSibling())
{}

void NodeHierarchyOperation::undo()
{
    if (m_oldParent)
        m_oldParent->insertChild(m_node, m_oldNextSibling);
    else
        m_node->setParent(0);
}

void NodeHierarchyOperation::execute()
{
    if (m_newParent)
        m_newParent->insertChild(m_node, m_newNextSibling);
    else
        m_node->setParent(0);
}
