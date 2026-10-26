#include "ColorModule.h"
#include "SpecialColorModule.h"
#include "ColorManagementModule.h"
#include "TextureModule.h"
#include "plasma/Engine.h"
#include "plasma/GenericWidget.h"
#include "ColorPickerWidget.h"
#include "FormatModule.h"    // The edit box filters.
#include "ClipboardModule.h" // HsvToRgb.
#include <sstream>
#include <iomanip>
#include <cctype>

HtmlColorFilter g_htmlColorFilter;

static const int HtmlRedShift = 16;
static const int HtmlGreenShift = 8;
static const int MaximumHtmlColorInputLength = 7;

// Layout positions, extents and spacing use display units; colour tuples style the overlays. The default fill
// palette is opaque blue with a 0.3 green component.
ColorModule::ColorModule()
    : m_fillColor(0.0f, 0.3f, 1.0f, 1.0f),
      m_strokeColor(0.0f, 0.0f, 0.0f, 1.0f),
      m_extrusionColor(0.5f, 0.5f, 0.5f, 1.0f),
      m_mode(COLOR_TARGET_FILL),
      m_link(false),
      m_pickAlpha(true),
      m_pickColor(true)
{
    {
        std::wstring name(L"module:color");
        std::wstring kind(L"sidewidget");
        m_panel = g_workspace.m_engine.createGenericWidget(name, 0.0f, 0.0f, 180.0f, 290.0f, g_workspace.m_scalableNode,
                                                           kind);
    }
    m_panel->m_flags |= (1 << plasma::WIDGET_RENDER_SURFACE);
    m_panel->setCaption(L"Color", true);
}

ColorModule::~ColorModule() {}

void ColorModule::chooseFill()
{
    m_mode = COLOR_TARGET_FILL;
    setColor(m_fillColor, true, true, false);
}

void ColorModule::chooseStroke()
{
    m_mode = COLOR_TARGET_STROKE;
    setColor(m_strokeColor, true, true, false);
}

void ColorModule::chooseExtrusion()
{
    m_mode = COLOR_TARGET_EXTRUSION;
    setColor(m_extrusionColor, true, true, false);
}

void ColorModule::editHtml()
{
    std::wstring text = getPanel()->getNode()->getText(L"color:html", true);
    if (text.size() == 0)
        return;
    int start = 0;
    if (text[0] == L'#')
        start = 1;
    int packed;

    // The extraction leaves packed uninitialized if the text cannot be parsed.
    std::wstringstream(text.substr(start, text.size() - start)) >> std::hex >> packed;
    EditorColor color((packed >> HtmlRedShift & 255) / 255.0, (packed >> HtmlGreenShift & 255) / 255.0,
                      (packed & 255) / 255.0, getColor().m_elements[RGBA_ALPHA]);
    setColor(color, true, true, false);
}

void ColorModule::editRed()
{
    int value;

    // The stream extraction has no fallback value on failure.
    std::wstringstream(getPanel()->getNode()->getText(L"color:red", true)) >> value;
    {
        EditorColor color;
        color.m_elements[RGBA_RED] = (float)((double)value / 255.0);
        color.m_elements[RGBA_GREEN] = getColor().m_elements[RGBA_GREEN];
        color.m_elements[RGBA_BLUE] = getColor().m_elements[RGBA_BLUE];
        color.m_elements[RGBA_ALPHA] = getColor().m_elements[RGBA_ALPHA];
        setColor(color, true, true, false);
    }
}

void ColorModule::dragRed(plasma::Widget *sender)
{
    if (!isDragging(sender))
        return;
    int value;

    // The stream extraction has no fallback value on failure.
    std::wstringstream(getPanel()->getNode()->getText(L"color:red", true)) >> value;
    plasma::Vector<2, float> movement = m_workspace->m_engine.m_pointer - m_workspace->m_engine.m_previousPointer;
    value -= (int)movement.m_elements[1];
    {
        EditorColor color;
        color.m_elements[RGBA_RED] = (float)((double)value / 255.0);
        color.m_elements[RGBA_GREEN] = getColor().m_elements[RGBA_GREEN];
        color.m_elements[RGBA_BLUE] = getColor().m_elements[RGBA_BLUE];
        color.m_elements[RGBA_ALPHA] = getColor().m_elements[RGBA_ALPHA];
        setColor(color, true, true, false);
    }
}

void ColorModule::stepRed(plasma::Widget *sender)
{
    int value;

    // The stream extraction has no fallback value on failure.
    std::wstringstream(getPanel()->getNode()->getText(L"color:red", true)) >> value;
    value += pressDragButton(sender);
    {
        EditorColor color;
        color.m_elements[RGBA_RED] = (float)((double)value / 255.0);
        color.m_elements[RGBA_GREEN] = getColor().m_elements[RGBA_GREEN];
        color.m_elements[RGBA_BLUE] = getColor().m_elements[RGBA_BLUE];
        color.m_elements[RGBA_ALPHA] = getColor().m_elements[RGBA_ALPHA];
        setColor(color, true, true, false);
    }
}

