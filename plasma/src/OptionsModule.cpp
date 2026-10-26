#include "plasma/FilePath.h"
#include "OptionsModule.h"
#include "FormatModule.h" // The edit box filters.
#include "main.h"
#include "plasma/Engine.h"
#include "plasma/GenericWidget.h"
#include <fstream>
#include <sstream>

std::wstring g_defaultTtfCharacters;

static const int DefaultCursorStickiness = 5;
static const int DefaultGridSnapDistance = 10;
static const int DefaultGridOpacityPercent = 20;
static const int DefaultJpegQualityPercent = 90;
static const int DefaultBackupInterval = 50;
static const int FirstPrintableAscii = 32;
static const int LastPrintableAscii = 126;
static const int OptionsLineCapacity = 1000;
static const int MaximumCursorStickinessPixels = 20;
static const int MinimumGridOpacityPercent = 10;
static const int MaximumGridSnapDistancePixels = 50;

// Layout positions, extents and spacing use display units; colour tuples style the overlays. Supported GUI zoom
// presets are 50 through 100 percent in ten-percentage-point steps; unsupported stored values reset to full scale.
OptionsModule::OptionsModule()
    : m_cursorStickiness(DefaultCursorStickiness),
      m_cursorHardware(true),
      m_gridSnapDistance(DefaultGridSnapDistance),
      m_gridOpacity(DefaultGridOpacityPercent),
      m_exportJpegQuality(DefaultJpegQualityPercent),
      m_exportTransparency(true),
      m_backupInterval(DefaultBackupInterval),
      m_backupEnabled(false),
      m_guiZoom(1.0f)
{
    {
        std::wstring name(L"module:options");
        std::wstring kind(L"sidewidget");
        m_panel =
            g_workspace.m_engine.createGenericWidget(name, 0.0f, 0.0f, 400.0f, 430.0f, g_workspace.m_guiNode, kind);
    }
    m_panel->m_flags |= (1 << plasma::WIDGET_RENDER_SURFACE);
    getPanel()->setCaption(L"Options", true);
    getPanel()->setViewPosition((plasma::Vector<2, float>((float)g_workspace.m_engine.viewportWidth(),
                                                          (float)g_workspace.m_engine.viewportHeight()) -
                                 getPanel()->getViewSize()) *
                                    0.5,
                                true);
    getPanel()->m_horizontalAlignment = plasma::ALIGN_CENTER;
    getPanel()->m_verticalAlignment = plasma::ALIGN_MIDDLE;
    for (unsigned character = FirstPrintableAscii; character <= LastPrintableAscii; character++)
        g_defaultTtfCharacters += (wchar_t)character;
    m_ttfCharacters = g_defaultTtfCharacters;
    std::wifstream file(PLASMA_FILE_PATH(getAppDataPath() + L"\\options"));
    if (file.is_open()) {
        while (!file.eof()) {
            wchar_t line[OptionsLineCapacity];
            file.getline(line, OptionsLineCapacity);
            std::wstringstream stream((std::wstring(line)));
            std::wstring name;
            stream >> name;
            if (name == L"cursorStickiness")
                stream >> m_cursorStickiness;
            else if (name == L"cursorHardware")
                stream >> m_cursorHardware;
            else if (name == L"gridSnapDistance")
                stream >> m_gridSnapDistance;
            else if (name == L"gridOpacity")
                stream >> m_gridOpacity;
            else if (name == L"ttfCharacters")
                stream >> m_ttfCharacters;
            else if (name == L"exportJPEGQuality")
                stream >> m_exportJpegQuality;
            else if (name == L"exportTransparency")
                stream >> m_exportTransparency;
            else if (name == L"backupEnabled")
                stream >> m_backupEnabled;
            else if (name == L"backupInterval")
                stream >> m_backupInterval;
            else if (name == L"guiZoom") {
                stream >> m_guiZoom;
                if (m_guiZoom != 0.5f && m_guiZoom != 0.6f && m_guiZoom != 0.7f && m_guiZoom != 0.8f &&
                    m_guiZoom != 0.9f && m_guiZoom != 1.0f)
                    m_guiZoom = 1.0f;
            }
        }
    }
}

