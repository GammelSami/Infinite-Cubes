#include "StampModule.h"
#include "FormatModule.h" // The edit box filters.
#include "main.h"
#include "plasma/Engine.h"
#include "plasma/GenericWidget.h"
#include <cstdlib>
#include <sstream>

static const int DefaultStampFrequencyPercent = 80;

// Layout positions, extents and spacing use display units; colour tuples style the overlays.
StampModule::StampModule()
    : m_scale(1.0f, 1.0f),
      m_shear(0.0f, 0.0f),
      m_rotation(0.0f),
      m_scaleJitter(0.0f),
      m_rotationJitter(0.0f),
      m_positionJitter(0.0f),
      m_frequency(DefaultStampFrequencyPercent),
      m_shapeBoundary(false)
{
    {
        std::wstring name(L"module:stamp");
        std::wstring kind(L"bar");
        m_panel = g_workspace.m_engine.createGenericWidget(name, 70.0f, 140.0f, 120.0f, 100.0f,
                                                           g_workspace.m_guiLeftNode, kind);
    }
    m_panel->m_flags |= (1 << plasma::WIDGET_RENDER_SURFACE);
    m_panel->setCaption(L"Stamp", true);
}

// Layout positions, extents and spacing use display units; colour tuples style the overlays.
void StampModule::build()
{
    m_cursor.m_elements[0] = 30;
    m_cursor.m_elements[1] = 25;
    m_margin.m_elements[0] = 30;
    std::list<ModuleWidgetItem> items;
    items.clear();
    items.push_back(ModuleWidgetItem(L"Copy", L"stamp:copy", L"radiobutton2", 0, 0, 100, 25));
    items.push_back(ModuleWidgetItem(L"Instance", L"stamp:instance", L"radiobutton2", 100, 0, 100, 25));
    addPanel(items, L"", 3);
    m_panel->setChecked(L"stamp:copy", true, false, true);
    newRow(3);
    addWidget(L"Scale X:", L"", L"static", 70, 20, 0);
    items.clear();
    items.push_back(ModuleWidgetItem(L" ", L"stamp:scale:x", L"editbox", 0, 0, 60, 20));
    items.push_back(ModuleWidgetItem(L" ", L"stamp:scale:x:drag",
                                     m_orientation == ORIENTATION_HORIZONTAL ? L"hdragbutton" : L"vdragbutton", 60, 0,
                                     20, 20));
    addPanel(items, L"", 3);
    addWidget(L"%", L"", L"static", 20, 20, 0);
    getPanel()->getNode()->setEditFilter(L"stamp:scale:x", &plasma::g_floatFilter, true);

    // The link buttons span two rows without moving the bottom edge.
    float bottom = m_extent.m_elements[1];
    items.clear();
    items.push_back(ModuleWidgetItem(L" ", L"stamp:link:scale", L"checkbutton2", 0, 0, 25, 43));
    addPanel(items, L"", 0);
    placeIcon(L"stamp:link:scale", L"icon:link");
    m_extent.m_elements[1] = bottom;
    newRow(3);
    addWidget(L"Scale Y:", L"", L"static", 70, 20, 0);
    items.clear();
    items.push_back(ModuleWidgetItem(L" ", L"stamp:scale:y", L"editbox", 0, 0, 60, 20));
    items.push_back(ModuleWidgetItem(L" ", L"stamp:scale:y:drag",
                                     m_orientation == ORIENTATION_HORIZONTAL ? L"hdragbutton" : L"vdragbutton", 60, 0,
                                     20, 20));
    addPanel(items, L"", 3);
    addWidget(L"%", L"", L"static", 20, 20, 0);
    getPanel()->getNode()->setEditFilter(L"stamp:scale:y", &plasma::g_floatFilter, true);
    newRow(3);
    addWidget(L"Shear X:", L"", L"static", 70, 20, 0);
    items.clear();
    items.push_back(ModuleWidgetItem(L" ", L"stamp:shear:x", L"editbox", 0, 0, 60, 20));
    items.push_back(ModuleWidgetItem(L" ", L"stamp:shear:x:drag",
                                     m_orientation == ORIENTATION_HORIZONTAL ? L"hdragbutton" : L"vdragbutton", 60, 0,
                                     20, 20));
    addPanel(items, L"", 3);
    addWidget(L"%", L"", L"static", 20, 20, 0);
    getPanel()->getNode()->setEditFilter(L"stamp:shear:x", &plasma::g_floatFilter, true);
    bottom = m_extent.m_elements[1];
    items.clear();
    items.push_back(ModuleWidgetItem(L" ", L"stamp:link:shear", L"checkbutton2", 0, 0, 25, 43));
    addPanel(items, L"", 0);
    placeIcon(L"stamp:link:shear", L"icon:link");
    m_extent.m_elements[1] = bottom;
    newRow(3);
    addWidget(L"Shear Y:", L"", L"static", 70, 20, 0);
    items.clear();
    items.push_back(ModuleWidgetItem(L" ", L"stamp:shear:y", L"editbox", 0, 0, 60, 20));
    items.push_back(ModuleWidgetItem(L" ", L"stamp:shear:y:drag",
                                     m_orientation == ORIENTATION_HORIZONTAL ? L"hdragbutton" : L"vdragbutton", 60, 0,
                                     20, 20));
    addPanel(items, L"", 3);
    addWidget(L"%", L"", L"static", 20, 20, 0);
    getPanel()->getNode()->setEditFilter(L"stamp:shear:y", &plasma::g_floatFilter, true);
    newRow(3);
    addWidget(L"Angle:", L"", L"static", 70, 20, 0);
    items.clear();
    items.push_back(ModuleWidgetItem(L" ", L"stamp:rotation", L"editbox", 0, 0, 60, 20));
    items.push_back(ModuleWidgetItem(L" ", L"stamp:rotation:drag",
                                     m_orientation == ORIENTATION_HORIZONTAL ? L"hdragbutton" : L"vdragbutton", 60, 0,
                                     20, 20));
    addPanel(items, L"", 3);
    addWidget(L"\u00b0", L"", L"static", 20, 20, 0);
    getPanel()->getNode()->setEditFilter(L"stamp:rotation", &plasma::g_floatFilter, true);
    newRow(3);
    items.clear();
    items.push_back(ModuleWidgetItem(L"Reset", L"stamp:reset", L"button2", 0, 0, 200, 30));
    addPanel(items, L"", 0);
    newRow(3);
    addWidget(L"Bounds:", L"", L"static", 70, 20, 0);
    items.clear();
    items.push_back(ModuleWidgetItem(L" ", L"stamp:boundary:control", L"radiobutton2", 0, 0, 25, 25));
    items.push_back(ModuleWidgetItem(L" ", L"stamp:boundary:shape", L"radiobutton2", 25, 0, 25, 25));
    addPanel(items, L"", 0);
    placeIcon(L"stamp:boundary:control", L"icon:bounds:control");
    placeIcon(L"stamp:boundary:shape", L"icon:bounds:shape");
    newRow(3);
    addWidget(L"Frequency:", L"", L"static", 70, 20, 0);
    items.clear();
    items.push_back(ModuleWidgetItem(L" ", L"stamp:frequency", L"editbox", 0, 0, 60, 20));
    items.push_back(ModuleWidgetItem(L" ", L"stamp:frequency:drag",
                                     m_orientation == ORIENTATION_HORIZONTAL ? L"hdragbutton" : L"vdragbutton", 60, 0,
                                     20, 20));
    addPanel(items, L"", 0);
    addWidget(L"%", L"", L"static", 20, 20, 0);
    getPanel()->getNode()->setEditFilter(L"stamp:frequency", &plasma::g_unsignedIntegerFilter, true);
    newRow(3);
    addWidget(L"Position jitter:", L"", L"static", 70, 20, 0);
    items.clear();
    items.push_back(ModuleWidgetItem(L" ", L"stamp:position:jitter", L"editbox", 0, 0, 60, 20));
    items.push_back(ModuleWidgetItem(L" ", L"stamp:position:jitter:drag",
                                     m_orientation == ORIENTATION_HORIZONTAL ? L"hdragbutton" : L"vdragbutton", 60, 0,
                                     20, 20));
    addPanel(items, L"", 0);
    addWidget(L"px", L"text:unit", L"static", 20, 20, 0);
    getPanel()->getNode()->setEditFilter(L"stamp:position:jitter", &plasma::g_unsignedFloatFilter, true);
    newRow(3);
    addWidget(L"Rotation jitter:", L"", L"static", 70, 20, 0);
    items.clear();
    items.push_back(ModuleWidgetItem(L" ", L"stamp:rotation:jitter", L"editbox", 0, 0, 60, 20));
    items.push_back(ModuleWidgetItem(L" ", L"stamp:rotation:jitter:drag",
                                     m_orientation == ORIENTATION_HORIZONTAL ? L"hdragbutton" : L"vdragbutton", 60, 0,
                                     20, 20));
    addPanel(items, L"", 0);
    addWidget(L"\u00b0", L"", L"static", 20, 20, 0);
    getPanel()->getNode()->setEditFilter(L"stamp:rotation:jitter", &plasma::g_unsignedFloatFilter, true);
    newRow(3);
    addWidget(L"Scale jitter:", L"", L"static", 70, 20, 0);
    items.clear();
    items.push_back(ModuleWidgetItem(L" ", L"stamp:scale:jitter", L"editbox", 0, 0, 60, 20));
    items.push_back(ModuleWidgetItem(L" ", L"stamp:scale:jitter:drag",
                                     m_orientation == ORIENTATION_HORIZONTAL ? L"hdragbutton" : L"vdragbutton", 60, 0,
                                     20, 20));
    addPanel(items, L"", 0);
    addWidget(L"%", L"", L"static", 20, 20, 0);
    getPanel()->getNode()->setEditFilter(L"stamp:scale:jitter", &plasma::g_unsignedFloatFilter, true);
    m_margin.m_elements[0] = 10;
    fitWidth(10);
    fitHeight(10);
    addZoomBar();
    getPanel()->getNode()->connectWidget(L"stamp:scale:x", plasma::WIDGET_EVENT_FOCUS_LOST, this,
                                         &StampModule::editScaleX, true);
    getPanel()->getNode()->connectWidget(L"stamp:scale:x:drag", plasma::WIDGET_EVENT_POINTER_MOVED, this,
                                         &StampModule::dragScaleX, true);
    getPanel()->getNode()->connectWidget(L"stamp:scale:x:drag", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &StampModule::stepScaleX, true);
    getPanel()->getNode()->connectWidget(L"stamp:scale:x:drag", plasma::WIDGET_EVENT_POINTER_DOUBLE_CLICKED, this,
                                         &StampModule::stepScaleX, true);
    getPanel()->getNode()->connectWidget(L"stamp:scale:y", plasma::WIDGET_EVENT_FOCUS_LOST, this,
                                         &StampModule::editScaleY, true);
    getPanel()->getNode()->connectWidget(L"stamp:scale:y:drag", plasma::WIDGET_EVENT_POINTER_MOVED, this,
                                         &StampModule::dragScaleY, true);
    getPanel()->getNode()->connectWidget(L"stamp:scale:y:drag", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &StampModule::stepScaleY, true);
    getPanel()->getNode()->connectWidget(L"stamp:scale:y:drag", plasma::WIDGET_EVENT_POINTER_DOUBLE_CLICKED, this,
                                         &StampModule::stepScaleY, true);
    getPanel()->getNode()->connectWidget(L"stamp:shear:x", plasma::WIDGET_EVENT_FOCUS_LOST, this,
                                         &StampModule::editShearX, true);
    getPanel()->getNode()->connectWidget(L"stamp:shear:x:drag", plasma::WIDGET_EVENT_POINTER_MOVED, this,
                                         &StampModule::dragShearX, true);
    getPanel()->getNode()->connectWidget(L"stamp:shear:x:drag", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &StampModule::stepShearX, true);
    getPanel()->getNode()->connectWidget(L"stamp:shear:x:drag", plasma::WIDGET_EVENT_POINTER_DOUBLE_CLICKED, this,
                                         &StampModule::stepShearX, true);
    getPanel()->getNode()->connectWidget(L"stamp:shear:y", plasma::WIDGET_EVENT_FOCUS_LOST, this,
                                         &StampModule::editShearY, true);
    getPanel()->getNode()->connectWidget(L"stamp:shear:y:drag", plasma::WIDGET_EVENT_POINTER_MOVED, this,
                                         &StampModule::dragShearY, true);
    getPanel()->getNode()->connectWidget(L"stamp:shear:y:drag", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &StampModule::stepShearY, true);
    getPanel()->getNode()->connectWidget(L"stamp:shear:y:drag", plasma::WIDGET_EVENT_POINTER_DOUBLE_CLICKED, this,
                                         &StampModule::stepShearY, true);
    getPanel()->getNode()->connectWidget(L"stamp:rotation", plasma::WIDGET_EVENT_FOCUS_LOST, this,
                                         &StampModule::editRotation, true);
    getPanel()->getNode()->connectWidget(L"stamp:rotation:drag", plasma::WIDGET_EVENT_POINTER_MOVED, this,
                                         &StampModule::dragRotation, true);
    getPanel()->getNode()->connectWidget(L"stamp:rotation:drag", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &StampModule::stepRotation, true);
    getPanel()->getNode()->connectWidget(L"stamp:rotation:drag", plasma::WIDGET_EVENT_POINTER_DOUBLE_CLICKED, this,
                                         &StampModule::stepRotation, true);
    getPanel()->getNode()->connectWidget(L"stamp:frequency", plasma::WIDGET_EVENT_FOCUS_LOST, this,
                                         &StampModule::editFrequency, true);
    getPanel()->getNode()->connectWidget(L"stamp:frequency:drag", plasma::WIDGET_EVENT_POINTER_MOVED, this,
                                         &StampModule::dragFrequency, true);
    getPanel()->getNode()->connectWidget(L"stamp:frequency:drag", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &StampModule::stepFrequency, true);
    getPanel()->getNode()->connectWidget(L"stamp:frequency:drag", plasma::WIDGET_EVENT_POINTER_DOUBLE_CLICKED, this,
                                         &StampModule::stepFrequency, true);
    getPanel()->getNode()->connectWidget(L"stamp:position:jitter", plasma::WIDGET_EVENT_FOCUS_LOST, this,
                                         &StampModule::editPositionJitter, true);
    getPanel()->getNode()->connectWidget(L"stamp:position:jitter:drag", plasma::WIDGET_EVENT_POINTER_MOVED, this,
                                         &StampModule::dragPositionJitter, true);
    getPanel()->getNode()->connectWidget(L"stamp:position:jitter:drag", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &StampModule::stepPositionJitter, true);
    getPanel()->getNode()->connectWidget(L"stamp:position:jitter:drag", plasma::WIDGET_EVENT_POINTER_DOUBLE_CLICKED,
                                         this, &StampModule::stepPositionJitter, true);
    getPanel()->getNode()->connectWidget(L"stamp:rotation:jitter", plasma::WIDGET_EVENT_FOCUS_LOST, this,
                                         &StampModule::editRotationJitter, true);
    getPanel()->getNode()->connectWidget(L"stamp:rotation:jitter:drag", plasma::WIDGET_EVENT_POINTER_MOVED, this,
                                         &StampModule::dragRotationJitter, true);
    getPanel()->getNode()->connectWidget(L"stamp:rotation:jitter:drag", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &StampModule::stepRotationJitter, true);
    getPanel()->getNode()->connectWidget(L"stamp:rotation:jitter:drag", plasma::WIDGET_EVENT_POINTER_DOUBLE_CLICKED,
                                         this, &StampModule::stepRotationJitter, true);
    getPanel()->getNode()->connectWidget(L"stamp:scale:jitter", plasma::WIDGET_EVENT_FOCUS_LOST, this,
                                         &StampModule::editScaleJitter, true);
    getPanel()->getNode()->connectWidget(L"stamp:scale:jitter:drag", plasma::WIDGET_EVENT_POINTER_MOVED, this,
                                         &StampModule::dragScaleJitter, true);
    getPanel()->getNode()->connectWidget(L"stamp:scale:jitter:drag", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &StampModule::stepScaleJitter, true);
    getPanel()->getNode()->connectWidget(L"stamp:scale:jitter:drag", plasma::WIDGET_EVENT_POINTER_DOUBLE_CLICKED, this,
                                         &StampModule::stepScaleJitter, true);
    getPanel()->getNode()->connectWidget(L"stamp:reset", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &StampModule::reset, true);
    getPanel()->getNode()->connectWidget(L"stamp:boundary:control", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &StampModule::useControlBoundary, true);
    getPanel()->getNode()->connectWidget(L"stamp:boundary:shape", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &StampModule::useShapeBoundary, true);
    update();
    createZoomSequences();
}