void ColorModule::editGreen()
{
    int value;

    // The stream extraction has no fallback value on failure.
    std::wstringstream(getPanel()->getNode()->getText(L"color:green", true)) >> value;
    {
        EditorColor color;
        color.m_elements[RGBA_RED] = getColor().m_elements[RGBA_RED];
        color.m_elements[RGBA_GREEN] = (float)((double)value / 255.0);
        color.m_elements[RGBA_BLUE] = getColor().m_elements[RGBA_BLUE];
        color.m_elements[RGBA_ALPHA] = getColor().m_elements[RGBA_ALPHA];
        setColor(color, true, true, false);
    }
}

void ColorModule::dragGreen(plasma::Widget *sender)
{
    if (!isDragging(sender))
        return;
    int value;

    // The stream extraction has no fallback value on failure.
    std::wstringstream(getPanel()->getNode()->getText(L"color:green", true)) >> value;
    plasma::Vector<2, float> movement = m_workspace->m_engine.m_pointer - m_workspace->m_engine.m_previousPointer;
    value -= (int)movement.m_elements[1];
    {
        EditorColor color;
        color.m_elements[RGBA_RED] = getColor().m_elements[RGBA_RED];
        color.m_elements[RGBA_GREEN] = (float)((double)value / 255.0);
        color.m_elements[RGBA_BLUE] = getColor().m_elements[RGBA_BLUE];
        color.m_elements[RGBA_ALPHA] = getColor().m_elements[RGBA_ALPHA];
        setColor(color, true, true, false);
    }
}

void ColorModule::stepGreen(plasma::Widget *sender)
{
    int value;

    // The stream extraction has no fallback value on failure.
    std::wstringstream(getPanel()->getNode()->getText(L"color:green", true)) >> value;
    value += pressDragButton(sender);
    {
        EditorColor color;
        color.m_elements[RGBA_RED] = getColor().m_elements[RGBA_RED];
        color.m_elements[RGBA_GREEN] = (float)((double)value / 255.0);
        color.m_elements[RGBA_BLUE] = getColor().m_elements[RGBA_BLUE];
        color.m_elements[RGBA_ALPHA] = getColor().m_elements[RGBA_ALPHA];
        setColor(color, true, true, false);
    }
}

void ColorModule::editBlue()
{
    int value;

    // The stream extraction has no fallback value on failure.
    std::wstringstream(getPanel()->getNode()->getText(L"color:blue", true)) >> value;
    {
        EditorColor color;
        color.m_elements[RGBA_RED] = getColor().m_elements[RGBA_RED];
        color.m_elements[RGBA_GREEN] = getColor().m_elements[RGBA_GREEN];
        color.m_elements[RGBA_BLUE] = (float)((double)value / 255.0);
        color.m_elements[RGBA_ALPHA] = getColor().m_elements[RGBA_ALPHA];
        setColor(color, true, true, false);
    }
}

void ColorModule::dragBlue(plasma::Widget *sender)
{
    if (!isDragging(sender))
        return;
    int value;

    // The stream extraction has no fallback value on failure.
    std::wstringstream(getPanel()->getNode()->getText(L"color:blue", true)) >> value;
    plasma::Vector<2, float> movement = m_workspace->m_engine.m_pointer - m_workspace->m_engine.m_previousPointer;
    value -= (int)movement.m_elements[1];
    {
        EditorColor color;
        color.m_elements[RGBA_RED] = getColor().m_elements[RGBA_RED];
        color.m_elements[RGBA_GREEN] = getColor().m_elements[RGBA_GREEN];
        color.m_elements[RGBA_BLUE] = (float)((double)value / 255.0);
        color.m_elements[RGBA_ALPHA] = getColor().m_elements[RGBA_ALPHA];
        setColor(color, true, true, false);
    }
}

void ColorModule::stepBlue(plasma::Widget *sender)
{
    int value;

    // The stream extraction has no fallback value on failure.
    std::wstringstream(getPanel()->getNode()->getText(L"color:blue", true)) >> value;
    value += pressDragButton(sender);
    {
        EditorColor color;
        color.m_elements[RGBA_RED] = getColor().m_elements[RGBA_RED];
        color.m_elements[RGBA_GREEN] = getColor().m_elements[RGBA_GREEN];
        color.m_elements[RGBA_BLUE] = (float)((double)value / 255.0);
        color.m_elements[RGBA_ALPHA] = getColor().m_elements[RGBA_ALPHA];
        setColor(color, true, true, false);
    }
}

