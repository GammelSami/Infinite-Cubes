#include "Operations.h"

NodeOperation::NodeOperation(DesignSheet *sheet, const std::string &name, NodeInterface *node, const NodeState &before,
                             const NodeState &after)
    : Operation(sheet, name, true),
      m_before(before),
      m_after(after)
{
    m_node = node;
}

void NodeOperation::execute()
{
    m_node->restoreState(m_after);
}

void NodeOperation::undo()
{
    m_node->restoreState(m_before);
}
