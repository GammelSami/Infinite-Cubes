#include "plasma/PointerTypes.h"
#include "SpecialColorModule.h"
#include "plasma/Engine.h"
#include "plasma/GenericWidget.h"
#include "FormatModule.h" // The edit box filters.
#include "plasma/Node.h"
#include "Operations.h"
#include <windows.h>
#include <sstream>
#include <GL/glew.h>
#include "ColorManagementModule.h"
#include "Thread.h"

static const int ColorWorkerPollMilliseconds = 100;

// Layout positions, extents and spacing use display units; colour tuples style the overlays.
SpecialColorModule::SpecialColorModule()
    : m_sourcePanel(0),
      m_targetPanel(0),
      m_sourceColor(1.0f, 1.0f, 1.0f, 1.0f),
      m_targetColor(1.0f, 1.0f, 1.0f, 1.0f)
{
    m_drawing = true;
    m_tolerance = 0.1f;
    m_previewPending = false;
    m_previewRunning = false;
    m_cancel = false;
#ifdef PLASMA_PORTABLE
    m_workerRunning = false;
    m_workerFinished = true;
#else
    m_workerRunning = true;
    m_workerFinished = false;
#endif
    {
        std::wstring name(L"module:special:color");
        std::wstring kind(L"bar");
        m_panel = g_workspace.m_engine.createGenericWidget(name, 0.0f, 0.0f, 230.0f, 310.0f, g_workspace.m_guiRightNode,
                                                           kind);
    }
    plasma::Widget::setDisplayVisibility(m_panel->m_node->m_display, 0);
    m_panel->m_flags |= (1 << plasma::WIDGET_RENDER_SURFACE);
}

SpecialColorModule::~SpecialColorModule()
{
    m_workerRunning = false;

    // The polling uses ordinary bools across threads, a data race in C++.
    while (!m_workerFinished)
        Sleep(ColorWorkerPollMilliseconds);
}

