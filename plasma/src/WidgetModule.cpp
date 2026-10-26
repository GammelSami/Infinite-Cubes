#include "WidgetModule.h"
#include "DesignSheet.h"
#include "Operations.h"
#include "plasma/Button.h"
#include "plasma/Display.h"
#include "plasma/Edit.h"
#include "plasma/Engine.h"
#include "plasma/GenericShape.h"
#include "plasma/GenericWidget.h"
#include "plasma/ListWidget.h"
#include "plasma/ScrollButton.h"
#include "plasma/ScrollSlider.h"
#include "NodeInterface.h"
#include "Workspace.h"
#include <list>
#include <map>

// Layout positions, extents and spacing use display units; colour tuples style the overlays.
WidgetModule::WidgetModule()
{
    m_widget = 0;
    m_button = 0;
    m_scrollButton = 0;
    m_scrollSlider = 0;
    {
        std::wstring name(L"");
        std::wstring kind(L"bar");
        m_panel = g_workspace.m_engine.createGenericWidget(name, 90.0f, 60.0f, 700.0f, 120.0f,
                                                           g_workspace.m_scalableNode, kind);
    }
    m_panel->m_flags |= (1 << plasma::WIDGET_RENDER_SURFACE);
    m_panel->setCaption(L"Widget", true);

    // Hidden until the widget tool shows it.
    m_panel->getNode()->m_display->m_visibility.value() = 0;
}

// Layout positions, extents and spacing use display units; colour tuples style the overlays.
void WidgetModule::build()
{
    addWidgetAt(ModuleWidgetItem(L"Caption:", L"", L"static", 55, 30, 0, 0));
    addWidgetAt(ModuleWidgetItem(L"", L"widget:caption", L"editbox", 110, 30, 200, 20));
    std::list<ModuleWidgetItem> items;
    items.push_back(ModuleWidgetItem(L"Scroll X", L"widget:scroll:horizontally", L"checkbutton2", 0, 0, 90, 20));
    items.push_back(ModuleWidgetItem(L"Scroll Y", L"widget:scroll:vertically", L"checkbutton2", 90, 0, 90, 20));
    items.push_back(ModuleWidgetItem(L"Adapt Width", L"widget:adapt:width", L"checkbutton2", 0, 20, 90, 20));
    items.push_back(ModuleWidgetItem(L"Adapt Height", L"widget:adapt:height", L"checkbutton2", 90, 20, 90, 20));
    addPanelAt(items, L"", 60, 60);
    items.clear();
    items.push_back(ModuleWidgetItem(L"Render surface", L"widget:rendersurface", L"checkbutton2", 0, 0, 90, 20));
    items.push_back(ModuleWidgetItem(L"Pop-up", L"widget:popup", L"checkbutton2", 0, 20, 90, 20));
    items.push_back(ModuleWidgetItem(L"Resizable", L"widget:resizable", L"checkbutton2", 0, 40, 90, 20));
    addPanelAt(items, L"", 350, 30);
    items.clear();
    items.push_back(ModuleWidgetItem(L"<", L"widget:horizontal:left", L"radiobutton2", 0, 0, 20, 20));
    items.push_back(ModuleWidgetItem(L"|", L"widget:horizontal:center", L"radiobutton2", 20, 0, 20, 20));
    items.push_back(ModuleWidgetItem(L">", L"widget:horizontal:right", L"radiobutton2", 40, 0, 20, 20));
    addPanelAt(items, L"", 250, 70);
    items.clear();
    items.push_back(ModuleWidgetItem(L"^", L"widget:vertical:top", L"radiobutton2", 0, 0, 20, 20));
    items.push_back(ModuleWidgetItem(L"-", L"widget:vertical:center", L"radiobutton2", 0, 20, 20, 20));
    items.push_back(ModuleWidgetItem(L"v", L"widget:vertical:bottom", L"radiobutton2", 0, 40, 20, 20));
    addPanelAt(items, L"", 320, 30);
    addWidgetAt(ModuleWidgetItem(L"Button", L"", L"static", 460, 10, 0, 0));
    items.clear();
    items.push_back(ModuleWidgetItem(L"Press Button", L"button:normal", L"radiobutton2", 0, 0, 90, 20));
    items.push_back(ModuleWidgetItem(L"Check Button", L"button:check", L"radiobutton2", 0, 20, 90, 20));
    items.push_back(ModuleWidgetItem(L"Radio Button", L"button:radio", L"radiobutton2", 0, 40, 90, 20));
    addPanelAt(items, L"", 460, 30);
    addWidgetAt(ModuleWidgetItem(L"Scroll Button", L"", L"static", 560, 10, 0, 0));
    items.clear();
    items.push_back(ModuleWidgetItem(L"^", L"scrollbutton:up", L"radiobutton2", 0, 0, 60, 20));
    items.push_back(ModuleWidgetItem(L"<", L"scrollbutton:left", L"radiobutton2", 0, 20, 30, 20));
    items.push_back(ModuleWidgetItem(L">", L"scrollbutton:right", L"radiobutton2", 30, 20, 30, 20));
    items.push_back(ModuleWidgetItem(L"v", L"scrollbutton:down", L"radiobutton2", 0, 40, 60, 20));
    addPanelAt(items, L"", 560, 30);
    addWidgetAt(ModuleWidgetItem(L"Scroll Slider", L"", L"static", 650, 10, 0, 0));
    items.clear();
    items.push_back(ModuleWidgetItem(L"Horizontal", L"scrollslider:horizontal", L"radiobutton2", 0, 0, 90, 20));
    items.push_back(ModuleWidgetItem(L"Vertical", L"scrollslider:vertical", L"radiobutton2", 0, 20, 90, 20));
    addPanelAt(items, L"", 650, 30);
    fitWidth(10);
    getPanel()->getNode()->connectWidget(L"widget:horizontal:left", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &WidgetModule::alignLeft, true);
    getPanel()->getNode()->connectWidget(L"widget:horizontal:right", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &WidgetModule::alignRight, true);
    getPanel()->getNode()->connectWidget(L"widget:horizontal:center", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &WidgetModule::centerHorizontally, true);
    getPanel()->getNode()->connectWidget(L"widget:vertical:top", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &WidgetModule::alignTop, true);
    getPanel()->getNode()->connectWidget(L"widget:vertical:bottom", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &WidgetModule::alignBottom, true);
    getPanel()->getNode()->connectWidget(L"widget:vertical:center", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &WidgetModule::centerVertically, true);
    getPanel()->getNode()->connectWidget(L"widget:adapt:width", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &WidgetModule::adaptWidth, true);
    getPanel()->getNode()->connectWidget(L"widget:adapt:height", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &WidgetModule::adaptHeight, true);
    getPanel()->getNode()->connectWidget(L"widget:scroll:horizontally", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &WidgetModule::scrollHorizontally, true);
    getPanel()->getNode()->connectWidget(L"widget:scroll:vertically", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &WidgetModule::scrollVertically, true);
    getPanel()->getNode()->connectWidget(L"widget:popup", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &WidgetModule::setPopup, true);
    getPanel()->getNode()->connectWidget(L"widget:rendersurface", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &WidgetModule::setRenderSurface, true);
    getPanel()->getNode()->connectWidget(L"widget:resizable", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &WidgetModule::setResizable, true);
    getPanel()->getNode()->connectWidget(L"widget:caption", plasma::WIDGET_EVENT_FOCUS_LOST, this,
                                         &WidgetModule::editCaption, true);
    getPanel()->getNode()->connectWidget(L"widget:caption", plasma::WIDGET_EVENT_TEXT_CHANGED, this,
                                         &WidgetModule::editCaption, true);
    getPanel()->getNode()->connectWidget(L"button:normal", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &WidgetModule::setButtonNormal, true);
    getPanel()->getNode()->connectWidget(L"button:check", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &WidgetModule::setButtonCheck, true);
    getPanel()->getNode()->connectWidget(L"button:radio", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &WidgetModule::setButtonRadio, true);
    getPanel()->getNode()->connectWidget(L"scrollbutton:left", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &WidgetModule::setScrollButtonLeft, true);
    getPanel()->getNode()->connectWidget(L"scrollbutton:right", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &WidgetModule::setScrollButtonRight, true);
    getPanel()->getNode()->connectWidget(L"scrollbutton:up", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &WidgetModule::setScrollButtonUp, true);
    getPanel()->getNode()->connectWidget(L"scrollbutton:down", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &WidgetModule::setScrollButtonDown, true);
    getPanel()->getNode()->connectWidget(L"scrollslider:horizontal", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &WidgetModule::setScrollSliderHorizontal, true);
    getPanel()->getNode()->connectWidget(L"scrollslider:vertical", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &WidgetModule::setScrollSliderVertical, true);
    createZoomSequences();
}

