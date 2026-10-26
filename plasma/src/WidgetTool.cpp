#include "WidgetTool.h"
#include "DesignSheet.h"
#include "WidgetModule.h"
#include "plasma/ListWidget.h"
#include "plasma/Edit.h"
#include "plasma/ScrollButton.h"

WidgetTool::WidgetTool()
    : NodeTool(TOOL_WIDGET)
{}

void WidgetTool::noWidget()
{
    if (m_workspace->getOpenDesignSheet()) {
        DesignSheet *sheet = m_workspace->getOpenDesignSheet();
        std::map<plasma::Node *, NodeInterface *>::iterator it;
        if (sheet->isBusy())
            it = sheet->m_sheetNodes.end();
        else
            it = sheet->m_sheetNodes.begin();
        for (; it != m_workspace->getOpenDesignSheet()->m_sheetNodes.end(); ++it) {
            if (it->second->m_selected) {
                m_workspace->getOpenDesignSheet()->addOperation(new AssignWidgetOperation(
                    m_workspace->getOpenDesignSheet(), "no widget", it->first, it->first->widget(), 0));
                it->first->setWidget(0);
            }
        }
        m_workspace->getOpenDesignSheet()->commit();
        m_workspace->selectionChanged();
    }
}

void WidgetTool::assignWidget()
{
    if (m_workspace->getOpenDesignSheet()) {
        DesignSheet *sheet = m_workspace->getOpenDesignSheet();
        std::map<plasma::Node *, NodeInterface *>::iterator it;
        if (sheet->isBusy())
            it = sheet->m_sheetNodes.end();
        else
            it = sheet->m_sheetNodes.begin();
        for (; it != m_workspace->getOpenDesignSheet()->m_sheetNodes.end(); ++it) {
            if (it->second->m_selected) {
                plasma::Widget *oldWidget = it->first->m_widget;
                m_workspace->m_engine.createWidget(it->first, it->first->getName());
                m_workspace->getOpenDesignSheet()->addOperation(new AssignWidgetOperation(
                    m_workspace->getOpenDesignSheet(), "Widget", it->first, oldWidget, it->first->widget()));
            }
        }
        if (m_workspace->m_widgetModule)
            m_workspace->m_widgetModule->bindWidget();
        m_workspace->selectionChanged();
    }
}

void WidgetTool::assignButton()
{
    if (m_workspace->getOpenDesignSheet()) {
        DesignSheet *sheet = m_workspace->getOpenDesignSheet();
        std::map<plasma::Node *, NodeInterface *>::iterator it;
        if (sheet->isBusy())
            it = sheet->m_sheetNodes.end();
        else
            it = sheet->m_sheetNodes.begin();
        for (; it != m_workspace->getOpenDesignSheet()->m_sheetNodes.end(); ++it) {
            if (it->second->m_selected) {
                plasma::Widget *oldWidget = it->first->m_widget;
                m_workspace->m_engine.createButton(it->first, it->first->getName());
                m_workspace->getOpenDesignSheet()->addOperation(new AssignWidgetOperation(
                    m_workspace->getOpenDesignSheet(), "Button", it->first, oldWidget, it->first->widget()));
            }
        }
        if (m_workspace->m_widgetModule)
            m_workspace->m_widgetModule->bindWidget();
        m_workspace->selectionChanged();
    }
}

void WidgetTool::assignEdit()
{
    if (m_workspace->getOpenDesignSheet()) {
        DesignSheet *sheet = m_workspace->getOpenDesignSheet();
        std::map<plasma::Node *, NodeInterface *>::iterator it;
        if (sheet->isBusy())
            it = sheet->m_sheetNodes.end();
        else
            it = sheet->m_sheetNodes.begin();
        for (; it != m_workspace->getOpenDesignSheet()->m_sheetNodes.end(); ++it) {
            if (it->second->m_selected) {
                plasma::Widget *oldWidget = it->first->m_widget;
                m_workspace->m_engine.createEdit(it->first, it->first->getName());
                m_workspace->getOpenDesignSheet()->addOperation(new AssignWidgetOperation(
                    m_workspace->getOpenDesignSheet(), "Edit", it->first, oldWidget, it->first->widget()));
            }
        }
        if (m_workspace->m_widgetModule)
            m_workspace->m_widgetModule->bindWidget();
        m_workspace->selectionChanged();
    }
}

void WidgetTool::assignScrollButton()
{
    if (m_workspace->getOpenDesignSheet()) {
        DesignSheet *sheet = m_workspace->getOpenDesignSheet();
        std::map<plasma::Node *, NodeInterface *>::iterator it;
        if (sheet->isBusy())
            it = sheet->m_sheetNodes.end();
        else
            it = sheet->m_sheetNodes.begin();
        for (; it != m_workspace->getOpenDesignSheet()->m_sheetNodes.end(); ++it) {
            if (it->second->m_selected) {
                plasma::Widget *oldWidget = it->first->m_widget;
                m_workspace->m_engine.createScrollButton(it->first, it->first->getName());
                m_workspace->getOpenDesignSheet()->addOperation(new AssignWidgetOperation(
                    m_workspace->getOpenDesignSheet(), "ScrollButton", it->first, oldWidget, it->first->widget()));
            }
        }
        if (m_workspace->m_widgetModule)
            m_workspace->m_widgetModule->bindWidget();
        m_workspace->selectionChanged();
    }
}