void ColorModule::editAlpha()
{
    int value;

    // The stream extraction has no fallback value on failure.
    std::wstringstream(getPanel()->getNode()->getText(L"color:alpha", true)) >> value;
    {
        EditorColor color;
        color.m_elements[RGBA_RED] = getColor().m_elements[RGBA_RED];
        color.m_elements[RGBA_GREEN] = getColor().m_elements[RGBA_GREEN];
        color.m_elements[RGBA_BLUE] = getColor().m_elements[RGBA_BLUE];
        color.m_elements[RGBA_ALPHA] = (float)((double)value / 255.0);
        setColor(color, true, true, false);
    }
}

void ColorModule::dragAlpha(plasma::Widget *sender)
{
    if (!isDragging(sender))
        return;
    int value;

    // The stream extraction has no fallback value on failure.
    std::wstringstream(getPanel()->getNode()->getText(L"color:alpha", true)) >> value;
    plasma::Vector<2, float> movement = m_workspace->m_engine.m_pointer - m_workspace->m_engine.m_previousPointer;
    value -= (int)movement.m_elements[1];
    {
        EditorColor color;
        color.m_elements[RGBA_RED] = getColor().m_elements[RGBA_RED];
        color.m_elements[RGBA_GREEN] = getColor().m_elements[RGBA_GREEN];
        color.m_elements[RGBA_BLUE] = getColor().m_elements[RGBA_BLUE];
        color.m_elements[RGBA_ALPHA] = (float)((double)value / 255.0);
        setColor(color, true, true, false);
    }
}

void ColorModule::stepAlpha(plasma::Widget *sender)
{
    int value;

    // The stream extraction has no fallback value on failure.
    std::wstringstream(getPanel()->getNode()->getText(L"color:alpha", true)) >> value;
    value += pressDragButton(sender);
    {
        EditorColor color;
        color.m_elements[RGBA_RED] = getColor().m_elements[RGBA_RED];
        color.m_elements[RGBA_GREEN] = getColor().m_elements[RGBA_GREEN];
        color.m_elements[RGBA_BLUE] = getColor().m_elements[RGBA_BLUE];
        color.m_elements[RGBA_ALPHA] = (float)((double)value / 255.0);
        setColor(color, true, true, false);
    }
}

bool HtmlColorFilter::validate(const std::wstring &text)
{
    int length = text.size();
    if (!length)
        return false;
    if (length > MaximumHtmlColorInputLength)
        return false;
    for (int i = 0; i < length; ++i) {
        if ((text[i] < L'0' || text[i] > L'9') && (text[i] < L'a' || text[i] > L'f') &&
            (text[i] < L'A' || text[i] > L'F'))
            return false;
    }
    return true;
}