void WidgetModule::clearCurrentNode()
{
    m_widget = 0;
    m_button = 0;
    m_scrollButton = 0;
    m_scrollSlider = 0;
}

void WidgetModule::currentNodeChanged()
{
    if (m_workspace->getOpenDesignSheet() && m_workspace->getOpenDesignSheet()->m_currentNode) {
        plasma::Node *node = m_workspace->getOpenDesignSheet()->m_currentNode;
        if (node && node->widget()) {
            m_widget = node->widget();
            if (dynamic_cast<plasma::Button *>(node->widget()))
                m_button = dynamic_cast<plasma::Button *>(node->widget());
            if (dynamic_cast<plasma::ScrollButton *>(node->widget()))
                m_scrollButton = dynamic_cast<plasma::ScrollButton *>(node->widget());
            if (dynamic_cast<plasma::ScrollSlider *>(node->widget()))
                m_scrollSlider = dynamic_cast<plasma::ScrollSlider *>(node->widget());
        }
    }
}

void WidgetModule::updatePanel()
{
    if (m_widget) {
        getPanel()->getNode()->setEnabled(L"widget:caption", true, true);
        getPanel()->getNode()->setEnabled(L"widget:horizontal:left", true, true);
        getPanel()->getNode()->setEnabled(L"widget:horizontal:center", true, true);
        getPanel()->getNode()->setEnabled(L"widget:horizontal:right", true, true);
        getPanel()->getNode()->setEnabled(L"widget:vertical:top", true, true);
        getPanel()->getNode()->setEnabled(L"widget:vertical:center", true, true);
        getPanel()->getNode()->setEnabled(L"widget:vertical:bottom", true, true);
        getPanel()->getNode()->setEnabled(L"widget:adapt:width", true, true);
        getPanel()->getNode()->setEnabled(L"widget:adapt:height", true, true);
        getPanel()->getNode()->setEnabled(L"widget:scroll:horizontally", true, true);
        getPanel()->getNode()->setEnabled(L"widget:scroll:vertically", true, true);
        getPanel()->getNode()->setEnabled(L"widget:popup", true, true);
        getPanel()->getNode()->setEnabled(L"widget:rendersurface", true, true);
        getPanel()->getNode()->setEnabled(L"widget:resizable", true, true);
        getPanel()->getNode()->setText(L"widget:caption", m_widget->m_caption, true);

        // A widget drawn by a generic shape shows the shape's source instead.
        plasma::GenericShape *shape = dynamic_cast<plasma::GenericShape *>(m_widget->getNode()->getShape());
        if (shape)
            getPanel()->getNode()->setText(L"widget:caption", shape->m_source, true);
        switch (m_widget->m_horizontalAlignment) {
        case plasma::ALIGN_LEFT:
            m_panel->setChecked(L"widget:horizontal:left", true, false, true);
            break;
        case plasma::ALIGN_CENTER:
            m_panel->setChecked(L"widget:horizontal:center", true, false, true);
            break;
        case plasma::ALIGN_RIGHT:
            m_panel->setChecked(L"widget:horizontal:right", true, false, true);
            break;
        }
        switch (m_widget->m_verticalAlignment) {
        case plasma::ALIGN_TOP:
            m_panel->setChecked(L"widget:vertical:top", true, false, true);
            break;
        case plasma::ALIGN_MIDDLE:
            m_panel->setChecked(L"widget:vertical:center", true, false, true);
            break;
        case plasma::ALIGN_BOTTOM:
            m_panel->setChecked(L"widget:vertical:bottom", true, false, true);
            break;
        }
        getPanel()->getNode()->setChecked(L"widget:adapt:width", m_widget->stretchesX(), false, true);
        getPanel()->getNode()->setChecked(L"widget:adapt:height", m_widget->stretchesY(), false, true);
        getPanel()->getNode()->setChecked(L"widget:scroll:horizontally", m_widget->isMovableX(), false, true);
        getPanel()->getNode()->setChecked(L"widget:scroll:vertically", m_widget->isMovableY(), false, true);
        getPanel()->getNode()->setChecked(L"widget:popup", (m_widget->m_flags & (1 << plasma::WIDGET_POPUP)) != 0,
                                          false, true);
        getPanel()->getNode()->setChecked(L"widget:rendersurface", m_widget->rendersToSurface(), false, true);
        getPanel()->getNode()->setChecked(L"widget:resizable", m_widget->isResizable(), false, true);
    } else {
        getPanel()->getNode()->setText(L"widget:caption", L"", true);
        getPanel()->getNode()->setEnabled(L"widget:caption", false, true);
        getPanel()->getNode()->setEnabled(L"widget:horizontal:left", false, true);
        getPanel()->getNode()->setEnabled(L"widget:horizontal:center", false, true);
        getPanel()->getNode()->setEnabled(L"widget:horizontal:right", false, true);
        getPanel()->getNode()->setEnabled(L"widget:vertical:top", false, true);
        getPanel()->getNode()->setEnabled(L"widget:vertical:center", false, true);
        getPanel()->getNode()->setEnabled(L"widget:vertical:bottom", false, true);
        getPanel()->getNode()->setEnabled(L"widget:adapt:width", false, true);
        getPanel()->getNode()->setEnabled(L"widget:adapt:height", false, true);
        getPanel()->getNode()->setEnabled(L"widget:scroll:horizontally", false, true);
        getPanel()->getNode()->setEnabled(L"widget:scroll:vertically", false, true);
        getPanel()->getNode()->setEnabled(L"widget:popup", false, true);
        getPanel()->getNode()->setEnabled(L"widget:rendersurface", false, true);
        getPanel()->getNode()->setEnabled(L"widget:resizable", false, true);
    }
    if (m_button) {
        getPanel()->getNode()->setEnabled(L"button:normal", true, true);
        getPanel()->getNode()->setEnabled(L"button:check", true, true);
        getPanel()->getNode()->setEnabled(L"button:radio", true, true);
        switch (m_button->m_type) {
        case plasma::Button::push:
            m_panel->setChecked(L"button:normal", true, false, true);
            break;
        case plasma::Button::Toggle:
            m_panel->setChecked(L"button:check", true, false, true);
            break;
        case plasma::Button::Radio:
            m_panel->setChecked(L"button:radio", true, false, true);
            break;
        }
    } else {
        getPanel()->getNode()->setEnabled(L"button:normal", false, true);
        getPanel()->getNode()->setEnabled(L"button:check", false, true);
        getPanel()->getNode()->setEnabled(L"button:radio", false, true);
    }
    if (m_scrollButton) {
        getPanel()->getNode()->setEnabled(L"scrollbutton:left", true, true);
        getPanel()->getNode()->setEnabled(L"scrollbutton:right", true, true);
        getPanel()->getNode()->setEnabled(L"scrollbutton:up", true, true);
        getPanel()->getNode()->setEnabled(L"scrollbutton:down", true, true);
        switch (m_scrollButton->m_direction) {
        case plasma::SCROLL_LEFT:
            m_panel->setChecked(L"scrollbutton:left", true, false, true);
            break;
        case plasma::SCROLL_RIGHT:
            m_panel->setChecked(L"scrollbutton:right", true, false, true);
            break;
        case plasma::SCROLL_UP:
            m_panel->setChecked(L"scrollbutton:up", true, false, true);
            break;
        case plasma::SCROLL_DOWN:
            m_panel->setChecked(L"scrollbutton:down", true, false, true);
            break;
        }
    } else {
        getPanel()->getNode()->setEnabled(L"scrollbutton:left", false, true);
        getPanel()->getNode()->setEnabled(L"scrollbutton:right", false, true);
        getPanel()->getNode()->setEnabled(L"scrollbutton:up", false, true);
        getPanel()->getNode()->setEnabled(L"scrollbutton:down", false, true);
    }
    if (m_scrollSlider) {
        getPanel()->getNode()->setEnabled(L"scrollslider:horizontal", true, true);
        getPanel()->getNode()->setEnabled(L"scrollslider:vertical", true, true);
        switch (m_scrollSlider->m_direction) {
        case plasma::SLIDER_HORIZONTAL:
            m_panel->setChecked(L"scrollslider:horizontal", true, false, true);
            break;
        case plasma::SLIDER_VERTICAL:
            m_panel->setChecked(L"scrollslider:vertical", true, false, true);
            break;
        }
    } else {
        getPanel()->getNode()->setEnabled(L"scrollslider:horizontal", false, true);
        getPanel()->getNode()->setEnabled(L"scrollslider:vertical", false, true);
    }
}

