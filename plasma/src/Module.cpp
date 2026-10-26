#include "Module.h"
#include "ColorModule.h"
#include "plasma/Display.h"
#include "plasma/Transformation.h"
#include "plasma/GenericWidget.h"
#include "plasma/RenderSurface.h"

static const int ZoomAnimationKeyTime = 800;

// Layout positions, extents and spacing use display units; colour tuples style the overlays.
Module::Module()
{
    m_workspace = &g_workspace;
    m_margin.m_elements[0] = 10;
    m_margin.m_elements[1] = 30;
    m_orientation = ORIENTATION_VERTICAL;
    m_fade = FADE_IDLE;
    m_panel = 0;
    m_cursor = m_margin;
    m_extent = m_margin;
}

plasma::Vector<2, int> Module::addPanelAt(const std::list<ModuleWidgetItem> &items, const std::wstring &name, int x,
                                          int y)
{
    int width = 0;
    int height = 0;
    for (std::list<ModuleWidgetItem>::const_iterator it = items.begin(); it != items.end(); ++it) {
        if (it->m_left + it->m_width > width)
            width = it->m_left + it->m_width;
        if (it->m_top + it->m_height > height)
            height = it->m_top + it->m_height;
    }
    plasma::GenericWidget *panel = m_workspace->m_engine.createGenericWidget(
        name, (float)x, (float)y, (float)width, (float)height, getPanel()->getNode(), L"panel");
    m_workspace->m_engine.resolveSources(panel->m_node);
    plasma::Node *background = panel->m_node->findNode(name + L":background");
    if (!background)
        return plasma::Vector<2, int>(0, 0);
    for (std::list<ModuleWidgetItem>::const_iterator it = items.begin(); it != items.end(); ++it) {
        plasma::GenericWidget *widget =
            m_workspace->m_engine.createGenericWidget(it->m_name, (float)it->m_left, (float)it->m_top,
                                                      (float)it->m_width, (float)it->m_height, background, it->m_shape);
        widget->setCaption(it->m_caption, true);
    }
    m_workspace->m_engine.resolveSources(background);
    m_panel->getNode()->initializeWidgets();
    return plasma::Vector<2, int>(x + width, y + height);
}

// Layout positions, extents and spacing use display units; colour tuples style the overlays.
plasma::Vector<2, int> Module::addUpperPanel(const std::wstring &name, const std::list<ModuleWidgetItem> &items, int x)
{
    int width = 0;
    int height = 0;
    for (std::list<ModuleWidgetItem>::const_iterator it = items.begin(); it != items.end(); ++it) {
        if (it->m_left + it->m_width > width)
            width = it->m_left + it->m_width;
        if (it->m_top + it->m_height > height)
            height = it->m_top + it->m_height;
    }
    plasma::GenericWidget *panel = m_workspace->m_engine.createGenericWidget(
        name, (float)x, 5.0f, (float)width, (float)height, getPanel()->getNode(), L"upperpanel");
    m_workspace->m_engine.resolveSources(panel->m_node);
    plasma::Node *background = panel->m_node->findNode(name + L":background");
    if (!background)
        return plasma::Vector<2, int>(0, 0);
    for (std::list<ModuleWidgetItem>::const_iterator it = items.begin(); it != items.end(); ++it) {
        plasma::GenericWidget *widget =
            m_workspace->m_engine.createGenericWidget(it->m_name, (float)it->m_left, (float)it->m_top,
                                                      (float)it->m_width, (float)it->m_height, background, it->m_shape);
        widget->setCaption(it->m_caption, true);
    }
    m_workspace->m_engine.resolveSources(background);
    m_panel->getNode()->initializeWidgets();
    return plasma::Vector<2, int>(x + width, height + 5);
}