// Layout positions, extents and spacing use display units; colour tuples style the overlays.
void SpecialColorModule::build()
{
    std::list<ModuleWidgetItem> items;
    m_cursor.m_elements[1] = 10;
    items.clear();
    items.push_back(ModuleWidgetItem(L"Drawing", L"special:color:drawing", L"radiobutton2", 0, 0, 90, 30));
    items.push_back(ModuleWidgetItem(L"Selection", L"special:color:selection", L"radiobutton2", 90, 0, 90, 30));
    addPanel(items, L"", 0);
    m_panel->setChecked(L"special:color:drawing", true, false, true);
    newRow(3);

    // A background panel behind the three adjustments.
    addWidget(L"", L"", L"panel", 180, 80, 0);
    newRow(-70);
    m_extent.m_elements[1] -= 80;
    addWidget(L"Brightness:", L"", L"static", 60, 20, 5);
    items.clear();
    items.push_back(ModuleWidgetItem(L" ", L"special:color:brightness", L"editbox", 0, 0, 40, 20));
    items.push_back(ModuleWidgetItem(L" ", L"special:color:brightness:drag",
                                     m_orientation == ORIENTATION_HORIZONTAL ? L"hdragbutton" : L"vdragbutton", 40, 0,
                                     20, 20));
    addPanel(items, L"", 5);
    getPanel()->getNode()->setEditFilter(L"special:color:brightness", &plasma::g_floatFilter, true);
    newRow(3);
    addWidget(L"Contrast:", L"", L"static", 60, 20, 5);
    items.clear();
    items.push_back(ModuleWidgetItem(L" ", L"special:color:contrast", L"editbox", 0, 0, 40, 20));
    items.push_back(ModuleWidgetItem(L" ", L"special:color:contrast:drag",
                                     m_orientation == ORIENTATION_HORIZONTAL ? L"hdragbutton" : L"vdragbutton", 40, 0,
                                     20, 20));
    addPanel(items, L"", 5);
    getPanel()->getNode()->setEditFilter(L"special:color:contrast", &plasma::g_floatFilter, true);
    newRow(3);
    addWidget(L"Saturation:", L"", L"static", 60, 20, 5);
    items.clear();
    items.push_back(ModuleWidgetItem(L" ", L"special:color:saturation", L"editbox", 0, 0, 40, 20));
    items.push_back(ModuleWidgetItem(L" ", L"special:color:saturation:drag",
                                     m_orientation == ORIENTATION_HORIZONTAL ? L"hdragbutton" : L"vdragbutton", 40, 0,
                                     20, 20));
    addPanel(items, L"", 5);
    getPanel()->getNode()->setEditFilter(L"special:color:saturation", &plasma::g_floatFilter, true);
    newRow(8);
    addWidget(L"", L"", L"panel", 180, 142, 0);
    newRow(-132);
    m_extent.m_elements[1] -= 142;
    addWidget(L"Color shift:", L"", L"static", 60, 20, 5);
    newRow(3);
    items.clear();
    items.push_back(ModuleWidgetItem(L" ", L"special:color:shift:source", L"colorbutton", 0, 0, 80, 80));
    items.push_back(ModuleWidgetItem(L" ", L"special:color:shift:target", L"colorbutton", 80, 0, 80, 80));
    addPanel(items, L"special:color:shift:block", 10);
    placeIcon(L"special:color:shift:block", L"icon:next");
    items.clear();
    items.push_back(ModuleWidgetItem(L" ", L"special:color:apply", L"button2", 0, 0, 90, 30));
    items.push_back(ModuleWidgetItem(L" ", L"special:color:cancel", L"button2", 90, 0, 90, 30));
    addPanelAt(items, L"", 10, (int)(getPanel()->getViewHeight() - 40.0));
    newRow(5);
    addWidget(L"Tolerance:", L"", L"static", 60, 20, 10);
    items.clear();
    items.push_back(ModuleWidgetItem(L" ", L"special:color:tolerance", L"editbox", 0, 0, 40, 20));
    items.push_back(ModuleWidgetItem(L" ", L"special:color:tolerance:drag",
                                     m_orientation == ORIENTATION_HORIZONTAL ? L"hdragbutton" : L"vdragbutton", 40, 0,
                                     20, 20));
    addPanel(items, L"", 5);
    addWidget(L"%", L"", L"static", 20, 20, 0);
    getPanel()->getNode()->setEditFilter(L"special:color:tolerance", &plasma::g_unsignedFloatFilter, true);
    placeIcon(L"special:color:apply", L"icon:apply:big");
    placeIcon(L"special:color:cancel", L"icon:cancel");
    m_panel->setChecked(L"special:color:shift:source", true, false, true);
    getPanel()->getNode()->connectWidget(L"special:color:drawing", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &SpecialColorModule::chooseDrawing, true);
    getPanel()->getNode()->connectWidget(L"special:color:selection", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &SpecialColorModule::chooseSelection, true);
    getPanel()->getNode()->connectWidget(L"special:color:apply", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &SpecialColorModule::apply, true);
    getPanel()->getNode()->connectWidget(L"special:color:cancel", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &SpecialColorModule::cancel, true);
    getPanel()->getNode()->connectWidget(L"special:color:brightness", plasma::WIDGET_EVENT_FOCUS_LOST, this,
                                         &SpecialColorModule::editBrightness, true);
    getPanel()->getNode()->connectWidget(L"special:color:brightness:drag", plasma::WIDGET_EVENT_POINTER_MOVED, this,
                                         &SpecialColorModule::dragBrightness, true);
    getPanel()->getNode()->connectWidget(L"special:color:brightness:drag", plasma::WIDGET_EVENT_POINTER_RELEASED, this,
                                         &SpecialColorModule::editBrightness, true);
    getPanel()->getNode()->connectWidget(L"special:color:brightness:drag", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &SpecialColorModule::stepBrightness, true);
    getPanel()->getNode()->connectWidget(L"special:color:brightness:drag", plasma::WIDGET_EVENT_POINTER_DOUBLE_CLICKED,
                                         this, &SpecialColorModule::stepBrightness, true);
    getPanel()->getNode()->connectWidget(L"special:color:contrast", plasma::WIDGET_EVENT_FOCUS_LOST, this,
                                         &SpecialColorModule::editContrast, true);
    getPanel()->getNode()->connectWidget(L"special:color:contrast:drag", plasma::WIDGET_EVENT_POINTER_MOVED, this,
                                         &SpecialColorModule::dragContrast, true);
    getPanel()->getNode()->connectWidget(L"special:color:contrast:drag", plasma::WIDGET_EVENT_POINTER_RELEASED, this,
                                         &SpecialColorModule::editContrast, true);
    getPanel()->getNode()->connectWidget(L"special:color:contrast:drag", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &SpecialColorModule::stepContrast, true);
    getPanel()->getNode()->connectWidget(L"special:color:contrast:drag", plasma::WIDGET_EVENT_POINTER_DOUBLE_CLICKED,
                                         this, &SpecialColorModule::stepContrast, true);
    getPanel()->getNode()->connectWidget(L"special:color:saturation", plasma::WIDGET_EVENT_FOCUS_LOST, this,
                                         &SpecialColorModule::editSaturation, true);
    getPanel()->getNode()->connectWidget(L"special:color:saturation:drag", plasma::WIDGET_EVENT_POINTER_MOVED, this,
                                         &SpecialColorModule::dragSaturation, true);
    getPanel()->getNode()->connectWidget(L"special:color:saturation:drag", plasma::WIDGET_EVENT_POINTER_RELEASED, this,
                                         &SpecialColorModule::editSaturation, true);
    getPanel()->getNode()->connectWidget(L"special:color:saturation:drag", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &SpecialColorModule::stepSaturation, true);
    getPanel()->getNode()->connectWidget(L"special:color:saturation:drag", plasma::WIDGET_EVENT_POINTER_DOUBLE_CLICKED,
                                         this, &SpecialColorModule::stepSaturation, true);
    getPanel()->getNode()->connectWidget(L"special:color:tolerance", plasma::WIDGET_EVENT_FOCUS_LOST, this,
                                         &SpecialColorModule::editTolerance, true);
    getPanel()->getNode()->connectWidget(L"special:color:tolerance:drag", plasma::WIDGET_EVENT_POINTER_MOVED, this,
                                         &SpecialColorModule::dragTolerance, true);
    getPanel()->getNode()->connectWidget(L"special:color:tolerance:drag", plasma::WIDGET_EVENT_POINTER_RELEASED, this,
                                         &SpecialColorModule::editTolerance, true);
    getPanel()->getNode()->connectWidget(L"special:color:tolerance:drag", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &SpecialColorModule::stepTolerance, true);
    getPanel()->getNode()->connectWidget(L"special:color:tolerance:drag", plasma::WIDGET_EVENT_POINTER_DOUBLE_CLICKED,
                                         this, &SpecialColorModule::stepTolerance, true);
    m_sourcePanel = m_workspace->m_engine.findNode(L"special:color:shift:source:colorpanel");
    if (m_sourcePanel)
        m_sourcePanel->setDisplay(m_sourcePanel->m_display->clone());
    m_targetPanel = m_workspace->m_engine.findNode(L"special:color:shift:target:colorpanel");
    if (m_targetPanel)
        m_targetPanel->setDisplay(m_targetPanel->m_display->clone());
    setBrightness(100.0f);
    setContrast(100.0f);
    setSaturation(100.0f);
    setTolerance(m_tolerance);
#ifndef PLASMA_PORTABLE
    startGLThread([this]() {
        while (m_workerRunning) {
            while (m_workerRunning) {
                if (!m_previewPending || !isVisible())
                    break;
                m_previewRunning = true;
                preview();
                m_previewRunning = false;
                if (m_cancel) {
                    m_previewRunning = false;
                    m_cancel = false;
                    m_previewPending = false;
                    break;
                }
            }
            Sleep(ColorWorkerPollMilliseconds);
        }
        m_workerFinished = true;
    });
#endif
}

