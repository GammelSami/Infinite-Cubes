#include "Operations.h"
#include "DesignSheet.h"

NodeRootOperation::NodeRootOperation(DesignSheet *sheet, const std::string &name, plasma::Node *oldRoot,
                                     plasma::Node *newRoot)
    : Operation(sheet, name, true),
      m_newRoot(newRoot),
      m_oldRoot(oldRoot)
{}

void NodeRootOperation::undo()
{
    m_sheet->setEditRoot(m_oldRoot, false);
}

void NodeRootOperation::execute()
{
    m_sheet->setEditRoot(m_newRoot, false);
}
