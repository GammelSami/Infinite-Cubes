#include "FormatModule.h"
#include "Operations.h"
#include "main.h"
#include "plasma/Engine.h"
#include "plasma/GenericWidget.h"
#include "plasma/ListWidget.h"
#include "plasma/PopUpButton.h"
#include <algorithm>
#include <cmath>
#include <sstream>
#include <cstdlib>

// Copies each temporary paper-format name into the list.
PaperFormat g_paperFormats[PAPER_FORMAT_COUNT] = {
    PaperFormat(L"A4", 210.0f, 297.0f, UNIT_MM),    PaperFormat(L"US Letter", 8.5f, 11.0f, UNIT_IN),
    PaperFormat(L"US Legal", 8.5f, 14.0f, UNIT_IN), PaperFormat(L"US Executive", 7.3f, 10.5f, UNIT_IN),
    PaperFormat(L"A0", 841.0f, 1189.0f, UNIT_MM),   PaperFormat(L"A1", 594.0f, 841.0f, UNIT_MM),
    PaperFormat(L"A2", 420.0f, 594.0f, UNIT_MM),    PaperFormat(L"A3", 297.0f, 420.0f, UNIT_MM),
    PaperFormat(L"A5", 148.0f, 210.0f, UNIT_MM),
};

// Layout positions, extents and spacing use display units; colour tuples style the overlays.
FormatModule::FormatModule()
    : m_page(1000.0f, 1000.0f, 72.0f) // Pixels, white.
{
    m_visible = false;
    {
        std::wstring name(L"module:format");
        std::wstring kind(L"toolbar");
        m_panel =
            g_workspace.m_engine.createGenericWidget(name, 0.0f, 0.0f, 400.0f, 35.0f, g_workspace.m_scalableNode, kind);
    }
    m_panel->m_flags |= (1 << plasma::WIDGET_RENDER_SURFACE);
    m_orientation = ORIENTATION_HORIZONTAL;
}

void FormatModule::toggleVisible()
{
    m_visible = m_panel->isChecked(L"format:visible", true);
}

void FormatModule::savePage()
{
    if (m_workspace->getOpenSheet())
        m_page = m_workspace->getOpenSheet()->m_page;
}

void FormatModule::recordPageChange()
{
    if (m_workspace->getDesignSheet()) {
        const plasma::Engine::Page &page = m_page;
        DesignSheet *sheet = m_workspace->getDesignSheet();
        if (page.size[0] != sheet->m_page.size[PAGE_WIDTH] || page.size[1] != sheet->m_page.size[PAGE_HEIGHT] ||
            page.unit != sheet->m_page.unit

            // The code compares the saved color with itself; NaNs can differ.
            || page.color != page.color) {
            m_workspace->getDesignSheet()->addOperation(
                new PageOperation(m_workspace->getDesignSheet(), "change page format", page, sheet->m_page));
            m_workspace->getDesignSheet()->commit();
        }
    }
}

