#include "SheetTool.h"
#include <windows.h>
#include "DesignSheet.h"
#include "plasma/ListWidget.h"

SheetTool::SheetTool()
    : Tool(TOOL_SHEET)
{}

void SheetTool::pointerPressed()
{
    if (m_workspace->getOpenSheet()) {
        if (m_workspace->m_engine.isKeyDown(VK_SHIFT))
            bringToFront();
        else if (m_workspace->m_engine.isKeyDown(VK_CONTROL))
            sendToBack();
    }
}

void SheetTool::fillContextMenu()
{
    {
        std::wstring caption(L"Resize Tool Radius...");
        Workspace *workspace = m_workspace;
        plasma::Widget *item = workspace->m_popupMenu->addItem(workspace->m_popupMenuItem, true);
        item->setPopUpItem(true, true);
        item->setCaption(caption, true);
        item->connect(plasma::WIDGET_EVENT_POINTER_PRESSED, workspace, &Workspace::resizeToolRadius, true, true);
    }
    m_workspace->addPopupMenuSeparator();
    if (m_workspace->m_currentSheetOnly) {
        {
            std::wstring caption(L"Restore Sheet");
            Sheet *sheet = m_workspace->getOpenSheet();
            Workspace *workspace = m_workspace;
            plasma::Widget *item = workspace->m_popupMenu->addItem(workspace->m_popupMenuItem, true);
            item->setPopUpItem(true, true);
            item->setCaption(caption, true);
            item->connect(plasma::WIDGET_EVENT_POINTER_PRESSED, workspace, &Workspace::showAllSheets, true, true);
            if (!sheet)
                item->setEnabled(false);
        }
    } else {
        {
            std::wstring caption(L"Maximize Sheet");
            Sheet *sheet = m_workspace->getOpenSheet();
            Workspace *workspace = m_workspace;
            plasma::Widget *item = workspace->m_popupMenu->addItem(workspace->m_popupMenuItem, true);
            item->setPopUpItem(true, true);
            item->setCaption(caption, true);
            item->connect(plasma::WIDGET_EVENT_POINTER_PRESSED, workspace, &Workspace::showCurrentSheetOnly, true,
                          true);
            if (!sheet)
                item->setEnabled(false);
        }
    }
    m_workspace->addPopupMenuSeparator();
    {
        std::wstring caption(L"Bring Sheet to Front");
        Sheet *sheet = m_workspace->getOpenSheet();
        Workspace *workspace = m_workspace;
        plasma::Widget *item = workspace->m_popupMenu->addItem(workspace->m_popupMenuItem, true);
        item->setPopUpItem(true, true);
        item->setCaption(caption, true);
        item->connect(plasma::WIDGET_EVENT_POINTER_PRESSED, this, &SheetTool::bringToFront, true, true);
        if (!sheet)
            item->setEnabled(false);
    }
    {
        std::wstring caption(L"Bring Sheet Forward");
        Sheet *sheet = m_workspace->getOpenSheet();
        Workspace *workspace = m_workspace;
        plasma::Widget *item = workspace->m_popupMenu->addItem(workspace->m_popupMenuItem, true);
        item->setPopUpItem(true, true);
        item->setCaption(caption, true);
        item->connect(plasma::WIDGET_EVENT_POINTER_PRESSED, this, &SheetTool::bringForward, true, true);
        if (!sheet)
            item->setEnabled(false);
    }
    {
        std::wstring caption(L"Send Sheet Backward");
        Sheet *sheet = m_workspace->getOpenSheet();
        Workspace *workspace = m_workspace;
        plasma::Widget *item = workspace->m_popupMenu->addItem(workspace->m_popupMenuItem, true);
        item->setPopUpItem(true, true);
        item->setCaption(caption, true);
        item->connect(plasma::WIDGET_EVENT_POINTER_PRESSED, this, &SheetTool::sendBackward, true, true);
        if (!sheet)
            item->setEnabled(false);
    }
    {
        std::wstring caption(L"Send Sheet to Back");
        Sheet *sheet = m_workspace->getOpenSheet();
        Workspace *workspace = m_workspace;
        plasma::Widget *item = workspace->m_popupMenu->addItem(workspace->m_popupMenuItem, true);
        item->setPopUpItem(true, true);
        item->setCaption(caption, true);
        item->connect(plasma::WIDGET_EVENT_POINTER_PRESSED, this, &SheetTool::sendToBack, true, true);
        if (!sheet)
            item->setEnabled(false);
    }
    m_workspace->addPopupMenuSeparator();
    {
        std::wstring caption(L"Create Sheet");
        Workspace *workspace = m_workspace;
        plasma::Widget *item = workspace->m_popupMenu->addItem(workspace->m_popupMenuItem, true);
        item->setPopUpItem(true, true);
        item->setCaption(caption, true);
        item->connect(plasma::WIDGET_EVENT_POINTER_PRESSED, workspace, &Workspace::newSheet, true, true);
    }
    {
        std::wstring caption(L"Close Sheet");
        Sheet *sheet = m_workspace->getOpenSheet();
        Workspace *workspace = m_workspace;
        plasma::Widget *item = workspace->m_popupMenu->addItem(workspace->m_popupMenuItem, true);
        item->setPopUpItem(true, true);
        item->setCaption(caption, true);
        item->connect(plasma::WIDGET_EVENT_POINTER_PRESSED, workspace, &Workspace::closeSheet, true, true);
        if (!sheet)
            item->setEnabled(false);
    }
    m_workspace->addPopupMenuSeparator();
    {
        std::wstring caption(L"Tile Sheets Horizontally");
        Workspace *workspace = m_workspace;
        plasma::Widget *item = workspace->m_popupMenu->addItem(workspace->m_popupMenuItem, true);
        item->setPopUpItem(true, true);
        item->setCaption(caption, true);
        item->connect(plasma::WIDGET_EVENT_POINTER_PRESSED, workspace, &Workspace::tileHorizontally, true, true);
    }
    {
        // The menu binds both tile rows to the horizontal command.
        std::wstring caption(L"Tile Sheets Vertically");
        Workspace *workspace = m_workspace;
        plasma::Widget *item = workspace->m_popupMenu->addItem(workspace->m_popupMenuItem, true);
        item->setPopUpItem(true, true);
        item->setCaption(caption, true);
        item->connect(plasma::WIDGET_EVENT_POINTER_PRESSED, workspace, &Workspace::tileHorizontally, true, true);
    }
    m_workspace->addPopupMenuSeparator();
    {
        std::wstring caption(L"Set Sheet Color");
        Sheet *sheet = m_workspace->getOpenSheet();
        Workspace *workspace = m_workspace;
        plasma::Widget *item = workspace->m_popupMenu->addItem(workspace->m_popupMenuItem, true);
        item->setPopUpItem(true, true);
        item->setCaption(caption, true);
        item->connect(plasma::WIDGET_EVENT_POINTER_PRESSED, workspace, &Workspace::setSheetColor, true, true);
        if (!sheet)
            item->setEnabled(false);
    }
    {
        std::wstring caption(L"Get Sheet Color");
        Sheet *sheet = m_workspace->getOpenSheet();
        Workspace *workspace = m_workspace;
        plasma::Widget *item = workspace->m_popupMenu->addItem(workspace->m_popupMenuItem, true);
        item->setPopUpItem(true, true);
        item->setCaption(caption, true);
        item->connect(plasma::WIDGET_EVENT_POINTER_PRESSED, workspace, &Workspace::getSheetColor, true, true);
        if (!sheet)
            item->setEnabled(false);
    }
}
