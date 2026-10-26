#include "ExportModule.h"
#include "FormatModule.h" // The edit box filters.
#include "plasma/ListWidget.h"
#include "plasma/Engine.h"
#include "plasma/GenericWidget.h"
#include "plasma/Transformation.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include "Image.h"
#include "OptionsModule.h"
#include "ColorManagementModule.h"
#include "ProgressModule.h"
#include "main.h" // NarrowString, AskQuestion, ShowMessageBox, SaveFileBox, PumpMessages.
#include "plasma/Display.h"
#include "plasma/Drawing.h"
#include "plasma/GLRenderSurface.h"
#include <GL/glew.h>
#include "../vendor/lcms2-2.0/include/lcms2.h"
#include <algorithm>
#include <cctype>
#include <list>
#include <map>
#include <stdio.h>
extern "C" {
#ifdef PLASMA_PORTABLE
#include <jpeglib.h>
#else
#include "../vendor/libjpeg-8b/jpeglib.h"
#endif
}

// Raster conversion and export through FreeImage.
#include <FreeImage.h>

enum {
    LARGE_IMAGE_DIMENSION = 10000,
    ICC_PROFILE_SIGNATURE_SIZE = 12,
    ICC_PROFILE_HEADER_SIZE = 14,
    ICC_PROFILE_SEGMENT_SIZE = 65519,
    DEMO_MAX_IMAGE_SIZE = 500, // "... a maximal width and height of 500 pixels.".
    DEMO_MAX_JPEG_QUALITY = 75 // "... a maximal JPEG quality of 75%.".
};

// The characters Windows does not allow in file names become '-' (Export makes a file name from a node's name;
// std::for_each).
struct ReplaceReservedFileNameCharacter {
    void operator()(wchar_t &c) const
    {
        if (c == L'<' || c == L'>' || c == L':' || c == L'"' || c == L'/' || c == L'\\' || c == L'|' || c == L'?')
            c = L'-';
    }
};

static const int FormattedDimensionBufferCapacity = 200;
static const int MaximumExportedIconDimension = 48;

// Layout positions, extents and spacing use display units; colour tuples style the overlays.
ExportModule::ExportModule()
{
    m_antialiasing = 2;
    m_pixelPreview = false;
    {
        std::wstring name(L"module:export");
        std::wstring kind(L"toolbar");
        m_panel =
            g_workspace.m_engine.createGenericWidget(name, 0.0f, 0.0f, 455.0f, 35.0f, g_workspace.m_scalableNode, kind);
    }
    m_panel->m_flags |= (1 << plasma::WIDGET_RENDER_SURFACE);
    m_orientation = ORIENTATION_HORIZONTAL;
}

