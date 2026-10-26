#include "ScaleBoxModule.h"
#include "BoundingBoxTool.h"
#include "DesignSheet.h"
#include "Operations.h"
#include "ToolModule.h"
#include "plasma/Engine.h"
#include "plasma/GenericWidget.h"
#include "plasma/ListWidget.h"
#include "FormatModule.h" // The edit box filters.
#include <sstream>

// Layout positions, extents and spacing use display units; colour tuples style the overlays.
ScaleBoxModule::ScaleBoxModule()
{
    m_boxPosition = plasma::Vector<2, float>(0.0f, 0.0f);
    m_boxSize = plasma::Vector<2, float>(0.0f, 0.0f);
    m_expansion = 1.0f;
    m_expandDirection = 0;
    m_rotation = 0.0f;
    {
        std::wstring name(L"module:scalebox");
        std::wstring kind(L"bar");
        m_panel = g_workspace.m_engine.createGenericWidget(name, 380.0f, 75.0f, 120.0f, 100.0f,
                                                           g_workspace.m_guiLeftNode, kind);
    }
    m_panel->m_flags |= (1 << plasma::WIDGET_RENDER_SURFACE);
    m_orientation = ORIENTATION_HORIZONTAL;
}

// Layout positions, extents and spacing use display units; colour tuples style the overlays.
void ScaleBoxModule::build()
{
    std::list<ModuleWidgetItem> items;
    m_cursor.m_elements[0] = 20;
    m_cursor.m_elements[1] = 10;
    addWidget(L"X:", L"", L"static", 15, 20, 0);
    items.clear();
    items.push_back(ModuleWidgetItem(L" ", L"scalebox:x", L"editbox", 0, 0, 45, 20));
    items.push_back(ModuleWidgetItem(L" ", L"scalebox:x:drag",
                                     m_orientation == ORIENTATION_HORIZONTAL ? L"hdragbutton" : L"vdragbutton", 45, 0,
                                     20, 20));
    addPanel(items, L"", 0);
    getPanel()->getNode()->setEditFilter(L"scalebox:x", &plasma::g_floatFilter, true);
    addWidget(L"Y:", L"", L"static", 15, 20, 0);
    items.clear();
    items.push_back(ModuleWidgetItem(L" ", L"scalebox:y", L"editbox", 0, 0, 45, 20));
    items.push_back(ModuleWidgetItem(L" ", L"scalebox:y:drag",
                                     m_orientation == ORIENTATION_HORIZONTAL ? L"hdragbutton" : L"vdragbutton", 45, 0,
                                     20, 20));
    addPanel(items, L"", 0);
    getPanel()->getNode()->setEditFilter(L"scalebox:y", &plasma::g_floatFilter, true);
    addWidget(L"W:", L"", L"static", 17, 20, 0);
    items.clear();
    items.push_back(ModuleWidgetItem(L" ", L"scalebox:width", L"editbox", 0, 0, 45, 20));
    items.push_back(ModuleWidgetItem(L" ", L"scalebox:width:drag",
                                     m_orientation == ORIENTATION_HORIZONTAL ? L"hdragbutton" : L"vdragbutton", 45, 0,
                                     20, 20));
    addPanel(items, L"", 0);
    getPanel()->getNode()->setEditFilter(L"scalebox:width", &plasma::g_floatFilter, true);
    addWidget(L"H:", L"", L"static", 15, 20, 0);
    items.clear();
    items.push_back(ModuleWidgetItem(L" ", L"scalebox:height", L"editbox", 0, 0, 45, 20));
    items.push_back(ModuleWidgetItem(L" ", L"scalebox:height:drag",
                                     m_orientation == ORIENTATION_HORIZONTAL ? L"hdragbutton" : L"vdragbutton", 45, 0,
                                     20, 20));
    addPanel(items, L"", 0);
    getPanel()->getNode()->setEditFilter(L"scalebox:height", &plasma::g_floatFilter, true);
    items.clear();
    items.push_back(ModuleWidgetItem(L"Rotate", L"scalebox:rotate", L"button2", 0, 0, 40, 20));
    items.push_back(ModuleWidgetItem(L" ", L"scalebox:rotation", L"editbox", 40, 0, 40, 20));
    items.push_back(ModuleWidgetItem(L" ", L"scalebox:rotation:drag",
                                     m_orientation == ORIENTATION_HORIZONTAL ? L"hdragbutton" : L"vdragbutton", 80, 0,
                                     20, 20));
    addPanel(items, L"", 3);
    getPanel()->getNode()->setEditFilter(L"scalebox:rotation", &plasma::g_floatFilter, true);
    m_cursor.m_elements[1] += 3;
    addWidget(L" ", L"scalebox:expand", L"expander", 15, 15, 3);
    {
        std::wstring expandName(L"scalebox:expand");
        m_panel->setChecked(expandName, true, false, true);
        fitWidth(10);
    }
    fitHeight(10);

    // The widgets keep their place on the right when the panel expands.
    std::list<plasma::Widget *> widgets = m_panel->getChildWidgets();
    for (std::list<plasma::Widget *>::iterator it = widgets.begin(); it != widgets.end(); ++it)
        (*it)->m_horizontalAlignment = plasma::ALIGN_RIGHT;

    m_panel->getNode()->connectWidget(L"scalebox:x", plasma::WIDGET_EVENT_FOCUS_LOST, this, &ScaleBoxModule::editX,
                                      true);
    m_panel->getNode()->connectWidget(L"scalebox:x:drag", plasma::WIDGET_EVENT_POINTER_MOVED, this,
                                      &ScaleBoxModule::dragX, true);
    m_panel->getNode()->connectWidget(L"scalebox:x:drag", plasma::WIDGET_EVENT_POINTER_RELEASED, this,
                                      &ScaleBoxModule::endDrag, true);
    m_panel->getNode()->connectWidget(L"scalebox:x:drag", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                      &ScaleBoxModule::stepX, true);
    m_panel->getNode()->connectWidget(L"scalebox:x:drag", plasma::WIDGET_EVENT_POINTER_DOUBLE_CLICKED, this,
                                      &ScaleBoxModule::stepX, true);
    m_panel->getNode()->connectWidget(L"scalebox:y", plasma::WIDGET_EVENT_FOCUS_LOST, this, &ScaleBoxModule::editY,
                                      true);
    m_panel->getNode()->connectWidget(L"scalebox:y:drag", plasma::WIDGET_EVENT_POINTER_MOVED, this,
                                      &ScaleBoxModule::dragY, true);
    m_panel->getNode()->connectWidget(L"scalebox:y:drag", plasma::WIDGET_EVENT_POINTER_RELEASED, this,
                                      &ScaleBoxModule::endDrag, true);
    m_panel->getNode()->connectWidget(L"scalebox:y:drag", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                      &ScaleBoxModule::stepY, true);
    m_panel->getNode()->connectWidget(L"scalebox:y:drag", plasma::WIDGET_EVENT_POINTER_DOUBLE_CLICKED, this,
                                      &ScaleBoxModule::stepY, true);
    m_panel->getNode()->connectWidget(L"scalebox:width", plasma::WIDGET_EVENT_FOCUS_LOST, this,
                                      &ScaleBoxModule::editWidth, true);
    m_panel->getNode()->connectWidget(L"scalebox:width:drag", plasma::WIDGET_EVENT_POINTER_MOVED, this,
                                      &ScaleBoxModule::dragWidth, true);
    m_panel->getNode()->connectWidget(L"scalebox:width:drag", plasma::WIDGET_EVENT_POINTER_RELEASED, this,
                                      &ScaleBoxModule::endDrag, true);
    m_panel->getNode()->connectWidget(L"scalebox:width:drag", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                      &ScaleBoxModule::stepWidth, true);
    m_panel->getNode()->connectWidget(L"scalebox:width:drag", plasma::WIDGET_EVENT_POINTER_DOUBLE_CLICKED, this,
                                      &ScaleBoxModule::stepWidth, true);
    getPanel()->getNode()->connectWidget(L"scalebox:height", plasma::WIDGET_EVENT_FOCUS_LOST, this,
                                         &ScaleBoxModule::editHeight, true);
    getPanel()->getNode()->connectWidget(L"scalebox:height:drag", plasma::WIDGET_EVENT_POINTER_MOVED, this,
                                         &ScaleBoxModule::dragHeight, true);
    getPanel()->getNode()->connectWidget(L"scalebox:height:drag", plasma::WIDGET_EVENT_POINTER_RELEASED, this,
                                         &ScaleBoxModule::endDrag, true);
    getPanel()->getNode()->connectWidget(L"scalebox:height:drag", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &ScaleBoxModule::stepHeight, true);
    getPanel()->getNode()->connectWidget(L"scalebox:height:drag", plasma::WIDGET_EVENT_POINTER_DOUBLE_CLICKED, this,
                                         &ScaleBoxModule::stepHeight, true);
    getPanel()->getNode()->connectWidget(L"scalebox:rotation", plasma::WIDGET_EVENT_FOCUS_LOST, this,
                                         &ScaleBoxModule::editRotation, true);
    getPanel()->getNode()->connectWidget(L"scalebox:rotation:drag", plasma::WIDGET_EVENT_POINTER_MOVED, this,
                                         &ScaleBoxModule::dragRotation, true);
    getPanel()->getNode()->connectWidget(L"scalebox:rotation:drag", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &ScaleBoxModule::stepRotation, true);
    getPanel()->getNode()->connectWidget(L"scalebox:rotation:drag", plasma::WIDGET_EVENT_POINTER_DOUBLE_CLICKED, this,
                                         &ScaleBoxModule::stepRotation, true);
    getPanel()->getNode()->connectWidget(L"scalebox:rotate", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &ScaleBoxModule::rotate, true);
    getPanel()->getNode()->connectWidget(L"scalebox:rotate", plasma::WIDGET_EVENT_POINTER_DOUBLE_CLICKED, this,
                                         &ScaleBoxModule::rotate, true);
    getPanel()->getNode()->connectWidget(L"scalebox:expand", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &ScaleBoxModule::chooseExpand, true);
    getPanel()->getNode()->connectWidget(L"scalebox:expand", plasma::WIDGET_EVENT_POINTER_DOUBLE_CLICKED, this,
                                         &ScaleBoxModule::chooseExpand, true);

    std::wstringstream text;
    text << roundToHundredths(m_rotation);
    m_panel->setText(L"scalebox:rotation", text.str(), true);
}

