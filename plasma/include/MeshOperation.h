// MeshOperation: the mesh edits ("modify mesh", "subdivide mesh", "add border", "delete vertices", "connect
// vertices", the MeshModule settings...) keep the mesh's state before and after the edit.
#pragma once
#include <string>
#include "Operation.h"
#include "MeshModule.h"

class DesignSheet;
class MeshInterface;

class MeshOperation : public Operation {
public:
    MeshOperation(DesignSheet *sheet, const std::string &name, MeshInterface *mesh, const MeshState &before,
                  const MeshState &after);
    virtual void execute();
    virtual void undo();
    virtual void forgetTexture(plasma::Texture *texture);

    MeshState m_before;
    MeshState m_after;
    MeshInterface *m_mesh;
};
