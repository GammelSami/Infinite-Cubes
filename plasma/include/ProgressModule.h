// The "module:progress" status panel.
#pragma once
#include "Module.h"

class ProgressModule : public Module {
public:
    ProgressModule();
    virtual void build();
    void advance(float amount);
    void setProgress(float progress);
    void setText(const std::wstring &text);

    float m_progress;
    plasma::Widget *m_bar; // "progress:bar".
};
