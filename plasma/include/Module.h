// Module: the base of the editor's panels (about 34 derived modules: colour, layer, mesh, text, tool...). A module
// builds its widgets into one panel, laying them out left to right in rows from a cursor, and keeps the extent the
// widgets reached.
#pragma once
#include <list>
#include <set>
#include "plasma/NamedObject.h"
#include "plasma/Vector.h"
#include "plasma/Widget.h"
#include "Workspace.h"

namespace plasma {
class Node;
}
// A widget for the panel builders: its caption, name and shape (the widget kind: "editbox", "button2"...) and its
// place in its parent (copy, destructor).
struct ModuleWidgetItem {
    ModuleWidgetItem(const std::wstring &caption, const std::wstring &name, const std::wstring &shape, int left,
                     int top, int width, int height);
    ModuleWidgetItem(const ModuleWidgetItem &other);
    ~ModuleWidgetItem();
    std::wstring m_caption;
    std::wstring m_name;
    std::wstring m_shape;
    int m_left;
    int m_top;
    int m_width;
    int m_height;
};

#ifdef PLASMA_PORTABLE
extern Workspace &g_workspace;
#else
extern Workspace g_workspace; // WindowsUI.cpp.
#endif

class Module {
public:
    enum Orientation {
        ORIENTATION_HORIZONTAL = 0,
        ORIENTATION_VERTICAL = 1
    };
    enum ZoomLevel {
        ZOOM_TINY = -2,
        ZOOM_SMALL = -1,
        ZOOM_NORMAL = 0,
        ZOOM_LARGE = 1
    };
    enum FadeState {
        FADE_IDLE = 0,
        FADE_IN = 1,
        FADE_OUT = 2
    };
    Module();
    virtual ~Module() {}

    // Builds the panel (Workspace::Init, once per module).
    virtual void build() {}
    // The frame's update with the milliseconds since the last frame (Workspace::Update calls it on the timeline
    // module, which is never created).
    virtual void advanceFrame(int) {}
    // Shows the module's state on its panel's widgets (after a pointer release, a document change, the tool or mode
    // changed).
    virtual void updatePanel() {}
    // Shows the selected nodes' values: the most subdivisions in MeshModule, the name and shared flags in NodeModule,
    // and the values in TextModule and WidgetModule. Currently unused.
    virtual void showSelectedValues() {}
    // Forgets WidgetModule's current node widgets. Currently unused.
    virtual void clearCurrentNode() {}
    // The open design sheet's current node changed (Workspace:: UpdateModules, after selection, undo/redo, clipboard
    // commands); the "pick:auto" panels read the node's settings.
    virtual void currentNodeChanged() {}
    // The display unit changed (Workspace::UnitChanged, from FormatModule::Update): values shown in units are
    // rewritten.
    virtual void unitChanged() {}
    // Another sheet became current, or the current one was loaded, replaced or closed (Workspace::SetSheet, Update,
    // NewFile, CloseSheet).
    virtual void sheetChanged() {}

    // Starts a new row: back to the left margin, below everything so far.
    void newRow(int spacing);

    // Records the mouse position where a drag starts.
    void beginDrag(float start);

    // Adds the drag movement to the drag position.
    void updateDrag();

    // The mouse movement since the last frame along the module's axis (horizontal: x, vertical: upwards).
    float getDragMovement();
    bool isDragging(plasma::Widget *widget);

    // Where the pointer is on a drag button: -1 or 1 on its end zones (a step), 0 in the middle (a drag starts).
    int pressDragButton(plasma::Widget *sender);
    void setFadingIn(bool fadingIn);
    // Fades the panel's surface in or out by the frame time; Workspace::Update calls it for each module.
    void updateFade();

    // The panel builders that the derived modules' Build functions use. A widget item is placed at its own position in
    // its parent; the "At" forms return the right and bottom edges reached, the others place the widget at the cursor
    // (after spacing) and move the cursor and extent.
    plasma::Vector<2, int> addPanelAt(const std::list<ModuleWidgetItem> &items, const std::wstring &name, int x, int y);
    plasma::Vector<2, int> addUpperPanel(const std::wstring &name, const std::list<ModuleWidgetItem> &items, int x);
    plasma::Vector<2, int> addLeftPanel(const std::wstring &name, const std::list<ModuleWidgetItem> &items);
    plasma::Vector<2, int> addWidgetAt(const ModuleWidgetItem &item);

    // A "combobutton" with its drop-down "listbox".
    plasma::Vector<2, int> addComboButtonAt(const std::wstring &caption, const std::wstring &name,
                                            const std::wstring &listName, int x, int y, int width, int height,
                                            int listWidth, int listHeight);
    void addPanel(const std::list<ModuleWidgetItem> &items, const std::wstring &name, int spacing);
    void addWidget(const std::wstring &caption, const std::wstring &name, const std::wstring &shape, int width,
                   int height, int spacing);
    void addComboButton(const std::wstring &caption, const std::wstring &name, const std::wstring &listName, int width,
                        int height, int listWidth, int listHeight, int spacing);

    // Sizes the panel to its shown widgets plus the margin.
    void fitWidth(int margin);
    void fitHeight(int margin);

    // Puts an instance of the interface icon iconName into the panel's node nodeName, centred on it.
    void placeIcon(const std::wstring &nodeName, const std::wstring &iconName);

    // The "<prefix>:apply" and "<prefix>:pick" buttons with their "auto" check buttons, above the panel's middle
    // (moved by offset).
    void addApplyBar(const std::wstring &prefix, int offset);

    // The zoom bar ("zoombar:zoom:tiny"... "large") and its callbacks, which play the panel's "module:zoom:*"
    // sequences.
    void addZoomBar();
    void zoomTiny();
    void zoomSmall();
    void zoomNormal();
    void zoomLarge();

    // Records the panel's "module:zoom:*" deformation sequences, connects ZoomWithWheel to the panel's wheel event and
    // starts at the normal step.
    void createZoomSequences();

    // Ctrl + wheel: steps m_zoom and plays its sequence.
    void zoomWithWheel();

    // The module's panel. WindowsUI.cpp shows the about panel through it; ColorModule marks the texture panel changed.
    plasma::Widget *getPanel() const { return m_panel; }

protected:
    friend class Workspace; // Shows the panels.
    friend class NodeTool;  // Shows the shadow panel.
    Workspace *m_workspace;
    plasma::Vector<2, int> m_cursor; // Where the next widget goes.
    plasma::Vector<2, int> m_extent; // The right and bottom edges reached.
    plasma::Vector<2, int> m_margin; // The panel's left and top margins.
    float m_dragPosition;
    float m_dragStart;
    plasma::Vector<2, int> m_dragStartMouse;
    int m_orientation; // Orientation; nonzero values select the vertical axis.
    bool m_dragging;
    int m_fade; // FadeState.
    plasma::Widget *m_panel;
    // The panel's zoom step, from -2 to 1 ("module:zoom:tiny" through "module:zoom:large"), adjusted with Ctrl and the
    // mouse wheel.
    int m_zoom; // ZoomLevel.
};

// (ColorManagementModule.cpp): the value rounded to two decimals, half away from zero and without a negative zero;
// the panels show their numbers through it.
float roundToHundredths(float value);

// Compatibility includes for existing module users. Each panel has one owner.
#include "VertexModule.h"
#include "HistoryModule.h"