void StampModule::update()
{
    std::wstringstream text;
    text << roundToHundredths(m_rotation);
    m_panel->setText(L"stamp:rotation", text.str(), true);
    text.str(L"");
    text << roundToHundredths(m_scale.m_elements[0] * 100.0f);
    m_panel->setText(L"stamp:scale:x", text.str(), true);
    text.str(L"");
    text << roundToHundredths(m_scale.m_elements[1] * 100.0f);
    m_panel->setText(L"stamp:scale:y", text.str(), true);
    if (getPanel()->getNode()->isChecked(L"stamp:link:shear", true)) {
        m_shear.m_elements[0] = 0.0f;
        m_shear.m_elements[1] = 0.0f;
    }
    text.str(L"");
    text << roundToHundredths(m_shear.m_elements[0] * 100.0f);
    m_panel->setText(L"stamp:shear:x", text.str(), true);
    text.str(L"");
    text << roundToHundredths(m_shear.m_elements[1] * 100.0f);
    m_panel->setText(L"stamp:shear:y", text.str(), true);
    if (m_frequency < 0)
        m_frequency = 0;
    if (m_frequency > 100)
        m_frequency = 100;
    text.str(L"");
    text << roundToHundredths((float)m_frequency);
    m_panel->setText(L"stamp:frequency", text.str(), true);
    if (m_positionJitter < 0.0f)
        m_positionJitter = 0.0f;
    text.str(L"");
    text << roundToHundredths(m_positionJitter);
    m_panel->setText(L"stamp:position:jitter", text.str(), true);
    text.str(L"");
    if (m_scaleJitter < 0.0f)
        m_scaleJitter = 0.0f;
    text << roundToHundredths(m_scaleJitter * 100.0f);
    m_panel->setText(L"stamp:scale:jitter", text.str(), true);
    if (m_rotationJitter < 0.0f)
        m_rotationJitter = 0.0f;
    text.str(L"");
    text << roundToHundredths(m_rotationJitter);
    m_panel->setText(L"stamp:rotation:jitter", text.str(), true);
    getPanel()->getNode()->setChecked(L"stamp:boundary:shape", m_shapeBoundary, false, true);
    getPanel()->getNode()->setChecked(L"stamp:boundary:control", !m_shapeBoundary, false, true);
}