void WidgetTool::assignScrollSlider()
{
    if (m_workspace->getOpenDesignSheet()) {
        DesignSheet *sheet = m_workspace->getOpenDesignSheet();
        std::map<plasma::Node *, NodeInterface *>::iterator it;
        if (sheet->isBusy())
            it = sheet->m_sheetNodes.end();
        else
            it = sheet->m_sheetNodes.begin();
        for (; it != m_workspace->getOpenDesignSheet()->m_sheetNodes.end(); ++it) {
            if (it->second->m_selected) {
                plasma::Widget *oldWidget = it->first->m_widget;
                m_workspace->m_engine.createScrollSlider(it->first, it->first->getName());
                m_workspace->getOpenDesignSheet()->addOperation(new AssignWidgetOperation(
                    m_workspace->getOpenDesignSheet(), "ScrollSlider", it->first, oldWidget, it->first->widget()));
            }
        }
        if (m_workspace->m_widgetModule)
            m_workspace->m_widgetModule->bindWidget();
        m_workspace->selectionChanged();
    }
}

void WidgetTool::fillContextMenu()
{
    std::list<plasma::Node *> selected;
    m_workspace->getOpenDesignSheet()->getSelectedNodes(selected);
    int count = selected.size();
    {
        std::wstring caption(L"Resize Tool Radius...");
        Workspace *workspace = m_workspace;
        plasma::Widget *item = workspace->m_popupMenu->addItem(workspace->m_popupMenuItem, true);
        item->setPopUpItem(true, true);
        item->setCaption(caption, true);
        item->connect(plasma::WIDGET_EVENT_POINTER_PRESSED, workspace, &Workspace::resizeToolRadius, true, true);
    }
    m_workspace->addPopupMenuSeparator();
    {
        std::wstring caption(L"No Widget");
        plasma::Widget *item = m_workspace->m_popupMenu->addItem(m_workspace->m_popupMenuItem, true);
        item->setPopUpItem(true, true);
        item->setCaption(caption, true);
        item->connect(plasma::WIDGET_EVENT_POINTER_PRESSED, this, &WidgetTool::noWidget, true, true);
        if (count <= 0)
            item->setEnabled(false);
    }
    {
        std::wstring caption(L"Widget");
        plasma::Widget *item = m_workspace->m_popupMenu->addItem(m_workspace->m_popupMenuItem, true);
        item->setPopUpItem(true, true);
        item->setCaption(caption, true);
        item->connect(plasma::WIDGET_EVENT_POINTER_PRESSED, this, &WidgetTool::assignWidget, true, true);
        if (count <= 0)
            item->setEnabled(false);
    }
    {
        std::wstring caption(L"Button");
        plasma::Widget *item = m_workspace->m_popupMenu->addItem(m_workspace->m_popupMenuItem, true);
        item->setPopUpItem(true, true);
        item->setCaption(caption, true);
        item->connect(plasma::WIDGET_EVENT_POINTER_PRESSED, this, &WidgetTool::assignButton, true, true);
        if (count <= 0)
            item->setEnabled(false);
    }
    {
        std::wstring caption(L"Edit");
        plasma::Widget *item = m_workspace->m_popupMenu->addItem(m_workspace->m_popupMenuItem, true);
        item->setPopUpItem(true, true);
        item->setCaption(caption, true);
        item->connect(plasma::WIDGET_EVENT_POINTER_PRESSED, this, &WidgetTool::assignEdit, true, true);
        if (count <= 0)
            item->setEnabled(false);
    }
    {
        std::wstring caption(L"ScrollButton");
        plasma::Widget *item = m_workspace->m_popupMenu->addItem(m_workspace->m_popupMenuItem, true);
        item->setPopUpItem(true, true);
        item->setCaption(caption, true);
        item->connect(plasma::WIDGET_EVENT_POINTER_PRESSED, this, &WidgetTool::assignScrollButton, true, true);
        if (count <= 0)
            item->setEnabled(false);
    }
    {
        std::wstring caption(L"ScrollSlider");
        plasma::Widget *item = m_workspace->m_popupMenu->addItem(m_workspace->m_popupMenuItem, true);
        item->setPopUpItem(true, true);
        item->setCaption(caption, true);
        item->connect(plasma::WIDGET_EVENT_POINTER_PRESSED, this, &WidgetTool::assignScrollSlider, true, true);
        if (count <= 0)
            item->setEnabled(false);
    }
}