// Layout positions, extents and spacing use display units; colour tuples style the overlays.
void ColorModule::build()
{
    std::list<ModuleWidgetItem> items;
    items.clear();
    items.push_back(ModuleWidgetItem(L" ", L"color:get:fill", L"button2", 0, 0, 53, 20));
    items.push_back(ModuleWidgetItem(L" ", L"color:get:stroke", L"button2", 53, 0, 53, 20));
    items.push_back(ModuleWidgetItem(L" ", L"color:get:extrusion", L"button2", 106, 0, 54, 20));
    items.push_back(ModuleWidgetItem(L" ", L"color:fill", L"colorbutton", 0, 20, 53, 50));
    items.push_back(ModuleWidgetItem(L" ", L"color:stroke", L"colorbutton", 53, 20, 53, 50));
    items.push_back(ModuleWidgetItem(L" ", L"color:extrusion", L"colorbutton", 106, 20, 54, 50));
    items.push_back(ModuleWidgetItem(L" ", L"color:link", L"checkbutton2", 0, 70, 160, 20));
    addPanelAt(items, L"", 10, 30);
    placeIcon(L"color:link", L"icon:link:colors");
    addWidgetAt(ModuleWidgetItem(L"", L"attach:colorpicker", L"", 10, 125, 160, 160));
    m_cursor.m_elements[0] = 10;
    m_cursor.m_elements[1] = 290;
    m_margin.m_elements[0] = m_cursor.m_elements[0];
    addWidget(L"RGB:", L"", L"static", 29, 20, 0);
    items.clear();
    items.push_back(ModuleWidgetItem(L" ", L"color:red", L"editbox", 0, 0, 30, 20));
    items.push_back(ModuleWidgetItem(L" ", L"color:red:drag",
                                     m_orientation == ORIENTATION_HORIZONTAL ? L"hdragbutton" : L"vdragbutton", 30, 0,
                                     14, 20));
    addPanel(items, L"", 0);
    items.clear();
    items.push_back(ModuleWidgetItem(L" ", L"color:green", L"editbox", 0, 0, 30, 20));
    items.push_back(ModuleWidgetItem(L" ", L"color:green:drag",
                                     m_orientation == ORIENTATION_HORIZONTAL ? L"hdragbutton" : L"vdragbutton", 30, 0,
                                     14, 20));
    addPanel(items, L"", 0);
    items.clear();
    items.push_back(ModuleWidgetItem(L" ", L"color:blue", L"editbox", 0, 0, 30, 20));
    items.push_back(ModuleWidgetItem(L" ", L"color:blue:drag",
                                     m_orientation == ORIENTATION_HORIZONTAL ? L"hdragbutton" : L"vdragbutton", 30, 0,
                                     14, 20));
    addPanel(items, L"", 0);
    m_cursor.m_elements[0] = 50;
    m_margin.m_elements[0] = 50;
    newRow(3);
    addWidget(L"HSB:", L"", L"static", 30, 20, 0);
    addWidget(L" ", L"color:hue", L"editbox", 30, 20, 0);
    addWidget(L" ", L"color:saturation", L"editbox", 30, 20, 0);
    addWidget(L" ", L"color:brightness", L"editbox", 30, 20, 0);
    newRow(3);
    addWidget(L"", L"", L"alpha", 20, 20, 0);
    addWidget(L"", L"color:alpha", L"editbox", 30, 20, 0);
    getPanel()->getNode()->setEditFilter(L"color:alpha", &plasma::g_unsignedIntegerFilter, true);
    addWidget(L"#:", L"", L"static", 15, 20, 5);
    addWidget(L"", L"color:html", L"editbox", 50, 20, 0);
    getPanel()->getNode()->setEditFilter(L"color:html", &g_htmlColorFilter, true);
    newRow(3);
    addWidget(L"CMYK:", L"", L"static", 20, 20, 0);
    addWidget(L" ", L"color:cyan", L"static:right", 20, 20, 0);
    addWidget(L" ", L"color:magenta", L"static:right", 20, 20, 0);
    addWidget(L" ", L"color:yellow", L"static:right", 20, 20, 0);
    addWidget(L" ", L"color:key", L"static:right", 20, 20, 0);
    items.clear();
    items.push_back(ModuleWidgetItem(L" ", L"color:pick:coloralpha", L"radiobutton2", 0, 0, 18, 23));
    items.push_back(ModuleWidgetItem(L" ", L"color:pick:color", L"radiobutton2", 18, 0, 18, 23));
    items.push_back(ModuleWidgetItem(L" ", L"color:pick:alpha", L"radiobutton2", 36, 0, 18, 23));
    addUpperPanel(L"", items, (int)(m_panel->viewWidth() * 0.5 - 26.0));
    placeIcon(L"color:pick:coloralpha", L"icon:pick:coloralpha");
    placeIcon(L"color:pick:color", L"icon:pick:color");
    placeIcon(L"color:pick:alpha", L"icon:pick:alpha");
    items.clear();
    items.push_back(ModuleWidgetItem(L" ", L"color:special", L"checkbutton3", 0, 0, 40, 35));
    items.push_back(ModuleWidgetItem(L" ", L"color:management", L"checkbutton3", 0, 35, 40, 35));
    addLeftPanel(L"", items);
    placeIcon(L"color:special", L"icon:special:color");
    placeIcon(L"color:management", L"icon:cmyk");
    fitHeight(10);
    addZoomBar();
    placeIcon(L"color:fill", L"icon:colormode:fill");
    placeIcon(L"color:stroke", L"icon:colormode:stroke");
    placeIcon(L"color:extrusion", L"icon:colormode:extrusion");
    placeIcon(L"color:get:fill", L"icon:pick");
    placeIcon(L"color:get:stroke", L"icon:pick");
    placeIcon(L"color:get:extrusion", L"icon:pick");
    plasma::Node *node = m_workspace->m_engine.createNode(0, 0, 0, 0, L"");
    m_colorWidget = new ColorPickerWidget(&m_workspace->m_engine, m_workspace, node);
    getPanel()->getNode()->moveIntoWidget(L"attach:colorpicker", m_colorWidget, true);

    // The colour buttons' panels get displays of their own.
    m_fillPanel = m_workspace->m_engine.findNode(L"color:fill:colorpanel");
    if (m_fillPanel)
        m_fillPanel->setDisplay(m_fillPanel->m_display->clone());
    m_strokePanel = m_workspace->m_engine.findNode(L"color:stroke:colorpanel");
    if (m_strokePanel)
        m_strokePanel->setDisplay(m_strokePanel->m_display->clone());
    m_extrusionPanel = m_workspace->m_engine.findNode(L"color:extrusion:colorpanel");
    if (m_extrusionPanel)
        m_extrusionPanel->setDisplay(m_extrusionPanel->m_display->clone());
    fitHeight(10);
    getPanel()->getNode()->connectWidget(L"color:get:fill", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &ColorModule::copyFill, true);
    getPanel()->getNode()->connectWidget(L"color:get:stroke", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &ColorModule::copyStroke, true);
    getPanel()->getNode()->connectWidget(L"color:get:extrusion", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &ColorModule::copyExtrusion, true);
    getPanel()->getNode()->connectWidget(L"color:link", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &ColorModule::linkColors, true);
    getPanel()->getNode()->connectWidget(L"color:link", plasma::WIDGET_EVENT_POINTER_DOUBLE_CLICKED, this,
                                         &ColorModule::linkColors, true);
    getPanel()->getNode()->connectWidget(L"color:fill", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &ColorModule::chooseFill, true);
    getPanel()->getNode()->connectWidget(L"color:stroke", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &ColorModule::chooseStroke, true);
    getPanel()->getNode()->connectWidget(L"color:extrusion", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &ColorModule::chooseExtrusion, true);
    getPanel()->getNode()->connectWidget(L"color:fill", plasma::WIDGET_EVENT_POINTER_DOUBLE_CLICKED, this,
                                         &ColorModule::chooseFill, true);
    getPanel()->getNode()->connectWidget(L"color:stroke", plasma::WIDGET_EVENT_POINTER_DOUBLE_CLICKED, this,
                                         &ColorModule::chooseStroke, true);
    getPanel()->getNode()->connectWidget(L"color:extrusion", plasma::WIDGET_EVENT_POINTER_DOUBLE_CLICKED, this,
                                         &ColorModule::chooseExtrusion, true);
    getPanel()->getNode()->connectWidget(L"color:html", plasma::WIDGET_EVENT_FOCUS_LOST, this, &ColorModule::editHtml,
                                         true);
    getPanel()->getNode()->connectWidget(L"color:cyan", plasma::WIDGET_EVENT_FOCUS_LOST, this, &ColorModule::editCmyk,
                                         true);
    getPanel()->getNode()->connectWidget(L"color:magenta", plasma::WIDGET_EVENT_FOCUS_LOST, this,
                                         &ColorModule::editCmyk, true);
    getPanel()->getNode()->connectWidget(L"color:yellow", plasma::WIDGET_EVENT_FOCUS_LOST, this, &ColorModule::editCmyk,
                                         true);
    getPanel()->getNode()->connectWidget(L"color:key", plasma::WIDGET_EVENT_FOCUS_LOST, this, &ColorModule::editCmyk,
                                         true);
    getPanel()->getNode()->connectWidget(L"color:red", plasma::WIDGET_EVENT_FOCUS_LOST, this, &ColorModule::editRed,
                                         true);
    getPanel()->getNode()->connectWidget(L"color:red:drag", plasma::WIDGET_EVENT_POINTER_MOVED, this,
                                         &ColorModule::dragRed, true);
    getPanel()->getNode()->connectWidget(L"color:red:drag", plasma::WIDGET_EVENT_POINTER_RELEASED, this,
                                         &ColorModule::editRed, true);
    getPanel()->getNode()->connectWidget(L"color:red:drag", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &ColorModule::stepRed, true);
    getPanel()->getNode()->connectWidget(L"color:red:drag", plasma::WIDGET_EVENT_POINTER_DOUBLE_CLICKED, this,
                                         &ColorModule::stepRed, true);
    getPanel()->getNode()->connectWidget(L"color:green", plasma::WIDGET_EVENT_FOCUS_LOST, this, &ColorModule::editGreen,
                                         true);
    getPanel()->getNode()->connectWidget(L"color:green:drag", plasma::WIDGET_EVENT_POINTER_MOVED, this,
                                         &ColorModule::dragGreen, true);
    getPanel()->getNode()->connectWidget(L"color:green:drag", plasma::WIDGET_EVENT_POINTER_RELEASED, this,
                                         &ColorModule::editGreen, true);
    getPanel()->getNode()->connectWidget(L"color:green:drag", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &ColorModule::stepGreen, true);
    getPanel()->getNode()->connectWidget(L"color:green:drag", plasma::WIDGET_EVENT_POINTER_DOUBLE_CLICKED, this,
                                         &ColorModule::stepGreen, true);
    getPanel()->getNode()->connectWidget(L"color:hue", plasma::WIDGET_EVENT_FOCUS_LOST, this, &ColorModule::editHue,
                                         true);
    getPanel()->getNode()->connectWidget(L"color:saturation", plasma::WIDGET_EVENT_FOCUS_LOST, this,
                                         &ColorModule::editSaturation, true);
    getPanel()->getNode()->connectWidget(L"color:brightness", plasma::WIDGET_EVENT_FOCUS_LOST, this,
                                         &ColorModule::editBrightness, true);
    getPanel()->getNode()->connectWidget(L"color:blue", plasma::WIDGET_EVENT_FOCUS_LOST, this, &ColorModule::editBlue,
                                         true);
    getPanel()->getNode()->connectWidget(L"color:blue:drag", plasma::WIDGET_EVENT_POINTER_MOVED, this,
                                         &ColorModule::dragBlue, true);
    getPanel()->getNode()->connectWidget(L"color:blue:drag", plasma::WIDGET_EVENT_POINTER_RELEASED, this,
                                         &ColorModule::editBlue, true);
    getPanel()->getNode()->connectWidget(L"color:blue:drag", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &ColorModule::stepBlue, true);
    getPanel()->getNode()->connectWidget(L"color:blue:drag", plasma::WIDGET_EVENT_POINTER_DOUBLE_CLICKED, this,
                                         &ColorModule::stepBlue, true);
    getPanel()->getNode()->connectWidget(L"color:alpha", plasma::WIDGET_EVENT_FOCUS_LOST, this, &ColorModule::editAlpha,
                                         true);
    getPanel()->getNode()->connectWidget(L"color:alpha:drag", plasma::WIDGET_EVENT_POINTER_MOVED, this,
                                         &ColorModule::dragAlpha, true);
    getPanel()->getNode()->connectWidget(L"color:alpha:drag", plasma::WIDGET_EVENT_POINTER_RELEASED, this,
                                         &ColorModule::editAlpha, true);
    getPanel()->getNode()->connectWidget(L"color:alpha:drag", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &ColorModule::stepAlpha, true);
    getPanel()->getNode()->connectWidget(L"color:alpha:drag", plasma::WIDGET_EVENT_POINTER_DOUBLE_CLICKED, this,
                                         &ColorModule::stepAlpha, true);
    getPanel()->getNode()->connectWidget(L"color:special", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &ColorModule::showSpecial, true);
    getPanel()->getNode()->connectWidget(L"color:special", plasma::WIDGET_EVENT_POINTER_DOUBLE_CLICKED, this,
                                         &ColorModule::showSpecial, true);
    getPanel()->getNode()->connectWidget(L"color:management", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &ColorModule::showManagement, true);
    getPanel()->getNode()->connectWidget(L"color:management", plasma::WIDGET_EVENT_POINTER_DOUBLE_CLICKED, this,
                                         &ColorModule::showManagement, true);
    getPanel()->getNode()->connectWidget(L"color:pick:coloralpha", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &ColorModule::chooseColorAlpha, true);
    getPanel()->getNode()->connectWidget(L"color:pick:color", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &ColorModule::chooseColorOnly, true);
    getPanel()->getNode()->connectWidget(L"color:pick:alpha", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &ColorModule::chooseAlpha, true);
    setColor(m_fillColor, true, true, false);
    m_panel->setChecked(L"color:fill", true, false, true);
    m_panel->setChecked(L"color:pick:coloralpha", true, false, true);
    createZoomSequences();
}

