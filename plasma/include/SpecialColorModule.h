// The "module:special:color" panel and its color-preview worker.
#pragma once
#include "ColorModule.h"
#include "ShapeInterface.h"
namespace abstr {
class ScriptValue;
}

class SpecialColorModule : public Module {
public:
    SpecialColorModule();
    virtual ~SpecialColorModule();
    virtual void build();
    virtual void currentNodeChanged();
    void followColor(const EditorColor &color);
    void setSourceColor(const EditorColor &color);
    void setTargetColor(const EditorColor &color);
    void apply();
    void cancel();
    void preview();
#ifdef PLASMA_PORTABLE
    void tickPreview();
#endif
    void editBrightness();
    void editContrast();
    void editSaturation();
    void editTolerance();
    void setBrightness(float value);
    void setContrast(float value);
    void setSaturation(float value);
    void setTolerance(float value);
    void dragBrightness(plasma::Widget *sender);
    void stepBrightness(plasma::Widget *sender);
    void dragContrast(plasma::Widget *sender);
    void stepContrast(plasma::Widget *sender);
    void dragSaturation(plasma::Widget *sender);
    void stepSaturation(plasma::Widget *sender);
    void dragTolerance(plasma::Widget *sender);
    void stepTolerance(plasma::Widget *sender);
    void chooseDrawing();
    void chooseSelection();
    bool isVisible() { return plasma::Widget::isDisplayVisible(m_panel->m_node->m_display); }

    plasma::Node *m_sourcePanel;
    plasma::Node *m_targetPanel;
    EditorColor m_sourceColor;
    EditorColor m_targetColor;
    float m_brightness;
    float m_contrast;
    float m_saturation;
    float m_tolerance;
    bool m_drawing;

    // Flags shared with the colour-preview worker.
    volatile bool m_previewPending;
    volatile bool m_previewRunning;
    volatile bool m_cancel;
    volatile bool m_workerRunning;
    volatile bool m_workerFinished;
};

namespace plasma {
template<>
void Widget::setConnection<SpecialColorModule>(Connection *&connection, SpecialColorModule *object,
                                               void (SpecialColorModule::*method)(), bool enabledOnly);
}

#include "../vendor/pugixml-0.9/src/pugixml.hpp"
std::wstring getXmlText(const pugi::xml_node &node);
