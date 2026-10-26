// InventoryWidget displays one tab of the item vectors in m_tabs, scrolling by rows. For type 0, its first two tabs
// filter by the region selected with the left and right buttons.
#pragma once
#include "cube/InterfaceIds.h"
#include <list>
#include <vector>
#include "plasma/Vector.h"
#include "cube/widgets/BaseWidget.h"
#include "cube/Item.h"

namespace plasma {
class Node;
}

namespace cube {

class Game;

class InventoryWidget : public BaseWidget {
public:
    InventoryWidget(Game *game, int type, plasma::Node *node, plasma::Node *cellTemplate, plasma::Node *upButton,
                    plasma::Node *downButton, plasma::Node *scrollButton, plasma::Node *leftButton,
                    plasma::Node *rightButton, plasma::Node *selectionFrame);

    virtual void draw();
    virtual void initialize();
    // Rebuilds the cell grid: as many copies of m_cellTemplate under m_cellParent as fit (cell size + 5 apart, from
    // (10, 40)), kept in m_cellNodes.
    virtual void resized();

    // Scrolls the current tab by one row (the up and down buttons; also Game::HandleMouseWheel).s; behaviour agrees.
    void scrollUp();
    void scrollDown();

    // The right and left buttons: m_region becomes the next / previous entry of m_regions (wrapping), then the rows
    // are recounted.
    void selectNextRegion();
    void selectPreviousRegion();
    // The next / previous tab (wrapping over m_tabNodes), selecting its first visible item; a tab node's press selects
    // m_hoveredTab.
    void selectNextTab();
    void selectHoveredTab();
    void selectPreviousTab();
    // The scroll button's pointer-move handler: while the left button is down it follows the pointer and sets the
    // tab's scroll row.
    void dragScrollButton();
    void setTabNodes(const std::list<plasma::Node *> &tabNodes);
    // m_regions = the regions of the local creature's items in the current tab plus its own region, nearest (Chebyshev
    // distance) first; type 0 only.
    void collectRegions();
    // Recounts m_rowCount for the current tab (for type 0's first two tabs only the items of m_region count) and
    // clamps its scroll row.
    void updateRows();
    // Sizes and places the scroll button for the current tab's scroll row.
    void updateScrollButton();
    // Displays m_selectionFrame over the selected cell when that cell is visible.
    void updateSelection(bool select);

    // Selects the first cell of tab and clears the hover before updating.
    void activateTab(int tab)
    {
        m_tabIndex = tab;
        m_selectedCell = plasma::Vector<2, int>(tab, 0);
        m_hoveredCell = plasma::Vector<2, int>(-1, -1);
        updateRows();
        updateSelection(false);
    }

    // The cell under the pointer, by value.
    plasma::Vector<2, int> getHoveredCell() const { return m_hoveredCell; }
    // The selected cell, by value.
    plasma::Vector<2, int> getSelectedCell() const { return m_selectedCell; }

public:
    // Item tabs supplied by the owning inventory.
    std::vector<std::vector<ItemStack>> *m_tabs;
    // The region whose items the first two tabs show for type 0; the left and right buttons step it through m_regions.
    plasma::Vector<2, int> m_region;
    std::vector<plasma::Vector<2, int>> m_regions; // Freed by the destructor.
    Game *m_game;                                  // Used by scrollDown.
    plasma::Node *m_tabParent;                     // created by the constructor; the tab nodes go under it
    plasma::Node *m_cellParent;                    // created by the constructor; the cells go under it (Resized)
    plasma::Node *m_cellTemplate;                  // Template widget whose size determines each cell's size.
    plasma::Node *m_selectionFrame;
    plasma::Node *m_upButton;     // "upbutton" -> scrollUp
    plasma::Node *m_downButton;   // "downbutton" -> scrollDown
    plasma::Node *m_scrollButton; // Scroll button node.
    plasma::Node *m_leftButton;   // "leftbutton" -> previous region
    plasma::Node *m_rightButton;  // "rightbutton" -> next region
    std::list<plasma::Node *> m_tabNodes;
    plasma::Vector<2, int> m_hoveredCell;
    plasma::Vector<2, int> m_selectedCell;
    InventoryViewMode m_type; // 0, 2 or 3; 0 filters by region
    std::vector<int> m_scrollRows;
    bool m_unused2a0; // false; cleared again by draw; never set or read
    int m_rowCount;   // Number of scrollable rows, initially 1; scrollDown stops at m_rowCount - 1.
    int m_cellWidth;  // Cell width, initially 40 and then taken from the cell template.
    int m_cellHeight; // Cell height, initially 40 and then taken from the cell template.
    int m_hoveredTab; // Hovered tab index, initially -1; set by draw.
    int m_tabIndex;   // Currently displayed tab.
    std::vector<plasma::Node *> m_cellNodes; // Cell nodes laid out by resize and rendered by draw.
};

}
