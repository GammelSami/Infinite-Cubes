// The "module:zoom" toolbar.
#pragma once
#include "Module.h"

class ZoomModule : public Module {
public:
    ZoomModule();
    virtual void build();
    virtual void sheetChanged();
    void reset();
    void editZoom();
    void dragZoom(plasma::Widget *sender);
    void stepZoom(plasma::Widget *sender);
    void update();
};

