// NodeInterface represents one node of a design sheet: the "sheet node" in DesignSheet::m_sheetNodes. The node
// keeps a pointer back to it. It holds the shape's editing interface and a saved copy of the node's state.
#pragma once
#include <cstddef>
#include <string>
#include "plasma/ContinuousAttribute.h"
#include "plasma/DiscreteAttribute.h"
#include "plasma/Matrix.h"
#include "plasma/Vector.h"
#include "ShapeInterface.h"

namespace plasma {
class Node;
class GenericShape;
}

class DesignSheet;
class ShapeInterface;

// A saved copy of a node's state: transformation and display attributes, name and flags. NodeOperation keeps
// states before and after a change; modules also compare snapshots around edits.
struct NodeState {
    // Each attribute is default-constructed: an empty name and a default-constructed initial value.
    NodeState() {}

    // The transformation's attributes.
    plasma::ContinuousAttribute<plasma::Vector<2, float>> m_translation;
    plasma::ContinuousAttribute<plasma::Matrix<float>> m_deformation;
    plasma::ContinuousAttribute<plasma::Vector<3, float>> m_rotation;
    plasma::ContinuousAttribute<plasma::Vector<2, float>> m_pivot;

    // The display's attributes.
    plasma::DiscreteAttribute<int> m_visibility;
    plasma::DiscreteAttribute<int> m_clipping;
    plasma::ContinuousAttribute<plasma::Vector<4, float>> m_fillColor;
    plasma::ContinuousAttribute<plasma::Vector<4, float>> m_strokeColor;
    plasma::ContinuousAttribute<float> m_blurRadius;
    std::wstring m_name; // The node's name.
    unsigned m_flags;    // The node's flags.
    int m_displayFlags;  // The display's flags (Display::m_flags).
    bool m_selected;     // The sheet node's selection.
};

class NodeInterface {
public:
    NodeInterface(DesignSheet *sheet, plasma::Node *node);
    virtual ~NodeInterface();

    void setSelected(bool selected);
    plasma::Node *getNode() const { return m_node; }
    // Returns the shape's editing interface.
    ShapeInterface *getShapeInterface() const { return m_shapeInterface; }
    // Replaces the shape's editing interface with a new one for the shape's type: a MeshInterface, TextInterface or
    // GenericInterface.
    void createShapeInterface();
    // Saves the node's name, flags, selection and its transformation's and display's attributes.
    void saveState(NodeState &state);
    // Gives the node the saved state back; the transformation is told its attributes changed (NodeOperation's Do and
    // Undo).
    void restoreState(const NodeState &state);
    // Saves it into m_state.
    void saveState();
    // Calls the interface's BeginModify (EndModify) here and in the shown subtree, except below widgets.
    void beginModifyRecursively();
    void endModifyRecursively();
    // Records "modify node": a NodeOperation from the state SaveState kept (m_state) to the current one.
    void recordModification();
    // Draws the node's interface and those of its shown children, and the pivot and widget frames of the node and
    // widget tools.
    void draw();

    DesignSheet *m_sheet;                     // The owning sheet (the constructor's first argument).
    NodeState m_state;                        // Saved by SaveState.
    unsigned char m_unused3FC[0x41c - 0x3fc]; // Not initialized by the constructor and otherwise unused.

    // The layer panel's row layout, set as NodeTreeWidget::WalkRows visits the node: its depth, its row and the row
    // after its subtree so far.
    int m_layerDepth;
    int m_layerRow;
    int m_layerEndRow;
    unsigned char m_unused428[0x438 - 0x428]; // Unused.

    // The layer panel lists the node's children (LayerModule reads it, sets it).
    bool m_expanded;
    // Cleared with m_expanded by the constructor and otherwise unused.
    bool m_unused439;
    bool m_unused43A;
    bool m_unused43B;
    bool m_selected;

    // Set by the edits that saved the node's state first (NodeModule's drags, NodeTool, MeshInterface);
    // DesignSheet::RecordModifiedNodes records "modify node" for it and clears it.
    bool m_modificationPending;
    plasma::Node *m_node;
    ShapeInterface *m_shapeInterface; // The shape's editing interface, if any.
};

#if !defined(PLASMA_PORTABLE) && !defined(_WIN64)

#endif

// The interface of the other shapes (GenericInterface): a GenericShape (a source instance or a placeholder
// rectangle) has no editable vertices, only its bounds. NodeInterface::Refresh makes one for a GenericShape.
class GenericInterface : public ShapeInterface {
public:
    GenericInterface(NodeInterface *node, plasma::GenericShape *shape);
    virtual void draw();

    // The shape's corners, through its node's widget and matrix.
    virtual void getBounds(plasma::Vector<2, float> *minimum, plasma::Vector<2, float> *maximum);

    unsigned char m_unused58[0x10]; // Not initialized and otherwise unused.
    std::string m_unusedString;     // Constructed empty and destroyed; never read.
    plasma::GenericShape *m_shape;
    unsigned char m_unused88[4]; // Not initialized and otherwise unused.
};