void WidgetModule::showSelectedValues()
{
    bool alignLeft = false;
    bool alignRight = false;
    bool alignTop = false;
    bool alignBottom = false;
    bool adaptWidth = false;
    bool adaptHeight = false;
    bool centerHorizontally = false;
    bool centerVertically = false;
    bool scrollHorizontally = false;
    bool scrollVertically = false;
    bool popup = false;
    bool renderSurface = false;
    bool resizable = false;
    std::wstring caption;
    bool buttonNormal = false;
    bool buttonCheck = false;
    bool buttonRadio = false;
    bool scrollButtonLeft = false;
    bool scrollButtonRight = false;
    bool scrollButtonUp = false;
    bool scrollButtonDown = false;
    bool scrollSliderHorizontal = false;
    bool scrollSliderVertical = false;
    std::list<plasma::Node *> nodes;
    m_workspace->getSelectedNodes(nodes);
    for (std::list<plasma::Node *>::iterator it = nodes.begin(); it != nodes.end(); ++it) {
        if ((*it)->widget()) {
            if ((*it)->widget()->m_caption.size())
                caption = (*it)->widget()->m_caption;
            switch ((*it)->widget()->m_horizontalAlignment) {
            case plasma::ALIGN_LEFT:
                alignLeft = true;
                break;
            case plasma::ALIGN_RIGHT:
                alignRight = true;
                break;
            case plasma::ALIGN_CENTER:
                centerHorizontally = true;
                break;
            }
            switch ((*it)->widget()->m_verticalAlignment) {
            case plasma::ALIGN_TOP:
                alignTop = true;
                break;
            case plasma::ALIGN_BOTTOM:
                alignBottom = true;
                break;
            case plasma::ALIGN_MIDDLE:
                centerVertically = true;
                break;
            }
            if ((*it)->widget()->isMovableY())
                scrollVertically = true;
            if ((*it)->widget()->isMovableX())
                scrollHorizontally = true;
            if ((*it)->widget()->isPopUpWidget())
                popup = true;
            if ((*it)->widget()->rendersToSurface())
                renderSurface = true;
            if ((*it)->widget()->isResizable())
                resizable = true;
            if ((*it)->widget()->stretchesX())
                adaptWidth = true;
            if ((*it)->widget()->stretchesY())
                adaptHeight = true;
            if (dynamic_cast<plasma::ScrollButton *>((*it)->widget())) {
                switch (dynamic_cast<plasma::ScrollButton *>((*it)->widget())->m_direction) {
                case plasma::SCROLL_LEFT:
                    scrollButtonLeft = true;
                    break;
                case plasma::SCROLL_RIGHT:
                    scrollButtonRight = true;
                    break;
                case plasma::SCROLL_UP:
                    scrollButtonUp = true;
                    break;
                case plasma::SCROLL_DOWN:
                    scrollButtonDown = true;
                    break;
                }
            }
            if (dynamic_cast<plasma::ScrollSlider *>((*it)->widget())) {
                if (dynamic_cast<plasma::ScrollSlider *>((*it)->widget())->m_direction == plasma::SLIDER_HORIZONTAL)
                    scrollSliderHorizontal = true;
                else
                    scrollSliderVertical = true;
            }
            if (dynamic_cast<plasma::Button *>((*it)->widget())) {
                switch (dynamic_cast<plasma::Button *>((*it)->widget())->m_type) {
                case plasma::Button::push:
                    buttonNormal = true;
                    break;
                case plasma::Button::Toggle:
                    buttonCheck = true;
                    break;
                case plasma::Button::Radio:
                    buttonRadio = true;
                    break;
                }
            }

            // The code also tests for edit boxes and list widgets and does nothing with them.
            if (dynamic_cast<plasma::Edit *>((*it)->widget())) {
            }
            if (dynamic_cast<plasma::ListWidget *>((*it)->widget())) {
            }
        }
    }
    m_panel->setText(L"widget:caption", caption, true);
    m_panel->setChecked(L"widget:align:left", alignLeft, true, true);
    m_panel->setChecked(L"widget:center:horizontally", centerHorizontally, true, true);
    m_panel->setChecked(L"widget:align:right", alignRight, true, true);
    m_panel->setChecked(L"widget:align:top", alignTop, true, true);
    m_panel->setChecked(L"widget:center:vertically", centerVertically, true, true);
    m_panel->setChecked(L"widget:align:bottom", alignBottom, true, true);
    m_panel->setChecked(L"widget:adapt:width", adaptWidth, true, true);
    m_panel->setChecked(L"widget:adapt:height", adaptHeight, true, true);
    m_panel->setChecked(L"widget:scroll:horizontally", scrollHorizontally, false, true);
    m_panel->setChecked(L"widget:scroll:vertically", scrollVertically, false, true);
    m_panel->setChecked(L"widget:popup", popup, false, true);
    m_panel->setChecked(L"widget:rendersurface", renderSurface, false, true);
    m_panel->setChecked(L"widget:resizable", resizable, false, true);
    m_panel->setChecked(L"button:normal", buttonNormal, true, true);
    m_panel->setChecked(L"button:check", buttonCheck, true, true);
    m_panel->setChecked(L"button:radio", buttonRadio, true, true);
    m_panel->setChecked(L"scrollbutton:left", scrollButtonLeft, true, true);
    m_panel->setChecked(L"scrollbutton:right", scrollButtonRight, true, true);
    m_panel->setChecked(L"scrollbutton:up", scrollButtonUp, true, true);
    m_panel->setChecked(L"scrollbutton:down", scrollButtonDown, true, true);
    m_panel->setChecked(L"scrollslider:horizontal", scrollSliderHorizontal, true, true);
    m_panel->setChecked(L"scrollslider:vertical", scrollSliderVertical, true, true);
}