// Layout positions, extents and spacing use display units; colour tuples style the overlays.
void FormatModule::build()
{
    m_margin.m_elements[0] = 2;
    m_margin.m_elements[1] = 2;
    std::list<ModuleWidgetItem> items;
    items.push_back(ModuleWidgetItem(L" ", L"format:visible", L"checkbutton2", 0, 0, 25, 25));
    addPanelAt(items, L"", 2, 2);
    placeIcon(L"format:visible", L"icon:page");
    addComboButtonAt(L"Format", L"format:format", L"format:listbox", 30, 2, 120, 25, 180, 200);

    // The format button keeps its caption when a format is chosen.
    plasma::PopUpButton *formatButton =
        dynamic_cast<plasma::PopUpButton *>(getPanel()->getNode()->findWidget(L"format:format:combobutton"));
    if (formatButton)
        formatButton->m_updateCaption = false;
    items.clear();
    items.push_back(ModuleWidgetItem(L" ", L"format:width", L"editbox", 0, 0, 50, 20));
    items.push_back(ModuleWidgetItem(L" ", L"format:width:drag",
                                     m_orientation == ORIENTATION_HORIZONTAL ? L"hdragbutton" : L"vdragbutton", 50, 0,
                                     20, 20));
    addPanelAt(items, L"", 155, 5);
    addWidgetAt(ModuleWidgetItem(L"x", L"", L"static", 228, 5, 80, 20));
    items.clear();
    items.push_back(ModuleWidgetItem(L" ", L"format:height", L"editbox", 0, 0, 50, 20));
    items.push_back(ModuleWidgetItem(L" ", L"format:height:drag",
                                     m_orientation == ORIENTATION_HORIZONTAL ? L"hdragbutton" : L"vdragbutton", 50, 0,
                                     20, 20));
    addPanelAt(items, L"", 240, 5);
    getPanel()->getNode()->setEditFilter(L"format:width", &plasma::g_unsignedFloatFilter, true);
    getPanel()->getNode()->setEditFilter(L"format:height", &plasma::g_unsignedFloatFilter, true);
    addComboButtonAt(L"px", L"format:unit", L"format:unit:listbox", 315, 2, 55, 25, 100, 200);
    items.clear();
    items.push_back(ModuleWidgetItem(L" ", L"format:landscape", L"button2", 0, 0, 25, 25));
    addPanelAt(items, L"", 375, 2);
    placeIcon(L"format:landscape", L"icon:landscape");
    plasma::ListWidget *unitList =
        dynamic_cast<plasma::ListWidget *>(m_workspace->m_engine.findWidget(L"format:unit:listbox:list"));
    if (unitList) {
        std::list<plasma::ListItem> units;
        units.push_back(plasma::ListItem(L"px", L"unit:px", L"listitem", false));
        units.push_back(plasma::ListItem(L"in", L"unit:in", L"listitem", false));
        units.push_back(plasma::ListItem(L"ft", L"unit:ft", L"listitem", false));
        units.push_back(plasma::ListItem(L"mm", L"unit:mm", L"listitem", false));
        units.push_back(plasma::ListItem(L"cm", L"unit:cm", L"listitem", false));
        units.push_back(plasma::ListItem(L"m", L"unit:m", L"listitem", false));
        units.push_back(plasma::ListItem(L"pt", L"unit:pt", L"listitem", false));
        units.push_back(plasma::ListItem(L"pc", L"unit:pc", L"listitem", false));
        unitList->setItems(units);
    }
    fitWidth(3);
    getPanel()->getNode()->connectWidget(L"unit:px", plasma::WIDGET_EVENT_POINTER_RELEASED, this,
                                         &FormatModule::setUnitPixels, true);
    getPanel()->getNode()->connectWidget(L"unit:in", plasma::WIDGET_EVENT_POINTER_RELEASED, this,
                                         &FormatModule::setUnitInches, true);
    getPanel()->getNode()->connectWidget(L"unit:ft", plasma::WIDGET_EVENT_POINTER_RELEASED, this,
                                         &FormatModule::setUnitFeet, true);
    getPanel()->getNode()->connectWidget(L"unit:mm", plasma::WIDGET_EVENT_POINTER_RELEASED, this,
                                         &FormatModule::setUnitMillimetres, true);
    getPanel()->getNode()->connectWidget(L"unit:cm", plasma::WIDGET_EVENT_POINTER_RELEASED, this,
                                         &FormatModule::setUnitCentimetres, true);
    getPanel()->getNode()->connectWidget(L"unit:m", plasma::WIDGET_EVENT_POINTER_RELEASED, this,
                                         &FormatModule::setUnitMetres, true);
    getPanel()->getNode()->connectWidget(L"unit:pt", plasma::WIDGET_EVENT_POINTER_RELEASED, this,
                                         &FormatModule::setUnitPoints, true);
    getPanel()->getNode()->connectWidget(L"unit:pc", plasma::WIDGET_EVENT_POINTER_RELEASED, this,
                                         &FormatModule::setUnitPicas, true);
    plasma::ListWidget *formatList =
        dynamic_cast<plasma::ListWidget *>(m_workspace->m_engine.findWidget(L"format:listbox:list"));
    if (formatList) {
        std::list<plasma::ListItem> formats;
        for (int i = 0; i < PAPER_FORMAT_COUNT; i++) {
            std::wstringstream caption;
            const char *unit;
            switch (g_paperFormats[i].unit) {
            case UNIT_PX:
                unit = "px";
                break;
            case UNIT_IN:
                unit = "in";
                break;
            case UNIT_MM:
                unit = "mm";
                break;
            default:
                unit = "";
                break;
            }
            caption << g_paperFormats[i].name << L" (" << roundToHundredths(g_paperFormats[i].width) << L" x "
                    << roundToHundredths(g_paperFormats[i].height) << L" " << unit << L")";
            formats.push_back(plasma::ListItem(caption.str(), g_paperFormats[i].name, L"listitem", false));
        }
        formatList->setItems(formats);
        std::list<plasma::Widget *> children = formatList->getChildWidgets();
        for (std::list<plasma::Widget *>::iterator it = children.begin(); it != children.end(); ++it)
            (*it)->connect(plasma::WIDGET_EVENT_POINTER_RELEASED, this, &FormatModule::choosePaperFormat, true, true);
    }
    getPanel()->getNode()->connectWidget(L"format:width", plasma::WIDGET_EVENT_FOCUS_LOST, this,
                                         &FormatModule::editWidth, true);
    getPanel()->getNode()->connectWidget(L"format:width:drag", plasma::WIDGET_EVENT_POINTER_MOVED, this,
                                         &FormatModule::dragWidth, true);
    getPanel()->getNode()->connectWidget(L"format:width:drag", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &FormatModule::stepWidth, true);
    getPanel()->getNode()->connectWidget(L"format:width:drag", plasma::WIDGET_EVENT_POINTER_DOUBLE_CLICKED, this,
                                         &FormatModule::stepWidth, true);
    getPanel()->getNode()->connectWidget(L"format:width:drag", plasma::WIDGET_EVENT_POINTER_RELEASED, this,
                                         &FormatModule::recordPageChange, true);
    getPanel()->getNode()->connectWidget(L"format:height", plasma::WIDGET_EVENT_FOCUS_LOST, this,
                                         &FormatModule::editHeight, true);
    getPanel()->getNode()->connectWidget(L"format:height:drag", plasma::WIDGET_EVENT_POINTER_MOVED, this,
                                         &FormatModule::dragHeight, true);
    getPanel()->getNode()->connectWidget(L"format:height:drag", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &FormatModule::stepHeight, true);
    getPanel()->getNode()->connectWidget(L"format:height:drag", plasma::WIDGET_EVENT_POINTER_DOUBLE_CLICKED, this,
                                         &FormatModule::stepHeight, true);
    getPanel()->getNode()->connectWidget(L"format:height:drag", plasma::WIDGET_EVENT_POINTER_RELEASED, this,
                                         &FormatModule::recordPageChange, true);
    getPanel()->getNode()->connectWidget(L"format:landscape", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &FormatModule::swapOrientation, true);
    getPanel()->getNode()->connectWidget(L"format:landscape", plasma::WIDGET_EVENT_POINTER_DOUBLE_CLICKED, this,
                                         &FormatModule::swapOrientation, true);
    getPanel()->getNode()->connectWidget(L"format:visible", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &FormatModule::toggleVisible, true);
    getPanel()->getNode()->connectWidget(L"format:visible", plasma::WIDGET_EVENT_POINTER_DOUBLE_CLICKED, this,
                                         &FormatModule::toggleVisible, true);
    m_panel->setText(L"format:width", L"", true);
    m_panel->setText(L"format:height", L"", true);
    m_panel->setText(L"format:unit:combobutton", L"px", true);
    createZoomSequences();
}

