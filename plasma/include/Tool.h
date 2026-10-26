// Tool: the base of the editor's tools (17 derived: node, vertex, edge, face, draw, text, colour...). The
// workspace forwards input and drawing to the active tool through the virtual methods; the base implementations do
// nothing, except the node selection shortcuts.
#pragma once
#include "Module.h"

// How a tool's pointer drag selects (BoundingBoxTool and NodeTool m_selectionMode while dragging, taken from
// NodeToolModule's or VertexModule's m_selectionMode: "node:tool:selection:box"/":lasso", "vertex:selection:box"/
// ":lasso").
enum SelectionMode {
    SELECTION_NONE = 0, // Not dragging a selection.
    SELECTION_BOX = 1,
    SELECTION_LASSO = 2
};

// What a drag on the selection does (NodeToolModule's and VertexModule's m_editMode, by their select/move/rotate
// buttons "node:tool:select"..., "vertex:select"...; the tools' IsRotating swaps move and rotate with Alt).
enum EditMode {
    EDIT_SELECT = 0,
    EDIT_MOVE = 1,
    EDIT_ROTATE = 2
};

class Tool {
public:
    // The tool kinds (m_type, GetType): ToolModule's m_icons index and SetTool argument, named after the tool classes
    // and the tool panel's buttons ("tool:sheet", "tool:node"...; "tool:draw" is DrawFaceTool, "tool:split"
    // DrawEdgeTool).
    enum Type {
        TOOL_SHEET = 0,
        TOOL_NODE = 1,
        TOOL_VERTEX = 2,
        TOOL_EDGE = 3,
        TOOL_FACE = 4,
        TOOL_COLOR = 5,
        TOOL_TEXT = 6,
        TOOL_GENERIC = 7,
        TOOL_DRAW_FACE = 8,
        TOOL_DRAW_FREEHAND = 9,
        TOOL_DRAW_EDGE = 10,
        TOOL_WIDTH = 11,
        TOOL_WIDGET = 12,
        TOOL_EXTRUSION = 13,
        TOOL_PROCEDURAL = 14,
        TOOL_STAMP = 15
    };

    explicit Tool(int type);

    // The workspace forwards input events through these methods. Tool has no virtual destructor. ToolModule::Build
    // calls Build on every tool once its panel is built; no tool overrides it.
    virtual void initialize() {}
    // Empty in every tool and currently unused.
    virtual void unused1(int) {}
    // A key went down or up (Workspace); the colour and width tools pick with Alt (VK_MENU) held.
    virtual void keyPressed(unsigned short key) {}
    virtual void keyReleased(unsigned short key) {}

    // The pointer moved (Workspace::MouseMove, unless the view pans).
    virtual void pointerMoved(int x, int y) {}
    virtual void pointerWheel(int delta) {}
    // The left button (Workspace).
    virtual void pointerPressed() {}
    virtual void pointerReleased() {}
    virtual void middleButtonPressed() {}
    virtual void middleButtonReleased() {}
    // The right button on the current sheet (Workspace).
    virtual void rightButtonPressed() {}
    virtual void rightButtonReleased() {}
    // A double click (Workspace). Here: selects the node under the mouse, as an undoable operation.
    virtual void pointerDoubleClicked();
    // Whether the pointer is over one of the tool's handles; the workspace does not hit-test the scene for hovering
    // then.
    virtual bool isOverHandle() { return false; }

    // Whether a handle is being dragged; the workspace snaps the drag position only then.
    virtual bool isDraggingHandle() { return false; }

    // The tool became current (ToolModule::Update) or stops being current (ToolModule's Select*Tool before switching;
    // ToggleSelectAll calls it too): the drawing tools stop drawing, the text tool ends editing.
    virtual void activate() {}
    virtual void deactivate() {}

    // Deletes the tool's selection (the workspace's Delete action, the context menus' "Delete", "layers:delete").
    virtual void deleteSelected() {}

    // Deselects all nodes if any is selected, otherwise selects all.
    virtual void toggleSelectAll();
    virtual void draw() {}

    // Adds the tool's items to the scene's context menu (Workspace::ShowPopupMenu).
    virtual void fillContextMenu() {}

    // Appends the offsets of the tool's snap points from point (the bounding box tools: the box's corners).
    virtual bool getSnapOffsets(const plasma::Vector<2, float> &, std::list<plasma::Vector<2, float>> &)
    {
        return false;
    }
    // Whether a drag rotates instead of moving: the node and mesh tools combine Alt with the module's move or rotate
    // mode; the procedural tool uses Alt.
    virtual bool isRotating() { return false; }

    // For each shown sheet node whose shape interface has changes (m_hasChanges): tells the node's shape
    // (AttributesChanged(plasma::ATTRIBUTES_CHANGED_ALL)) and the interface (UpdateChildren). The colour, width and
    // mesh tools call it when a stroke or drag ends.
    void refreshChangedShapes();

    // Draws the interface icon of that name ("icon:constrain:x"...) with its pivot at the position, in window
    // coordinates.
    void drawIcon(const std::wstring &name, const plasma::Vector<2, float> &position);
    // The tool's kind, a Type (MeshInterface tests TOOL_EXTRUSION and TOOL_VERTEX, TOOL_EDGE, TOOL_FACE).
    int getType() const { return m_type; }

protected:
    Workspace *m_workspace;
    int m_type; // Type.
};