// Layout positions, extents and spacing use display units; colour tuples style the overlays.
plasma::Vector<2, int> Module::addLeftPanel(const std::wstring &name, const std::list<ModuleWidgetItem> &items)
{
    int width = 0;
    int height = 0;
    for (std::list<ModuleWidgetItem>::const_iterator it = items.begin(); it != items.end(); ++it) {
        if (it->m_left + it->m_width > width)
            width = it->m_left + it->m_width;
        if (it->m_top + it->m_height > height)
            height = it->m_top + it->m_height;
    }
    plasma::GenericWidget *panel = m_workspace->getEngine()->createGenericWidget(
        name, 5.0f, 313.0f, (float)width, (float)height, getPanel()->getNode(), L"leftpanel");
    m_workspace->getEngine()->resolveSources(panel->m_node);
    plasma::Node *background = panel->m_node->findNode(name + L":background");
    if (!background)
        return plasma::Vector<2, int>(0, 0);
    for (std::list<ModuleWidgetItem>::const_iterator it = items.begin(); it != items.end(); ++it) {
        plasma::GenericWidget *widget = m_workspace->getEngine()->createGenericWidget(
            it->m_name, (float)it->m_left, (float)it->m_top, (float)it->m_width, (float)it->m_height, background,
            it->m_shape);
        widget->setCaption(it->m_caption, true);
    }
    m_workspace->getEngine()->resolveSources(background);
    m_panel->getNode()->initializeWidgets();
    return plasma::Vector<2, int>(width + 5, height + 313);
}

plasma::Vector<2, int> Module::addWidgetAt(const ModuleWidgetItem &item)
{
    plasma::GenericWidget *widget = m_workspace->m_engine.createGenericWidget(
        item.m_name, (float)item.m_left, (float)item.m_top, (float)item.m_width, (float)item.m_height,
        m_panel->getNode(), item.m_shape);
    widget->setCaption(item.m_caption, true);
    m_workspace->m_engine.resolveSources(widget->m_node);
    widget->m_node->initializeWidgets();
    return plasma::Vector<2, int>(item.m_left + item.m_width, item.m_top + item.m_height);
}

plasma::Vector<2, int> Module::addComboButtonAt(const std::wstring &caption, const std::wstring &name,
                                                const std::wstring &listName, int x, int y, int width, int height,
                                                int listWidth, int listHeight)
{
    plasma::GenericWidget *button = m_workspace->m_engine.createGenericWidget(
        name, (float)x, (float)y, (float)width, (float)height, getPanel()->getNode(), L"combobutton");
    button->setCaption(caption, true);
    plasma::GenericWidget *list = m_workspace->m_engine.createGenericWidget(
        listName, 0.0f, 0.0f, (float)listWidth, (float)listHeight, button->getNode(), L"listbox");
    list->m_flags |= (1 << plasma::WIDGET_POPUP);
    m_workspace->m_engine.resolveSources(button->getNode());
    button->getNode()->initializeWidgets();
    plasma::Vector<2, int> extent;
    extent[0] = x + width;
    extent[1] = y + height;
    return extent;
}

void Module::addPanel(const std::list<ModuleWidgetItem> &items, const std::wstring &name, int spacing)
{
    plasma::Vector<2, int> end = addPanelAt(items, name, m_cursor[0] + spacing, m_cursor[1]);
    m_cursor[0] = end[0];
    if (end[0] > m_extent[0])
        m_extent[0] = end[0];
    if (end[1] > m_extent[1])
        m_extent[1] = end[1];
}

void Module::addWidget(const std::wstring &caption, const std::wstring &name, const std::wstring &shape, int width,
                       int height, int spacing)
{
    const plasma::Vector<2, int> &end = addWidgetAt(ModuleWidgetItem(
        caption, name, shape, m_cursor.m_elements[0] + spacing, m_cursor.m_elements[1], width, height));
    m_cursor.m_elements[0] = end.m_elements[0];
    if (end.m_elements[0] > m_extent.m_elements[0])
        m_extent.m_elements[0] = end.m_elements[0];
    if (end.m_elements[1] > m_extent.m_elements[1])
        m_extent.m_elements[1] = end.m_elements[1];
}

void Module::addComboButton(const std::wstring &caption, const std::wstring &name, const std::wstring &listName,
                            int width, int height, int listWidth, int listHeight, int spacing)
{
    plasma::Vector<2, int> end = addComboButtonAt(caption, name, listName, m_cursor.m_elements[0] + spacing,
                                                  m_cursor.m_elements[1], width, height, listWidth, listHeight);
    m_cursor.m_elements[0] = end.m_elements[0];
    if (end.m_elements[0] > m_extent.m_elements[0])
        m_extent.m_elements[0] = end.m_elements[0];
    if (end.m_elements[1] > m_extent.m_elements[1])
        m_extent.m_elements[1] = end.m_elements[1];
}

void Module::newRow(int spacing)
{
    m_cursor.m_elements[0] = m_margin.m_elements[0];
    m_cursor.m_elements[1] = m_extent.m_elements[1] + spacing;
}