void StampModule::reset()
{
    m_rotation = 0.0f;
    m_scale = plasma::Vector<2, float>(1.0f, 1.0f);
    m_shear = plasma::Vector<2, float>(0.0f, 0.0f);
    update();
}

void StampModule::editScaleX()
{
    m_scale.m_elements[0] =
        (float)(atof(narrowString(getPanel()->getNode()->getText(L"stamp:scale:x", true)).c_str()) * 0.01f);
    if (m_panel->isChecked(L"stamp:link:scale", true))
        m_scale.m_elements[1] = m_scale.m_elements[0];
    update();
}

void StampModule::dragScaleX(plasma::Widget *sender)
{
    if (isDragging(sender)) {
        m_scale.m_elements[0] = m_scale.m_elements[0] - (float)(m_workspace->m_engine.m_pointer.m_elements[1] -
                                                                m_workspace->m_engine.m_previousPointer.m_elements[1]) *
                                                            0.01f;
        if (m_panel->isChecked(L"stamp:link:scale", true))
            m_scale.m_elements[1] = m_scale.m_elements[0];
        update();
    }
}

void StampModule::stepScaleX(plasma::Widget *sender)
{
    m_scale.m_elements[0] += pressDragButton(sender) * 0.01f;
    if (m_panel->isChecked(L"stamp:link:scale", true))
        m_scale.m_elements[1] = m_scale.m_elements[0];
    update();
}