void ColorModule::chooseColorAlpha()
{
    m_pickAlpha = true;
    m_pickColor = true;
}

void ColorModule::chooseColorOnly()
{
    m_pickAlpha = false;
    m_pickColor = true;
}

void ColorModule::chooseAlpha()
{
    m_pickAlpha = true;
    m_pickColor = false;
}

int ColorModule::getMode()
{
    return m_mode;
}

const EditorColor &ColorModule::getColor()
{
    switch (m_mode) {
    case COLOR_TARGET_FILL:
        return m_fillColor;
    case COLOR_TARGET_STROKE:
        return m_strokeColor;
    case COLOR_TARGET_EXTRUSION:
        return m_extrusionColor;
    default:
        return m_fillColor;
    }
}

void ColorModule::setColor(const EditorColor &input, bool updatePicker, bool preview, bool fromCmyk)
{
    EditorColor color;
    color = input;
    if (updatePicker) {
        if (!m_pickAlpha)
            color.m_elements[RGBA_ALPHA] = getColor()[RGBA_ALPHA];
        if (!m_pickColor) {
            color.m_elements[RGBA_RED] = getColor()[RGBA_RED];
            color.m_elements[RGBA_GREEN] = getColor()[RGBA_GREEN];
            color.m_elements[RGBA_BLUE] = getColor()[RGBA_BLUE];
        }
    }
    if (color[RGBA_RED] < 0.0f)
        color[RGBA_RED] = 0.0f;
    else if (color[RGBA_RED] > 1.0f)
        color[RGBA_RED] = 1.0f;
    if (color[RGBA_GREEN] < 0.0f)
        color[RGBA_GREEN] = 0.0f;
    else if (color[RGBA_GREEN] > 1.0f)
        color[RGBA_GREEN] = 1.0f;
    if (color[RGBA_BLUE] < 0.0f)
        color[RGBA_BLUE] = 0.0f;
    else if (color[RGBA_BLUE] > 1.0f)
        color[RGBA_BLUE] = 1.0f;
    if (color[RGBA_ALPHA] < 0.0f)
        color[RGBA_ALPHA] = 0.0f;
    else if (color[RGBA_ALPHA] > 1.0f)
        color[RGBA_ALPHA] = 1.0f;
    if (m_link) {
        m_fillColor = color;
        m_strokeColor = color;
        m_extrusionColor = color;
    } else {
        switch (getMode()) {
        case COLOR_TARGET_FILL:
            m_fillColor = color;
            break;
        case COLOR_TARGET_STROKE:
            m_strokeColor = color;
            break;
        case COLOR_TARGET_EXTRUSION:
            m_extrusionColor = color;
            break;
        }
    }
    if (m_fillPanel)
        m_fillPanel->m_display->m_fillColor.value() = m_fillColor;
    if (m_strokePanel)
        m_strokePanel->m_display->m_fillColor.value() = m_strokeColor;
    if (m_extrusionPanel)
        m_extrusionPanel->m_display->m_fillColor.value() = m_extrusionColor;
    if (updatePicker)
        m_panel->m_changed = true;
#ifdef PLASMA_PORTABLE

    // Alternate hosts can use the colour panel without a texture editor.
    if (m_workspace->getTextureModule())
#endif
        m_workspace->getTextureModule()->getPanel()->m_changed = true;
    if (updatePicker)
        m_colorWidget->m_hsv = rgbToHsv(color);
    m_workspace->m_specialColorModule->followColor(color);
    if (m_workspace->m_specialColorModule->isVisible() && preview)
        m_workspace->m_specialColorModule->m_previewPending = true;
    std::wstringstream text;
    text << std::setw(2) << std::setfill(L'0') << std::hex << (int)(color.m_elements[RGBA_RED] * 255.0);
    text << std::setw(2) << std::setfill(L'0') << std::hex << (int)(color.m_elements[RGBA_GREEN] * 255.0);
    text << std::setw(2) << std::setfill(L'0') << std::hex << (int)(color.m_elements[RGBA_BLUE] * 255.0);
    std::wstring html = text.str();
    for (std::wstring::iterator it = html.begin(); it != html.end(); ++it)
        *it = (wchar_t)toupper(*it);
    m_panel->setText(L"color:html", html, true);
    text << std::dec;
    text.str(L"");
    text << (int)(color.m_elements[RGBA_RED] * 255.0);
    m_panel->setText(L"color:red", text.str(), true);
    text.str(L"");
    text << (int)(color.m_elements[RGBA_GREEN] * 255.0);
    m_panel->setText(L"color:green", text.str(), true);
    text.str(L"");
    text << (int)(color.m_elements[RGBA_BLUE] * 255.0);
    m_panel->setText(L"color:blue", text.str(), true);
    text.str(L"");
    text << (int)(color.m_elements[RGBA_ALPHA] * 255.0);
    m_panel->setText(L"color:alpha", text.str(), true);
    plasma::Vector<3, float> hsv(m_colorWidget->m_hsv[HSV_HUE], m_colorWidget->m_hsv[HSV_SATURATION],
                                 m_colorWidget->m_hsv[HSV_BRIGHTNESS]);
    text << std::dec;
    text.str(L"");
    text << (int)hsv[HSV_HUE];
    m_panel->setText(L"color:hue", text.str(), true);
    text.str(L"");
    text << (int)hsv[HSV_SATURATION];
    m_panel->setText(L"color:saturation", text.str(), true);
    text.str(L"");
    text << (int)hsv[HSV_BRIGHTNESS];
    m_panel->setText(L"color:brightness", text.str(), true);
    text.str(L"");
    if (!fromCmyk) {
        plasma::Vector<4, int> cmyk;
        cmyk = m_workspace->m_colorManagementModule->toCmyk(color);
        text.str(L"");
        text << (int)cmyk.m_elements[CMYK_CYAN];
        m_panel->setText(L"color:cyan", text.str(), true);
        text.str(L"");
        text << (int)cmyk.m_elements[CMYK_MAGENTA];
        m_panel->setText(L"color:magenta", text.str(), true);
        text.str(L"");
        text << (int)cmyk.m_elements[CMYK_YELLOW];
        m_panel->setText(L"color:yellow", text.str(), true);
        text.str(L"");
        text << (int)cmyk.m_elements[CMYK_KEY];
        m_panel->setText(L"color:key", text.str(), true);
    }
}