void Module::fitWidth(int margin)
{
    std::list<plasma::Widget *> widgets = m_panel->getChildWidgets();
    int right = 0;
    for (std::list<plasma::Widget *>::iterator it = widgets.begin(); it != widgets.end(); ++it) {
        if (!(*it)->m_node->isInternal()) {
            int edge = (int)((*it)->viewX() + (*it)->viewWidth());
            if (edge > right)
                right = edge;
        }
    }
    plasma::Widget *panel = m_panel;
    panel->resizeTo((float)(margin + right), panel->getViewHeight());
}

void Module::fitHeight(int margin)
{
    std::list<plasma::Widget *> widgets = m_panel->getChildWidgets();
    int bottom = 0;
    for (std::list<plasma::Widget *>::iterator it = widgets.begin(); it != widgets.end(); ++it) {
        if (!(*it)->m_node->isInternal()) {
            int edge = (int)((*it)->viewY() + (*it)->getViewHeight());
            if (edge > bottom)
                bottom = edge;
        }
    }
    plasma::Widget *panel = m_panel;
    panel->resizeTo(panel->viewWidth(), (float)(margin + bottom));
}

void Module::placeIcon(const std::wstring &nodeName, const std::wstring &iconName)
{
    plasma::Node *node = getPanel()->getNode()->findNode(nodeName);
    if (!node || iconName.empty())
        return;
    plasma::Node *source = m_workspace->findIcon(iconName);
    if (!source)
        return;
    plasma::Node *icon = source->instance(0);
    icon->setTransformation(icon->m_transformation->clone());
    icon->setDisplay(icon->m_display->clone());
    icon->setHitTestable(false);
    icon->m_flags |= (1 << plasma::NODE_IGNORES_POINTER);
    if (!icon->m_widget) {
        plasma::Widget *widget = m_workspace->m_engine.createWidget(icon);
        plasma::Matrix<float> identity;
        identity.identity();
        widget->setGeometry(plasma::Vector<2, float>(0.0f, 0.0f), plasma::Vector<2, float>(1.0f, 1.0f),
                            plasma::Vector<2, float>(0.0f, 0.0f), plasma::Vector<2, float>(1.0f, 1.0f), identity);
    }
    icon->initializeWidgets();
    icon->animate(0);
    node->m_transformation->attributesChanged(plasma::ATTRIBUTES_CHANGED_ALL);

    // Centre the icon's bounds on its origin, then move it to the centre of the node's bounds.
    icon->m_transformation->m_translation.value() -= (icon->getMinimum(0) + icon->getMaximum(0)) * 0.5f;
    icon->m_transformation->m_translation.value() += node->m_transformation->m_inverseMatrix.transform(
        (node->getMinimum(1 << plasma::BOUNDS_NO_CHILDREN) + node->getMaximum(1 << plasma::BOUNDS_NO_CHILDREN)) * 0.5f);
    icon->m_transformation->attributesChanged(plasma::ATTRIBUTES_CHANGED_ALL);
    node->addChild(icon);
    icon->animate(0);
}

// Module zoom keyframes use large 2x, small 0.75x and tiny 0.5x scale, with the normal key restoring the identity
// transform.
void Module::createZoomSequences()
{
    getPanel()->getNode()->transformation()->m_deformation.value().scale(2.0f, 2.0f, 1.0f);
    getPanel()->getNode()->transformation()->m_deformation.setKey(L"module:zoom:large", ZoomAnimationKeyTime, 0.0f);
    getPanel()->getNode()->transformation()->m_deformation.value().identity();
    getPanel()->getNode()->transformation()->m_deformation.value().scale(0.75f, 0.75f, 1.0f);
    getPanel()->getNode()->transformation()->m_deformation.setKey(L"module:zoom:small", ZoomAnimationKeyTime, 0.0f);
    getPanel()->getNode()->transformation()->m_deformation.value().identity();
    getPanel()->getNode()->transformation()->m_deformation.value().scale(0.5f, 0.5f, 1.0f);
    getPanel()->getNode()->transformation()->m_deformation.setKey(L"module:zoom:tiny", ZoomAnimationKeyTime, 0.0f);
    getPanel()->getNode()->transformation()->m_deformation.value().identity();
    getPanel()->getNode()->transformation()->m_deformation.setKey(L"module:zoom:normal", ZoomAnimationKeyTime, 0.0f);
    m_panel->connect(plasma::WIDGET_EVENT_POINTER_WHEEL, this, &Module::zoomWithWheel, true, false);
    m_zoom = ZOOM_NORMAL;
}