void WidgetModule::bindWidget()
{
    if (m_workspace->getOpenDesignSheet()) {
        DesignSheet *sheet = m_workspace->getOpenDesignSheet();
        std::map<plasma::Node *, NodeInterface *>::iterator it;
        if (sheet->isBusy())
            it = sheet->m_sheetNodes.end();
        else
            it = sheet->m_sheetNodes.begin();
        for (; it != m_workspace->getOpenDesignSheet()->m_sheetNodes.end(); ++it) {
            if (it->first->isShown() && it->second->m_selected && it->first->widget()) {
                it->second->beginModifyRecursively();
                it->second->endModifyRecursively();
                plasma::Vector<2, float> oldBindPos = it->first->widget()->getBindPos();
                plasma::Vector<2, float> oldBindSize = it->first->widget()->getBindSize();
                plasma::Vector<2, float> oldInnerBindPos = it->first->widget()->getInnerBindPos();
                plasma::Vector<2, float> oldInnerBindSize = it->first->widget()->getInnerBindSize();
                plasma::Matrix<float> oldBindMatrix = it->first->widget()->m_bindMatrix;

                // The widget takes the shape's bounds in the sheet's coordinates, its middle third as the scaled area.
                plasma::Vector<2, float> minimum;
                plasma::Vector<2, float> maximum;
                it->first->getShapeBounds(minimum, maximum);
                minimum.transform(m_workspace->getOpenDesignSheet()->getRoot()->getInverseMatrix());
                maximum.transform(m_workspace->getOpenDesignSheet()->getRoot()->getInverseMatrix());
                plasma::Matrix<float> matrix =
                    m_workspace->getOpenDesignSheet()->getRoot()->getInverseMatrix() * it->first->getMatrix();
                it->first->widget()->setGeometry(minimum, maximum - minimum,
                                                 minimum * 0.6666667f + maximum * 0.33333334f,
                                                 (maximum - minimum) * 0.33333334f, matrix);
                plasma::Widget *widget = it->first->widget();
                m_workspace->getOpenDesignSheet()->addOperation(new WidgetBindOperation(
                    m_workspace->getOpenDesignSheet(), "bind widget", widget, oldBindPos, oldBindSize, oldInnerBindPos,
                    oldInnerBindSize, oldBindMatrix, widget->getBindPos(), widget->getBindSize(),
                    widget->getInnerBindPos(), widget->getInnerBindSize(), widget->m_bindMatrix));
                it->second->beginModifyRecursively();
                it->second->endModifyRecursively();
                m_workspace->m_engine.resolveSources(it->first);
            }
        }
        m_workspace->getOpenDesignSheet()->commit();
    }
}

