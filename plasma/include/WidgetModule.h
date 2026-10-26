// WidgetModule: the editor's widget panel (captioned "Widget"): the caption, alignment ("widget:align:left",
// "widget:center:horizontally"...), the widget flags ("widget:adapt:width", "widget:scroll:horizontally",
// "widget:popup", "widget:rendersurface", "widget:resizable") and the button, scroll button and scroll slider
// kinds of the selected nodes' widgets.
#pragma once
#include "Module.h"
#include "FormatModule.h"
#include "Operation.h"
#include "plasma/Matrix.h"

namespace plasma {
class Button;
class ScrollButton;
class ScrollSlider;
}

class WidgetModule : public Module {
public:
    WidgetModule();

    virtual void build();

    // Enables the panel's rows that apply to the current node's widget.
    virtual void updatePanel();

    // Shows the selected widgets' caption, alignment, flags and kinds.
    virtual void showSelectedValues();

    // Forgets the current widget.
    virtual void clearCurrentNode();

    // Takes the current node's widget and what kind of widget it is.
    virtual void currentNodeChanged();

    // The widget tool binds the selected widgets ("bind widget").
    void bindWidget();

    // Each sets the value on the selected nodes' widgets.
    void setButtonNormal();           // "button:normal".
    void setButtonCheck();            // "button:check".
    void setButtonRadio();            // "button:radio".
    void alignLeft();                 // "widget:horizontal:left".
    void alignRight();                // "widget:horizontal:right".
    void centerHorizontally();        // "widget:horizontal:center".
    void alignTop();                  // "widget:vertical:top".
    void alignBottom();               // "widget:vertical:bottom".
    void centerVertically();          // "widget:vertical:center".
    void adaptWidth();                // "widget:adapt:width".
    void adaptHeight();               // "widget:adapt:height".
    void scrollVertically();          // "widget:scroll:vertically".
    void scrollHorizontally();        // "widget:scroll:horizontally".
    void setPopup();                  // "widget:popup".
    void setRenderSurface();          // "widget:rendersurface".
    void setResizable();              // "widget:resizable".
    void setScrollButtonLeft();       // "scrollbutton:left".
    void setScrollButtonRight();      // "scrollbutton:right".
    void setScrollButtonUp();         // "scrollbutton:up".
    void setScrollButtonDown();       // "scrollbutton:down".
    void setScrollSliderHorizontal(); // "scrollslider:horizontal".
    void setScrollSliderVertical();   // "scrollslider:vertical".
    void editCaption();               // "widget:caption".

    plasma::Widget *m_widget; // The current node's widget.
    plasma::Button *m_button; // The same if it is a button.
    plasma::ScrollButton *m_scrollButton;
    plasma::ScrollSlider *m_scrollSlider;
};

// WidgetBindOperation records widget geometry before and after "bind widget". Only WidgetModule::BindWidget
// constructs it.
class WidgetBindOperation : public Operation {
public:
    WidgetBindOperation(DesignSheet *sheet, const std::string &name, plasma::Widget *widget,
                        const plasma::Vector<2, float> &oldBindPos, const plasma::Vector<2, float> &oldBindSize,
                        const plasma::Vector<2, float> &oldInnerBindPos,
                        const plasma::Vector<2, float> &oldInnerBindSize, const plasma::Matrix<float> &oldBindMatrix,
                        const plasma::Vector<2, float> &newBindPos, const plasma::Vector<2, float> &newBindSize,
                        const plasma::Vector<2, float> &newInnerBindPos,
                        const plasma::Vector<2, float> &newInnerBindSize, const plasma::Matrix<float> &newBindMatrix);
    virtual void execute();
    virtual void undo();

protected:
    plasma::Vector<2, float> m_oldBindPos;
    plasma::Vector<2, float> m_oldBindSize;
    plasma::Vector<2, float> m_oldInnerBindPos;
    plasma::Vector<2, float> m_oldInnerBindSize;
    plasma::Matrix<float> m_oldBindMatrix;
    plasma::Vector<2, float> m_newBindPos;
    plasma::Vector<2, float> m_newBindSize;
    plasma::Vector<2, float> m_newInnerBindPos;
    plasma::Vector<2, float> m_newInnerBindSize;
    plasma::Matrix<float> m_newBindMatrix;
    plasma::Widget *m_widget;
};