void ScaleBoxModule::update()
{
    ToolModule *toolModule = m_workspace->m_toolModule;
    BoundingBoxTool *tool = dynamic_cast<BoundingBoxTool *>(toolModule->getCurrentTool());
    if (!tool)
        return;
    if (tool->m_boxMax.m_elements[0] < tool->m_boxMin.m_elements[0]) {
        // No box.
        m_panel->setText(L"scalebox:x", L" ", true);
        m_panel->setText(L"scalebox:y", L" ", true);
        m_panel->setText(L"scalebox:width", L" ", true);
        m_panel->setText(L"scalebox:height", L" ", true);
        getPanel()->getNode()->setEnabled(L"scalebox:x", false, true);
        getPanel()->getNode()->setEnabled(L"scalebox:y", false, true);
        getPanel()->getNode()->setEnabled(L"scalebox:width", false, true);
        getPanel()->getNode()->setEnabled(L"scalebox:height", false, true);
        getPanel()->getNode()->setEnabled(L"scalebox:drag:x", false, true);
        getPanel()->getNode()->setEnabled(L"scalebox:drag:y", false, true);
        getPanel()->getNode()->setEnabled(L"scalebox:drag:width", false, true);
        getPanel()->getNode()->setEnabled(L"scalebox:drag:height", false, true);
    } else {
        getPanel()->getNode()->setEnabled(L"scalebox:x", true, true);
        getPanel()->getNode()->setEnabled(L"scalebox:y", true, true);
        getPanel()->getNode()->setEnabled(L"scalebox:width", true, true);
        getPanel()->getNode()->setEnabled(L"scalebox:height", true, true);

        // The drag buttons are named "scalebox:x:drag" and so on; the names used here do not match any widget.
        getPanel()->getNode()->setEnabled(L"scalebox:drag:x", true, true);
        getPanel()->getNode()->setEnabled(L"scalebox:drag:y", true, true);
        getPanel()->getNode()->setEnabled(L"scalebox:drag:width", true, true);
        getPanel()->getNode()->setEnabled(L"scalebox:drag:height", true, true);
        std::wstringstream text;
        text.str(L"");
        text << roundToHundredths(m_workspace->toDisplayUnits(tool->m_boxMin.m_elements[0]));
        m_panel->setText(L"scalebox:x", text.str(), true);
        text.str(L"");
        text << roundToHundredths(m_workspace->toDisplayUnits(tool->m_boxMin.m_elements[1]));
        m_panel->setText(L"scalebox:y", text.str(), true);
        text.str(L"");
        float width = tool->m_boxMax.m_elements[0] - tool->m_boxMin.m_elements[0];
        text << roundToHundredths(m_workspace->toDisplayUnits(width));
        m_panel->setText(L"scalebox:width", text.str(), true);
        text.str(L"");
        float height = tool->m_boxMax.m_elements[1] - tool->m_boxMin.m_elements[1];
        text << roundToHundredths(m_workspace->toDisplayUnits(height));
        m_panel->setText(L"scalebox:height", text.str(), true);
    }
}

