#include "CreateNodeOperation.h"
#include "DesignSheet.h"
#include "plasma/Node.h"

CreateNodeOperation::CreateNodeOperation(DesignSheet *sheet, const std::string &name, plasma::Node *node)
    : Operation(sheet, name, true),
      m_node(node),
      m_parent(node->m_parent),
      m_nextSibling(node->getNextSibling())
{}

CreateNodeOperation::~CreateNodeOperation() {}

void CreateNodeOperation::execute()
{
    m_parent->insertChild(m_node, m_nextSibling);
    m_node->setShownRecursively(true);
}

void CreateNodeOperation::undo()
{
    m_node->setParent(m_sheet->m_deletedNodes);
    m_node->setShownRecursively(false);
}