void SpecialColorModule::followColor(const EditorColor &color)
{
    if (m_panel->isChecked(L"special:color:shift:source", true))
        setSourceColor(color);
    setTargetColor(color);
}

void SpecialColorModule::setSourceColor(const EditorColor &color)
{
    m_sourceColor = color;
    m_sourceColor.m_elements[RGBA_ALPHA] = 1.0f;
    if (m_sourcePanel) {
        m_sourcePanel->m_display->m_fillColor.value() = m_sourceColor;
        m_sourcePanel->m_display->m_changed = true;
    }
}

void SpecialColorModule::setTargetColor(const EditorColor &color)
{
    m_targetColor = color;
    m_targetColor.m_elements[RGBA_ALPHA] = 1.0f;
    if (m_targetPanel) {
        m_targetPanel->m_display->m_fillColor.value() = m_targetColor;
        m_targetPanel->m_display->m_changed = true;
    }
}

void SpecialColorModule::apply()
{
    if (m_workspace->getOpenDesignSheet()) {
        for (std::map<plasma::Node *, NodeInterface *>::iterator it =
                 m_workspace->getOpenDesignSheet()->sheetNodesBegin();
             it != m_workspace->getOpenDesignSheet()->m_sheetNodes.end(); ++it) {
            if ((it->second->m_selected || m_drawing) && it->first->isShown() && it->second->m_shapeInterface &&
                it->second->m_shapeInterface->m_colorPreview) {
                it->second->m_shapeInterface->beginModify();
                it->second->m_shapeInterface->applyColorPreview();
                it->second->m_shapeInterface->endModify();
            }
        }
        m_workspace->getOpenDesignSheet()->commit();
        setSourceColor(m_targetColor);
        setBrightness(100.0f);
        setContrast(100.0f);
        setSaturation(100.0f);
    }
}