// Layout positions, extents and spacing use display units; colour tuples style the overlays.
void ExportModule::build()
{
    m_cursor = plasma::Vector<2, float>(2.0f, 2.0f);
    m_margin = m_cursor;
    std::list<ModuleWidgetItem> items;
    items.push_back(ModuleWidgetItem(L" ", L"export:export", L"button2", 0, 0, 25, 25));
    items.push_back(ModuleWidgetItem(L" ", L"export:print", L"button2", 25, 0, 25, 25));
    addPanel(items, L"", 0);
    placeIcon(L"export:export", L"icon:export");
    placeIcon(L"export:print", L"icon:print");
    addComboButton(L"Page", L"export:area", L"export:area:listbox", 110, 25, 180, 100, 3);
    m_cursor.m_elements[1] += 3;
    addWidget(L"AA:", L"", L"static", 20, 20, 3);
    m_cursor.m_elements[1] -= 3;
    items.clear();
    items.push_back(ModuleWidgetItem(L"-", L"export:aa:1", L"radiobutton2", 0, 0, 25, 25));
    items.push_back(ModuleWidgetItem(L"4x", L"export:aa:2", L"radiobutton2", 25, 0, 25, 25));
    items.push_back(ModuleWidgetItem(L"9x", L"export:aa:3", L"radiobutton2", 50, 0, 25, 25));
    items.push_back(ModuleWidgetItem(L"16x ", L"export:aa:4", L"radiobutton2", 75, 0, 25, 25));
    addPanel(items, L"", 3);
    newRow(3);
    items.clear();
    items.push_back(ModuleWidgetItem(L" ", L"export:width", L"editbox", 0, 0, 50, 20));
    items.push_back(ModuleWidgetItem(L" ", L"export:width:drag",
                                     m_orientation == ORIENTATION_HORIZONTAL ? L"hdragbutton" : L"vdragbutton", 50, 0,
                                     20, 20));
    addPanel(items, L"", 3);
    addWidget(L"x", L"", L"static", 10, 20, 3);
    items.clear();
    items.push_back(ModuleWidgetItem(L" ", L"export:height", L"editbox", 0, 0, 50, 20));
    items.push_back(ModuleWidgetItem(L" ", L"export:height:drag",
                                     m_orientation == ORIENTATION_HORIZONTAL ? L"hdragbutton" : L"vdragbutton", 50, 0,
                                     20, 20));
    addPanel(items, L"", 3);
    addWidget(L"px @", L"", L"static", 25, 20, 3);
    getPanel()->getNode()->setEditFilter(L"export:width", &plasma::g_unsignedIntegerFilter, true);
    getPanel()->getNode()->setEditFilter(L"export:height", &plasma::g_unsignedIntegerFilter, true);
    items.clear();
    items.push_back(ModuleWidgetItem(L" ", L"export:dpi", L"editbox", 0, 0, 45, 20));
    items.push_back(ModuleWidgetItem(L" ", L"export:dpi:drag",
                                     m_orientation == ORIENTATION_HORIZONTAL ? L"hdragbutton" : L"vdragbutton", 45, 0,
                                     20, 20));
    addPanel(items, L"", 3);

    // The code passes "export:width" here, not "export:dpi".
    getPanel()->getNode()->setEditFilter(L"export:width", &plasma::g_unsignedFloatFilter, true);
    addWidget(L"DPI", L"", L"static", 20, 20, 3);
    m_cursor.m_elements[1] -= 32;
    items.clear();
    items.push_back(ModuleWidgetItem(L" ", L"export:pixelpreview", L"checkbutton2", 0, 0, 50, 52));
    addPanel(items, L"", 13);
    placeIcon(L"export:pixelpreview", L"icon:preview");
    m_panel->setChecked(L"export:aa:2", true, false, true);
    fitWidth(3);
    fitHeight(10);
    plasma::ListWidget *areaList =
        dynamic_cast<plasma::ListWidget *>(m_workspace->m_engine.findWidget(L"export:area:listbox:list"));
    if (areaList) {
        std::list<plasma::ListItem> areas;
        areas.push_back(plasma::ListItem(L"Page", L"export:page", L"listitem", false));
        areas.push_back(plasma::ListItem(L"Drawing", L"export:drawing", L"listitem", false));
        areas.push_back(plasma::ListItem(L"Selected area", L"export:selection", L"listitem", false));
        areas.push_back(plasma::ListItem(L"Selected objects", L"export:nodes", L"listitem", false));
        areaList->setItems(areas);
    }
    m_panel->setChecked(L"export:page", true, false, true);
    m_panel->setText(L"export:area:checkbutton", L"Page", true);
    getPanel()->getNode()->connectWidget(L"export:aa:1", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &ExportModule::chooseAntialiasing1, true);
    getPanel()->getNode()->connectWidget(L"export:aa:2", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &ExportModule::chooseAntialiasing4, true);
    getPanel()->getNode()->connectWidget(L"export:aa:3", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &ExportModule::chooseAntialiasing9, true);
    getPanel()->getNode()->connectWidget(L"export:aa:4", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &ExportModule::chooseAntialiasing16, true);
    getPanel()->getNode()->connectWidget(L"export:pixelpreview", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &ExportModule::choosePixelPreview, true);
    getPanel()->getNode()->connectWidget(L"export:pixelpreview", plasma::WIDGET_EVENT_POINTER_DOUBLE_CLICKED, this,
                                         &ExportModule::choosePixelPreview, true);
    getPanel()->getNode()->connectWidget(L"export:page", plasma::WIDGET_EVENT_POINTER_RELEASED, this,
                                         &ExportModule::chooseArea, true);
    getPanel()->getNode()->connectWidget(L"export:drawing", plasma::WIDGET_EVENT_POINTER_RELEASED, this,
                                         &ExportModule::chooseArea, true);
    getPanel()->getNode()->connectWidget(L"export:selection", plasma::WIDGET_EVENT_POINTER_RELEASED, this,
                                         &ExportModule::chooseArea, true);
    getPanel()->getNode()->connectWidget(L"export:nodes", plasma::WIDGET_EVENT_POINTER_RELEASED, this,
                                         &ExportModule::chooseArea, true);
    getPanel()->getNode()->connectWidget(L"export:width", plasma::WIDGET_EVENT_FOCUS_LOST, this,
                                         &ExportModule::editWidth, true);
    getPanel()->getNode()->connectWidget(L"export:height", plasma::WIDGET_EVENT_FOCUS_LOST, this,
                                         &ExportModule::editHeight, true);
    getPanel()->getNode()->connectWidget(L"export:export", plasma::WIDGET_EVENT_POINTER_RELEASED, this,
                                         &ExportModule::exportFile, true);
    getPanel()->getNode()->connectWidget(L"export:print", plasma::WIDGET_EVENT_POINTER_RELEASED, this,
                                         &ExportModule::print, true);
    getPanel()->getNode()->connectWidget(L"export:dpi", plasma::WIDGET_EVENT_FOCUS_LOST, this, &ExportModule::editDPI,
                                         true);
    getPanel()->getNode()->connectWidget(L"export:dpi:drag", plasma::WIDGET_EVENT_POINTER_MOVED, this,
                                         &ExportModule::dragDPI, true);
    getPanel()->getNode()->connectWidget(L"export:dpi:drag", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &ExportModule::stepDPI, true);
    getPanel()->getNode()->connectWidget(L"export:dpi:drag", plasma::WIDGET_EVENT_POINTER_DOUBLE_CLICKED, this,
                                         &ExportModule::stepDPI, true);
    getPanel()->getNode()->connectWidget(L"export:width", plasma::WIDGET_EVENT_FOCUS_LOST, this,
                                         &ExportModule::editWidth, true);
    getPanel()->getNode()->connectWidget(L"export:width:drag", plasma::WIDGET_EVENT_POINTER_MOVED, this,
                                         &ExportModule::dragWidth, true);
    getPanel()->getNode()->connectWidget(L"export:width:drag", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &ExportModule::stepWidth, true);
    getPanel()->getNode()->connectWidget(L"export:width:drag", plasma::WIDGET_EVENT_POINTER_DOUBLE_CLICKED, this,
                                         &ExportModule::stepWidth, true);

    // The code connects "export:Height" (no such widget) a second time.
    getPanel()->getNode()->connectWidget(L"export:Height", plasma::WIDGET_EVENT_FOCUS_LOST, this,
                                         &ExportModule::editHeight, true);
    getPanel()->getNode()->connectWidget(L"export:height:drag", plasma::WIDGET_EVENT_POINTER_MOVED, this,
                                         &ExportModule::dragHeight, true);
    getPanel()->getNode()->connectWidget(L"export:height:drag", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &ExportModule::stepHeight, true);
    getPanel()->getNode()->connectWidget(L"export:height:drag", plasma::WIDGET_EVENT_POINTER_DOUBLE_CLICKED, this,
                                         &ExportModule::stepHeight, true);
    m_panel->setText(L"export:width", L"", true);
    m_panel->setText(L"export:height", L"", true);
    m_panel->setText(L"export:dpi", L"", true);
    createZoomSequences();
}

void ExportModule::chooseArea(plasma::Widget *sender)
{
    m_panel->setText(L"export:area", sender->getNode()->getText(), true);
    if (m_workspace->getOpenDesignSheet()) {
        pageChanged();
        updateWidgets();
    }
}

// Empty bounds start with positive minima and negative maxima.
void ExportModule::getExportBounds(plasma::Vector<2, float> &min, plasma::Vector<2, float> &max)
{
    if (!m_workspace->getOpenDesignSheet())
        return;
    min = plasma::Vector<2, float>(0.0f, 0.0f);
    max = plasma::Vector<2, float>(m_workspace->getOpenDesignSheet()->m_page.size.m_elements[PAGE_WIDTH],
                                   m_workspace->getOpenDesignSheet()->m_page.size.m_elements[PAGE_HEIGHT]);
    if (m_workspace->getGuiNode()->isChecked(L"export:drawing", true)) {
        min = plasma::Vector<2, float>(100000000.0f, 100000000.0f);
        max = plasma::Vector<2, float>(-100000000.0f, -100000000.0f);
        bool first = true;
        plasma::Matrix<float> identity;
        identity.identity();
        m_workspace->getOpenDesignSheet()->m_root->extendBounds(min, max, identity, first, true);
    } else if (m_workspace->getGuiNode()->isChecked(L"export:selection", true) ||
               m_workspace->getGuiNode()->isChecked(L"export:nodes", true)) {
        min = plasma::Vector<2, float>(100000000.0f, 100000000.0f);
        max = plasma::Vector<2, float>(-100000000.0f, -100000000.0f);
        bool first = true;
        m_workspace->getOpenDesignSheet()->m_root->updateMatricesRecursively();
        for (std::map<plasma::Node *, NodeInterface *>::iterator it =
                 m_workspace->getOpenDesignSheet()->sheetNodesBegin();
             it != m_workspace->getOpenDesignSheet()->m_sheetNodes.end(); ++it) {
            if (!it->second->m_selected)
                continue;
            if (!it->first->isShown() || it->first->isLocked())
                continue;

            // The node's matrix relative to the sheet's root.
            plasma::Matrix<float> matrix =
                m_workspace->getOpenDesignSheet()->m_root->m_inverseMatrix * it->first->m_matrix;
            it->first->extendBounds(min, max, matrix, first, true);
        }
    }
    if (max.m_elements[0] < min.m_elements[0] || max.m_elements[1] < min.m_elements[1]) {
        min = plasma::Vector<2, float>(0.0f, 0.0f);
        max = plasma::Vector<2, float>(0.0f, 0.0f);
    }
}

