#include "HistoryState.h"
#include "Operation.h"

HistoryState::HistoryState(DesignSheet *sheet)
    : m_sheet(sheet),
      m_undoCurrentNode(0),
      m_redoCurrentNode(0)
{}

HistoryState::~HistoryState()
{
    for (std::list<Operation *>::iterator it = m_operations.begin(); it != m_operations.end(); ++it)
        delete *it;
}

void HistoryState::add(Operation *operation)
{
    m_operations.push_back(operation);
}

void HistoryState::redo()
{
    for (std::list<Operation *>::iterator it = m_operations.begin(); it != m_operations.end(); ++it)
        (*it)->execute();
}

void HistoryState::undo()
{
    for (std::list<Operation *>::reverse_iterator it = m_operations.rbegin(); it != m_operations.rend(); ++it)
        (*it)->undo();
}

bool HistoryState::changesDocument()
{
    for (std::list<Operation *>::iterator it = m_operations.begin(); it != m_operations.end(); ++it)
        if ((*it)->changesDocument())
            return true;
    return false;
}
