// Undoable edits defined by DesignSheet.cpp and the individual operation source files.
#pragma once
#include <list>
#include <string>
#include <vector>
#include "Operation.h"
#include "Module.h"

namespace plasma {
class Node;
class Shape;
class Transformation;
class Display;
class SmoothMeshShape;
class Texture;
}

#include "DesignSheet.h"
#include "MeshInterface.h"
#include "NodeInterface.h"

// Changes the texture of a smooth mesh shape's fill ("apply texture") or stroke (m_stroke: the texture module's
// stroke mode, m_mode == 1) and its texture opacity, brightness, contrast, saturation and stretch (the stretch is
// the stroke texture's in both cases).
class ApplyTextureOperation : public Operation {
public:
    ApplyTextureOperation(DesignSheet *sheet, const std::string &name, plasma::SmoothMeshShape *shape,
                          plasma::Texture *oldTexture, float oldOpacity, float oldBrightness, float oldContrast,
                          float oldSaturation, float oldStretch, plasma::Texture *newTexture, float newOpacity,
                          float newBrightness, float newContrast, float newSaturation, float newStretch, bool stroke);

    virtual void execute();
    virtual void undo();
    virtual void forgetTexture(plasma::Texture *texture);

protected:
    plasma::SmoothMeshShape *m_shape;

    // Pairs of the parameter values before and after the change: opacity, brightness, contrast, saturation and
    // stretch.
    float m_oldOpacity, m_newOpacity;
    float m_oldBrightness, m_newBrightness;
    float m_oldContrast, m_newContrast;
    float m_oldSaturation, m_newSaturation;
    float m_oldStretch, m_newStretch;
    plasma::Texture *m_oldTexture;
    plasma::Texture *m_newTexture;
    bool m_stroke;
};

// Gives a sheet node another shape, transformation and display.
class InstanceOperation : public Operation {
public:
    InstanceOperation(DesignSheet *sheet, const std::string &name, NodeInterface *node, plasma::Shape *oldShape,
                      plasma::Transformation *oldTransformation, plasma::Display *oldDisplay, plasma::Shape *newShape,
                      plasma::Transformation *newTransformation, plasma::Display *newDisplay);

    virtual void execute();
    virtual void undo();

protected:
    plasma::Shape *m_oldShape;
    plasma::Transformation *m_oldTransformation;
    plasma::Display *m_oldDisplay;
    plasma::Shape *m_newShape;
    plasma::Transformation *m_newTransformation;
    plasma::Display *m_newDisplay;
    NodeInterface *m_node;
};

// The mesh selection operations keep one flag per element before and after.
class MeshEdgeSelectionOperation : public Operation {
public:
    MeshEdgeSelectionOperation(DesignSheet *sheet, const std::string &name, MeshInterface *mesh,
                               const std::vector<bool> &before, const std::vector<bool> &after);

    virtual void execute();
    virtual void undo();

protected:
    MeshInterface *m_mesh;
    std::vector<bool> m_before;
    std::vector<bool> m_after;
};

class MeshFaceSelectionOperation : public Operation {
public:
    MeshFaceSelectionOperation(DesignSheet *sheet, const std::string &name, MeshInterface *mesh,
                               const std::vector<bool> &before, const std::vector<bool> &after);

    virtual void execute();
    virtual void undo();

protected:
    MeshInterface *m_mesh;
    std::vector<bool> m_before;
    std::vector<bool> m_after;
};

class MeshVertexSelectionOperation : public Operation {
public:
    MeshVertexSelectionOperation(DesignSheet *sheet, const std::string &name, MeshInterface *mesh,
                                 const std::vector<bool> &before, const std::vector<bool> &after);

    virtual void execute();
    virtual void undo();

protected:
    MeshInterface *m_mesh;
    std::vector<bool> m_before;
    std::vector<bool> m_after;
};

// Selects or deselects one sheet node.
class NodeSelectionOperation : public Operation {
public:
    NodeSelectionOperation(DesignSheet *sheet, const std::string &name, NodeInterface *node, bool select);

    virtual void execute();
    virtual void undo();

protected:
    bool m_select;
    NodeInterface *m_node;
};

// Changes the page settings.
class PageOperation : public Operation {
public:
    PageOperation(DesignSheet *sheet, const std::string &name, const plasma::Engine::Page &before,
                  const plasma::Engine::Page &after);

    virtual void execute();
    virtual void undo();

protected:
    plasma::Engine::Page m_before;
    plasma::Engine::Page m_after;
};

// Makes another node the sheet's root of editing ("set root").
// Operations defined in their own files.
class NodeRootOperation : public Operation {
public:
    NodeRootOperation(DesignSheet *sheet, const std::string &name, plasma::Node *oldRoot, plasma::Node *newRoot);

    virtual void execute();
    virtual void undo();

protected:
    plasma::Node *m_newRoot;
    plasma::Node *m_oldRoot;
};

// Deletes a node ("delete node"): moves it under the sheet's deleted nodes; Undo puts it back before its old next
// sibling and selects it again.
class DeleteNodeOperation : public Operation {
public:
    DeleteNodeOperation(DesignSheet *sheet, const std::string &name, plasma::Node *node, plasma::Node *parent,
                        plasma::Node *nextSibling);
    virtual ~DeleteNodeOperation();

    virtual void execute();
    virtual void undo();

protected:
    plasma::Node *m_node;
    plasma::Node *m_parent;
    plasma::Node *m_nextSibling;
};

// Changes a node's transformation, display, name and flags, keeping states before and after ("set no deformer",
// "adapt to children", and other edits). Defined in NodeOperation.cpp.
class NodeOperation : public Operation {
public:
    NodeOperation(DesignSheet *sheet, const std::string &name, NodeInterface *node, const NodeState &before,
                  const NodeState &after);

    virtual void execute();
    virtual void undo();

protected:
    NodeState m_before;
    NodeState m_after;
    NodeInterface *m_node;
};