// Layout positions, extents and spacing use display units; colour tuples style the overlays.
void OptionsModule::build()
{
    std::list<ModuleWidgetItem> items;
    addWidgetAt(ModuleWidgetItem(L"Cursor", L"", L"sidewidget", 10, 30, 380, 100));
    addWidgetAt(ModuleWidgetItem(L"Stickiness:", L"", L"static", 20, 60, 0, 0));
    items.push_back(ModuleWidgetItem(L"", L"options:cursor:stickiness", L"editbox", 0, 0, 80, 20));
    items.push_back(ModuleWidgetItem(L"", L"options:cursor:stickiness:drag",
                                     m_orientation == ORIENTATION_HORIZONTAL ? L"hdragbutton" : L"vdragbutton", 80, 0,
                                     20, 20));
    addPanelAt(items, L"", 120, 60);
    getPanel()->getNode()->setEditFilter(L"options:cursor:stickiness", &plasma::g_unsignedIntegerFilter, true);
    addWidgetAt(ModuleWidgetItem(L"px", L"", L"static", 220, 60, 0, 0));
    addWidgetAt(ModuleWidgetItem(L"Hardware cursor:", L"", L"static", 20, 90, 0, 0));
    items.clear();
    items.push_back(ModuleWidgetItem(L"Enabled", L"options:cursor:hardware", L"checkbutton2", 0, 0, 150, 25));
    addPanelAt(items, L"", 120, 85);
    items.clear();
    items.push_back(ModuleWidgetItem(L" ", L"options:cursor:reset", L"button2", 0, 0, 25, 25));
    addPanelAt(items, L"", (int)(getPanel()->viewWidth() - 45.0), 40);
    placeIcon(L"options:cursor:reset", L"icon:undo");
    addWidgetAt(ModuleWidgetItem(L"Grid", L"", L"sidewidget", 10, 130, 380, 90));
    items.clear();
    items.push_back(ModuleWidgetItem(L" ", L"options:grid:reset", L"button2", 0, 0, 25, 25));
    addPanelAt(items, L"", (int)(getPanel()->viewWidth() - 45.0), 140);
    placeIcon(L"options:grid:reset", L"icon:undo");
    addWidgetAt(ModuleWidgetItem(L"Snap distance:", L"", L"static", 20, 160, 0, 0));
    items.clear();
    items.push_back(ModuleWidgetItem(L"", L"options:grid:snapdistance", L"editbox", 0, 0, 80, 20));
    items.push_back(ModuleWidgetItem(L"", L"options:grid:snapdistance:drag",
                                     m_orientation == ORIENTATION_HORIZONTAL ? L"hdragbutton" : L"vdragbutton", 80, 0,
                                     20, 20));
    addPanelAt(items, L"", 120, 160);
    getPanel()->getNode()->setEditFilter(L"options:grid:snapdistance", &plasma::g_unsignedIntegerFilter, true);
    addWidgetAt(ModuleWidgetItem(L"px", L"", L"static", 220, 160, 0, 0));
    addWidgetAt(ModuleWidgetItem(L"Opacity:", L"", L"static", 20, 185, 0, 0));
    items.clear();
    items.push_back(ModuleWidgetItem(L"", L"options:grid:opacity", L"editbox", 0, 0, 80, 20));
    items.push_back(ModuleWidgetItem(L"", L"options:grid:opacity:drag",
                                     m_orientation == ORIENTATION_HORIZONTAL ? L"hdragbutton" : L"vdragbutton", 80, 0,
                                     20, 20));
    addPanelAt(items, L"", 120, 185);
    getPanel()->getNode()->setEditFilter(L"options:grid:opacity", &plasma::g_unsignedIntegerFilter, true);
    addWidgetAt(ModuleWidgetItem(L"%", L"", L"static", 220, 185, 0, 0));
    addWidgetAt(ModuleWidgetItem(L"TrueType Font Import", L"", L"sidewidget", 10, 220, 380, 70));
    items.clear();
    items.push_back(ModuleWidgetItem(L" ", L"options:ttf:reset", L"button2", 0, 0, 25, 25));
    addPanelAt(items, L"", (int)(getPanel()->viewWidth() - 45.0), 230);
    placeIcon(L"options:ttf:reset", L"icon:undo");
    addWidgetAt(ModuleWidgetItem(L"Characters:", L"", L"static", 20, 250, 0, 0));
    items.clear();
    items.push_back(ModuleWidgetItem(L"", L"options:ttf:characters", L"editbox", 0, 0, 220, 20));
    addPanelAt(items, L"", 120, 250);
    addWidgetAt(ModuleWidgetItem(L"Image Export", L"", L"sidewidget", 10, 290, 380, 100));
    items.clear();
    items.push_back(ModuleWidgetItem(L" ", L"options:export:reset", L"button2", 0, 0, 25, 25));
    addPanelAt(items, L"", (int)(getPanel()->viewWidth() - 45.0), 300);
    placeIcon(L"options:export:reset", L"icon:undo");
    addWidgetAt(ModuleWidgetItem(L"JPEG quality:", L"", L"static", 20, 320, 0, 0));
    items.clear();
    items.push_back(ModuleWidgetItem(L"", L"options:export:jpeg:quality", L"editbox", 0, 0, 80, 20));
    items.push_back(ModuleWidgetItem(L"", L"options:export:jpeg:quality:drag",
                                     m_orientation == ORIENTATION_HORIZONTAL ? L"hdragbutton" : L"vdragbutton", 80, 0,
                                     20, 20));
    addPanelAt(items, L"", 120, 320);
    getPanel()->getNode()->setEditFilter(L"options:export:jpeg:quality", &plasma::g_unsignedIntegerFilter, true);
    addWidgetAt(ModuleWidgetItem(L"%", L"", L"static", 220, 320, 0, 0));
    addWidgetAt(ModuleWidgetItem(L"Transparency:", L"", L"static", 20, 350, 0, 0));
    items.clear();
    items.push_back(ModuleWidgetItem(L"Enabled (PNG, TIFF, TGA, ICO)", L"options:export:transparency", L"checkbutton2",
                                     0, 0, 180, 25));
    addPanelAt(items, L"", 120, 345);
    items.clear();
    items.push_back(ModuleWidgetItem(L" ", L"options:ok", L"button2", 0, 0, 100, 30));
    items.push_back(ModuleWidgetItem(L" ", L"options:cancel", L"button2", 100, 0, 100, 30));
    addPanelAt(items, L"", (int)(getPanel()->viewWidth() * 0.5 - 100.0), (int)(getPanel()->getViewHeight() - 40.0));
    placeIcon(L"options:ok", L"icon:apply:big");
    placeIcon(L"options:cancel", L"icon:cancel");
    getPanel()->getNode()->connectWidget(L"options:ok", plasma::WIDGET_EVENT_POINTER_RELEASED, this,
                                         &OptionsModule::accept, true);
    getPanel()->getNode()->connectWidget(L"options:cancel", plasma::WIDGET_EVENT_POINTER_RELEASED, this,
                                         &OptionsModule::close, true);
    getPanel()->getNode()->connectWidget(L"options:cursor:reset", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &OptionsModule::resetCursor, true);
    getPanel()->getNode()->connectWidget(L"options:grid:reset", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &OptionsModule::resetGrid, true);
    getPanel()->getNode()->connectWidget(L"options:ttf:reset", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &OptionsModule::resetTtfCharacters, true);
    getPanel()->getNode()->connectWidget(L"options:export:reset", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &OptionsModule::resetExport, true);
    getPanel()->getNode()->connectWidget(L"options:cursor:stickiness", plasma::WIDGET_EVENT_FOCUS_LOST, this,
                                         &OptionsModule::editCursorStickiness, true);
    getPanel()->getNode()->connectWidget(L"options:cursor:stickiness:drag", plasma::WIDGET_EVENT_POINTER_MOVED, this,
                                         &OptionsModule::dragCursorStickiness, true);
    getPanel()->getNode()->connectWidget(L"options:cursor:stickiness:drag", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &OptionsModule::stepCursorStickiness, true);
    getPanel()->getNode()->connectWidget(L"options:cursor:stickiness:drag", plasma::WIDGET_EVENT_POINTER_DOUBLE_CLICKED,
                                         this, &OptionsModule::stepCursorStickiness, true);
    getPanel()->getNode()->connectWidget(L"options:grid:opacity", plasma::WIDGET_EVENT_FOCUS_LOST, this,
                                         &OptionsModule::editGridOpacity, true);
    getPanel()->getNode()->connectWidget(L"options:grid:opacity:drag", plasma::WIDGET_EVENT_POINTER_MOVED, this,
                                         &OptionsModule::dragGridOpacity, true);
    getPanel()->getNode()->connectWidget(L"options:grid:opacity:drag", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &OptionsModule::stepGridOpacity, true);
    getPanel()->getNode()->connectWidget(L"options:grid:opacity:drag", plasma::WIDGET_EVENT_POINTER_DOUBLE_CLICKED,
                                         this, &OptionsModule::stepGridOpacity, true);
    getPanel()->getNode()->connectWidget(L"options:grid:snapdistance", plasma::WIDGET_EVENT_FOCUS_LOST, this,
                                         &OptionsModule::editGridSnapDistance, true);
    getPanel()->getNode()->connectWidget(L"options:grid:snapdistance:drag", plasma::WIDGET_EVENT_POINTER_MOVED, this,
                                         &OptionsModule::dragGridSnapDistance, true);
    getPanel()->getNode()->connectWidget(L"options:grid:snapdistance:drag", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &OptionsModule::stepGridSnapDistance, true);
    getPanel()->getNode()->connectWidget(L"options:grid:snapdistance:drag", plasma::WIDGET_EVENT_POINTER_DOUBLE_CLICKED,
                                         this, &OptionsModule::stepGridSnapDistance, true);
    getPanel()->getNode()->connectWidget(L"options:export:jpeg:quality", plasma::WIDGET_EVENT_FOCUS_LOST, this,
                                         &OptionsModule::editExportJpegQuality, true);
    getPanel()->getNode()->connectWidget(L"options:export:jpeg:quality:drag", plasma::WIDGET_EVENT_POINTER_MOVED, this,
                                         &OptionsModule::dragExportJpegQuality, true);
    getPanel()->getNode()->connectWidget(L"options:export:jpeg:quality:drag", plasma::WIDGET_EVENT_POINTER_PRESSED,
                                         this, &OptionsModule::stepExportJpegQuality, true);
    getPanel()->getNode()->connectWidget(L"options:export:jpeg:quality:drag",
                                         plasma::WIDGET_EVENT_POINTER_DOUBLE_CLICKED, this,
                                         &OptionsModule::stepExportJpegQuality, true);
    getPanel()->getNode()->connectWidget(L"options:backup:interval", plasma::WIDGET_EVENT_FOCUS_LOST, this,
                                         &OptionsModule::editBackupInterval, true);
    getPanel()->getNode()->connectWidget(L"options:backup:interval:drag", plasma::WIDGET_EVENT_POINTER_MOVED, this,
                                         &OptionsModule::dragBackupInterval, true);
    getPanel()->getNode()->connectWidget(L"options:backup:interval:drag", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &OptionsModule::stepBackupInterval, true);
    getPanel()->getNode()->connectWidget(L"options:backup:interval:drag", plasma::WIDGET_EVENT_POINTER_DOUBLE_CLICKED,
                                         this, &OptionsModule::stepBackupInterval, true);
    std::wstringstream stream;
    stream << m_cursorStickiness;
    getPanel()->getNode()->setText(L"options:cursor:stickiness", stream.str(), true);
    getPanel()->getNode()->setChecked(L"options:cursor:hardware", m_cursorHardware, false, true);
    stream.str(L"");
    stream << m_gridSnapDistance;
    getPanel()->getNode()->setText(L"options:grid:snapdistance", stream.str(), true);
    stream.str(L"");
    stream << m_gridOpacity;
    getPanel()->getNode()->setText(L"options:grid:opacity", stream.str(), true);
    getPanel()->getNode()->setText(L"options:ttf:characters", m_ttfCharacters, true);
    stream.str(L"");
    stream << m_exportJpegQuality;
    getPanel()->getNode()->setText(L"options:export:jpeg:quality", stream.str(), true);
    getPanel()->getNode()->setChecked(L"options:export:transparency", m_exportTransparency, false, true);
    stream.str(L"");
    stream << m_backupInterval;
    getPanel()->getNode()->setText(L"options:backup:interval", stream.str(), true);
    getPanel()->getNode()->setChecked(L"options:backup:enabled", m_backupEnabled, false, true);
}

