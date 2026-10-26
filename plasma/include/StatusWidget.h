// The tooltip's custom widget, named "statuswidget" by its constructor.
#pragma once
#include "plasma/Widget.h"
class Workspace;

class StatusWidget : public plasma::Widget {
public:
    StatusWidget(plasma::Engine *engine, Workspace *workspace, plasma::Node *node);

    ~StatusWidget();
    virtual void draw();
    virtual void initialize();

    Workspace *m_workspace;
    std::wstring m_text;       // StatusInfo XML "text" (StatusModule::UpdatePanel).
    std::wstring m_annotation; // StatusInfo XML "annotation", drawn above it.
    std::wstring m_extraText;  // StatusInfo; not drawn.
};