void FormatModule::choosePaperFormat(plasma::Widget *sender)
{
    if (m_workspace->getOpenSheet()) {
        savePage();
        for (int i = 0; i < sizeof(g_paperFormats) / sizeof(g_paperFormats[0]); i++) {
            if (g_paperFormats[i].name == sender->getName()) {
                m_workspace->getOpenSheet()->m_page.unit = g_paperFormats[i].unit;
                m_workspace->getOpenSheet()->m_page.size[PAGE_WIDTH] =
                    plasma::toInternalUnits(g_paperFormats[i].unit, g_paperFormats[i].width);
                m_workspace->getOpenSheet()->m_page.size[PAGE_HEIGHT] =
                    plasma::toInternalUnits(g_paperFormats[i].unit, g_paperFormats[i].height);
                break;
            }
        }
        recordPageChange();
        update();
    }
}

void FormatModule::setUnitPixels()
{
    if (m_workspace->getOpenSheet()) {
        savePage();
        m_workspace->getOpenSheet()->m_page.unit = UNIT_PX;
        recordPageChange();
        update();
    }
}

void FormatModule::setUnitInches()
{
    if (m_workspace->getOpenSheet()) {
        savePage();
        m_workspace->getOpenSheet()->m_page.unit = UNIT_IN;
        recordPageChange();
        update();
    }
}

void FormatModule::setUnitFeet()
{
    if (m_workspace->getOpenSheet()) {
        savePage();
        m_workspace->getOpenSheet()->m_page.unit = UNIT_FT;
        recordPageChange();
        update();
    }
}

void FormatModule::setUnitMillimetres()
{
    if (m_workspace->getOpenSheet()) {
        savePage();
        m_workspace->getOpenSheet()->m_page.unit = UNIT_MM;
        recordPageChange();
        update();
    }
}

