// SheetTool: the tool that arranges the workspace's sheets (tool type 0): a press brings a sheet to the front
// (Shift) or sends it to the back (Ctrl), a double click maximizes or restores it; its menu stacks, creates,
// closes, tiles and colours sheets.
#pragma once
#include "Tool.h"

class SheetTool : public Tool {
public:
    SheetTool();
    virtual void pointerPressed();
    virtual void pointerDoubleClicked();
    virtual void fillContextMenu();

    void bringToFront();
    void sendToBack();
    void bringForward();
    void sendBackward();
};