void ScaleBoxModule::editX()
{
    ToolModule *toolModule = m_workspace->m_toolModule;
    BoundingBoxTool *tool = dynamic_cast<BoundingBoxTool *>(toolModule->getCurrentTool());
    if (tool) {
        float value; // The code leaves this uninitialized if extraction is skipped.
        std::wstringstream(getPanel()->getNode()->getText(L"scalebox:x", true)) >> value;
        value = m_workspace->fromDisplayUnits(value);
        plasma::Vector<2, float> boxMin = tool->m_boxMin;
        plasma::Vector<2, float> boxMax = tool->m_boxMax;
        boxMax.m_elements[0] = boxMax.m_elements[0] + value - boxMin.m_elements[0];
        boxMin.m_elements[0] = value;
        tool->beginBoxChange();
        tool->setBox(boxMin, boxMax);
        tool->endBoxChange();
    }
}

void ScaleBoxModule::dragX(plasma::Widget *sender)
{
    if (!m_workspace->getOpenDesignSheet())
        return;
    ToolModule *toolModule = m_workspace->m_toolModule;
    BoundingBoxTool *tool = dynamic_cast<BoundingBoxTool *>(toolModule->getCurrentTool());
    if (tool && isDragging(sender)) {
        updateDrag();
        float value = m_workspace->fromDisplayUnits(m_dragPosition) + m_dragStart;
        std::wstringstream text;
        text << plasma::fromInternalUnits(m_workspace->getOpenSheet()->m_page.unit, value);
        m_panel->setText(L"scalebox:x", text.str(), true);
        plasma::Vector<2, float> boxMin = m_boxPosition;
        plasma::Vector<2, float> boxMax = m_boxPosition + m_boxSize;
        boxMax.m_elements[0] = boxMax.m_elements[0] + value - boxMin.m_elements[0];
        boxMin.m_elements[0] = value;
        tool->setBox(boxMin, boxMax);
    }
}