plasma::Vector<2, float> ExportModule::getExportSize()
{
    if (!m_workspace->getOpenDesignSheet())
        return plasma::Vector<2, float>(0.0f, 0.0f);
    plasma::Vector<2, float> min;
    plasma::Vector<2, float> max;
    getExportBounds(min, max);
    return max - min;
}

void ExportModule::updateWidgets()
{
    if (!m_workspace->getOpenDesignSheet()) {
        getPanel()->setText(L"export:width", L"", true);
        getPanel()->setText(L"export:height", L"", true);
        getPanel()->setText(L"export:dpi", L"", true);
        return;
    }
    getExportBounds(m_boundsMin, m_boundsMax);
    plasma::Vector<2, float> size = m_boundsMax - m_boundsMin;

    // The page's DPI over the 72 points per inch of page units.
    float scale = (float)(m_workspace->getOpenDesignSheet()->m_page.size.m_elements[PAGE_RESOLUTION] / 72.0);
    plasma::Vector<2, float> dimensions = size * scale;
    char buffer[FormattedDimensionBufferCapacity];
    sprintf(buffer, "%i", (int)(dimensions.m_elements[0] + 0.5));
    {
        std::string text(buffer);
        std::wstring name(L"export:width");
        getPanel()->setText(name, widenString(text), true);
    }
    sprintf(buffer, "%i", (int)(dimensions.m_elements[1] + 0.5));
    {
        std::string text(buffer);
        std::wstring name(L"export:height");
        getPanel()->setText(name, widenString(text), true);
    }
    sprintf(buffer, "%g",
            (double)roundToHundredths(m_workspace->getOpenDesignSheet()->m_page.size.m_elements[PAGE_RESOLUTION]));
    {
        std::string text(buffer);
        std::wstring name(L"export:dpi");
        getPanel()->setText(name, widenString(text), true);
    }
}

void ExportModule::sheetChanged()
{
    updateWidgets();
}

void ExportModule::editWidth()
{
    if (m_workspace->getOpenDesignSheet()) {
        plasma::Vector<2, float> size = getExportSize();
        int width = atoi(narrowString(m_workspace->getGuiNode()->getText(L"export:width", true)).c_str());
        m_workspace->getOpenDesignSheet()->m_page.size.m_elements[PAGE_RESOLUTION] =
            (float)(width * 72.0 / size.m_elements[0]);
        m_workspace->getOpenDesignSheet()->m_modified = true;
        pageChanged();
        updateWidgets();
    }
}

void ExportModule::dragWidth(plasma::Widget *sender)
{
    if (m_workspace->getOpenDesignSheet() && isDragging(sender)) {
        plasma::Vector<2, float> size = getExportSize();
        int width = atoi(narrowString(m_workspace->getGuiNode()->getText(L"export:width", true)).c_str());
        width = (int)(getDragMovement() + width);
        m_workspace->getOpenDesignSheet()->m_page.size.m_elements[PAGE_RESOLUTION] =
            (float)(width * 72.0 / size.m_elements[0]);
        m_workspace->getOpenDesignSheet()->m_modified = true;
        pageChanged();
        updateWidgets();
    }
}

void ExportModule::stepWidth(plasma::Widget *sender)
{
    if (m_workspace->getOpenDesignSheet()) {
        plasma::Vector<2, float> size = getExportSize();
        int width = atoi(narrowString(m_workspace->getGuiNode()->getText(L"export:width", true)).c_str());
        width = pressDragButton(sender) + width;
        if (width < 0)
            width = 0;
        m_workspace->getOpenDesignSheet()->m_page.size.m_elements[PAGE_RESOLUTION] =
            (float)(width * 72.0 / size.m_elements[0]);
        m_workspace->getOpenDesignSheet()->m_modified = true;
        pageChanged();
        updateWidgets();
    }
}

void ExportModule::editHeight()
{
    if (m_workspace->getOpenDesignSheet()) {
        plasma::Vector<2, float> size = getExportSize();
        int height = atoi(narrowString(m_workspace->getGuiNode()->getText(L"export:height", true)).c_str());
        m_workspace->getOpenDesignSheet()->m_page.size.m_elements[PAGE_RESOLUTION] =
            (float)(height * 72.0 / size.m_elements[1]);
        m_workspace->getOpenDesignSheet()->m_modified = true;
        pageChanged();
        updateWidgets();
    }
}

void ExportModule::dragHeight(plasma::Widget *sender)
{
    if (m_workspace->getOpenDesignSheet() && isDragging(sender)) {
        plasma::Vector<2, float> size = getExportSize();
        int height = atoi(narrowString(m_workspace->getGuiNode()->getText(L"export:height", true)).c_str());
        height = (int)(getDragMovement() + height);
        m_workspace->getOpenDesignSheet()->m_page.size.m_elements[PAGE_RESOLUTION] =
            (float)(height * 72.0 / size.m_elements[1]);
        m_workspace->getOpenDesignSheet()->m_modified = true;
        pageChanged();
        updateWidgets();
    }
}

void ExportModule::stepHeight(plasma::Widget *sender)
{
    if (m_workspace->getOpenDesignSheet()) {
        plasma::Vector<2, float> size = getExportSize();
        int height = atoi(narrowString(m_workspace->getGuiNode()->getText(L"export:height", true)).c_str());

        // The height is stepped one further than the width.
        height = pressDragButton(sender) + height + 1;
        if (height < 0)
            height = 0;
        m_workspace->getOpenDesignSheet()->m_page.size.m_elements[PAGE_RESOLUTION] =
            (float)(height * 72.0 / size.m_elements[1]);
        m_workspace->getOpenDesignSheet()->m_modified = true;
        pageChanged();
        updateWidgets();
    }
}

void ExportModule::editDPI()
{
    if (m_workspace->getOpenDesignSheet()) {
        float dpi = (float)atof(narrowString(m_workspace->getGuiNode()->getText(L"export:dpi", true)).c_str());
        if (dpi < 0.0f)
            dpi = 0.0f;
        m_workspace->getOpenDesignSheet()->m_page.size.m_elements[PAGE_RESOLUTION] = dpi;
        m_workspace->getOpenDesignSheet()->m_modified = true;
        pageChanged();
        updateWidgets();
    }
}