void OptionsModule::save()
{
    std::wstringstream(getPanel()->getNode()->getText(L"options:cursor:stickiness", true)) >> m_cursorStickiness;
    m_cursorHardware = m_panel->isChecked(L"options:cursor:hardware", true);
    std::wstringstream(getPanel()->getNode()->getText(L"options:grid:snapdistance", true)) >> m_gridSnapDistance;
    std::wstringstream(getPanel()->getNode()->getText(L"options:grid:opacity", true)) >> m_gridOpacity;
    m_ttfCharacters = getPanel()->getNode()->getText(L"options:ttf:characters", true);
    std::wstringstream(getPanel()->getNode()->getText(L"options:export:jpeg:quality", true)) >> m_exportJpegQuality;
    m_exportTransparency = m_panel->isChecked(L"options:export:transparency", true);
    m_backupEnabled = m_panel->isChecked(L"options:backup:enabled", true);
    std::wstringstream(getPanel()->getNode()->getText(L"options:backup:interval", true)) >> m_backupInterval;
    std::wofstream file(PLASMA_FILE_PATH(getAppDataPath() + L"\\options"));
    if (file.is_open()) {
        file << L"cursorStickiness " << m_cursorStickiness << std::endl;
        file << L"cursorHardware " << m_cursorHardware << std::endl;
        file << L"gridSnapDistance " << m_gridSnapDistance << std::endl;
        file << L"gridOpacity " << m_gridOpacity << std::endl;
        file << L"ttfCharacters " << m_ttfCharacters << std::endl;
        file << L"exportJPEGQuality " << m_exportJpegQuality << std::endl;
        file << L"exportTransparency " << m_exportTransparency << std::endl;
        file << L"backupEnabled " << m_backupEnabled << std::endl;
        file << L"backupInterval " << m_backupInterval << std::endl;
        file << L"guiZoom " << m_guiZoom << std::endl;
    }
}