void SpecialColorModule::cancel()
{
    if (m_workspace->getOpenDesignSheet()) {
        for (std::map<plasma::Node *, NodeInterface *>::iterator it =
                 m_workspace->getOpenDesignSheet()->sheetNodesBegin();
             it != m_workspace->getOpenDesignSheet()->m_sheetNodes.end(); ++it) {
            if (it->first->isShown() && it->second->m_shapeInterface && it->second->m_shapeInterface->m_colorPreview) {
                it->second->m_shapeInterface->setColorPreview(false);
                it->first->m_shape->attributesChanged(plasma::ATTRIBUTES_CHANGED_ALL);
            }
        }
        setTargetColor(m_sourceColor);
        setBrightness(100.0f);
        setContrast(100.0f);
        setSaturation(100.0f);
    }
}

void SpecialColorModule::preview()
{
    if (m_workspace->getOpenDesignSheet()) {
        std::list<EditorColor> source;
        source.push_back(m_sourceColor);
        m_previewPending = false;
        bool drawing = m_drawing;

        // Shape pointer keys are ordered as unsigned integers.
        std::set<PLASMA_POINTER_UINT> visited;
        for (std::map<plasma::Node *, NodeInterface *>::iterator it =
                 m_workspace->getOpenDesignSheet()->sheetNodesBegin();
             it != m_workspace->getOpenDesignSheet()->m_sheetNodes.end(); ++it) {
            if (m_cancel)
                return;
            if (it->second->getShapeInterface() && it->first->isShown()) {
                if (visited.find((PLASMA_POINTER_UINT)it->first->m_shape) == visited.end() &&
                    (it->second->m_selected || drawing || it->second->getShapeInterface()->m_colorPreview)) {
                    visited.insert((PLASMA_POINTER_UINT)it->first->m_shape);
                    it->second->getShapeInterface()->setColorPreview(false);
                    if (it->second->m_selected || drawing) {
                        it->second->getShapeInterface()->setColorPreview(true);
                        if (m_targetColor != m_sourceColor) {
                            if (m_workspace->m_colorModule->m_link) {
                                it->second->getShapeInterface()->shiftColors(source, m_targetColor, m_tolerance,
                                                                             COLOR_TARGET_FILL, false);
                                it->second->getShapeInterface()->shiftColors(source, m_targetColor, m_tolerance,
                                                                             COLOR_TARGET_STROKE, false);
                                it->second->getShapeInterface()->shiftColors(source, m_targetColor, m_tolerance,
                                                                             COLOR_TARGET_EXTRUSION, false);
                            } else {
                                it->second->getShapeInterface()->shiftColors(
                                    source, m_targetColor, m_tolerance, m_workspace->m_colorModule->getMode(), false);
                            }
                        }
                        if (m_workspace->m_colorModule->m_link) {
                            it->second->getShapeInterface()->adjustBrightness((float)(m_brightness * (double)0.01f),
                                                                              COLOR_TARGET_FILL, false);
                            it->second->getShapeInterface()->adjustBrightness((float)(m_brightness * (double)0.01f),
                                                                              COLOR_TARGET_STROKE, false);
                            it->second->getShapeInterface()->adjustBrightness((float)(m_brightness * (double)0.01f),
                                                                              COLOR_TARGET_EXTRUSION, false);
                            it->second->getShapeInterface()->adjustContrast((float)(m_contrast * (double)0.01f),
                                                                            COLOR_TARGET_FILL, false);
                            it->second->getShapeInterface()->adjustContrast((float)(m_contrast * (double)0.01f),
                                                                            COLOR_TARGET_STROKE, false);
                            it->second->getShapeInterface()->adjustContrast((float)(m_contrast * (double)0.01f),
                                                                            COLOR_TARGET_EXTRUSION, false);
                            it->second->getShapeInterface()->adjustSaturation((float)(m_saturation * (double)0.01f),
                                                                              COLOR_TARGET_FILL, false);
                            it->second->getShapeInterface()->adjustSaturation((float)(m_saturation * (double)0.01f),
                                                                              COLOR_TARGET_STROKE, false);
                            it->second->getShapeInterface()->adjustSaturation((float)(m_saturation * (double)0.01f),
                                                                              COLOR_TARGET_EXTRUSION, false);
                        } else {
                            it->second->getShapeInterface()->adjustBrightness(
                                (float)(m_brightness * (double)0.01f), m_workspace->m_colorModule->getMode(), false);
                            it->second->getShapeInterface()->adjustContrast(
                                (float)(m_contrast * (double)0.01f), m_workspace->m_colorModule->getMode(), false);
                            it->second->getShapeInterface()->adjustSaturation(
                                (float)(m_saturation * (double)0.01f), m_workspace->m_colorModule->getMode(), false);
                        }
                    }
                    it->first->m_shape->attributesChanged(plasma::ATTRIBUTES_CHANGED_ALL);
                    glFlush();
                }
            }
        }
    }
}