void ExportModule::dragDPI(plasma::Widget *sender)
{
    if (m_workspace->getOpenDesignSheet() && isDragging(sender)) {
        float dpi = (float)atof(narrowString(m_workspace->getGuiNode()->getText(L"export:dpi", true)).c_str());
        dpi = getDragMovement() + dpi;
        if (dpi < 0.0f)
            dpi = 0.0f;
        m_workspace->getOpenDesignSheet()->m_page.size.m_elements[PAGE_RESOLUTION] = dpi;
        m_workspace->getOpenDesignSheet()->m_modified = true;
        pageChanged();
        updateWidgets();
    }
}

void ExportModule::stepDPI(plasma::Widget *sender)
{
    if (m_workspace->getOpenDesignSheet()) {
        float dpi = (float)atof(narrowString(m_workspace->getGuiNode()->getText(L"export:dpi", true)).c_str());
        dpi = pressDragButton(sender) + dpi;
        if (dpi < 0.0f)
            dpi = 0.0f;
        m_workspace->getOpenDesignSheet()->m_page.size.m_elements[PAGE_RESOLUTION] = dpi;
        m_workspace->getOpenDesignSheet()->m_modified = true;
        pageChanged();
        updateWidgets();
    }
}

void ExportModule::choosePixelPreview()
{
    m_pixelPreview = m_panel->isChecked(L"export:pixelpreview", true);
    if (m_workspace->getOpenDesignSheet()) {
        pageChanged();
        updateWidgets();
    }
}

// The extension-to-dialog-filter table uses one-based positions in the save dialog filter list. "export:export":
// asks for the file (offering the last name used for the page, the drawing or the selection), then renders the
// area into it.
void ExportModule::exportFile()
{
    if (!m_workspace->getOpenDesignSheet())
        return;
    plasma::Vector<2, float> boundsMin(0.0f, 0.0f);
    plasma::Vector<2, float> boundsMax(0.0f, 0.0f);
    getExportBounds(boundsMin, boundsMax);

    // Unordered bounds pass this guard, as in Print.
    if (boundsMax.m_elements[0] <= boundsMin.m_elements[0] || boundsMax.m_elements[1] <= boundsMin.m_elements[1])
        return;
    int width = atoi(narrowString(getPanel()->getNode()->getText(L"export:width", true)).c_str());
    int height = atoi(narrowString(getPanel()->getNode()->getText(L"export:height", true)).c_str());
    if (width >= LARGE_IMAGE_DIMENSION || height >= LARGE_IMAGE_DIMENSION) {
        std::wstringstream message;
        message << "The selected image resolution (" << width << " x " << height
                << " pixels) is very large. Do you really want to export with this resolution?";
        if (askQuestion(L"Picroma Plasma", message.str(), false) == ANSWER_NO)
            return;
    }

    // The name last used for the page, the drawing or the selection.
    std::wstring *path = &m_workspace->getOpenDesignSheet()->m_pageExportPath;

    // The save dialog's filter index per file extension.
    std::map<std::wstring, int> filterIndices;
    filterIndices[L"jpg"] = 1;
    filterIndices[L"jpeg"] = 1;
    filterIndices[L"bmp"] = 2;
    filterIndices[L"png"] = 3;
    filterIndices[L"tif"] = 4;
    filterIndices[L"tiff"] = 4;
    filterIndices[L"tga"] = 5;
    filterIndices[L"gif"] = 6;
    filterIndices[L"ico"] = 7;
    filterIndices[L"xpm"] = 8;
    filterIndices[L"ppm"] = 9;
    std::wstring name;
    if (m_workspace->getGuiNode()->isChecked(L"export:drawing", true)) {
        path = &m_workspace->getOpenDesignSheet()->m_drawingExportPath;
    } else if (m_workspace->getGuiNode()->isChecked(L"export:selection", true) ||
               m_workspace->getGuiNode()->isChecked(L"export:nodes", true)) {
        std::list<plasma::Node *> nodes;
        m_workspace->getOpenDesignSheet()->getSelectedNodes(nodes);
        path = &m_workspace->getOpenDesignSheet()->m_selectionExportPaths[nodes];

        // A single node named like an image file names the file.
        if (nodes.size() == 1) {
            name = nodes.back()->getName();
            unsigned separator = name.rfind(L'.');
            std::wstring extension = name.substr(separator + 1, name.size() - separator - 1);
            std::transform(extension.begin(), extension.end(), extension.begin(), tolower);
            if (!path->size() || filterIndices.find(extension) != filterIndices.end()) {
                std::for_each(name.begin(), name.end(), ReplaceReservedFileNameCharacter());
                *path = name;
            }
        }
    }

    // The extension of the name offered, else the last one exported.
    std::wstring extension;
    if (path->size()) {
        int dot = path->rfind(L'.');
        if (dot > 0) {
            extension = path->substr(dot + 1, path->size() - dot - 1);
            std::transform(extension.begin(), extension.end(), extension.begin(), tolower);
        }
    }
    if (!extension.size())
        extension = m_workspace->getOpenDesignSheet()->m_exportFormat;
    int filterIndex = filterIndices[extension];
    enum ImageDialogFilter {
        JPEG_DIALOG_FILTER = 1
    };
    if (!filterIndex)
        filterIndex = JPEG_DIALOG_FILTER;
    std::wstring file = saveFileBox(L"Export image", path->c_str(),
                                    L"JPEG Image (*.jpg)\0*.jpg\0"
                                    L"Windows Bitmap (*.bmp)\0*.bmp\0"
                                    L"Portable Network Graphics (*.png)\0*.png\0"
                                    L"Tagged Image File Format (*.tif)\0*.tif\0"
                                    L"Truevision Targa (*.tga)\0*.tga\0"
                                    L"Graphics Interchange Format (*.gif)\0*.gif\0"
                                    L"Windows Icon (*.ico)\0*.ico\0"
                                    L"X11 Pixmap (*.xpm)\0*.xpm\0"
                                    L"Portable Pixelmap (*.ppm)\0*.ppm\0",
                                    filterIndex);
    if (file.size()) {
        *path = file;
        unsigned dot = path->rfind(L'.');
        extension = path->substr(dot + 1, path->size() - dot - 1);
        std::transform(extension.begin(), extension.end(), extension.begin(), tolower);
        m_workspace->getOpenDesignSheet()->m_exportFormat = extension;
        renderImageToFile(file, boundsMin, boundsMax);
    }
}

void ExportModule::print()
{
    if (m_workspace->isDemo()) {
        std::wstring text(L"Printing is disabled in the demo version supports a maximal width and height of 500 "
                          L"pixels. Please choose a lower resolution.");
        const std::wstring caption(L"Picroma Plasma Demo Version");
        showMessageBox(caption, text);
    } else {
        int width = atoi(narrowString(getPanel()->getNode()->getText(L"export:width", true)).c_str());
        int height = atoi(narrowString(getPanel()->getNode()->getText(L"export:height", true)).c_str());
        if (width > LARGE_IMAGE_DIMENSION || height > LARGE_IMAGE_DIMENSION) {
            std::wstringstream message;
            message << "The selected image resolution (" << width << " x " << height
                    << " pixels) is very large. Do you really want to print with this resolution?";
            if (askQuestion(L"Picroma Plasma", message.str(), false) == ANSWER_NO)
                return;
        }
        ::print();
    }
}

