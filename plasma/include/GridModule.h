// The "module:grid" toolbar and its "grid:*" controls.
#pragma once
#include "Module.h"

class GridModule : public Module {
public:
    GridModule();
    virtual void build();
    virtual void unitChanged();
    virtual void sheetChanged();
    void chooseEnabled();
    void editSize();
    void dragSize(plasma::Widget *sender);
    void stepSize(plasma::Widget *sender);
    void halveSize();
    void doubleSize();
    void chooseMoveXY();
    void chooseMoveX();
    void chooseMoveY();
    void updateSize();

    bool m_enabled; // "grid:enabled".
    bool m_moveX;   // "grid:move:x/xy".
    bool m_moveY;   // "grid:move:y/xy".
};

#if !defined(PLASMA_PORTABLE) && !defined(_WIN64)

#endif