void SpecialColorModule::editBrightness()
{
    std::wstringstream(getPanel()->getNode()->getText(L"special:color:brightness", true)) >> m_brightness;
    m_previewPending = true;
}

void SpecialColorModule::setBrightness(float value)
{
    m_brightness = value;
    std::wstringstream text;
    text << roundToHundredths(m_brightness);
    m_panel->setText(L"special:color:brightness", text.str(), true);
}

void SpecialColorModule::dragBrightness(plasma::Widget *sender)
{
    if (isDragging(sender)) {
        plasma::Vector<2, float> movement = m_workspace->m_engine.m_pointer - m_workspace->m_engine.m_previousPointer;
        float value = m_brightness - movement.m_elements[1];
        setBrightness(value);
        m_previewPending = true;
    }
}

void SpecialColorModule::stepBrightness(plasma::Widget *sender)
{
    float value = (double)pressDragButton(sender) + m_brightness;
    setBrightness(value);
    m_previewPending = true;
}

void SpecialColorModule::editContrast()
{
    std::wstringstream(getPanel()->getNode()->getText(L"special:color:contrast", true)) >> m_contrast;
    m_previewPending = true;
}

void SpecialColorModule::setContrast(float value)
{
    m_contrast = value;
    std::wstringstream text;
    text << roundToHundredths(m_contrast);
    m_panel->setText(L"special:color:contrast", text.str(), true);
}

void SpecialColorModule::dragContrast(plasma::Widget *sender)
{
    if (isDragging(sender)) {
        plasma::Vector<2, float> movement = m_workspace->m_engine.m_pointer - m_workspace->m_engine.m_previousPointer;
        float value = m_contrast - movement.m_elements[1];
        setContrast(value);
        m_previewPending = true;
    }
}

void SpecialColorModule::stepContrast(plasma::Widget *sender)
{
    float value = (double)pressDragButton(sender) + m_contrast;
    setContrast(value);
    m_previewPending = true;
}

void SpecialColorModule::editSaturation()
{
    std::wstringstream(getPanel()->getNode()->getText(L"special:color:saturation", true)) >> m_saturation;
    m_previewPending = true;
}

void SpecialColorModule::setSaturation(float value)
{
    m_saturation = value;
    std::wstringstream text;
    text << roundToHundredths(m_saturation);
    m_panel->setText(L"special:color:saturation", text.str(), true);
}

