// HistoryState: one step of a design sheet's undo history, the operations one user action performed. It owns the
// operations and deletes them.
#pragma once
#include <list>

namespace plasma {
class Node;
}

class DesignSheet;
class Operation;

class HistoryState {
    friend class DesignSheet;

public:
    explicit HistoryState(DesignSheet *sheet);
    virtual ~HistoryState();

    void add(Operation *operation);

    // Re-applies the operations in order.
    void redo();

    // Reverts the operations in reverse order.
    void undo();

    // Whether any operation marks the document modified.
    bool changesDocument();
    bool isEmpty() const { return m_operations.empty(); }

protected:
    DesignSheet *m_sheet;
    plasma::Node *m_undoCurrentNode; // The sheet's current node before the step (Commit; Undo restores it).

    // The sheet's current node after the action, restored by Redo (DesignSheet Commit stores it, Redo reads it).
    plasma::Node *m_redoCurrentNode;
    std::list<Operation *> m_operations;
};
