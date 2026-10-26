// The "FontWidget" list used by module:text.
#pragma once
#include "plasma/Widget.h"
class Workspace;

class FontWidget : public plasma::Widget {
public:
    FontWidget(plasma::Engine *engine, Workspace *workspace, plasma::Node *node);
    virtual ~FontWidget();
    virtual void draw();
    virtual plasma::Vector<2, float> getMaximum();
    virtual bool hasBounds();
    virtual bool contains(const plasma::Vector<2, float> &point);
    virtual void pointerReleased();

    // Each font row is this many units high (the hit test and the layout).
    enum {
        ROW_HEIGHT = 30
    };

    void refresh();

    Workspace *m_workspace;
    int m_unused174;                        // Untouched by the constructor; no reader in the tree.
    std::map<unsigned, void *> m_unusedMap; // Constructed and destroyed, but otherwise unused.
};
#if !defined(PLASMA_PORTABLE) && !defined(_WIN64)

#endif