void Module::zoomWithWheel()
{
    const std::set<unsigned short> &keys = getPanel()->getEngine()->m_pressedKeys;
    if (keys.find(VK_CONTROL) == keys.end())
        return;
    if (getPanel()->getEngine()->m_wheelDelta > 0)
        m_zoom++;
    else
        m_zoom--;
    if (m_zoom < ZOOM_TINY) {
        m_zoom = ZOOM_TINY;
    } else if (m_zoom > ZOOM_LARGE) {
        m_zoom = ZOOM_LARGE;
    } else {
        getPanel()->getNode()->transformation()->stop();
        switch (m_zoom) {
        case ZOOM_TINY:
            getPanel()->getNode()->transformation()->play(L"module:zoom:tiny", 0);
            m_panel->setChecked(L"zoombar:zoom:tiny", true, false, true);
            break;
        case ZOOM_SMALL:
            getPanel()->getNode()->transformation()->play(L"module:zoom:small", 0);
            m_panel->setChecked(L"zoombar:zoom:small", true, false, true);
            break;
        case ZOOM_NORMAL:
            getPanel()->getNode()->transformation()->play(L"module:zoom:normal", 0);
            m_panel->setChecked(L"zoombar:zoom:normal", true, false, true);
            break;
        case ZOOM_LARGE:
            getPanel()->getNode()->transformation()->play(L"module:zoom:large", 0);
            m_panel->setChecked(L"zoombar:zoom:large", true, false, true);
            break;
        }
    }
}

void Module::zoomTiny()
{
    getPanel()->getNode()->m_transformation->stop();
    getPanel()->getNode()->m_transformation->play(L"module:zoom:tiny", 0);
}

void Module::zoomSmall()
{
    getPanel()->getNode()->m_transformation->stop();
    getPanel()->getNode()->m_transformation->play(L"module:zoom:small", 0);
}

void Module::zoomNormal()
{
    getPanel()->getNode()->m_transformation->stop();
    getPanel()->getNode()->m_transformation->play(L"module:zoom:normal", 0);
}

void Module::zoomLarge()
{
    getPanel()->getNode()->m_transformation->stop();
    getPanel()->getNode()->m_transformation->play(L"module:zoom:large", 0);
}

// Layout positions, extents and spacing use display units; colour tuples style the overlays.
void Module::addApplyBar(const std::wstring &prefix, int offset)
{
    std::list<ModuleWidgetItem> items;
    items.push_back(ModuleWidgetItem(L" ", prefix + L":apply", L"button2", 0, 0, 30, 23));
    items.push_back(ModuleWidgetItem(L" ", prefix + L":apply:auto", L"checkbutton2", 30, 0, 20, 23));
    addUpperPanel(L"", items, (int)(m_panel->viewWidth() * 0.5 - 56.0 + (float)offset));
    items.clear();
    items.push_back(ModuleWidgetItem(L" ", prefix + L":pick", L"button2", 0, 0, 30, 23));
    items.push_back(ModuleWidgetItem(L" ", prefix + L":pick:auto", L"checkbutton2", 30, 0, 20, 23));
    addUpperPanel(L"", items, (int)(m_panel->viewWidth() * 0.5 + 1.0 + (float)offset));
    placeIcon(prefix + L":apply", L"icon:apply");
    placeIcon(prefix + L":pick", L"icon:pick");
    placeIcon(prefix + L":apply:auto", L"icon:auto");
    placeIcon(prefix + L":pick:auto", L"icon:auto");
}

// Layout positions, extents and spacing use display units; colour tuples style the overlays.
void Module::addZoomBar()
{
    addWidgetAt(ModuleWidgetItem(L" ", L"zoombar", L"zoombar", (int)(getPanel()->viewWidth() - 45.0), 9, 16, 16));
    getPanel()->getNode()->connectWidget(L"zoombar:zoom:tiny", plasma::WIDGET_EVENT_POINTER_RELEASED, this,
                                         &Module::zoomTiny, true);
    getPanel()->getNode()->connectWidget(L"zoombar:zoom:small", plasma::WIDGET_EVENT_POINTER_RELEASED, this,
                                         &Module::zoomSmall, true);
    getPanel()->getNode()->connectWidget(L"zoombar:zoom:normal", plasma::WIDGET_EVENT_POINTER_RELEASED, this,
                                         &Module::zoomNormal, true);
    getPanel()->getNode()->connectWidget(L"zoombar:zoom:large", plasma::WIDGET_EVENT_POINTER_RELEASED, this,
                                         &Module::zoomLarge, true);
    m_panel->setChecked(L"zoombar:zoom:normal", true, false, true);
}