void FormatModule::setUnitCentimetres()
{
    if (m_workspace->getOpenSheet()) {
        savePage();
        m_workspace->getOpenSheet()->m_page.unit = UNIT_CM;
        recordPageChange();
        update();
    }
}

void FormatModule::setUnitMetres()
{
    if (m_workspace->getOpenSheet()) {
        savePage();
        m_workspace->getOpenSheet()->m_page.unit = UNIT_M;
        recordPageChange();
        update();
    }
}

void FormatModule::setUnitPoints()
{
    if (m_workspace->getOpenSheet()) {
        savePage();
        m_workspace->getOpenSheet()->m_page.unit = UNIT_PT;
        recordPageChange();
        update();
    }
}

void FormatModule::setUnitPicas()
{
    if (m_workspace->getOpenSheet()) {
        savePage();
        m_workspace->getOpenSheet()->m_page.unit = UNIT_PC;
        recordPageChange();
        update();
    }
}

void FormatModule::editWidth()
{
    if (m_workspace->getOpenSheet()) {
        savePage();
        m_workspace->getOpenSheet()->m_page.size[PAGE_WIDTH] = plasma::toInternalUnits(
            m_workspace->getOpenSheet()->m_page.unit,
            (float)atof(narrowString(getPanel()->getNode()->getText(L"format:width", true)).c_str()));
        recordPageChange();
        update();
    }
}

void FormatModule::dragWidth(plasma::Widget *sender)
{
    if (m_workspace->getOpenSheet() && isDragging(sender)) {
        m_workspace->getOpenSheet()->m_page.size.m_elements[PAGE_WIDTH] += getDragMovement();
        if (m_workspace->getOpenSheet()->m_page.size.m_elements[PAGE_WIDTH] < 0.0f)
            m_workspace->getOpenSheet()->m_page.size.m_elements[PAGE_WIDTH] = 0.0f;
        update();
    }
}

void FormatModule::stepWidth(plasma::Widget *sender)
{
    if (m_workspace->getOpenSheet()) {
        savePage();
        m_workspace->getOpenSheet()->m_page.size.m_elements[PAGE_WIDTH] += pressDragButton(sender);
        if (m_workspace->getOpenSheet()->m_page.size.m_elements[PAGE_WIDTH] < 0.0f)
            m_workspace->getOpenSheet()->m_page.size.m_elements[PAGE_WIDTH] = 0.0f;
        update();
    }
}

void FormatModule::editHeight()
{
    if (m_workspace->getOpenSheet()) {
        savePage();
        m_workspace->getOpenSheet()->m_page.size[PAGE_HEIGHT] = plasma::toInternalUnits(
            m_workspace->getOpenSheet()->m_page.unit,
            (float)atof(narrowString(getPanel()->getNode()->getText(L"format:height", true)).c_str()));
        recordPageChange();
        update();
    }
}

void FormatModule::dragHeight(plasma::Widget *sender)
{
    if (m_workspace->getOpenSheet() && isDragging(sender)) {
        m_workspace->getOpenSheet()->m_page.size.m_elements[PAGE_HEIGHT] += getDragMovement();
        if (m_workspace->getOpenSheet()->m_page.size.m_elements[PAGE_HEIGHT] < 0.0f)
            m_workspace->getOpenSheet()->m_page.size.m_elements[PAGE_HEIGHT] = 0.0f;
        update();
    }
}

void FormatModule::stepHeight(plasma::Widget *sender)
{
    if (m_workspace->getOpenSheet()) {
        savePage();
        m_workspace->getOpenSheet()->m_page.size.m_elements[PAGE_HEIGHT] += pressDragButton(sender);
        if (m_workspace->getOpenSheet()->m_page.size.m_elements[PAGE_HEIGHT] < 0.0f)
            m_workspace->getOpenSheet()->m_page.size.m_elements[PAGE_HEIGHT] = 0.0f;
        update();
    }
}

void FormatModule::sheetChanged()
{
    update();
}

