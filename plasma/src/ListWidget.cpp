// ListWidget: list selection, scrolling and pointer interaction.
#include "plasma/PointerTypes.h"
#include "plasma/ListWidget.h"
#include "plasma/PageUnit.h"
#include "plasma/Engine.h"
#include "plasma/Node.h"
#include "plasma/Transformation.h"

namespace plasma {

ListWidget::ListWidget(Engine *engine, const std::wstring &name)
    : Widget(engine, 0, name)
{
    m_kind = WIDGET_LIST;
}

ListWidget::ListWidget(const ListWidget &other, Node *node)
    : Widget(other, node)
{
    m_kind = WIDGET_LIST;
}

ListWidget::~ListWidget() {}

Widget *ListWidget::clone(Node *node)
{
    return new ListWidget(*this, node);
}

Widget *ListWidget::addItem(Widget *source, bool deep, int position)
{
    m_changed = true;
    Widget *widget;
    if (deep) {
        widget = source->m_node->clone(0)->m_widget;
    } else {
        widget = source->m_node->instance(0)->m_widget;
        Node *node = widget->m_node;
        node->setTransformation(node->m_transformation->clone());
        widget->m_node->makeAnimatedUnique();
    }
    widget->m_node->initializeWidgets();
    if (position < 0) {
        m_node->addChild(widget->m_node);
    } else {
        std::list<Widget *> items = getChildWidgets();
        std::list<Widget *>::iterator item = items.begin();
        for (int i = 0; i < position && item != items.end(); ++i)
            ++item;
        if (item != items.end())
            m_node->insertChild(widget->m_node, (*item)->m_node);
        else
            m_node->addChild(widget->m_node);
    }
    return widget;
}

void ListWidget::clear()
{
    std::list<Widget *> children = getChildWidgets();
    for (std::list<Widget *>::iterator it = children.begin(); it != children.end(); ++it)
        m_engine->destroyNode((*it)->m_node);
}

void ListWidget::layout()
{
    float height = 0.0f;
    m_changed = true;
    std::list<Widget *> children = getChildWidgets();
    for (std::list<Widget *>::iterator it = children.begin(); it != children.end(); ++it) {
        (*it)->moveTo(0.0f, height, true);
        (*it)->m_node->initializeWidgets();
        height = (*it)->viewY() + (*it)->getViewHeight();
        if ((*it)->viewWidth() != viewWidth())
            (*it)->resizeTo(viewWidth(), (*it)->getViewHeight());
    }
    resizeTo(viewWidth(), height);
    clampToParent();
}

void ListWidget::setItems(const std::list<ListItem> &items)
{
    std::map<std::wstring, Widget *> sources;
    std::list<ListItem>::const_iterator it;
    for (it = items.begin(); it != items.end(); ++it) {
        if (sources.find(it->m_source) == sources.end()) {
            Node *sourceNode = m_engine->m_sources;
            sources[it->m_source] = sourceNode->findWidget(it->m_source);
        }
    }
    clear();
    for (it = items.begin(); it != items.end(); ++it) {
        Widget *source = sources[it->m_source];
        if (source) {
            Widget *widget = addItem(source, !it->m_shallow);
            widget->setCaption(it->m_caption, true);
            widget->setName(it->m_name);
            widget->m_node->setName(it->m_name);
            widget->setPopUpItem(it->m_popUpItem, true);
        }
    }
    m_node->initializeWidgets();
    layout();
}

// Points per display unit, indexed by FormatModule's UNIT_* values: px, in, ft, mm, cm, m, pt and pc.
static float s_pointsPerUnit[] = {1.0f, 72.0f, 864.0f, 2.8346457f, 28.346457f, 2834.6458f, 1.0006299f, 11.952f};

float toInternalUnits(unsigned unit, float value)
{
    if (unit <= UNIT_PC)
        return s_pointsPerUnit[unit] * value;
    return value;
}

float fromInternalUnits(unsigned unit, float value)
{
    if (unit <= UNIT_PC)
        return value / s_pointsPerUnit[unit];
    return value;
}

void CALLBACK appendDrawingIndex(void *vertex, void *polygon)
{
    int index = static_cast<int>(reinterpret_cast<PLASMA_POINTER_INT>(vertex));
    static_cast<Drawing *>(polygon)->m_pathIndices.push_back(index);
}

void CALLBACK combineDrawingVertex(double *coordinates, void **vertices, float *weights, void **result, void *polygon)
{
    Drawing *drawing = static_cast<Drawing *>(polygon);
    Vector<2, float> position(0.0f, 0.0f);
    Vector<4, float> color(0.0f, 0.0f, 0.0f, 0.0f);
    Vector<4, float> backColor(0.0f, 0.0f, 0.0f, 0.0f);
    Vector<2, float> texture(0.0f, 0.0f);
    for (int i = 0; i < 4; ++i) {
        position +=
            drawing->m_pathVertices[static_cast<int>(reinterpret_cast<PLASMA_POINTER_INT>(vertices[i]))] * weights[i];
        color +=
            drawing->m_pathColors[static_cast<int>(reinterpret_cast<PLASMA_POINTER_INT>(vertices[i]))] * weights[i];
        if (static_cast<int>(reinterpret_cast<PLASMA_POINTER_INT>(vertices[i])) < (int)drawing->m_pathBackColors.size())
            backColor +=
                drawing->m_pathBackColors[static_cast<int>(reinterpret_cast<PLASMA_POINTER_INT>(vertices[i]))] *
                weights[i];
        texture +=
            drawing->m_pathTextureCoordinates[static_cast<int>(reinterpret_cast<PLASMA_POINTER_INT>(vertices[i]))] *
            weights[i];
    }
    int index = drawing->m_pathVertices.size();
    drawing->m_pathVertices.push_back(position);
    drawing->m_pathColors.push_back(color);
    if (!drawing->m_pathBackColors.empty())
        drawing->m_pathBackColors.push_back(backColor);
    drawing->m_pathTextureCoordinates.push_back(texture);
    *result = reinterpret_cast<void *>(index);
}

}