void ColorModule::copyFill()
{
    m_fillColor = getColor();
    setColor(getColor(), false, true, false);
}

void ColorModule::copyStroke()
{
    m_strokeColor = getColor();
    setColor(getColor(), false, true, false);
}

void ColorModule::copyExtrusion()
{
    m_extrusionColor = getColor();
    setColor(getColor(), false, true, false);
}

void ColorModule::showSpecial()
{
    if (m_workspace->getSpecialColorModule()) {
        m_workspace->getSpecialColorModule()->setFadingIn(getPanel()->getNode()->isChecked(L"color:special", true));
        if (!getPanel()->getNode()->isChecked(L"color:special", true))
            m_workspace->getSpecialColorModule()->cancel();
    }
}

void ColorModule::showManagement()
{
    if (m_workspace->getColorManagementModule()) {
        m_workspace->getColorManagementModule()->setFadingIn(
            getPanel()->getNode()->isChecked(L"color:management", true));
        m_workspace->getColorManagementModule()->m_enabled =
            getPanel()->getNode()->isChecked(L"color:management", true);
        m_workspace->markSheetsChanged();
    }
}

void ColorModule::linkColors()
{
    m_link = m_panel->isChecked(L"color:link", true);
    if (m_link)
        setColor(getColor(), false, true, false);
}