// "options:ok": saves the settings and closes the dialog.
void OptionsModule::accept()
{
    save();
    plasma::Widget::setDisplayVisibility(m_panel->getNode()->m_display, 0);
}

void OptionsModule::editCursorStickiness()
{
    int value;
    std::wstringstream(getPanel()->getNode()->getText(L"options:cursor:stickiness", true)) >> value;
    if (value < 0)
        value = 0;
    else if (value > MaximumCursorStickinessPixels)
        value = MaximumCursorStickinessPixels;
    std::wstringstream text;
    text << value;
    m_panel->setText(L"options:cursor:stickiness", text.str(), true);
}

void OptionsModule::dragCursorStickiness(plasma::Widget *sender)
{
    if (m_workspace->m_engine.m_buttons & (1 << plasma::MOUSE_BUTTON_LEFT)) {
        updateDrag();
        int value = (int)(m_dragPosition * 0.2f + m_dragStart);
        if (value < 0)
            value = 0;
        else if (value > MaximumCursorStickinessPixels)
            value = MaximumCursorStickinessPixels;
        std::wstringstream text;
        text << value;
        m_panel->setText(L"options:cursor:stickiness", text.str(), true);
    }
}

void OptionsModule::stepCursorStickiness(plasma::Widget *sender)
{
    int value;
    std::wstringstream(getPanel()->getNode()->getText(L"options:cursor:stickiness", true)) >> value;
    value += pressDragButton(sender);
    if (value < 0)
        value = 0;
    else if (value > MaximumCursorStickinessPixels)
        value = MaximumCursorStickinessPixels;
    std::wstringstream text;
    text << value;
    m_panel->setText(L"options:cursor:stickiness", text.str(), true);
    beginDrag((float)value);
}