void StampModule::editScaleY()
{
    m_scale.m_elements[1] =
        (float)(atof(narrowString(getPanel()->getNode()->getText(L"stamp:scale:y", true)).c_str()) * 0.01f);
    if (m_panel->isChecked(L"stamp:link:scale", true))
        m_scale.m_elements[0] = m_scale.m_elements[1];
    update();
}

void StampModule::dragScaleY(plasma::Widget *sender)
{
    if (isDragging(sender)) {
        m_scale.m_elements[1] = m_scale.m_elements[1] - (float)(m_workspace->m_engine.m_pointer.m_elements[1] -
                                                                m_workspace->m_engine.m_previousPointer.m_elements[1]) *
                                                            0.01f;
        if (m_panel->isChecked(L"stamp:link:scale", true))
            m_scale.m_elements[0] = m_scale.m_elements[1];
        update();
    }
}

void StampModule::stepScaleY(plasma::Widget *sender)
{
    m_scale.m_elements[1] += pressDragButton(sender) * 0.01f;
    if (m_panel->isChecked(L"stamp:link:scale", true))
        m_scale.m_elements[0] = m_scale.m_elements[1];
    update();
}

void StampModule::editShearX()
{
    m_shear.m_elements[0] =
        (float)(atof(narrowString(getPanel()->getNode()->getText(L"stamp:shear:x", true)).c_str()) * 0.01f);
    if (m_panel->isChecked(L"stamp:link:shear", true))
        m_shear.m_elements[1] = m_shear.m_elements[0];
    update();
}