// Gives fully transparent black pixels the average colour of their opaque right and lower neighbours, then (second
// sweep, backwards) of their left and upper ones, twice; filtering at the edges of the exported image then does
// not darken them.
static void bleedTransparentPixels(unsigned char **rows, int width, int height)
{
    int passes = 2;
    do {
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                unsigned char *pixel = rows[y] + 4 * x;
                if (rows[y][4 * x + 3] || pixel[0] || pixel[1] || pixel[2])
                    continue;
                int red = 0;
                int green = 0;
                int count = 0;
                int blue = 0;
                if (x < width - 1 && (pixel[7] > 0 || pixel[4] || pixel[5] || pixel[6])) {
                    green = rows[y][4 * x + 5];
                    red = rows[y][4 * x + 4];
                    blue = rows[y][4 * x + 6];
                    count = 1;
                }
                if (y < height - 1) {
                    unsigned char *below = rows[y + 1] + 4 * x;
                    if (rows[y + 1][4 * x + 3] > 0 || below[0] || below[1] || below[2]) {
                        red += rows[y + 1][4 * x];
                        green += rows[y + 1][4 * x + 1];
                        blue += rows[y + 1][4 * x + 2];
                        count++;
                    }
                }
                if (count > 0) {
                    rows[y][4 * x] = red / count;
                    rows[y][4 * x + 1] = green / count;
                    rows[y][4 * x + 2] = blue / count;
                }
            }
        }
        for (int y = height - 1; y >= 0; y--) {
            for (int x = width - 1; x >= 0; x--) {
                unsigned char *pixel = rows[y] + 4 * x;
                if (rows[y][4 * x + 3] || pixel[0] || pixel[1] || pixel[2])
                    continue;
                int red = 0;
                int green = 0;
                int count = 0;
                int blue = 0;

                // X > 0 keeps these offsets inside the preceding four-byte pixel.
                if (x > 0 && (pixel[-1] > 0 || pixel[-4] || pixel[-3] || pixel[-2])) {
                    green = rows[y][4 * x - 3];
                    red = rows[y][4 * x - 4];
                    blue = rows[y][4 * x - 2];
                    count = 1;
                }
                if (y > 0) {
                    unsigned char *above = rows[y - 1] + 4 * x;
                    if (rows[y - 1][4 * x + 3] > 0 || above[0] || above[1] || above[2]) {
                        red += rows[y - 1][4 * x];
                        green += rows[y - 1][4 * x + 1];
                        blue += rows[y - 1][4 * x + 2];
                        count++;
                    }
                }
                if (count > 0) {
                    rows[y][4 * x] = red / count;
                    rows[y][4 * x + 1] = green / count;
                    rows[y][4 * x + 2] = blue / count;
                }
            }
        }
    } while (--passes);
}

// Renders the node (or only the mask drawing) between the bounds (page units) into a new image at the page's DPI,
// downsampled from the antialiasing factor with transparency (the PDF writer's rasterized nodes,
// PDFWriter::PrepareRasterizedNodes). Returns 0 for an empty area.
Image *ExportModule::renderImage(plasma::Node *node, const plasma::Vector<2, float> &minimum,
                                 const plasma::Vector<2, float> &maximum, plasma::Drawing *mask)
{
    // Pixels per page unit (the page's DPI over 72 points per inch).
    float scale = m_workspace->getOpenDesignSheet()->m_page.size.m_elements[PAGE_RESOLUTION] / 72.0;
    plasma::Vector<2, float> origin = minimum * (float)m_antialiasing * scale;
    plasma::Vector<2, float> end = maximum * (float)m_antialiasing * scale;
    if (origin.m_elements[0] >= end.m_elements[0] || origin.m_elements[1] >= end.m_elements[1])
        return 0;

    plasma::GLRenderSurface *surface =
        dynamic_cast<plasma::GLRenderSurface *>(m_workspace->m_engine.createRenderSurface(0));
    surface->resize(m_workspace->m_engine.viewportWidth(), m_workspace->m_engine.viewportHeight());

    // The page's clear colour is computed but unused here.
    float alpha = m_workspace->getOpenDesignSheet()->m_page.color.m_elements[3];
    plasma::Vector<4, float> background =
        m_workspace->getOpenDesignSheet()->m_page.color * plasma::Vector<4, float>(alpha, alpha, alpha, 1.0f);
    int width = m_antialiasing * (int)((end.m_elements[0] - origin.m_elements[0]) / (float)m_antialiasing + 0.5);
    int height = m_antialiasing * (int)((end.m_elements[1] - origin.m_elements[1]) / (float)m_antialiasing + 0.5);
    int tileWidth = surface->m_width / 2;
    int tileHeight = surface->m_height / 2;
    int tilesX = (width + tileWidth - 1) / tileWidth;
    int tilesY = (height + tileHeight - 1) / tileHeight;

    std::wstringstream message;
    message << "Rendering " << node->getName() << std::endl;
    message << width / m_antialiasing << " x " << height / m_antialiasing << " px (" << m_antialiasing * m_antialiasing
            << "x AA)";
    m_workspace->getProgressModule()->setText(message.str());
    m_workspace->getProgressModule()->setProgress(0.0f);
    pumpMessages();

    Image image(width, height, 4);
    plasma::Matrix<float> matrix = m_workspace->getOpenDesignSheet()->getNodeMatrix(node->getParent());
    renderTiles(node, &image, matrix, surface, tilesX, tilesY, scale, origin, mask);

    message.str(L"");
    message << "Rendering " << node->getName() << std::endl;
    message << width / m_antialiasing << " x " << height / m_antialiasing << " px (" << m_antialiasing * m_antialiasing
            << "x AA)" << std::endl;
    message << "Resampling..." << std::endl;
    m_workspace->getProgressModule()->setText(message.str());
    m_workspace->getProgressModule()->advance(0.0f);
    pumpMessages();
    Image *result = DownsampleImage(&image, true, m_antialiasing);

    message.str(L"");
    message << "Rendering " << node->getName() << std::endl;
    message << width / m_antialiasing << " x " << height / m_antialiasing << " px (" << m_antialiasing * m_antialiasing
            << "x AA)" << std::endl;
    message << "Writing..." << std::endl;
    m_workspace->getProgressModule()->setText(message.str());
    m_workspace->getProgressModule()->advance(0.0f);
    pumpMessages();
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    plasma::Engine::deleteRenderSurface(surface);
    m_workspace->getProgressModule()->setProgress(1.0f);
    pumpMessages();
    return result;
}