void OptionsModule::editGridOpacity()
{
    int value;
    std::wstringstream(getPanel()->getNode()->getText(L"options:grid:opacity", true)) >> value;
    if (value < MinimumGridOpacityPercent)
        value = MinimumGridOpacityPercent;
    else if (value > 100)
        value = 100;
    std::wstringstream text;
    text << value;
    m_panel->setText(L"options:grid:opacity", text.str(), true);
}

void OptionsModule::dragGridOpacity(plasma::Widget *sender)
{
    if (m_workspace->m_engine.m_buttons & (1 << plasma::MOUSE_BUTTON_LEFT)) {
        updateDrag();
        int value = (int)(m_dragPosition * 0.5f + m_dragStart);
        if (value < MinimumGridOpacityPercent)
            value = MinimumGridOpacityPercent;
        else if (value > 100)
            value = 100;
        std::wstringstream text;
        text << value;
        m_panel->setText(L"options:grid:opacity", text.str(), true);
    }
}

void OptionsModule::stepGridOpacity(plasma::Widget *sender)
{
    int value;
    std::wstringstream(getPanel()->getNode()->getText(L"options:grid:opacity", true)) >> value;
    value += pressDragButton(sender);
    if (value < MinimumGridOpacityPercent)
        value = MinimumGridOpacityPercent;
    else if (value > 100)
        value = 100;
    std::wstringstream text;
    text << value;
    m_panel->setText(L"options:grid:opacity", text.str(), true);
    beginDrag((float)value);
}

