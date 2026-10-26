#include "plasma/FilePath.h"
#include "PaletteModule.h"
#include "ClipboardModule.h"
#include "PaletteWidget.h"
#include "OnlineModule.h"
#include "TextModule.h"
#include "plasma/Engine.h"
#include "plasma/GenericWidget.h"
#include "FormatModule.h"
#include <sstream>
#include <cstdlib>

#include "main.h"
#include <fstream>
#include <cstring>

static const int MaximumPaletteColorCountMinusOne = 0xffff;

// Layout positions, extents and spacing use display units; colour tuples style the overlays.
PaletteModule::PaletteModule()
    : m_selection(0),
      m_expandDirection(0),
      m_expansion(0.0f)
{
    {
        std::wstring name(L"module:palette");
        std::wstring kind(L"bar");
        m_panel = g_workspace.m_engine.createGenericWidget(name, 0.0f, 0.0f, 122.0f, 316.0f, g_workspace.m_guiRightNode,
                                                           kind);
    }
    m_panel->m_flags |= (1 << plasma::WIDGET_RENDER_SURFACE);
    m_panel->setCaption(L"Palette", true);
}

// Layout positions, extents and spacing use display units; colour tuples style the overlays.
void PaletteModule::build()
{
    m_cursor = plasma::Vector<2, float>(8.0f, 12.0f);
    m_margin = m_cursor;
    addWidget(L" ", L"palette:expand", L"expander", 15, 15, 0);
    m_cursor.m_elements[1] = 10;
    std::list<ModuleWidgetItem> items;
    items.push_back(ModuleWidgetItem(L" ", L"palette:swatch:new", L"button2", 0, 0, 25, 20));
    items.push_back(ModuleWidgetItem(L" ", L"palette:swatch:delete", L"button2", 25, 0, 25, 20));
    items.push_back(ModuleWidgetItem(L" ", L"palette:swatch:update", L"button2", 50, 0, 25, 20));
    addPanel(items, L"", 8);
    placeIcon(L"palette:swatch:new", L"icon:swatch:new");
    placeIcon(L"palette:swatch:delete", L"icon:swatch:delete");
    placeIcon(L"palette:swatch:update", L"icon:pick");
    m_cursor = plasma::Vector<2, float>(10.0f, 20.0f);
    m_margin = m_cursor;
    newRow(3);
    addWidget(L"", L"palette:listbox", L"listbox", 98, 270, 0);

    // The swatch grid goes into the list box.
    plasma::Node *node = m_workspace->m_engine.createNode(0, 0, 0, 0);
    m_paletteWidget = new PaletteWidget(&m_workspace->m_engine, m_workspace, node);
    getPanel()->getNode()->moveIntoWidget(L"palette:listbox:list", m_paletteWidget, true);
    m_paletteWidget->moveTo(1.0f, 0.0f, true);
    getPanel()->getNode()->connectWidget(L"palette:expand", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &PaletteModule::chooseExpand, true);
    getPanel()->getNode()->connectWidget(L"palette:expand", plasma::WIDGET_EVENT_POINTER_DOUBLE_CLICKED, this,
                                         &PaletteModule::chooseExpand, true);
    getPanel()->getNode()->connectWidget(L"palette:swatch:new", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &PaletteModule::addColor, true);
    getPanel()->getNode()->connectWidget(L"palette:swatch:new", plasma::WIDGET_EVENT_POINTER_DOUBLE_CLICKED, this,
                                         &PaletteModule::addColor, true);
    getPanel()->getNode()->connectWidget(L"palette:swatch:delete", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &PaletteModule::removeColor, true);
    getPanel()->getNode()->connectWidget(L"palette:swatch:delete", plasma::WIDGET_EVENT_POINTER_DOUBLE_CLICKED, this,
                                         &PaletteModule::removeColor, true);
    getPanel()->getNode()->connectWidget(L"palette:swatch:update", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &PaletteModule::replaceColor, true);
    getPanel()->getNode()->connectWidget(L"palette:swatch:update", plasma::WIDGET_EVENT_POINTER_DOUBLE_CLICKED, this,
                                         &PaletteModule::replaceColor, true);
    loadDefaultPalette();
    openPaletteFile(getAppDataPath() + L"\\palette.plp");
}

plasma::Vector<4, float> PaletteModule::getColor()
{
    if (m_selection < 0 || m_selection >= (int)m_colors.size())
        return plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f);
    return m_colors[m_selection];
}

void PaletteModule::setColor(const plasma::Vector<4, float> &color)
{
    if (m_selection >= 0) {
        if (m_selection >= (int)m_colors.size())
            m_colors.resize(m_selection + 1, plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f));
        m_colors[m_selection] = color;
        m_panel->m_changed = true;
    }
}

void PaletteModule::chooseExpand()
{
    m_expandDirection = m_panel->isChecked(L"palette:expand", true) ? -1 : 1;
}

bool PaletteModule::updateExpansion()
{
    if (m_expandDirection < 0) {
        if (m_expansion < 1.0f) {
            m_expansion = (double)m_workspace->m_engine.m_frameTime * (double)0.002f + m_expansion;
            if (m_expansion > 1.0f)
                m_expansion = 1.0f;
        } else {
            m_expandDirection = 0;
        }
        return true;
    }
    if (m_expandDirection > 0) {
        if (m_expansion > 0.0f) {
            m_expansion = m_expansion - (double)m_workspace->m_engine.m_frameTime * (double)0.002f;
            if (m_expansion < 0.0f)
                m_expansion = 0.0f;
        } else {
            m_expandDirection = 0;
        }
        return true;
    }
    return false;
}

