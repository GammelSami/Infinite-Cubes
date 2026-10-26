// The "NodeTreeWidget" control shown by "layers:listbox:list".
#pragma once
#include "plasma/Widget.h"

class Workspace;
class NodeInterface;

class NodeTreeWidget : public plasma::Widget {
public:
    NodeTreeWidget(plasma::Engine *engine, Workspace *workspace, plasma::Node *node);
    virtual ~NodeTreeWidget();
    virtual void draw(); // Renders the rows.
    virtual plasma::Vector<2, float> getMaximum();
    virtual bool hasBounds();
    virtual bool contains(const plasma::Vector<2, float> &point);
    virtual void pointerPressed();
    virtual void pointerReleased();
    virtual void pointerDoubleClicked(); // Double click.
    virtual void pointerMoved();

    // Each layer row is this many units high (the row layout, the hit tests and the visible row range).
    enum {
        ROW_HEIGHT = 40
    };

    typedef bool (NodeTreeWidget::*RowFunction)(int depth, int row, plasma::Node *node);

    void rebuild();
    bool walkRows(plasma::Node *node, bool visibleOnly, RowFunction function);
    bool renderThumbnail(int depth, int row, plasma::Node *node);
    void renderLabel(const std::wstring &name, const plasma::Vector<4, float> &fill, int depth, int row,
                     const plasma::Vector<4, float> &outline, bool bold);
    bool renderRow(int depth, int row, plasma::Node *node);
    bool measureRow(int depth, int row, plasma::Node *node);
    static void deselectSheetNode(NodeInterface *node);
    bool clickRow(int depth, int row, plasma::Node *node);
    bool hoverRow(int depth, int row, plasma::Node *node);

    plasma::Vector<2, int> getCurrentNodePosition() const { return m_currentNodePosition; }

    Workspace *m_workspace;
    int m_firstVisibleRow;
    int m_lastVisibleRow;
    int m_depth;          // Current walk depth.
    int m_row;            // Current walk row.
    float m_contentWidth; // Measured label extent.
    bool
        m_doubleClick; // Set by a double click (PointerDoubleClicked), clear for a press: the lock column then locks recursively and the label sets the edit root.

    // The current node's row position, set by the row layout.
    plasma::Vector<2, int> m_currentNodePosition; // Current row position.
    plasma::Node *m_lockedIcon;                   // "icon:locked".
    plasma::Node *m_unlockedIcon;                 // "icon:unlocked".
    plasma::Node *m_visibleIcon;                  // "icon:visible2".
    plasma::Node *m_invisibleIcon;                // "icon:invisible".
    plasma::Node *m_openIcon;                     // "icon:expander:open".
    plasma::Node *m_closedIcon;                   // "icon:expander:closed".
    plasma::Node *m_disabledIcon;                 // "icon:expander:disabled".
};

// The thumbnail renderer limits expensive subtrees to 100 visible shapes.
int countThumbnailShapes(plasma::Node *node);
