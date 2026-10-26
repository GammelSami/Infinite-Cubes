// A list lays out cloned source widgets vertically.
#pragma once
#include "plasma/Widget.h"
#include "plasma/ScrollSlider.h"
#include "plasma/Drawing.h"

namespace plasma {

struct ListItem {
    ListItem() {}
    // An item shown as a clone of the source widget (the module Builds' list boxes: ColorManagementModule,
    // ExportModule, FormatModule call it).
    ListItem(const std::wstring &caption, const std::wstring &name, const std::wstring &source, bool shallow)
        : m_caption(caption),
          m_name(name),
          m_source(source),
          m_popUpItem(true),
          m_shallow(shallow)
    {}
    std::wstring m_caption;
    std::wstring m_name;
    std::wstring m_source;
    bool m_popUpItem; // The item's widget gets WIDGET_POPUP_ITEM (Widget::SetPopUpItem).
    bool m_shallow;
};

class ListWidget : public Widget {
    friend class PLXStreamReader;
    friend class PLXStreamWriter;

public:
    ListWidget(Engine *engine, const std::wstring &name);
    ListWidget(const ListWidget &other, Node *node);
    virtual ~ListWidget();
    virtual Widget *clone(Node *node);

    // Adds a deep clone or an instance of source as the last item, or ahead of the item at position. Current callers
    // append without specifying a position.
    Widget *addItem(Widget *source, bool deep, int position = -1);
    void clear();
    void layout();
    void setItems(const std::list<ListItem> &items);
};

// Helpers for the item map used by SetItems.
float toInternalUnits(unsigned unit, float value);
float fromInternalUnits(unsigned unit, float value);

// GLU_TESS_VERTEX_DATA and GLU_TESS_COMBINE_DATA use CALLBACK on Windows.
void CALLBACK appendDrawingIndex(void *vertex, void *polygon);
void CALLBACK combineDrawingVertex(double *coordinates, void **vertices, float *weights, void **result, void *polygon);

}
