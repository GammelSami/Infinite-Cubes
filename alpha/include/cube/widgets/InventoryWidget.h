// InventoryWidget displays inventory, crafting and vendor pages.
#pragma once
#include <list>
#include <vector>
#include "plasma/Widget.h"
#include "cube/Inventory.h"
namespace cube {
class GameController;
// InventoryWidget::draw selects titles and item presentation by this mode; GameController
// constructs the bag, crafting and vendor views.
enum InventoryViewMode {
    InventoryViewBag = 0,
    InventoryViewEquipment = 1,
    InventoryViewCrafting = 2,
    InventoryViewVendor = 3
};
// UpdateFormulas fills these six formula groups.
enum CraftingTab {
    CraftingTabWeapons = 0,
    CraftingTabArmor = 1,
    CraftingTabAmulets = 2,
    CraftingTabCooking = 3,
    CraftingTabAlchemy = 4,
    CraftingTabMaterials = 5
};
// UpdateVendor fills buyback from the last sold stacks; the constructor loads buy.png and
// buy-back.png for these tabs.
enum VendorTab {
    VendorTabStock = 0,
    VendorTabBuyback = 1
};
class InventoryWidget : public plasma::Widget {
public:
    InventoryWidget(GameController *game, int viewMode, plasma::Node *node, plasma::Node *cellTemplate,
                    plasma::Node *upButton, plasma::Node *downButton, plasma::Node *scrollButton,
                    plasma::Node *selectionFrame);
    virtual void draw();
    virtual bool contains(const plasma::Vector<2, float> &point);
    virtual void resized();
    plasma::Vector<2, int> GetHoveredCell() const { return m_hoveredCell; }
    void scrollDown();
    void scrollUp();
    void selectHoveredTab();
    void dragScrollButton();
    void setTabNodes(const std::list<plasma::Node *> &tabNodes);

    void setTabIndex(int index) { m_tabIndex = index; }
    void updateRows();
    void updateScrollButton();
    void updateSelection(bool select);

    // The shown inventory; null until GameController assigns one.
    Inventory *m_inventory;
    GameController *m_game;
    plasma::Node *m_cellTemplate;
    plasma::Node *m_selectionFrame;
    plasma::Node *m_upButton;
    plasma::Node *m_downButton;
    plasma::Node *m_scrollButton;
    std::list<plasma::Node *> m_tabNodes;
    plasma::Vector<2, int> m_hoveredCell;
    plasma::Vector<2, int> getSelectedCell() const { return m_selectedCell; }
    plasma::Vector<2, int> m_selectedCell;
    int m_viewMode;
    std::vector<int> m_scrollRows;
    int m_rowCount;
    int m_cellWidth;
    int m_cellHeight;
    int m_hoveredTab;
    int m_tabIndex;
};
}
