// CreateNodeOperation (CreateNodeOperation.cpp): records a node added to the sheet ("create layer").
#pragma once
#include "Operation.h"

namespace plasma {
class Node;
}

class CreateNodeOperation : public Operation {
public:
    // Keeps the node's parent and next sibling for Do.
    CreateNodeOperation(DesignSheet *sheet, const std::string &name, plasma::Node *node);

    virtual ~CreateNodeOperation();

    // Puts the node back under its parent and shows its subtree.
    virtual void execute();
    // Moves the node under the deleted nodes and hides its subtree.
    virtual void undo();

protected:
    plasma::Node *m_node;
    plasma::Node *m_parent;
    plasma::Node *m_nextSibling;
};