void WidgetModule::setButtonNormal()
{
    if (m_workspace->getOpenDesignSheet()) {
        // Starts with the first sheet node, or the end while the sheet is busy.
        DesignSheet *sheet = m_workspace->getOpenDesignSheet();
        std::map<plasma::Node *, NodeInterface *>::iterator it;
        if (sheet->isBusy())
            it = sheet->m_sheetNodes.end();
        else
            it = sheet->m_sheetNodes.begin();
        for (; it != m_workspace->getOpenDesignSheet()->m_sheetNodes.end(); ++it) {
            if (it->first->isShown() && it->second->m_selected) {
                if (it->first->m_widget) {
                    plasma::Button *button = dynamic_cast<plasma::Button *>(it->first->m_widget);
                    if (button)
                        button->m_type = plasma::Button::push;
                }
            }
        }
    }
}

void WidgetModule::setButtonCheck()
{
    if (m_workspace->getOpenDesignSheet()) {
        // Starts with the first sheet node, or the end while the sheet is busy.
        DesignSheet *sheet = m_workspace->getOpenDesignSheet();
        std::map<plasma::Node *, NodeInterface *>::iterator it;
        if (sheet->isBusy())
            it = sheet->m_sheetNodes.end();
        else
            it = sheet->m_sheetNodes.begin();
        for (; it != m_workspace->getOpenDesignSheet()->m_sheetNodes.end(); ++it) {
            if (it->first->isShown() && it->second->m_selected) {
                if (it->first->m_widget) {
                    plasma::Button *button = dynamic_cast<plasma::Button *>(it->first->m_widget);
                    if (button)
                        button->m_type = plasma::Button::Toggle;
                }
            }
        }
    }
}

void WidgetModule::setButtonRadio()
{
    if (m_workspace->getOpenDesignSheet()) {
        // Starts with the first sheet node, or the end while the sheet is busy.
        DesignSheet *sheet = m_workspace->getOpenDesignSheet();
        std::map<plasma::Node *, NodeInterface *>::iterator it;
        if (sheet->isBusy())
            it = sheet->m_sheetNodes.end();
        else
            it = sheet->m_sheetNodes.begin();
        for (; it != m_workspace->getOpenDesignSheet()->m_sheetNodes.end(); ++it) {
            if (it->first->isShown() && it->second->m_selected) {
                if (it->first->m_widget) {
                    plasma::Button *button = dynamic_cast<plasma::Button *>(it->first->m_widget);
                    if (button)
                        button->m_type = plasma::Button::Radio;
                }
            }
        }
    }
}

void WidgetModule::alignLeft()
{
    if (m_workspace->getOpenDesignSheet()) {
        // Starts with the first sheet node, or the end while the sheet is busy.
        DesignSheet *sheet = m_workspace->getOpenDesignSheet();
        std::map<plasma::Node *, NodeInterface *>::iterator it;
        if (sheet->isBusy())
            it = sheet->m_sheetNodes.end();
        else
            it = sheet->m_sheetNodes.begin();
        for (; it != m_workspace->getOpenDesignSheet()->m_sheetNodes.end(); ++it) {
            if (it->first->isShown() && it->second->m_selected) {
                if (it->first->m_widget)
                    it->first->m_widget->m_horizontalAlignment = plasma::ALIGN_LEFT;
            }
        }
    }
}

