// A button whose checked state controls a popup widget (WIDGET_POPUP).
#pragma once
#include "plasma/Button.h"

namespace plasma {

class PopUpButton : public Button {
    friend class PLXStreamReader;
    friend class PLXStreamWriter;

public:
    PopUpButton(Engine *engine, Node *node, const std::wstring &name);
    PopUpButton(const PopUpButton &other, Node *node);
    virtual ~PopUpButton();
    virtual void initialize();
    virtual void pointerPressed();
    virtual void pointerReleased();
    virtual void pointerReleasedAnywhere();
    virtual Widget *clone(Node *node);

    Widget *findPopUp();
    static bool isPopUpWidget(const Widget *widget) { return (widget->m_flags & (1 << WIDGET_POPUP)) != 0; }

    Widget *m_popUp;
    bool m_updateCaption;
};

}
