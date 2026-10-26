// The node tool panel, identified by "module:node:tool".
#pragma once
#include "Module.h"

class NodeToolModule : public Module {
public:
    NodeToolModule();
    virtual void build();
    void chooseShapeBoundary();
    void chooseControlBoundary();
    void chooseScaleBox();
    void chooseSelect();
    void chooseMove();
    void chooseRotate();
    void chooseBoxSelection();
    void chooseLassoSelection();

    bool m_scaleBox;      // "node:tool:scalebox".
    bool m_shapeBoundary; // "node:tool:boundary:shape".
    int m_editMode;       // 0 select, 1 move, 2 rotate.
    int m_selectionMode;  // SelectionMode, box or lasso.
};