void WidgetModule::alignRight()
{
    if (m_workspace->getOpenDesignSheet()) {
        // Starts with the first sheet node, or the end while the sheet is busy.
        DesignSheet *sheet = m_workspace->getOpenDesignSheet();
        std::map<plasma::Node *, NodeInterface *>::iterator it;
        if (sheet->isBusy())
            it = sheet->m_sheetNodes.end();
        else
            it = sheet->m_sheetNodes.begin();
        for (; it != m_workspace->getOpenDesignSheet()->m_sheetNodes.end(); ++it) {
            if (it->first->isShown() && it->second->m_selected) {
                if (it->first->m_widget)
                    it->first->m_widget->m_horizontalAlignment = plasma::ALIGN_RIGHT;
            }
        }
    }
}

void WidgetModule::centerHorizontally()
{
    if (m_workspace->getOpenDesignSheet()) {
        // Starts with the first sheet node, or the end while the sheet is busy.
        DesignSheet *sheet = m_workspace->getOpenDesignSheet();
        std::map<plasma::Node *, NodeInterface *>::iterator it;
        if (sheet->isBusy())
            it = sheet->m_sheetNodes.end();
        else
            it = sheet->m_sheetNodes.begin();
        for (; it != m_workspace->getOpenDesignSheet()->m_sheetNodes.end(); ++it) {
            if (it->first->isShown() && it->second->m_selected) {
                if (it->first->m_widget)
                    it->first->m_widget->m_horizontalAlignment = plasma::ALIGN_CENTER;
            }
        }
    }
}

void WidgetModule::alignTop()
{
    if (m_workspace->getOpenDesignSheet()) {
        // Starts with the first sheet node, or the end while the sheet is busy.
        DesignSheet *sheet = m_workspace->getOpenDesignSheet();
        std::map<plasma::Node *, NodeInterface *>::iterator it;
        if (sheet->isBusy())
            it = sheet->m_sheetNodes.end();
        else
            it = sheet->m_sheetNodes.begin();
        for (; it != m_workspace->getOpenDesignSheet()->m_sheetNodes.end(); ++it) {
            if (it->first->isShown() && it->second->m_selected) {
                if (it->first->m_widget)
                    it->first->m_widget->m_verticalAlignment = plasma::ALIGN_TOP;
            }
        }
    }
}

void WidgetModule::alignBottom()
{
    if (m_workspace->getOpenDesignSheet()) {
        // Starts with the first sheet node, or the end while the sheet is busy.
        DesignSheet *sheet = m_workspace->getOpenDesignSheet();
        std::map<plasma::Node *, NodeInterface *>::iterator it;
        if (sheet->isBusy())
            it = sheet->m_sheetNodes.end();
        else
            it = sheet->m_sheetNodes.begin();
        for (; it != m_workspace->getOpenDesignSheet()->m_sheetNodes.end(); ++it) {
            if (it->first->isShown() && it->second->m_selected) {
                if (it->first->m_widget)
                    it->first->m_widget->m_verticalAlignment = plasma::ALIGN_BOTTOM;
            }
        }
    }
}

void WidgetModule::centerVertically()
{
    if (m_workspace->getOpenDesignSheet()) {
        // Starts with the first sheet node, or the end while the sheet is busy.
        DesignSheet *sheet = m_workspace->getOpenDesignSheet();
        std::map<plasma::Node *, NodeInterface *>::iterator it;
        if (sheet->isBusy())
            it = sheet->m_sheetNodes.end();
        else
            it = sheet->m_sheetNodes.begin();
        for (; it != m_workspace->getOpenDesignSheet()->m_sheetNodes.end(); ++it) {
            if (it->first->isShown() && it->second->m_selected) {
                if (it->first->m_widget)
                    it->first->m_widget->m_verticalAlignment = plasma::ALIGN_MIDDLE;
            }
        }
    }
}

void WidgetModule::adaptWidth()
{
    bool checked = m_panel->isChecked(L"widget:adapt:width", true);
    if (m_workspace->getOpenDesignSheet()) {
        // Starts with the first sheet node, or the end while the sheet is busy.
        DesignSheet *sheet = m_workspace->getOpenDesignSheet();
        std::map<plasma::Node *, NodeInterface *>::iterator it;
        if (sheet->isBusy())
            it = sheet->m_sheetNodes.end();
        else
            it = sheet->m_sheetNodes.begin();
        for (; it != m_workspace->getOpenDesignSheet()->m_sheetNodes.end(); ++it) {
            if (it->first->isShown() && it->second->m_selected) {
                if (it->first->m_widget) {
                    if (checked)
                        it->first->m_widget->m_flags |= (1 << plasma::WIDGET_ADAPT_WIDTH);
                    else
                        it->first->m_widget->m_flags &= ~(1 << plasma::WIDGET_ADAPT_WIDTH);
                }
            }
        }
    }
}

void WidgetModule::adaptHeight()
{
    bool checked = m_panel->isChecked(L"widget:adapt:height", true);
    if (m_workspace->getOpenDesignSheet()) {
        // Starts with the first sheet node, or the end while the sheet is busy.
        DesignSheet *sheet = m_workspace->getOpenDesignSheet();
        std::map<plasma::Node *, NodeInterface *>::iterator it;
        if (sheet->isBusy())
            it = sheet->m_sheetNodes.end();
        else
            it = sheet->m_sheetNodes.begin();
        for (; it != m_workspace->getOpenDesignSheet()->m_sheetNodes.end(); ++it) {
            if (it->first->isShown() && it->second->m_selected) {
                if (it->first->m_widget) {
                    if (checked)
                        it->first->m_widget->m_flags |= (1 << plasma::WIDGET_ADAPT_HEIGHT);
                    else
                        it->first->m_widget->m_flags &= ~(1 << plasma::WIDGET_ADAPT_HEIGHT);
                }
            }
        }
    }
}