// Reduces an antialiased RGBA rendering by factor in each direction: with alpha, the colours weighted by their
// alpha (then bled into the transparent pixels); without, composited onto white as RGB.
Image *__stdcall DownsampleImage(Image *image, bool alpha, int factor)
{
    int width = image->m_width / factor;
    int height = image->m_height / factor;
    if (alpha) {
        Image *result = new Image(width, height, 4);
        for (int y = 0; y < height; y++) {
            unsigned char *out = result->m_rows[y];
            for (int x = 0; x < width; x++) {
                int red = 0;
                int green = 0;
                int blue = 0;
                int opacity = 0;
                int weight = 0;
                for (int sy = 0; sy < factor; sy++) {
                    unsigned char *in = image->m_rows[y * factor + sy];
                    for (int sx = 0; sx < factor; sx++) {
                        unsigned char a = in[4 * (x * factor + sx) + 3];
                        opacity += a;
                        if (in[4 * (x * factor + sx) + 3]) {
                            red += 255 * in[4 * (x * factor + sx)];
                            green += 255 * in[4 * (x * factor + sx) + 1];
                            weight += a;
                            blue += 255 * in[4 * (x * factor + sx) + 2];
                        }
                    }
                }
                if (weight > 0) {
                    red /= weight;
                    green /= weight;
                    blue /= weight;
                    opacity /= factor * factor;
                }
                out[4 * x] = red;
                out[4 * x + 1] = green;
                out[4 * x + 2] = blue;
                out[4 * x + 3] = opacity;
            }
        }
        bleedTransparentPixels(result->m_rows, result->m_width, result->m_height);
        return result;
    }
    Image *result = new Image(width, height, 3);
    for (int y = 0; y < height; y++) {
        unsigned char *out = result->m_rows[y];
        for (int x = 0; x < width; x++) {
            int red = 0;
            int green = 0;
            int blue = 0;
            int opacity = 0;
            for (int sy = 0; sy < factor; sy++) {
                unsigned char *in = image->m_rows[y * factor + sy];
                for (int sx = 0; sx < factor; sx++) {
                    int sampleOffset = 4 * x * factor + 4 * sx;
                    red += in[sampleOffset];
                    green += in[sampleOffset + 1];
                    blue += in[sampleOffset + 2];
                    opacity += in[sampleOffset + 3];
                }
            }
            red /= factor * factor;
            green /= factor * factor;
            blue /= factor * factor;
            opacity /= factor * factor;
            if (opacity > 0) {
                red = 255 * red / opacity;
                green = 255 * green / opacity;
                blue = 255 * blue / opacity;
            }

            // Over white: 255 - opacity * (255 - colour) / 255.
            out[3 * x] = (opacity * (red - 255) + 255 * 255) / 255;
            out[3 * x + 1] = (opacity * (green - 255) + 255 * 255) / 255;
            out[3 * x + 2] = (opacity * (blue - 255) + 255 * 255) / 255;
        }
    }
    return result;
}

// Renders the node (or only the mask drawing, in the node's frame) tile by tile into the surface and reads each
// tile back into the image; the image is the surface's centre half in each direction per tile. origin is the
// area's corner in surface pixels.
void ExportModule::renderTiles(plasma::Node *node, Image *image, const plasma::Matrix<float> &matrix,
                               plasma::GLRenderSurface *surface, int tilesX, int tilesY, float scale,
                               const plasma::Vector<2, float> &origin, plasma::Drawing *mask)
{
    if (!node->m_display->m_visibility.value() || !node->isShown())
        return;
    glReadBuffer(GL_COLOR_ATTACHMENT0_EXT);
    glEnable(GL_BLEND);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    float progress = 1.0 / (tilesX * tilesY + 1);
    for (int tileX = 0; tileX < tilesX; tileX++) {
        for (int tileY = 0; tileY < tilesY; tileY++) {
            m_workspace->m_engine.beginSurface(
                surface, plasma::Vector<2, float>(0.0f, 0.0f),
                plasma::Vector<2, float>((float)surface->m_width, (float)surface->m_height), 1.0f, false);
            glMatrixMode(GL_MODELVIEW);
            glPushMatrix();
            glLoadIdentity();
            m_workspace->m_engine.m_renderFlags &= ~(1 << plasma::RENDER_ANTIALIAS);
            bool savedFlag = m_workspace->m_engine.m_unused1F4;
            m_workspace->m_engine.m_unused1F4 = false;
            m_workspace->m_engine.m_renderFlags |= (1 << plasma::RENDER_BLUR);
            int tileWidth = image->m_width / tilesX;
            int tileHeight = image->m_height / tilesY;
            int left = (surface->m_width - tileWidth) / 2;
            int bottom = (surface->m_height - tileHeight) / 2;
            glTranslatef(left - (tileX * tileWidth + origin.m_elements[0]),
                         bottom - (tileY * tileHeight + origin.m_elements[1]), 0.0f);
            float zoom = m_antialiasing * scale;
            glScalef(zoom, zoom, 1.0f);
            glMultMatrixf(matrix.m_elements);

            // The last tiles take the remainder.
            if (tileX == tilesX - 1)
                tileWidth = image->m_width - tileX * tileWidth;
            if (tileY == tilesY - 1)
                tileHeight = image->m_height - tileY * tileHeight;
            surface->clear();
            if (mask) {
                glMultMatrixf(node->m_transformation->m_matrix.m_elements);
                mask->draw(0, plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f));
            } else {
                node->draw(0);
            }
            glRasterPos2i(0, 0);
            for (int row = 0; row < tileHeight; row++)
                glReadPixels(left, bottom + row, tileWidth, 1, GL_RGBA, GL_UNSIGNED_BYTE,
                             image->m_rows[row + tileY * (image->m_height / tilesY)] +
                                 4 * tileX * (image->m_width / tilesX));
            glPopMatrix();
            m_workspace->m_engine.endSurface();
            m_workspace->m_engine.m_renderFlags |= (1 << plasma::RENDER_ANTIALIAS);
            m_workspace->m_engine.m_unused1F4 = savedFlag;
            m_workspace->m_progressModule->advance(progress);
            pumpMessages();
        }
    }
}

void ExportModule::chooseAntialiasing1()
{
    m_antialiasing = 1;
    if (m_workspace->getOpenDesignSheet())
        pageChanged();
}

void ExportModule::chooseAntialiasing4()
{
    m_antialiasing = 2;
    if (m_workspace->getOpenDesignSheet())
        pageChanged();
}

void ExportModule::chooseAntialiasing9()
{
    m_antialiasing = 3;
    if (m_workspace->getOpenDesignSheet())
        pageChanged();
}

void ExportModule::chooseAntialiasing16()
{
    m_antialiasing = 4;
    if (m_workspace->getOpenDesignSheet())
        pageChanged();
}

