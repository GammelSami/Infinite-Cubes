#include "Operations.h"
#include "DesignSheet.h"
#include "NodeInterface.h"
#include "plasma/Node.h"

DeleteNodeOperation::DeleteNodeOperation(DesignSheet *sheet, const std::string &name, plasma::Node *node,
                                         plasma::Node *parent, plasma::Node *nextSibling)
    : Operation(sheet, name, true),
      m_node(node),
      m_parent(parent),
      m_nextSibling(nextSibling)
{}

DeleteNodeOperation::~DeleteNodeOperation() {}

void DeleteNodeOperation::execute()
{
    m_node->setParent(m_sheet->m_deletedNodes);
    m_node->setShownRecursively(false);
}

void DeleteNodeOperation::undo()
{
    m_parent->insertChild(m_node, m_nextSibling);
    m_sheet->findSheetNode(m_node)->setSelected(true);
    m_node->setShownRecursively(true);
}