void WidgetModule::scrollVertically()
{
    bool checked = m_panel->isChecked(L"widget:scroll:vertically", true);
    if (m_workspace->getOpenDesignSheet()) {
        // Starts with the first sheet node, or the end while the sheet is busy.
        DesignSheet *sheet = m_workspace->getOpenDesignSheet();
        std::map<plasma::Node *, NodeInterface *>::iterator it;
        if (sheet->isBusy())
            it = sheet->m_sheetNodes.end();
        else
            it = sheet->m_sheetNodes.begin();
        for (; it != m_workspace->getOpenDesignSheet()->m_sheetNodes.end(); ++it) {
            if (it->first->isShown() && it->second->m_selected) {
                if (it->first->m_widget) {
                    if (checked)
                        it->first->m_widget->m_flags |= (1 << plasma::WIDGET_SCROLL_VERTICALLY);
                    else
                        it->first->m_widget->m_flags &= ~(1 << plasma::WIDGET_SCROLL_VERTICALLY);
                }
            }
        }
    }
}

void WidgetModule::scrollHorizontally()
{
    bool checked = m_panel->isChecked(L"widget:scroll:horizontally", true);
    if (m_workspace->getOpenDesignSheet()) {
        // Starts with the first sheet node, or the end while the sheet is busy.
        DesignSheet *sheet = m_workspace->getOpenDesignSheet();
        std::map<plasma::Node *, NodeInterface *>::iterator it;
        if (sheet->isBusy())
            it = sheet->m_sheetNodes.end();
        else
            it = sheet->m_sheetNodes.begin();
        for (; it != m_workspace->getOpenDesignSheet()->m_sheetNodes.end(); ++it) {
            if (it->first->isShown() && it->second->m_selected) {
                if (it->first->m_widget) {
                    if (checked)
                        it->first->m_widget->m_flags |= (1 << plasma::WIDGET_SCROLL_HORIZONTALLY);
                    else
                        it->first->m_widget->m_flags &= ~(1 << plasma::WIDGET_SCROLL_HORIZONTALLY);
                }
            }
        }
    }
}

void WidgetModule::setPopup()
{
    bool checked = m_panel->isChecked(L"widget:popup", true);
    if (m_workspace->getOpenDesignSheet()) {
        // Starts with the first sheet node, or the end while the sheet is busy.
        DesignSheet *sheet = m_workspace->getOpenDesignSheet();
        std::map<plasma::Node *, NodeInterface *>::iterator it;
        if (sheet->isBusy())
            it = sheet->m_sheetNodes.end();
        else
            it = sheet->m_sheetNodes.begin();
        for (; it != m_workspace->getOpenDesignSheet()->m_sheetNodes.end(); ++it) {
            if (it->first->isShown() && it->second->m_selected) {
                if (it->first->m_widget) {
                    if (checked)
                        it->first->m_widget->m_flags |= (1 << plasma::WIDGET_POPUP);
                    else
                        it->first->m_widget->m_flags &= ~(1 << plasma::WIDGET_POPUP);
                }
            }
        }
    }
}

void WidgetModule::setRenderSurface()
{
    bool checked = m_panel->isChecked(L"widget:rendersurface", true);
    if (m_workspace->getOpenDesignSheet()) {
        // Starts with the first sheet node, or the end while the sheet is busy.
        DesignSheet *sheet = m_workspace->getOpenDesignSheet();
        std::map<plasma::Node *, NodeInterface *>::iterator it;
        if (sheet->isBusy())
            it = sheet->m_sheetNodes.end();
        else
            it = sheet->m_sheetNodes.begin();
        for (; it != m_workspace->getOpenDesignSheet()->m_sheetNodes.end(); ++it) {
            if (it->first->isShown() && it->second->m_selected) {
                if (it->first->m_widget) {
                    if (checked)
                        it->first->m_widget->m_flags |= (1 << plasma::WIDGET_RENDER_SURFACE);
                    else
                        it->first->m_widget->m_flags &= ~(1 << plasma::WIDGET_RENDER_SURFACE);
                }
            }
        }
    }
}

void WidgetModule::setResizable()
{
    bool checked = m_panel->isChecked(L"widget:resizable", true);
    if (m_workspace->getOpenDesignSheet()) {
        // Starts with the first sheet node, or the end while the sheet is busy.
        DesignSheet *sheet = m_workspace->getOpenDesignSheet();
        std::map<plasma::Node *, NodeInterface *>::iterator it;
        if (sheet->isBusy())
            it = sheet->m_sheetNodes.end();
        else
            it = sheet->m_sheetNodes.begin();
        for (; it != m_workspace->getOpenDesignSheet()->m_sheetNodes.end(); ++it) {
            if (it->first->isShown() && it->second->m_selected) {
                if (it->first->m_widget) {
                    if (checked)
                        it->first->m_widget->m_flags |= (1 << plasma::WIDGET_RESIZABLE);
                    else
                        it->first->m_widget->m_flags &= ~(1 << plasma::WIDGET_RESIZABLE);
                }
            }
        }
    }
}

