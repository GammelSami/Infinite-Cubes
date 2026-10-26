// Panel-item copying, widget layout and animation helpers used by modules.
#include "ColorModule.h"
#include "SpecialColorModule.h"
#include "ColorManagementModule.h"
#include "plasma/Engine.h"
#include "plasma/GenericWidget.h"
#include "ShapeInterface.h"
#include <windows.h>
#include <process.h>
#include <cmath>

extern "C" void StandIn_ModuleItems(plasma::Widget *sender, void *pixels, unsigned count)
{
    std::list<ModuleWidgetItem> items;
    ModuleWidgetItem item(L"editbox", L"color:red", L" ", 0, 0, 45, 20);
    items.push_back(item);
    items.clear();
    std::list<plasma::ListItem> entries;
    plasma::ListItem entry;
    entry.m_caption = sender->m_caption;
    entry.m_name = sender->getName();
    entry.m_source = L"listitem";
    entry.m_popUpItem = sender->m_changed;
    entry.m_shallow = false;
    entries.push_back(entry);
    entries.clear();

    // Render/export callers resize and reserve an array of four-byte RGBA vectors.
    std::vector<plasma::Vector<4, unsigned char>> rgba;
    rgba.resize(count);
    std::copy(rgba.begin(), rgba.end(), static_cast<plasma::Vector<4, unsigned char> *>(pixels));
}

#pragma comment(linker, "/INCLUDE:_StandIn_ModuleItems")

#include "plasma/Node.h"
#include "plasma/Shape.h"

#pragma comment(linker, "/INCLUDE:_StandIn_ModuleNodes")
extern "C" void StandIn_ModuleNodes(plasma::Node *node, plasma::Node *parent, bool checked)
{
    node->instance(parent);
    node->setChecked(checked, false);
}

#include "plasma/Node.h"
#include "plasma/Shape.h"
#include "plasma/Transformation.h"
#include "plasma/Display.h"
#include "plasma/Engine.h"

using namespace plasma;

struct ModuleAnimationTree {
    Node *node;
};

class ModuleAnimationCaller {
public:
    virtual ~ModuleAnimationCaller() {}

    virtual void animate(ModuleAnimationTree *tree) { tree->node->makeAnimatedUnique(); }
};

ModuleAnimationCaller *s_animationCaller;

#pragma comment(linker, "/INCLUDE:_StandIn_ModuleAnimation")
extern "C" int StandIn_ModuleAnimation(ModuleAnimationTree *tree)
{
    if (!s_animationCaller)
        s_animationCaller = new ModuleAnimationCaller;
    s_animationCaller->animate(tree);
    return 0;
}

#include "plasma/Edit.h"
#include "plasma/ListWidget.h"
#include "plasma/ScrollSlider.h"
#include "plasma/ScrollButton.h"
#include "plasma/PopUpButton.h"
#include "plasma/TextShape.h"
#include "plasma/Engine.h"
#include "plasma/Node.h"
#include "plasma/Transformation.h"
#include "plasma/Display.h"
#include "plasma/FontEngine.h"
#include <algorithm>

namespace plasma {

static ListWidget *createListCaller(Engine *engine, const std::wstring &name)
{
    return new ListWidget(engine, name);
}

#pragma comment(linker, "/INCLUDE:_StandIn_ModuleList")
extern "C" int StandIn_ModuleList(Engine *engine, const std::wstring *name)
{
    ListWidget *list = createListCaller(engine, *name);
    list->layout();
    delete list;
    return 0;
}

}