void ScaleBoxModule::stepX(plasma::Widget *sender)
{
    ToolModule *toolModule = m_workspace->m_toolModule;
    BoundingBoxTool *tool = dynamic_cast<BoundingBoxTool *>(toolModule->getCurrentTool());
    if (tool) {
        float value; // The code leaves this uninitialized if extraction is skipped.
        std::wstringstream(getPanel()->getNode()->getText(L"scalebox:x", true)) >> value;
        value = m_workspace->fromDisplayUnits(value);
        value += m_workspace->fromDisplayUnits((float)pressDragButton(sender));
        std::wstringstream text;
        text << m_workspace->toDisplayUnits(value);
        m_panel->setText(L"scalebox:x", text.str(), true);
        beginDrag(value);
        plasma::Vector<2, float> boxMin = tool->m_boxMin;
        plasma::Vector<2, float> boxMax = tool->m_boxMax;
        m_boxPosition = boxMin;
        m_boxSize = boxMax - boxMin;
        boxMax.m_elements[0] = boxMax.m_elements[0] + value - boxMin.m_elements[0];
        boxMin.m_elements[0] = value;
        tool->beginBoxChange();
        tool->setBox(boxMin, boxMax);
    }
}

void ScaleBoxModule::editY()
{
    ToolModule *toolModule = m_workspace->m_toolModule;
    BoundingBoxTool *tool = dynamic_cast<BoundingBoxTool *>(toolModule->getCurrentTool());
    if (tool) {
        float value; // The code leaves this uninitialized if extraction is skipped.
        std::wstringstream(getPanel()->getNode()->getText(L"scalebox:y", true)) >> value;
        value = m_workspace->fromDisplayUnits(value);
        plasma::Vector<2, float> boxMin = tool->m_boxMin;
        plasma::Vector<2, float> boxMax = tool->m_boxMax;
        boxMax.m_elements[1] = boxMax.m_elements[1] + value - boxMin.m_elements[1];
        boxMin.m_elements[1] = value;
        tool->beginBoxChange();
        tool->setBox(boxMin, boxMax);
        tool->endBoxChange();
    }
}