void StampModule::dragShearX(plasma::Widget *sender)
{
    if (isDragging(sender)) {
        m_shear.m_elements[0] = m_shear.m_elements[0] - (float)(m_workspace->m_engine.m_pointer.m_elements[1] -
                                                                m_workspace->m_engine.m_previousPointer.m_elements[1]) *
                                                            0.01f;
        if (m_panel->isChecked(L"stamp:link:shear", true))
            m_shear.m_elements[1] = m_shear.m_elements[0];
        update();
    }
}

void StampModule::stepShearX(plasma::Widget *sender)
{
    m_shear.m_elements[0] += pressDragButton(sender) * 0.01f;
    if (m_panel->isChecked(L"stamp:link:shear", true))
        m_shear.m_elements[1] = m_shear.m_elements[0];
    update();
}

void StampModule::editShearY()
{
    m_shear.m_elements[1] =
        (float)(atof(narrowString(getPanel()->getNode()->getText(L"stamp:shear:y", true)).c_str()) * 0.01f);
    if (m_panel->isChecked(L"stamp:link:shear", true))
        m_shear.m_elements[0] = m_shear.m_elements[1];
    update();
}

void StampModule::dragShearY(plasma::Widget *sender)
{
    if (isDragging(sender)) {
        m_shear.m_elements[1] = m_shear.m_elements[1] - (float)(m_workspace->m_engine.m_pointer.m_elements[1] -
                                                                m_workspace->m_engine.m_previousPointer.m_elements[1]) *
                                                            0.01f;
        if (m_panel->isChecked(L"stamp:link:shear", true))
            m_shear.m_elements[0] = m_shear.m_elements[1];
        update();
    }
}