// Renders the area between the bounds (page units) into an image file; the format follows the extension. The demo
// version writes only JPEG, at most 500 pixels a side and quality 75. Returns whether the file was written (always
// false for a CMYK JPEG).
bool ExportModule::renderImageToFile(const std::wstring &path, const plasma::Vector<2, float> &boundsMin,
                                     const plasma::Vector<2, float> &boundsMax)
{
    // Pixels per page unit (the page's DPI over 72 points per inch).
    float scale = m_workspace->getOpenDesignSheet()->m_page.size.m_elements[PAGE_RESOLUTION] / 72.0;
    unsigned dot = path.rfind(L'.');
    std::wstring extension = path.substr(dot + 1, path.size() - dot - 1);
    std::transform(extension.begin(), extension.end(), extension.begin(), toupper);
    bool demo = m_workspace->m_demo;
    int format = kFormatUnknown;
    int flags = 0;
    plasma::Vector<2, float> origin = boundsMin * (float)m_antialiasing * scale;
    plasma::Vector<2, float> end = boundsMax * (float)m_antialiasing * scale;
    int width = m_antialiasing * (int)((end.m_elements[0] - origin.m_elements[0]) / (float)m_antialiasing + 0.5);
    int height = m_antialiasing * (int)((end.m_elements[1] - origin.m_elements[1]) / (float)m_antialiasing + 0.5);
    if (demo && (width / m_antialiasing > DEMO_MAX_IMAGE_SIZE || height / m_antialiasing > DEMO_MAX_IMAGE_SIZE)) {
        std::wstring text(
            L"The demo version supports a maximal width and height of 500 pixels. Please choose a lower resolution.");
        std::wstring caption(L"Picroma Plasma Demo Version");
        showMessageBox(caption, text);
        return false;
    }
    if (extension == L"JPG" || extension == L"JPEG") {
        format = kFormatJPEG;
        flags = m_workspace->getOptionsModule()->m_exportJpegQuality;
        if (demo) {
            std::wstring text(L"Please note that the demo version supports only a maximal JPEG quality of 75%.");
            std::wstring caption(L"Picroma Plasma Demo Version");
            showMessageBox(caption, text);
            if (flags > DEMO_MAX_JPEG_QUALITY)
                flags = DEMO_MAX_JPEG_QUALITY;
        }
    } else if (demo) {
        showMessageBox(
            L"Picroma Plasma Demo Version",
            L"Unsupported format. The demo Version supports only JPEG export with a maximal quality of 75%.");
        return false;
    }
    if (extension == L"BMP")
        format = kFormatBMP;
    if (extension == L"PNG")
        format = kFormatPNG;
    if (extension == L"PPM")
        format = kFormatPPM;
    if (extension == L"TIF")
        format = kFormatTIFF;
    if (extension == L"TGA")
        format = kFormatTarga;
    if (extension == L"GIF")
        format = kFormatGIF;
    if (extension == L"ICO")
        format = kFormatICO;
    if (extension == L"XPM")
        format = kFormatXPM;

    // The code tests "PPM" twice.
    if (extension == L"PPM")
        format = kFormatPPM;
    if (format == kFormatUnknown)
        return false;
    if (end.m_elements[0] <= origin.m_elements[0] || end.m_elements[1] <= origin.m_elements[1])
        return false;

    plasma::GLRenderSurface *surface =
        dynamic_cast<plasma::GLRenderSurface *>(m_workspace->m_engine.createRenderSurface(0));

    // The page colour, premultiplied by its alpha.
    float alpha = m_workspace->getOpenDesignSheet()->m_page.color.m_elements[3];
    surface->m_clearColor =
        m_workspace->getOpenDesignSheet()->m_page.color * plasma::Vector<4, float>(alpha, alpha, alpha, 1.0f);
    surface->resize(m_workspace->m_engine.m_viewportWidth, m_workspace->m_engine.m_viewportHeight);
    int tileWidth = surface->m_width / 2;
    int tileHeight = surface->m_height / 2;
    int tilesX = (width + tileWidth - 1) / tileWidth;
    int tilesY = (height + tileHeight - 1) / tileHeight;

    // "Rendering <document> to <file>" in the progress panel.
    unsigned slash = path.rfind(L'/');
    unsigned backslash = path.rfind(L'\\');
    if (slash <= backslash)
        slash = backslash;
    std::wstring fileName = path.substr(slash + 1, path.size() - slash - 1);
    unsigned sheetSlash = m_workspace->getOpenDesignSheet()->m_path.rfind(L'/');
    unsigned sheetBackslash = m_workspace->getOpenDesignSheet()->m_path.rfind(L'\\');
    if (sheetSlash <= sheetBackslash)
        sheetSlash = sheetBackslash;
    std::wstring sheetName = m_workspace->getOpenDesignSheet()->m_path.substr(
        sheetSlash + 1, m_workspace->getOpenDesignSheet()->m_path.size() - sheetSlash - 1);
    std::wstringstream message;
    message << "Rendering " << sheetName << " to " << fileName.c_str() << std::endl;
    message << width / m_antialiasing << " x " << height / m_antialiasing << " px (" << m_antialiasing * m_antialiasing
            << "x AA)";
    m_workspace->getProgressModule()->setText(message.str());
    m_workspace->getProgressModule()->setProgress(0.0f);
    pumpMessages();

    Image *image = 0;
    Image *result = 0;
    FIBITMAP *bitmap = 0;
    try {
        image = new Image(width, height, 4);
        if (m_workspace->getGuiNode()->isChecked(L"export:nodes", true)) {
            // The selected nodes only: instances under a temporary root.
            plasma::Node *root = m_workspace->m_engine.createNode(0, 0, 0, 0, L"");
            instanceSelectedNodes(root, m_workspace->getOpenDesignSheet()->getRoot());
            plasma::Matrix<float> identity;
            identity.identity();
            renderTiles(root, image, identity, surface, tilesX, tilesY, scale, origin, 0);
            m_workspace->m_engine.destroyNode(root);
        } else {
            renderTiles(m_workspace->getOpenDesignSheet()->getRoot(), image,
                        m_workspace->getOpenDesignSheet()->getRoot()->transformation()->m_inverseMatrix, surface,
                        tilesX, tilesY, scale, origin, 0);
        }

        message.str(L"");
        message << "Rendering " << sheetName.c_str() << " to " << fileName.c_str() << std::endl;
        message << width / m_antialiasing << " x " << height / m_antialiasing << " px ("
                << m_antialiasing * m_antialiasing << "x AA)" << std::endl;
        message << "Resampling..." << std::endl;
        m_workspace->getProgressModule()->setText(message.str());
        m_workspace->getProgressModule()->advance(0.0f);
        pumpMessages();

        // PNG, TIFF, ICO and Targa keep the transparency when the options ask for it ("exportTransparency").
        bool transparent =
            (format == kFormatPNG || format == kFormatTIFF || format == kFormatICO || format == kFormatTarga) &&
            m_workspace->getOptionsModule()->m_exportTransparency;
        result = DownsampleImage(image, transparent, m_antialiasing);
        delete image;
        image = 0;

        message.str(L"");
        message << "Rendering " << sheetName.c_str() << " to " << fileName.c_str() << std::endl;
        message << width / m_antialiasing << " x " << height / m_antialiasing << " px ("
                << m_antialiasing * m_antialiasing << "x AA)" << std::endl;
        message << "Writing..." << std::endl;
        m_workspace->getProgressModule()->setText(message.str());
        m_workspace->getProgressModule()->advance(0.0f);
        pumpMessages();
        BOOL saved = FALSE;
        if (format != kFormatJPEG || !m_workspace->m_colorManagementModule->m_exportCmyk) {
            // FreeImage stores BGR(A) rows bottom up.
            int bytesPerPixel = result->m_bytesPerPixel;
            bitmap = FreeImage_Allocate(result->m_width, result->m_height, 8 * bytesPerPixel, 0, 0, 0);
            for (int y = 0; y < result->m_height; y++) {
                unsigned char *line = FreeImage_GetScanLine(bitmap, FreeImage_GetHeight(bitmap) - y - 1);
                for (int x = 0; x < result->m_width; x++) {
                    for (int c = 0; c < 3; c++)
                        line[c + bytesPerPixel * x] = result->m_rows[y][bytesPerPixel * x + 2 - c];
                    if (bytesPerPixel == 4)
                        line[4 * x + 3] = result->m_rows[y][4 * x + 3];
                }
            }
            if (format == kFormatGIF) {
                FIBITMAP *quantized = FreeImage_ColorQuantize(bitmap, FIQ_WUQUANT); // FIQ_WUQUANT.
                FreeImage_Unload(bitmap);
                bitmap = quantized;
            } else if (format == kFormatICO) {
                FIBITMAP *icon = FreeImage_MakeThumbnail(bitmap, MaximumExportedIconDimension, TRUE);
                FreeImage_Unload(bitmap);
                bitmap = icon;
            }
            saved = FreeImage_SaveU(static_cast<FREE_IMAGE_FORMAT>(format), bitmap, path.c_str(), flags);
            FreeImage_Unload(bitmap);
            bitmap = 0;
        } else {
            // Writes a CMYK JPEG with the output profile embedded. APP2 markers reserve an ICC header before each profile
            // segment.
            FILE *file = fopen(narrowString(path).c_str(), "wb");
            jpeg_compress_struct compressor;
            jpeg_error_mgr errorManager;
            compressor.err = jpeg_std_error(&errorManager);
            jpeg_create_compress(&compressor);
            jpeg_stdio_dest(&compressor, file);
            compressor.image_width = result->m_width;
            compressor.image_height = result->m_height;
            compressor.input_components = 4;
            compressor.in_color_space = JCS_CMYK;
            jpeg_set_defaults(&compressor);
            jpeg_set_colorspace(&compressor, JCS_CMYK);
            jpeg_set_quality(&compressor, m_workspace->getOptionsModule()->m_exportJpegQuality, TRUE);
            jpeg_start_compress(&compressor, TRUE);
            unsigned offset = 0;
            cmsUInt32Number profileSize = 0;
            cmsSaveProfileToMem(m_workspace->m_colorManagementModule->m_profile, 0, &profileSize);
            std::vector<char> profile(profileSize);
            cmsSaveProfileToMem(m_workspace->m_colorManagementModule->m_profile, &profile[0], &profileSize);
            if (profileSize) {
                char signature[ICC_PROFILE_SIGNATURE_SIZE] = "ICC_PROFILE";
                std::vector<char> marker(profileSize + ICC_PROFILE_HEADER_SIZE);
                memcpy(&marker[0], signature, ICC_PROFILE_SIGNATURE_SIZE);
                for (; offset < profileSize; offset += ICC_PROFILE_SEGMENT_SIZE) {
                    // The code uses a signed clamp, even for a size over INT_MAX.
                    int length = profileSize - offset;
                    if (length >= ICC_PROFILE_SEGMENT_SIZE)
                        length = ICC_PROFILE_SEGMENT_SIZE;
                    marker[ICC_PROFILE_SIGNATURE_SIZE] = offset / ICC_PROFILE_SEGMENT_SIZE + 1;
                    marker[ICC_PROFILE_SIGNATURE_SIZE + 1] = profileSize / ICC_PROFILE_SEGMENT_SIZE + 1;
                    memcpy(&marker[ICC_PROFILE_HEADER_SIZE], &profile[offset], length);
                    jpeg_write_marker(&compressor, JPEG_APP0 + 2, (const JOCTET *)&marker[0],
                                      length + ICC_PROFILE_HEADER_SIZE);
                }
            }
            std::vector<plasma::Vector<4, unsigned char>> cmyk(result->m_width);
            cmsHPROFILE sRGB = cmsCreate_sRGBProfile();
            cmsHTRANSFORM transform = cmsCreateTransform(
                sRGB, TYPE_RGB_8, m_workspace->m_colorManagementModule->m_profile, TYPE_CMYK_8, INTENT_PERCEPTUAL, 0);
            for (int y = 0; y < result->m_height; y++) {
                cmsDoTransform(transform, result->m_rows[y], &cmyk[0], result->m_width);

                // Adobe's CMYK JPEGs store the inks inverted.
                for (int x = 0; x < (int)cmyk.size(); x++)
                    cmyk[x] = plasma::Vector<4, unsigned char>(255, 255, 255, 255) - cmyk[x];
                JSAMPROW row = (JSAMPROW)&cmyk[0];
                jpeg_write_scanlines(&compressor, &row, 1);
            }
            cmsCloseProfile(sRGB);
            cmsDeleteTransform(transform);
            jpeg_finish_compress(&compressor);
            fclose(file);
        }
        delete result;
        result = 0;
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        plasma::Engine::deleteRenderSurface(surface);
        m_workspace->getProgressModule()->setProgress(1.0f);
        pumpMessages();
        return saved == TRUE;
    } catch (...) {
        {
            std::wstring text(L"Too much memory required. Please choose a lower resolution or anti-aliasing quality.");
            std::wstring caption(L"Plasma");
            showMessageBox(caption, text);
        }
        delete image;
        delete result;
        if (bitmap)
            FreeImage_Unload(bitmap);

        // The failure path leaves the render surface allocated.
        m_workspace->getProgressModule()->setProgress(1.0f);
        pumpMessages();
    }
    return false;
}

// "export:nodes": instances of the selected shown nodes of the subtree under parent, each placed as it is relative
// to the sheet's root.
void ExportModule::instanceSelectedNodes(plasma::Node *parent, plasma::Node *node)
{
    if (!node->isShown())
        return;
    if (m_workspace->getOpenDesignSheet()->findSheetNode(node)->m_selected) {
        plasma::Node *frame = m_workspace->m_engine.createNode(0, 0, 0, parent);
        plasma::Matrix<float> matrix =
            m_workspace->getOpenDesignSheet()->m_root->m_inverseMatrix * node->getParent()->m_matrix;
        frame->m_transformation->m_deformation.value() *= matrix;
        frame->m_transformation->attributesChanged(plasma::ATTRIBUTES_CHANGED_ALL);
        node->instance(frame);
    } else {
        for (std::list<plasma::Node *>::iterator it = node->m_children.begin(); it != node->m_children.end(); ++it)
            instanceSelectedNodes(parent, *it);
    }
}