void OptionsModule::editGridSnapDistance()
{
    int value;
    std::wstringstream(getPanel()->getNode()->getText(L"options:grid:snapdistance", true)) >> value;
    if (value < 0)
        value = 0;
    else if (value > MaximumGridSnapDistancePixels)
        value = MaximumGridSnapDistancePixels;
    std::wstringstream text;
    text << value;
    m_panel->setText(L"options:grid:snapdistance", text.str(), true);
}

void OptionsModule::dragGridSnapDistance(plasma::Widget *sender)
{
    if (m_workspace->m_engine.m_buttons & (1 << plasma::MOUSE_BUTTON_LEFT)) {
        updateDrag();
        int value = (int)(m_dragPosition * 0.5f + m_dragStart);
        if (value < 0)
            value = 0;
        else if (value > MaximumGridSnapDistancePixels)
            value = MaximumGridSnapDistancePixels;
        std::wstringstream text;
        text << value;
        m_panel->setText(L"options:grid:snapdistance", text.str(), true);
    }
}

void OptionsModule::stepGridSnapDistance(plasma::Widget *sender)
{
    int value;
    std::wstringstream(getPanel()->getNode()->getText(L"options:grid:snapdistance", true)) >> value;
    value += pressDragButton(sender);
    if (value < 0)
        value = 0;
    else if (value > MaximumGridSnapDistancePixels)
        value = MaximumGridSnapDistancePixels;
    std::wstringstream text;
    text << value;
    m_panel->setText(L"options:grid:snapdistance", text.str(), true);
    beginDrag((float)value);
}