void Module::beginDrag(float start)
{
    m_dragPosition = 0.0f;
    m_dragStart = start;
    plasma::Vector<2, float> mouse;
    mouse = m_workspace->m_engine.m_pointer;
    m_dragStartMouse = mouse;
}

void Module::updateDrag()
{
    m_dragPosition += getDragMovement();
    plasma::Vector<2, float> movement = m_workspace->m_engine.m_pointer - m_workspace->m_engine.m_previousPointer;
    plasma::Vector<2, float> pointer = m_workspace->m_engine.m_pointer;
    plasma::Vector<2, float> mouse = pointer - movement;

    // The code converts the pointer's previous position but never stores it.
    plasma::Vector<2, int> start;
    for (int i = 0; i < 2; i++)
        start.m_elements[i] = (int)mouse.m_elements[i];
}

float Module::getDragMovement()
{
    if (m_orientation == ORIENTATION_HORIZONTAL)
        return m_workspace->m_engine.m_pointer.m_elements[0] - m_workspace->m_engine.m_previousPointer.m_elements[0];
    float movement =
        m_workspace->m_engine.m_pointer.m_elements[1] - m_workspace->m_engine.m_previousPointer.m_elements[1];
    return -movement;
}

// Drag-button hit layout: the outer quarters step backward/forward, and the middle half drags; the far edge is
// inset one display unit.
int Module::pressDragButton(plasma::Widget *sender)
{
    m_dragging = false;
    if (m_orientation == ORIENTATION_HORIZONTAL) {
        if (sender->getPointerOffset()[0] <= sender->getViewSize()[0] * 0.25)
            return -1;
        if (sender->getPointerOffset()[0] >= sender->getViewSize()[0] * 3.0 * 0.25 - 1.0)
            return 1;
    } else if (sender->getPointerOffset()[1] <= sender->getViewSize()[1] * 0.25) {
        return 1;
    }

    // The code tests the vertical end zone in both orientations.
    if (sender->getPointerOffset()[1] >= sender->getViewSize()[1] * 3.0 * 0.25 - 1.0)
        return -1;
    m_dragging = true;
    return 0;
}

bool Module::isDragging(plasma::Widget *widget)
{
    return m_dragging && (m_workspace->m_engine.m_buttons & (1 << plasma::MOUSE_BUTTON_LEFT)) &&
           m_workspace->m_engine.m_pointerCapture == widget->m_node;
}

void Module::updateFade()
{
    m_panel->updateSurface();
    plasma::RenderSurface *surface = m_panel->m_surface;
    if (!surface)
        return;
    plasma::Vector<4, float> color = surface->m_drawColor;
    if (m_fade == FADE_IN) {
        if (!m_panel->m_node->m_display->m_visibility.value()) {
            surface->m_drawColor.m_elements[0] = 1.0f;
            surface->m_drawColor.m_elements[1] = 1.0f;
            surface->m_drawColor.m_elements[2] = 1.0f;
            surface->m_drawColor.m_elements[3] = 0.0f;
            m_panel->m_node->m_display->m_visibility.value() = 1;
            m_panel->m_node->animate(0);
            return;
        }
        color.m_elements[3] = m_workspace->m_engine.m_frameTime * (double)0.002f + color.m_elements[3];
        if (color.m_elements[3] >= 1.0f) {
            color.m_elements[3] = 1.0f;
            m_fade = FADE_IDLE;
        }
        m_panel->m_surface->m_drawColor = color;
    } else if (m_fade == FADE_OUT) {
        color.m_elements[3] = color.m_elements[3] - m_workspace->m_engine.m_frameTime * (double)0.002f;
        if (color.m_elements[3] <= 0.0f) {
            color.m_elements[3] = 0.0f;
            m_panel->m_node->m_display->m_visibility.value() = 0;
            m_fade = FADE_IDLE;
        }
        m_panel->m_surface->m_drawColor = color;
    }
}

void Module::setFadingIn(bool fadingIn)
{
    m_fade = fadingIn ? FADE_IN : FADE_OUT;
}