void ScaleBoxModule::dragY(plasma::Widget *sender)
{
    ToolModule *toolModule = m_workspace->m_toolModule;
    BoundingBoxTool *tool = dynamic_cast<BoundingBoxTool *>(toolModule->getCurrentTool());
    if (tool && isDragging(sender)) {
        updateDrag();
        float value = m_workspace->fromDisplayUnits(m_dragPosition) + m_dragStart;
        std::wstringstream text;
        text << m_workspace->toDisplayUnits(value);
        m_panel->setText(L"scalebox:y", text.str(), true);
        plasma::Vector<2, float> boxMin = m_boxPosition;
        plasma::Vector<2, float> boxMax = m_boxPosition + m_boxSize;
        boxMax.m_elements[1] = boxMax.m_elements[1] + value - boxMin.m_elements[1];
        boxMin.m_elements[1] = value;
        tool->setBox(boxMin, boxMax);
    }
}

void ScaleBoxModule::stepY(plasma::Widget *sender)
{
    ToolModule *toolModule = m_workspace->m_toolModule;
    BoundingBoxTool *tool = dynamic_cast<BoundingBoxTool *>(toolModule->getCurrentTool());
    if (tool) {
        float value; // The code leaves this uninitialized if extraction is skipped.
        std::wstringstream(getPanel()->getNode()->getText(L"scalebox:y", true)) >> value;
        value = m_workspace->fromDisplayUnits(value);
        value += m_workspace->fromDisplayUnits((float)pressDragButton(sender));
        std::wstringstream text;
        text << m_workspace->toDisplayUnits(value);
        m_panel->setText(L"scalebox:y", text.str(), true);
        beginDrag(value);
        plasma::Vector<2, float> boxMin = tool->m_boxMin;
        plasma::Vector<2, float> boxMax = tool->m_boxMax;
        m_boxPosition = boxMin;
        m_boxSize = boxMax - boxMin;
        boxMax.m_elements[1] = boxMax.m_elements[1] + value - boxMin.m_elements[1];
        boxMin.m_elements[1] = value;
        tool->beginBoxChange();
        tool->setBox(boxMin, boxMax);
    }
}

