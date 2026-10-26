// The toggle preview in "module:texture", derived from plasma::Button.
#pragma once
#include "plasma/Button.h"
class Workspace;

class TexturePreviewWidget : public plasma::Button {
public:
    TexturePreviewWidget(plasma::Engine *engine, Workspace *workspace, plasma::Node *node);
    virtual void draw();
    virtual bool contains(const plasma::Vector<2, float> &point);
    virtual void pointerPressed();
    virtual void pointerReleased();
    virtual void pointerDoubleClicked();
    virtual void pointerReleasedAnywhere();

    Workspace *m_workspace;
};

#if !defined(PLASMA_PORTABLE) && !defined(_WIN64)

#endif