void ColorModule::editCmyk()
{
    EditorColor cmyk;
    {
        std::wstringstream(getPanel()->getNode()->getText(L"color:cyan", true)) >> cmyk.m_elements[CMYK_CYAN];
    }
    {
        std::wstringstream(getPanel()->getNode()->getText(L"color:magenta", true)) >> cmyk.m_elements[CMYK_MAGENTA];
    }
    {
        std::wstringstream(getPanel()->getNode()->getText(L"color:yellow", true)) >> cmyk.m_elements[CMYK_YELLOW];
    }
    {
        std::wstringstream(getPanel()->getNode()->getText(L"color:key", true)) >> cmyk.m_elements[CMYK_KEY];
    }
    for (int i = 0; i < 4; ++i) {
        if (cmyk.m_elements[i] > 100.0)
            cmyk.m_elements[i] = 100.0f;
    }
    plasma::Vector<3, float> rgb = m_workspace->m_colorManagementModule->toRgb(cmyk);
    setColor(EditorColor(rgb[RGBA_RED], rgb[RGBA_GREEN], rgb[RGBA_BLUE], getColor()[RGBA_ALPHA]), true, true, true);
}

void ColorModule::editHue()
{
    EditorColor hsv;
    hsv = m_colorWidget->m_hsv;
    std::wstringstream(getPanel()->getNode()->getText(L"color:hue", true)) >> hsv.m_elements[HSV_HUE];
    if (hsv.m_elements[HSV_HUE] < 0.0f)
        hsv.m_elements[HSV_HUE] = 0.0f;
    if (hsv.m_elements[HSV_HUE] > 360.0f)
        hsv.m_elements[HSV_HUE] = 360.0f;
    m_colorWidget->m_hsv = hsv;
    setColor(hsvToRgb(hsv), false, true, false);
}