void ScaleBoxModule::editWidth()
{
    ToolModule *toolModule = m_workspace->m_toolModule;
    BoundingBoxTool *tool = dynamic_cast<BoundingBoxTool *>(toolModule->getCurrentTool());
    if (tool) {
        float value; // The code leaves this uninitialized if extraction is skipped.
        std::wstringstream(getPanel()->getNode()->getText(L"scalebox:width", true)) >> value;
        value = m_workspace->fromDisplayUnits(value);
        plasma::Vector<2, float> boxMin = tool->m_boxMin;
        plasma::Vector<2, float> boxMax = tool->m_boxMax;
        boxMax.m_elements[0] = boxMin.m_elements[0] + value;
        tool->beginBoxChange();
        tool->setBox(boxMin, boxMax);
        tool->endBoxChange();
    }
}

void ScaleBoxModule::dragWidth(plasma::Widget *sender)
{
    ToolModule *toolModule = m_workspace->m_toolModule;
    BoundingBoxTool *tool = dynamic_cast<BoundingBoxTool *>(toolModule->getCurrentTool());
    if (tool && isDragging(sender)) {
        updateDrag();
        float value = m_workspace->fromDisplayUnits(m_dragPosition) + m_dragStart;
        std::wstringstream text;
        text << m_workspace->toDisplayUnits(value);
        m_panel->setText(L"scalebox:width", text.str(), true);
        plasma::Vector<2, float> boxMin = m_boxPosition;
        plasma::Vector<2, float> boxMax = m_boxPosition + m_boxSize;
        boxMax.m_elements[0] = boxMin.m_elements[0] + value;
        tool->setBox(boxMin, boxMax);
    }
}

void ScaleBoxModule::stepWidth(plasma::Widget *sender)
{
    ToolModule *toolModule = m_workspace->m_toolModule;
    BoundingBoxTool *tool = dynamic_cast<BoundingBoxTool *>(toolModule->getCurrentTool());
    if (tool) {
        float value; // The code leaves this uninitialized if extraction is skipped.
        std::wstringstream(getPanel()->getNode()->getText(L"scalebox:width", true)) >> value;
        value = m_workspace->fromDisplayUnits(value);
        value += m_workspace->fromDisplayUnits((float)pressDragButton(sender));
        std::wstringstream text;
        text << m_workspace->toDisplayUnits(value);
        m_panel->setText(L"scalebox:width", text.str(), true);
        beginDrag(value);
        plasma::Vector<2, float> boxMin = tool->m_boxMin;
        plasma::Vector<2, float> boxMax = tool->m_boxMax;
        m_boxPosition = boxMin;
        m_boxSize = boxMax - boxMin;
        boxMax.m_elements[0] = boxMin.m_elements[0] + value;
        tool->beginBoxChange();
        tool->setBox(boxMin, boxMax);
    }
}

void ScaleBoxModule::editHeight()
{
    ToolModule *toolModule = m_workspace->m_toolModule;
    BoundingBoxTool *tool = dynamic_cast<BoundingBoxTool *>(toolModule->getCurrentTool());
    if (tool) {
        float value; // The code leaves this uninitialized if extraction is skipped.
        std::wstringstream(getPanel()->getNode()->getText(L"scalebox:height", true)) >> value;
        value = m_workspace->fromDisplayUnits(value);
        plasma::Vector<2, float> boxMin = tool->m_boxMin;
        plasma::Vector<2, float> boxMax = tool->m_boxMax;
        boxMax.m_elements[1] = boxMin.m_elements[1] + value;
        tool->beginBoxChange();
        tool->setBox(boxMin, boxMax);
        tool->endBoxChange();
    }
}

void ScaleBoxModule::dragHeight(plasma::Widget *sender)
{
    ToolModule *toolModule = m_workspace->m_toolModule;
    BoundingBoxTool *tool = dynamic_cast<BoundingBoxTool *>(toolModule->getCurrentTool());
    if (tool && isDragging(sender)) {
        updateDrag();
        float value = m_workspace->fromDisplayUnits(m_dragPosition) + m_dragStart;
        std::wstringstream text;
        text << m_workspace->toDisplayUnits(value);
        m_panel->setText(L"scalebox:height", text.str(), true);
        plasma::Vector<2, float> boxMin = m_boxPosition;
        plasma::Vector<2, float> boxMax = m_boxPosition + m_boxSize;
        boxMax.m_elements[1] = boxMin.m_elements[1] + value;
        tool->setBox(boxMin, boxMax);
    }
}

