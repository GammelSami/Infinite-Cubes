#include "ExportPDFModule.h"
#include "FormatModule.h"
#include "ColorManagementModule.h"
#include "DesignSheet.h"
#include "main.h"
#include "pdf/PDFWriter.h"
#include "plasma/GenericWidget.h"

// Layout positions, extents and spacing use display units; colour tuples style the overlays.
ExportPDFModule::ExportPDFModule()
{
    {
        std::wstring name(L"module:export:pdf");
        std::wstring kind(L"sidewidget");
        m_panel =
            g_workspace.m_engine.createGenericWidget(name, 300.0f, 140.0f, 120.0f, 100.0f, g_workspace.m_guiNode, kind);
    }
    m_panel->m_flags |= (1 << plasma::WIDGET_RENDER_SURFACE);
    m_panel->setCaption(L"Export PDF", true);
    plasma::Widget::setDisplayVisibility(m_panel->getNode()->m_display, plasma::DISPLAY_INVISIBLE);
}

// Layout positions, extents and spacing use display units; colour tuples style the overlays.
void ExportPDFModule::build()
{
    std::list<ModuleWidgetItem> items;
    items.clear();
    items.push_back(ModuleWidgetItem(L"Rasterize blur", L"export:pdf:rasterize:blur", L"checkbutton3", 0, 0, 160, 30));
    items.push_back(
        ModuleWidgetItem(L"Rasterize textures", L"export:pdf:rasterize:textures", L"checkbutton3", 0, 30, 160, 30));
    addPanel(items, L"", 0);
    m_panel->setChecked(L"export:pdf:rasterize:blur", true, false, true);
    m_panel->setChecked(L"export:pdf:rasterize:textures", true, false, true);
    newRow(3);
    items.clear();
    items.push_back(ModuleWidgetItem(L"Rasterize all", L"export:pdf:rasterize:all", L"checkbutton2", 0, 0, 160, 30));
    addPanel(items, L"", 0);
    newRow(3);
    items.clear();
    items.push_back(ModuleWidgetItem(L"Embed fonts", L"export:pdf:embedfonts", L"checkbutton2", 0, 0, 160, 30));
    addPanel(items, L"", 0);
    m_panel->setChecked(L"export:pdf:embedfonts", true, false, true);
    newRow(3);
    items.clear();
    items.push_back(ModuleWidgetItem(L"Single page", L"export:pdf:singlepage", L"radiobutton3", 0, 0, 160, 30));
    items.push_back(ModuleWidgetItem(L"Layers as pages", L"export:pdf:multipage", L"radiobutton3", 0, 30, 160, 30));
    addPanel(items, L"", 0);
    m_panel->setChecked(L"export:pdf:singlepage", true, false, true);
    newRow(5);
    items.clear();
    items.push_back(ModuleWidgetItem(L" ", L"export:pdf:ok", L"button2", 0, 0, 80, 30));
    items.push_back(ModuleWidgetItem(L" ", L"export:pdf:cancel", L"button2", 80, 0, 80, 30));
    addPanel(items, L"", 0);
    placeIcon(L"export:pdf:ok", L"icon:apply:big");
    placeIcon(L"export:pdf:cancel", L"icon:cancel");
    getPanel()->getNode()->connectWidget(L"export:pdf:ok", plasma::WIDGET_EVENT_POINTER_RELEASED, this,
                                         &ExportPDFModule::exportFile, true);
    getPanel()->getNode()->connectWidget(L"export:pdf:cancel", plasma::WIDGET_EVENT_POINTER_RELEASED, this,
                                         &ExportPDFModule::close, true);
    fitWidth(10);
    fitHeight(10);
}

void ExportPDFModule::exportFile()
{
    plasma::Widget::setDisplayVisibility(m_panel->getNode()->m_display, plasma::DISPLAY_INVISIBLE);
    if (m_workspace->getOpenDesignSheet()) {
        bool rasterizeBlur = m_panel->isChecked(L"export:pdf:rasterize:blur", true);
        bool rasterizeTextures = m_panel->isChecked(L"export:pdf:rasterize:textures", true);
        bool rasterizeAll = m_panel->isChecked(L"export:pdf:rasterize:all", true);
        bool embedFonts = m_panel->isChecked(L"export:pdf:embedfonts", true);
        bool multipage = m_panel->isChecked(L"export:pdf:multipage", true);
        if (m_workspace->getOpenDesignSheet()) {
            std::wstring path = saveFileBox(L"Export PDF", 0, L"Portable Document Format (*.pdf)\0*.pdf\0", 1);
            if (path.empty())
                return;
            if (m_workspace->m_demo) {
                showMessageBox(L"Picroma Plasma Demo Version", L"PDF export is not available in the demo version.");
            } else {
                int options = 0;
                if (rasterizeBlur)
                    options |= kPDFRasterizeBlur;
                if (rasterizeTextures)
                    options |= kPDFRasterizeTextures;
                if (rasterizeAll)
                    options |= kPDFRasterizeAll;
                if (embedFonts)
                    options |= kPDFEmbedFonts;
                if (multipage)
                    options |= kPDFMultipage;
                if (m_workspace->m_colorManagementModule->m_exportCmyk)
                    options |= kPDFOutputCMYK;
                if (!PDFWriter(m_workspace, path, options)
                         .write(m_workspace->getOpenDesignSheet()->getRoot(),
                                &m_workspace->getOpenDesignSheet()->m_page)) {
                    showMessageBox(L"Picroma Plasma", L"Could not write to file. Please make sure it is not write "
                                                      L"protected or in use by another application.");
                }
            }
        }
    }
}

void ExportPDFModule::close()
{
    plasma::Widget::setDisplayVisibility(m_panel->getNode()->m_display, plasma::DISPLAY_INVISIBLE);
}
