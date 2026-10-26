// The vertex tool panel ("module:vertex", "vertex:*" controls).
#pragma once
#include "Module.h"

class VertexModule : public Module {
public:
    VertexModule();
    virtual void build();
    void chooseScaleBox();
    void chooseSoftSelection();
    void chooseSelect();
    void chooseMove();
    void chooseRotate();
    void chooseBoxSelection();
    void chooseLassoSelection();

    bool m_softSelection;   // "vertex:softselection".
    bool m_scaleBoxEnabled; // "vertex:scalebox".
    int m_editMode; // An EditMode (Tool.h; "vertex:select", "vertex:move", "vertex:rotate"; the mesh tools' IsRotating reads it).

    // How the mesh tools select (their m_selectionMode, a SelectionMode): box or lasso.
    int m_selectionMode;
};