void ScaleBoxModule::stepHeight(plasma::Widget *sender)
{
    ToolModule *toolModule = m_workspace->m_toolModule;
    BoundingBoxTool *tool = dynamic_cast<BoundingBoxTool *>(toolModule->getCurrentTool());
    if (tool) {
        float value; // The code leaves this uninitialized if extraction is skipped.
        std::wstringstream(getPanel()->getNode()->getText(L"scalebox:height", true)) >> value;
        value = m_workspace->fromDisplayUnits(value);
        value += m_workspace->fromDisplayUnits((float)pressDragButton(sender));
        std::wstringstream text;
        text << m_workspace->toDisplayUnits(value);
        m_panel->setText(L"scalebox:height", text.str(), true);
        beginDrag(value);
        plasma::Vector<2, float> boxMin = tool->m_boxMin;
        plasma::Vector<2, float> boxMax = tool->m_boxMax;
        m_boxPosition = boxMin;
        m_boxSize = boxMax - boxMin;
        boxMax.m_elements[1] = boxMin.m_elements[1] + value;
        tool->beginBoxChange();
        tool->setBox(boxMin, boxMax);
    }
}

void ScaleBoxModule::editRotation()
{
    std::wstringstream(getPanel()->getNode()->getText(L"scalebox:rotation", true)) >> m_rotation;
}

void ScaleBoxModule::dragRotation(plasma::Widget *sender)
{
    if (isDragging(sender)) {
        updateDrag();
        m_rotation = m_dragStart + m_dragPosition;
        std::wstringstream text;
        text << m_rotation;
        m_panel->setText(L"scalebox:rotation", text.str(), true);
    }
}

void ScaleBoxModule::stepRotation(plasma::Widget *sender)
{
    ToolModule *toolModule = m_workspace->m_toolModule;
    BoundingBoxTool *tool = dynamic_cast<BoundingBoxTool *>(toolModule->getCurrentTool());
    if (tool) {
        m_rotation += pressDragButton(sender);
        std::wstringstream text;
        text << m_rotation;
        m_panel->setText(L"scalebox:rotation", text.str(), true);
        beginDrag(m_rotation);
    }
}

void ScaleBoxModule::rotate()
{
    ToolModule *toolModule = m_workspace->m_toolModule;
    BoundingBoxTool *tool = dynamic_cast<BoundingBoxTool *>(toolModule->getCurrentTool());
    if (tool)
        tool->rotateBox(m_rotation);
}

void ScaleBoxModule::endDrag()
{
    ToolModule *toolModule = m_workspace->m_toolModule;
    BoundingBoxTool *tool = dynamic_cast<BoundingBoxTool *>(toolModule->getCurrentTool());
    if (tool) {
        tool->endBoxChange();
        m_workspace->redraw();
    }
}

void ScaleBoxModule::chooseExpand()
{
    m_expandDirection = m_panel->isChecked(L"scalebox:expand", true) ? -1 : 1;
}

bool ScaleBoxModule::updateExpansion()
{
    if (m_expandDirection < 0) {
        if (m_expansion < 1.0f) {
            m_expansion += m_workspace->m_engine.m_frameTime * 0.002f;
            if (m_expansion > 1.0f)
                m_expansion = 1.0f;
        } else {
            m_expandDirection = 0;
        }
        return true;
    }
    if (m_expandDirection > 0) {
        if (m_expansion > 0.0f) {
            m_expansion -= m_workspace->m_engine.m_frameTime * 0.002f;
            if (m_expansion < 0.0f)
                m_expansion = 0.0f;
        } else {
            m_expandDirection = 0;
        }
        return true;
    }
    return false;
}
