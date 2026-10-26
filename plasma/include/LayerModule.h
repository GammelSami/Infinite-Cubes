// The "module:layers" panel: the open sheet's node tree (NodeTreeWidget) with new/delete/reorder buttons,
// "layers:find" (shows the current node) and the selected count.
#pragma once
#include "Module.h"

class NodeTreeWidget;

class LayerModule : public Module {
public:
    LayerModule();
    virtual void build();
    virtual void updatePanel();
    virtual void sheetChanged();
    void createLayer();
    void showCurrentNode();

    NodeTreeWidget *m_layerWidget; // "layers:listbox:list".
};