void OptionsModule::editExportJpegQuality()
{
    int value;
    std::wstringstream(getPanel()->getNode()->getText(L"options:export:jpeg:quality", true)) >> value;
    if (value < 0)
        value = 0;
    else if (value > 100)
        value = 100;
    std::wstringstream text;
    text << value;
    m_panel->setText(L"options:export:jpeg:quality", text.str(), true);
}

void OptionsModule::dragExportJpegQuality(plasma::Widget *sender)
{
    if (m_workspace->m_engine.m_buttons & (1 << plasma::MOUSE_BUTTON_LEFT)) {
        updateDrag();
        int value = (int)(m_dragPosition * 0.5f + m_dragStart);
        if (value < 0)
            value = 0;
        else if (value > 100)
            value = 100;
        std::wstringstream text;
        text << value;
        m_panel->setText(L"options:export:jpeg:quality", text.str(), true);
    }
}

void OptionsModule::stepExportJpegQuality(plasma::Widget *sender)
{
    int value;
    std::wstringstream(getPanel()->getNode()->getText(L"options:export:jpeg:quality", true)) >> value;
    value += pressDragButton(sender);
    if (value < 0)
        value = 0;
    else if (value > 100)
        value = 100;
    std::wstringstream text;
    text << value;
    m_panel->setText(L"options:export:jpeg:quality", text.str(), true);
    beginDrag((float)value);
}

void OptionsModule::editBackupInterval()
{
    int value;
    std::wstringstream(getPanel()->getNode()->getText(L"options:backup:interval", true)) >> value;
    if (value < 0)
        value = 0;
    std::wstringstream text;
    text << value;
    m_panel->setText(L"options:backup:interval", text.str(), true);
}

void OptionsModule::dragBackupInterval(plasma::Widget *sender)
{
    if (m_workspace->m_engine.m_buttons & (1 << plasma::MOUSE_BUTTON_LEFT)) {
        updateDrag();
        int value = (int)(m_dragPosition * 0.5f + m_dragStart);
        if (value < 0)
            value = 0;
        std::wstringstream text;
        text << value;
        m_panel->setText(L"options:backup:interval", text.str(), true);
    }
}

void OptionsModule::stepBackupInterval(plasma::Widget *sender)
{
    int value;
    std::wstringstream(getPanel()->getNode()->getText(L"options:backup:interval", true)) >> value;
    value += pressDragButton(sender);
    if (value < 0)
        value = 0;
    std::wstringstream text;
    text << value;
    m_panel->setText(L"options:backup:interval", text.str(), true);
    beginDrag((float)value);
}

void OptionsModule::resetCursor()
{
    m_cursorStickiness = DefaultCursorStickiness;
    m_cursorHardware = true;
    std::wstringstream text;
    text << m_cursorStickiness;
    m_panel->setText(L"options:cursor:stickiness", text.str(), true);
    getPanel()->getNode()->setChecked(L"options:cursor:hardware", m_cursorHardware, false, true);
}

void OptionsModule::resetGrid()
{
    m_gridSnapDistance = DefaultGridSnapDistance;
    m_gridOpacity = DefaultGridOpacityPercent;
    std::wstringstream text;
    text << m_gridSnapDistance;
    m_panel->setText(L"options:grid:snapdistance", text.str(), true);
    text.str(L"");
    text << m_gridOpacity;
    m_panel->setText(L"options:grid:opacity", text.str(), true);
}

void OptionsModule::resetTtfCharacters()
{
    m_ttfCharacters = g_defaultTtfCharacters;
    getPanel()->getNode()->setText(L"options:ttf:characters", m_ttfCharacters, true);
}

void OptionsModule::resetExport()
{
    m_exportJpegQuality = DefaultJpegQualityPercent;
    m_exportTransparency = true;
    std::wstringstream text;
    text << m_exportJpegQuality;
    m_panel->setText(L"options:export:jpeg:quality", text.str(), true);
    getPanel()->getNode()->setChecked(L"options:export:transparency", m_exportTransparency, false, true);
}

void OptionsModule::close()
{
    plasma::Widget::setDisplayVisibility(m_panel->getNode()->m_display, 0);
}