void ColorModule::editSaturation()
{
    EditorColor hsv;
    hsv = m_colorWidget->m_hsv;
    std::wstringstream(getPanel()->getNode()->getText(L"color:saturation", true)) >> hsv.m_elements[HSV_SATURATION];
    if (hsv.m_elements[HSV_SATURATION] < 0.0f)
        hsv.m_elements[HSV_SATURATION] = 0.0f;
    if (hsv.m_elements[HSV_SATURATION] > 100.0f)
        hsv.m_elements[HSV_SATURATION] = 100.0f;
    m_colorWidget->m_hsv = hsv;
    setColor(hsvToRgb(hsv), false, true, false);
}

void ColorModule::editBrightness()
{
    EditorColor hsv;
    hsv = m_colorWidget->m_hsv;
    std::wstringstream(getPanel()->getNode()->getText(L"color:brightness", true)) >> hsv.m_elements[HSV_BRIGHTNESS];
    if (hsv.m_elements[HSV_BRIGHTNESS] < 0.0f)
        hsv.m_elements[HSV_BRIGHTNESS] = 0.0f;
    if (hsv.m_elements[HSV_BRIGHTNESS] > 100.0f)
        hsv.m_elements[HSV_BRIGHTNESS] = 100.0f;
    m_colorWidget->m_hsv = hsv;
    setColor(hsvToRgb(hsv), false, true, false);
}

template void plasma::Node::connectWidget<ColorModule>(const std::wstring &, int, ColorModule *,
                                                       void (ColorModule::*)(), bool);

template void plasma::Node::connectWidget<ColorModule>(const std::wstring &, int, ColorModule *,
                                                       void (ColorModule::*)(plasma::Widget *), bool);

namespace plasma {
template<>
void Widget::setConnection<ColorModule>(Connection *&connection, ColorModule *object, void (ColorModule::*method)(),
                                        bool enabledOnly)
{
    if (connection)
        delete connection;
    connection = new MemberFunctionConnection<ColorModule>(object, method, enabledOnly);
}
}

template void plasma::Widget::setConnection<ColorModule>(plasma::Widget::Connection *&, ColorModule *,
                                                         void (ColorModule::*)(plasma::Widget *), bool);

template void plasma::Widget::connect<ColorModule>(int, ColorModule *, void (ColorModule::*)(), bool, bool);

template void plasma::Widget::connect<ColorModule>(int, ColorModule *, void (ColorModule::*)(plasma::Widget *), bool,
                                                   bool);