void StampModule::stepShearY(plasma::Widget *sender)
{
    m_shear.m_elements[1] += pressDragButton(sender) * 0.01f;
    if (m_panel->isChecked(L"stamp:link:shear", true))
        m_shear.m_elements[0] = m_shear.m_elements[1];
    update();
}

void StampModule::editRotation()
{
    m_rotation = (float)atof(narrowString(getPanel()->getNode()->getText(L"stamp:rotation", true)).c_str());
    update();
}

void StampModule::dragRotation(plasma::Widget *sender)
{
    if (isDragging(sender)) {
        float movement =
            m_workspace->m_engine.m_pointer.m_elements[1] - m_workspace->m_engine.m_previousPointer.m_elements[1];
        m_rotation -= movement;
        update();
    }
}

void StampModule::stepRotation(plasma::Widget *sender)
{
    m_rotation += pressDragButton(sender);
    update();
}

void StampModule::editFrequency()
{
    m_frequency = atoi(narrowString(getPanel()->getNode()->getText(L"stamp:frequency", true)).c_str());
    update();
}

void StampModule::dragFrequency(plasma::Widget *sender)
{
    if (isDragging(sender)) {
        float movement =
            m_workspace->m_engine.m_pointer.m_elements[1] - m_workspace->m_engine.m_previousPointer.m_elements[1];
        m_frequency = (int)(m_frequency - movement);
        update();
    }
}