void WidgetModule::setScrollButtonLeft()
{
    if (m_workspace->getOpenDesignSheet()) {
        // Starts with the first sheet node, or the end while the sheet is busy.
        DesignSheet *sheet = m_workspace->getOpenDesignSheet();
        std::map<plasma::Node *, NodeInterface *>::iterator it;
        if (sheet->isBusy())
            it = sheet->m_sheetNodes.end();
        else
            it = sheet->m_sheetNodes.begin();
        for (; it != m_workspace->getOpenDesignSheet()->m_sheetNodes.end(); ++it) {
            if (it->first->isShown() && it->second->m_selected) {
                plasma::ScrollButton *scrollButton = dynamic_cast<plasma::ScrollButton *>(it->first->m_widget);
                if (scrollButton)
                    scrollButton->m_direction = plasma::SCROLL_LEFT;
            }
        }
    }
}

void WidgetModule::setScrollButtonRight()
{
    if (m_workspace->getOpenDesignSheet()) {
        // Starts with the first sheet node, or the end while the sheet is busy.
        DesignSheet *sheet = m_workspace->getOpenDesignSheet();
        std::map<plasma::Node *, NodeInterface *>::iterator it;
        if (sheet->isBusy())
            it = sheet->m_sheetNodes.end();
        else
            it = sheet->m_sheetNodes.begin();
        for (; it != m_workspace->getOpenDesignSheet()->m_sheetNodes.end(); ++it) {
            if (it->first->isShown() && it->second->m_selected) {
                plasma::ScrollButton *scrollButton = dynamic_cast<plasma::ScrollButton *>(it->first->m_widget);
                if (scrollButton)
                    scrollButton->m_direction = plasma::SCROLL_RIGHT;
            }
        }
    }
}

void WidgetModule::setScrollButtonUp()
{
    if (m_workspace->getOpenDesignSheet()) {
        // Starts with the first sheet node, or the end while the sheet is busy.
        DesignSheet *sheet = m_workspace->getOpenDesignSheet();
        std::map<plasma::Node *, NodeInterface *>::iterator it;
        if (sheet->isBusy())
            it = sheet->m_sheetNodes.end();
        else
            it = sheet->m_sheetNodes.begin();
        for (; it != m_workspace->getOpenDesignSheet()->m_sheetNodes.end(); ++it) {
            if (it->first->isShown() && it->second->m_selected) {
                plasma::ScrollButton *scrollButton = dynamic_cast<plasma::ScrollButton *>(it->first->m_widget);
                if (scrollButton)
                    scrollButton->m_direction = plasma::SCROLL_UP;
            }
        }
    }
}

void WidgetModule::setScrollButtonDown()
{
    if (m_workspace->getOpenDesignSheet()) {
        // Starts with the first sheet node, or the end while the sheet is busy.
        DesignSheet *sheet = m_workspace->getOpenDesignSheet();
        std::map<plasma::Node *, NodeInterface *>::iterator it;
        if (sheet->isBusy())
            it = sheet->m_sheetNodes.end();
        else
            it = sheet->m_sheetNodes.begin();
        for (; it != m_workspace->getOpenDesignSheet()->m_sheetNodes.end(); ++it) {
            if (it->first->isShown() && it->second->m_selected) {
                plasma::ScrollButton *scrollButton = dynamic_cast<plasma::ScrollButton *>(it->first->m_widget);
                if (scrollButton)
                    scrollButton->m_direction = plasma::SCROLL_DOWN;
            }
        }
    }
}

void WidgetModule::setScrollSliderHorizontal()
{
    if (m_workspace->getOpenDesignSheet()) {
        // Starts with the first sheet node, or the end while the sheet is busy.
        DesignSheet *sheet = m_workspace->getOpenDesignSheet();
        std::map<plasma::Node *, NodeInterface *>::iterator it;
        if (sheet->isBusy())
            it = sheet->m_sheetNodes.end();
        else
            it = sheet->m_sheetNodes.begin();
        for (; it != m_workspace->getOpenDesignSheet()->m_sheetNodes.end(); ++it) {
            if (it->first->isShown() && it->second->m_selected) {
                plasma::ScrollSlider *scrollSlider = dynamic_cast<plasma::ScrollSlider *>(it->first->m_widget);
                if (scrollSlider)
                    scrollSlider->m_direction = plasma::SLIDER_HORIZONTAL;
            }
        }
    }
}

void WidgetModule::setScrollSliderVertical()
{
    if (m_workspace->getOpenDesignSheet()) {
        // Starts with the first sheet node, or the end while the sheet is busy.
        DesignSheet *sheet = m_workspace->getOpenDesignSheet();
        std::map<plasma::Node *, NodeInterface *>::iterator it;
        if (sheet->isBusy())
            it = sheet->m_sheetNodes.end();
        else
            it = sheet->m_sheetNodes.begin();
        for (; it != m_workspace->getOpenDesignSheet()->m_sheetNodes.end(); ++it) {
            if (it->first->isShown() && it->second->m_selected) {
                plasma::ScrollSlider *scrollSlider = dynamic_cast<plasma::ScrollSlider *>(it->first->m_widget);
                if (scrollSlider)
                    scrollSlider->m_direction = plasma::SLIDER_VERTICAL;
            }
        }
    }
}

void WidgetModule::editCaption()
{
    std::wstring caption = getPanel()->getNode()->getText(L"widget:caption:edit", true);
    if (m_workspace->getOpenDesignSheet()) {
        // Starts with the first sheet node, or the end while the sheet is busy.
        DesignSheet *sheet = m_workspace->getOpenDesignSheet();
        std::map<plasma::Node *, NodeInterface *>::iterator it;
        if (sheet->isBusy())
            it = sheet->m_sheetNodes.end();
        else
            it = sheet->m_sheetNodes.begin();
        for (; it != m_workspace->getOpenDesignSheet()->m_sheetNodes.end(); ++it) {
            if (it->first->isShown() && it->second->m_selected) {
                if (it->first->m_widget)
                    it->first->m_widget->setCaption(caption, true);
            }
        }
    }
}
