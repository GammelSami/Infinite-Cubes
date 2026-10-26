// Operation: one undoable edit of a design sheet. The sheet's history keeps operations in HistoryState groups; Do
// applies (or re-applies) the edit, Undo reverts it. Derived classes keep the values before and after the edit.
#pragma once
#include <string>

class DesignSheet;

namespace plasma {
class Texture;
}

class Operation {
public:
    Operation(DesignSheet *sheet, const std::string &name, bool changesDocument);
    virtual ~Operation();

    virtual void execute() = 0;
    virtual void undo() = 0;
    // Called before deleting a texture referenced by an operation. ApplyTextureOperation and MeshOperation clear the
    // stored texture or its id. Currently unused.
    virtual void forgetTexture(plasma::Texture *texture) {}

    bool changesDocument() const { return m_changesDocument; }

protected:
    DesignSheet *m_sheet;
    std::string m_name;

    // False for selection changes, which do not mark the document modified.
    bool m_changesDocument;
};