void StampModule::stepFrequency(plasma::Widget *sender)
{
    m_frequency += pressDragButton(sender);
    update();
}

void StampModule::editRotationJitter()
{
    m_rotationJitter =
        (float)atof(narrowString(getPanel()->getNode()->getText(L"stamp:rotation:jitter", true)).c_str());
    update();
}

void StampModule::dragRotationJitter(plasma::Widget *sender)
{
    if (isDragging(sender)) {
        float movement =
            m_workspace->m_engine.m_pointer.m_elements[1] - m_workspace->m_engine.m_previousPointer.m_elements[1];
        m_rotationJitter -= movement;
        update();
    }
}

void StampModule::stepRotationJitter(plasma::Widget *sender)
{
    m_rotationJitter += pressDragButton(sender);
    update();
}

void StampModule::editScaleJitter()
{
    m_scaleJitter =
        (float)(atof(narrowString(getPanel()->getNode()->getText(L"stamp:scale:jitter", true)).c_str()) * 0.01f);
    update();
}

void StampModule::dragScaleJitter(plasma::Widget *sender)
{
    if (isDragging(sender)) {
        float movement =
            m_workspace->m_engine.m_pointer.m_elements[1] - m_workspace->m_engine.m_previousPointer.m_elements[1];
        m_scaleJitter -= movement * 0.01f;
        update();
    }
}

void StampModule::stepScaleJitter(plasma::Widget *sender)
{
    m_scaleJitter += pressDragButton(sender) * 0.01f;
    update();
}

void StampModule::editPositionJitter()
{
    m_positionJitter =
        (float)atof(narrowString(getPanel()->getNode()->getText(L"stamp:position:jitter", true)).c_str());
    update();
}

void StampModule::dragPositionJitter(plasma::Widget *sender)
{
    if (isDragging(sender)) {
        float movement =
            m_workspace->m_engine.m_pointer.m_elements[1] - m_workspace->m_engine.m_previousPointer.m_elements[1];
        m_positionJitter -= movement;
        update();
    }
}

void StampModule::stepPositionJitter(plasma::Widget *sender)
{
    m_positionJitter += pressDragButton(sender);
    update();
}

void StampModule::setScale(const plasma::Vector<2, float> &scale)
{
    m_scale = scale;
    if (m_panel->isChecked(L"stamp:link:scale", true))
        m_scale.m_elements[1] = m_scale.m_elements[0];
    update();
}

void StampModule::setShear(const plasma::Vector<2, float> &shear)
{
    m_shear = shear;
    update();
}

void StampModule::setRotation(float rotation)
{
    m_rotation = rotation;
    update();
}

bool StampModule::isCopying()
{
    return m_panel->isChecked(L"stamp:copy", true);
}

void StampModule::useShapeBoundary()
{
    m_shapeBoundary = true;
    update();
}

void StampModule::useControlBoundary()
{
    m_shapeBoundary = false;
    update();
}