void SpecialColorModule::dragSaturation(plasma::Widget *sender)
{
    if (isDragging(sender)) {
        plasma::Vector<2, float> movement = m_workspace->m_engine.m_pointer - m_workspace->m_engine.m_previousPointer;
        float value = m_saturation - movement.m_elements[1];
        setSaturation(value);
        m_previewPending = true;
    }
}

void SpecialColorModule::stepSaturation(plasma::Widget *sender)
{
    float value = (double)pressDragButton(sender) + m_saturation;
    setSaturation(value);
}

void SpecialColorModule::editTolerance()
{
    std::wstringstream(getPanel()->getNode()->getText(L"special:color:tolerance", true)) >> m_tolerance;
    m_tolerance *= 0.01f;
    m_previewPending = true;
}

void SpecialColorModule::setTolerance(float value)
{
    m_tolerance = value;
    if (value < 0.0f)
        m_tolerance = 0.0f;
    std::wstringstream text;
    text << roundToHundredths((float)(m_tolerance * 100.0));
    m_panel->setText(L"special:color:tolerance", text.str(), true);
}

void SpecialColorModule::dragTolerance(plasma::Widget *sender)
{
    if (isDragging(sender)) {
        plasma::Vector<2, float> movement = m_workspace->m_engine.m_pointer - m_workspace->m_engine.m_previousPointer;
        float value = m_tolerance - movement.m_elements[1] * 0.01f;
        setTolerance(value);
        m_previewPending = true;
    }
}

void SpecialColorModule::stepTolerance(plasma::Widget *sender)
{
    float value = (double)pressDragButton(sender) * (double)0.01f + m_tolerance;
    setTolerance(value);
    m_previewPending = true;
}

void SpecialColorModule::chooseDrawing()
{
    if (m_workspace->getOpenDesignSheet()) {
        m_drawing = true;
        m_previewPending = true;
    }
}

void SpecialColorModule::chooseSelection()
{
    if (m_workspace->getOpenDesignSheet()) {
        m_drawing = false;
        m_previewPending = true;
    }
}

void SpecialColorModule::currentNodeChanged()
{
    if (!m_drawing)
        m_previewPending = true;
}

template void plasma::Node::connectWidget<SpecialColorModule>(const std::wstring &, int, SpecialColorModule *,
                                                              void (SpecialColorModule::*)(), bool);

template void plasma::Node::connectWidget<SpecialColorModule>(const std::wstring &, int, SpecialColorModule *,
                                                              void (SpecialColorModule::*)(plasma::Widget *), bool);

namespace plasma {
template<>
void Widget::setConnection<SpecialColorModule>(Connection *&connection, SpecialColorModule *object,
                                               void (SpecialColorModule::*method)(), bool enabledOnly)
{
    if (connection)
        delete connection;
    connection = new MemberFunctionConnection<SpecialColorModule>(object, method, enabledOnly);
}
}

template void plasma::Widget::setConnection<SpecialColorModule>(plasma::Widget::Connection *&, SpecialColorModule *,
                                                                void (SpecialColorModule::*)(plasma::Widget *), bool);

template void plasma::Widget::connect<SpecialColorModule>(int, SpecialColorModule *, void (SpecialColorModule::*)(),
                                                          bool, bool);

template void plasma::Widget::connect<SpecialColorModule>(int, SpecialColorModule *,
                                                          void (SpecialColorModule::*)(plasma::Widget *), bool, bool);

std::wstring getXmlText(const pugi::xml_node &node)
{
    std::wstring result;
    for (pugi::xml_node_iterator it = node.begin(); it != node.end(); it++) {
        result.append(widenString(std::string(it->value())));
    }
    return result;
}

namespace plasma {
template<> bool Vector<4, float>::operator!=(const Vector<4, float> &other) const
{
    for (int i = 0; i < 4; ++i) {
        if (m_elements[i] != other.m_elements[i])
            return true;
    }
    return false;
}
}

#ifdef PLASMA_PORTABLE

// Process one pending preview on the GL-owning event thread.
void SpecialColorModule::tickPreview()
{
    if (m_previewPending && isVisible()) {
        m_previewRunning = true;
        preview();
        m_previewRunning = false;
    }
    if (m_cancel) {
        m_cancel = false;
        m_previewPending = false;
    }
}
#endif
