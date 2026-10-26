// NodeTool: the tool that selects, moves and arranges whole nodes (type 1 in the tool module; WidgetTool derives
// from it). Its menu commands ("select node", "deselect node", "bring to front", "send to back", "create group",
// "lock"...) are called from the window procedure and the workspace's actions.
#pragma once
#include <list>
#include <set>
#include <string>
#include "BoundingBoxTool.h"
#include "Operation.h"

namespace plasma {
class Node;
class Shape;
class Transformation;
class Keyable;
}
class NodeInterface;

// NodeHierarchyOperation: moves a node in the hierarchy; it records the node's parent and next sibling before the
// move (the arguments) and after it (the node's current ones).
class NodeHierarchyOperation : public Operation {
public:
    NodeHierarchyOperation(DesignSheet *sheet, const std::string &name, plasma::Node *node, plasma::Node *oldParent,
                           plasma::Node *oldNextSibling);

    virtual void execute();
    virtual void undo();

protected:
    plasma::Node *m_node;
    plasma::Node *m_oldParent;
    plasma::Node *m_oldNextSibling;
    plasma::Node *m_newParent;
    plasma::Node *m_newNextSibling;
};

class NodeTool : public BoundingBoxTool {
public:
    explicit NodeTool(int type);

    virtual void pointerMoved(int, int);
    virtual void pointerPressed();

    // Ends a box selection: selects the nodes inside the box.
    virtual void pointerReleased();

    // Makes the hovered node the root of editing ("set root"); without one, goes back to the document's root.
    virtual void pointerDoubleClicked();
    virtual void deleteSelected();
    virtual void draw();

    // Fills the scene's context menu ("Bring to Front"... "Delete"; "Delete" is DeleteSelected).
    virtual void fillContextMenu();

    // Alt held in move mode, or not held in rotate mode.
    virtual bool isRotating();

    // Before and after the scale box changes the selection: the texture sheet's meshes keep their texture transform;
    // then the moved nodes' sheet nodes are refreshed and the change committed.
    virtual void beginBoxChange();
    virtual void endBoxChange();
    virtual void setBox(const plasma::Vector<2, float> &boxMin, const plasma::Vector<2, float> &boxMax);

    // Rotates the selected nodes by the angle (degrees) about the box's middle.
    virtual void rotateBox(float angle);

    // Deselects the sheet node ("deselect node") if it is selected.
    static void deselectSheetNode(NodeInterface *node);

    // Selects or deselects the shown subtree's nodes inside the selection rectangle or lasso (IsInsideBox; "select
    // node"); true when any changed.
    bool selectNodesInBox(plasma::Node *node, bool select);

    void bringToFront();
    void sendToBack();
    void bringForward();
    void sendBackward();
    bool bringToFront(plasma::Node *node); // ("bring to front").
    bool sendToBack(plasma::Node *node);   // ("send to back").
    bool bringForward(plasma::Node *node); // ("bring forward").
    bool sendBackward(plasma::Node *node); // (also named "bring forward").

    // The window menu's "Copy Instances": gives the instances copies of their shapes.
    void uninstantiate();
    bool uninstantiate(plasma::Node *node); // ("uninstantiate node").

    // Makes the current node the root of editing ("set root").
    void setCurrentNodeAsRoot();

    // Makes the document's root the root of editing again.
    void resetEditRoot();

    // Moves the shown children of the selected unlocked nodes out to the nodes' parents, after the nodes.
    void detachChildren(); // "Detach Children".
    void dropToShadow();   // ("drop to shadow").
    void createGroup();    // ("create group").

    // A new shape for the node's drop shadow or group: a smooth mesh of the given colour over the node's subtree.
    static plasma::Shape *createBoundaryMesh(plasma::Node *node, const plasma::Vector<4, float> &color);

    // Gives the subtree copies of its transformations.
    static void makeTransformationsUnique(plasma::Node *node);

    // Lock or unlock the selected nodes' subtrees (node flag bit 0); locking deselects them.
    void lockSelected();
    void unlockSelected();

    // Lock (deselecting them) or unlock the selected nodes' children; lock the children's subtrees.
    void lockChildren();
    void lockChildrenRecursively();
    void unlockChildren();

    // Selects the parents of the selected nodes (not the root, not locked ones), deselecting the nodes themselves
    // unless Shift is held.
    void selectParent();

    // Selects the selected nodes' unlocked shown children, deselecting the nodes themselves unless Shift is held.
    void selectChildren();
    void selectAll();
    void deselectAll();
    void selectHierarchy();

    // Locks or unlocks the shown subtree, deselecting the nodes it locks ("deselect node").
    void setLockedRecursively(plasma::Node *node, bool locked);

    // Selects the shown subtree's unselected nodes ("select node").
    void selectRecursively(plasma::Node *node, bool select);

    // Resets the box, grows it over the selected unlocked nodes' shapes (or, with the widget tool, their widgets'
    // rectangles; in the texture sheet's space while it is shown) and places the handles.
    void updateBox();

    // The selected smooth meshes' texture transforms in the texture sheet; each shared shape changes once.
    void moveTextureCoordinates(std::set<plasma::Shape *> &shapes, const plasma::Vector<2, float> &offset);
    void rotateTextureCoordinates(const plasma::Vector<2, float> &pivot, std::set<plasma::Shape *> &shapes,
                                  const plasma::Vector<3, float> &rotation);
    void scaleTextureCoordinates(std::set<plasma::Shape *> &shapes, const plasma::Vector<2, float> &pivot,
                                 const plasma::Vector<2, float> &scale);

    // Move, scale or rotate the shown selected nodes of the subtree (the design sheet's root without one, skipping
    // NODE_INTERNAL nodes); a transformation shared by several nodes changes once (transformations collects those
    // done).
    void moveNodes(std::set<plasma::Keyable *> &transformations, const plasma::Vector<2, float> &offset,
                   plasma::Node *node);
    void scaleNodes(const plasma::Vector<2, float> &center, const plasma::Vector<2, float> &scale,
                    std::set<plasma::Keyable *> &transformations, plasma::Node *node);
    void rotateNodes(const plasma::Vector<2, float> &pivot, const plasma::Vector<3, float> &rotation,
                     std::set<plasma::Keyable *> &transformations, plasma::Node *node);

    // Shows the drop shadow panel.
    void showShadowPanel();

    // The window menu's object and mesh commands; the rotations and flips act about the box's middle.
    void rotate45Right(); // ("Rotate 45° Right").
    void rotate45Left();
    void rotate90Right();
    void rotate90Left();
    void rotate180();
    void flipHorizontally();
    void flipVertically();
    void projectTextureCoordinates();

    // The selected meshes' texture transformation back to the identity.
    void resetTextureTransformation();

    // Lock the selected nodes (deselecting them: "lock", "deselect node"), or unlock every locked node ("unlock").
    void lockObjects(); // ("Lock Objects").
    void unlockAll();   // ("Unlock All").
};