void PaletteModule::savePalette()
{
    std::wstring filename = saveFileBox(L"Save palette", 0, L"Plasma palettes (*.plp)\0*.PLP\0", 1);
    if (filename.empty())
        return;
    savePaletteFile(filename);
}

void PaletteModule::savePaletteFile(const std::wstring &filename)
{
    std::ofstream file(PLASMA_FILE_PATH(filename), std::ios::binary);
    file.write("plp", 3);
    int version = 1;
    file.write((const char *)&version, 4);
    unsigned count = m_colors.size();
    file.write((const char *)&count, 4);

    // The code takes element zero even when the palette is empty.
    file.write((const char *)&m_colors[0], count * 16);
}

void PaletteModule::openPalette()
{
    std::wstring filename = openFileBox(L"Open palette", L"Plasma palettes (*.plp)\0*.PLP\0");
    if (filename.empty())
        return;
    openPaletteFile(filename);
}

void PaletteModule::openPaletteFile(const std::wstring &filename)
{
    std::ifstream file(PLASMA_FILE_PATH(filename), std::ios::binary);
    if (!file.is_open())
        return;
    char signature[4] = "123";
    file.read(signature, 3);
    if (strcmp(signature, "plp") || file.eof())
        return;
    int version = 0;
    file.read((char *)&version, 4);
    if (file.eof())
        return;
    unsigned count = 0;
    file.read((char *)&count, 4);
    if (!file.eof() && count - 1 <= MaximumPaletteColorCountMinusOne) {
        m_colors.resize(count);
        file.read((char *)&m_colors[0], 16 * count);
        m_selection = 0;
        getPaletteWidget()->m_changed = true;
        getPaletteWidget()->refresh();
    }
}

// Default palette recipe: 15 hues at 24 degrees with 20-percent saturation/brightness steps; tint families use
// 22.5-degree hues, five gray levels, 25/75 and 10/90 blends; finish with ten grayscale samples.
void PaletteModule::loadDefaultPalette()
{
    m_colors.erase(m_colors.begin(), m_colors.end());
    {
        plasma::Vector<4, float> hsv;
        hsv[2] = 100.0f;
        hsv[3] = 1.0f;
        for (int hue = 0; hue < 5400; hue += 360) {
            hsv[0] = (double)hue / 15.0;
            for (int saturation = 100; saturation > 0; saturation -= 20) {
                hsv[1] = (float)saturation;
                m_colors.push_back(hsvToRgb(hsv));
            }
        }
        hsv[1] = 100.0f;
        hsv[3] = 1.0f;
        for (int hue = 0; hue < 5400; hue += 360) {
            hsv[0] = (double)hue / 15.0;
            for (int brightness = 100; brightness > 0; brightness -= 20) {
                hsv[2] = (float)brightness;
                m_colors.push_back(hsvToRgb(hsv));
            }
        }
    }
    {
        plasma::Vector<4, float> hsv;
        hsv[1] = 100.0f;
        hsv[2] = 100.0f;
        hsv[3] = 1.0f;
        for (int hue = 0; hue < 5400; hue += 360) {
            hsv[0] = (double)hue * 0.0625;
            for (int gray = 0; gray < 5; ++gray) {
                plasma::Vector<4, float> color = hsvToRgb(hsv);
                float value = 1.0 - (double)gray * 0.25;
                plasma::Vector<4, float> grayColor(value, value, value, 1.0f);
                m_colors.push_back(color * 0.25f + grayColor * 0.75f);
            }
        }
    }
    {
        plasma::Vector<4, float> hsv;
        hsv[1] = 100.0f;
        hsv[2] = 100.0f;
        hsv[3] = 1.0f;
        for (int hue = 0; hue < 5400; hue += 360) {
            hsv[0] = (double)hue * 0.0625;
            for (int gray = 0; gray < 5; ++gray) {
                plasma::Vector<4, float> color = hsvToRgb(hsv);
                float value = 1.0 - (double)gray * 0.25;
                plasma::Vector<4, float> grayColor(value, value, value, 1.0f);
                m_colors.push_back(color * 0.1f + grayColor * 0.9f);
            }
        }
    }
    for (int gray = 0; gray < 10; ++gray) {
        float value = (double)gray / 9.0;
        m_colors.push_back(plasma::Vector<4, float>(value, value, value, 1.0f));
    }
    if (m_panel)
        m_panel->m_changed = true;
    m_paletteWidget->refresh();
}

void PaletteModule::addColor()
{
    m_selection = m_colors.size();
    m_colors.push_back(m_workspace->m_colorModule->getColor());
    m_paletteWidget->getParentWidget()->moveTo(0.0f, -m_paletteWidget->getViewHeight(), true);
    m_paletteWidget->refresh();
}

void PaletteModule::removeColor()
{
    if (m_selection >= 0 && m_selection < (int)m_colors.size()) {
        m_colors.erase(m_colors.begin() + m_selection);
        if (m_selection == (int)m_colors.size())
            --m_selection;
        m_paletteWidget->refresh();
    }
}

void PaletteModule::replaceColor()
{
    setColor(m_workspace->m_colorModule->getColor());
}

void PaletteModule::clearPalette()
{
    m_colors.erase(m_colors.begin(), m_colors.end());
    m_selection = 0;
    m_paletteWidget->refresh();
}