void FormatModule::update()
{
    if (!m_workspace->getOpenSheet()) {
        m_panel->setText(L"format:width", L"", true);
        m_panel->setText(L"format:height", L"", true);
        m_panel->setText(L"format:unit:combobutton", L"px", true);
        m_workspace->getGuiNode()->setText(L"text:unit", L"px", true);
        m_panel->setChecked(L"unit:px", true, false, true);
        return;
    }
    std::wstringstream text;
    text << roundToHundredths(plasma::fromInternalUnits(
        m_workspace->getOpenSheet()->m_page.unit, m_workspace->getOpenSheet()->m_page.size.m_elements[PAGE_WIDTH]));
    m_panel->setText(L"format:width", text.str(), true);
    text.str(L"");
    text << roundToHundredths(plasma::fromInternalUnits(
        m_workspace->getOpenSheet()->m_page.unit, m_workspace->getOpenSheet()->m_page.size.m_elements[PAGE_HEIGHT]));
    m_panel->setText(L"format:height", text.str(), true);
    switch (m_workspace->getOpenSheet()->m_page.unit) {
    case UNIT_PX:
        m_panel->setText(L"format:unit:combobutton", L"px", true);
        m_workspace->getGuiNode()->setText(L"text:unit", L"px", true);
        m_panel->setChecked(L"unit:px", true, false, true);
        break;
    case UNIT_IN:
        m_panel->setText(L"format:unit:combobutton", L"in", true);
        m_workspace->getGuiNode()->setText(L"text:unit", L"in", true);
        m_panel->setChecked(L"unit:in", true, false, true);
        break;
    case UNIT_FT:
        m_panel->setText(L"format:unit:combobutton", L"ft", true);
        m_workspace->getGuiNode()->setText(L"text:unit", L"ft", true);
        m_panel->setChecked(L"unit:ft", true, false, true);
        break;
    case UNIT_MM:
        m_panel->setText(L"format:unit:combobutton", L"mm", true);
        m_workspace->getGuiNode()->setText(L"text:unit", L"mm", true);
        m_panel->setChecked(L"unit:mm", true, false, true);
        break;
    case UNIT_CM:
        m_panel->setText(L"format:unit:combobutton", L"cm", true);
        m_workspace->getGuiNode()->setText(L"text:unit", L"cm", true);
        m_panel->setChecked(L"unit:cm", true, false, true);
        break;
    case UNIT_M:
        m_panel->setText(L"format:unit:combobutton", L"m", true);
        m_workspace->getGuiNode()->setText(L"text:unit", L"m", true);
        m_panel->setChecked(L"unit:m", true, false, true);
        break;
    case UNIT_PT:
        m_panel->setText(L"format:unit:combobutton", L"pt", true);
        m_workspace->getGuiNode()->setText(L"text:unit", L"pt", true);
        m_panel->setChecked(L"unit:pt", true, false, true);
        break;
    case UNIT_PC:
        m_panel->setText(L"format:unit:combobutton", L"pc", true);
        m_workspace->getGuiNode()->setText(L"text:unit", L"pc", true);
        m_panel->setChecked(L"unit:pc", true, false, true);
        break;
    }
    m_workspace->unitChanged();
    int format;
    for (format = 0; format < sizeof(g_paperFormats) / sizeof(g_paperFormats[0]); format++) {
        const PaperFormat &paper = g_paperFormats[format];
        if (std::fabs(
                plasma::fromInternalUnits(paper.unit, m_workspace->getOpenSheet()->m_page.size.m_elements[PAGE_WIDTH]) -
                paper.width) < 0.0001f &&
            std::fabs(plasma::fromInternalUnits(paper.unit,
                                                m_workspace->getOpenSheet()->m_page.size.m_elements[PAGE_HEIGHT]) -
                      paper.height) < 0.0001f)
            break;
        if (std::fabs(
                plasma::fromInternalUnits(paper.unit, m_workspace->getOpenSheet()->m_page.size.m_elements[PAGE_WIDTH]) -
                paper.height) < 0.0001f &&
            std::fabs(plasma::fromInternalUnits(paper.unit,
                                                m_workspace->getOpenSheet()->m_page.size.m_elements[PAGE_HEIGHT]) -
                      paper.width) < 0.0001f)
            break;
    }
    if (format == sizeof(g_paperFormats) / sizeof(g_paperFormats[0])) {
        m_panel->setText(L"format:format:combobutton", L"Custom format", true);
        m_workspace->selectionChanged();
        return;
    }
    std::wstring name = g_paperFormats[format].name;
    if (m_workspace->getOpenSheet()->m_page.size.m_elements[PAGE_HEIGHT] <
        m_workspace->getOpenSheet()->m_page.size.m_elements[PAGE_WIDTH])
        name += L" (landscape)";
    m_panel->setText(L"format:format:combobutton", name, true);
    m_workspace->getGuiNode()->setChecked(g_paperFormats[format].name, true, false, true);
}

void FormatModule::swapOrientation()
{
    if (m_workspace->getOpenSheet()) {
        savePage();
        std::swap(m_workspace->getOpenSheet()->m_page.size.m_elements[PAGE_HEIGHT],
                  m_workspace->getOpenSheet()->m_page.size.m_elements[PAGE_WIDTH]);
        recordPageChange();
        update();
    }
}
