// TextTool: the tool that places and edits text shapes (type 6 in the tool module, "tool:text").
#pragma once
#include "Tool.h"

class TextTool : public Tool {
public:
    TextTool();

    virtual void pointerMoved(int x, int y);
    virtual void pointerPressed();
    virtual void draw();
    virtual void fillContextMenu();

    // Ends text editing (Workspace::SetTextInterface(0)).
    virtual void deactivate();

    // The window menu's "Text to Mesh".
    void textToMesh();
};

// Gives a group an invisible rectangle around its children.
plasma::Shape *createGroupBoundary(const plasma::Vector<4, float> &color, plasma::Node *node);

// TextTool.cpp: sets the workspace's m_hoveredTextInterface to the text interface under the pointer in the node's
// shown subtree.
void findHoveredTextInterface(DesignSheet *sheet, plasma::Node *node);
