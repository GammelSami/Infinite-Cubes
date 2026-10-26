// InventoryWidget displays inventory, crafting and vendor pages.
#include "cube/widgets/InventoryWidget.h"
#include "cube/GameController.h"
#include "plasma/Node.h"
#include "plasma/Display.h"
#include "plasma/Engine.h"
#include "plasma/Transformation.h"
#include "plasma/FontEngine.h"
#include "plasma/Font.h"
#include "cube/Creature.h"
#include "cube/CubeShader.h"
#include "cube/SoundIds.h"
#include <iostream>
#include <sstream>
#include <math.h>
namespace cube {

// The grid's layout in pixels (anonymous: this file's own). The cells start 10 pixels in and 40
// down, 5 apart; the scroll track leaves 35 pixels at the top and at the bottom.
namespace {
enum {
    DefaultCellSize = 40
};
enum {
    InventoryGridLeft = 10
};
enum {
    InventoryGridTop = 40
};
enum {
    InventoryCellGap = 5
};
enum {
    ScrollTrackTop = 35
};
}

InventoryWidget::InventoryWidget(GameController *game, int viewMode, plasma::Node *node, plasma::Node *cellTemplate,
                                 plasma::Node *upButton, plasma::Node *downButton, plasma::Node *scrollButton,
                                 plasma::Node *selectionFrame)
    : Widget(game->m_engine, node),
      m_inventory(0),
      m_game(game),
      m_cellTemplate(cellTemplate),
      m_selectionFrame(selectionFrame),
      m_upButton(upButton),
      m_downButton(downButton),
      m_scrollButton(scrollButton),
      m_hoveredCell(-1, -1),
      m_selectedCell(0, 0),
      m_viewMode(viewMode),
      m_rowCount(1),
      m_cellWidth(DefaultCellSize),
      m_cellHeight(DefaultCellSize),
      m_hoveredTab(-1),
      m_tabIndex(0)
{
    if (m_selectionFrame) {
        m_selectionFrame = m_selectionFrame->instance(node);
        m_selectionFrame->setTransformation(m_selectionFrame->m_transformation->clone());
        m_selectionFrame->setDisplay(m_selectionFrame->m_display->clone());
        m_selectionFrame->setVisible(false);
    }
    if (m_upButton) {
        m_upButton = m_upButton->instance(node);
        m_upButton->setTransformation(m_upButton->m_transformation->clone());
        m_upButton->setDisplay(m_upButton->m_display->clone());
        m_upButton->connectWidget(L"upbutton", plasma::WIDGET_EVENT_POINTER_PRESSED, this, &InventoryWidget::scrollUp,
                                  true);
    }
    if (m_downButton) {
        m_downButton = m_downButton->instance(node);
        m_downButton->setTransformation(m_downButton->m_transformation->clone());
        m_downButton->setDisplay(m_downButton->m_display->clone());
        m_downButton->connectWidget(L"downbutton", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                    &InventoryWidget::scrollDown, true);
    }
    if (m_scrollButton) {
        m_scrollButton = m_scrollButton->instance(node);
        m_scrollButton->setTransformation(m_scrollButton->m_transformation->clone());
        m_scrollButton->setDisplay(m_scrollButton->m_display->clone());
        m_scrollButton->connectWidget(L"scrollbutton", plasma::WIDGET_EVENT_POINTER_MOVED, this,
                                      &InventoryWidget::dragScrollButton, true);
    }
    if (m_cellTemplate) {
        m_cellWidth = static_cast<int>(m_cellTemplate->widget()->viewWidth());
        m_cellHeight = static_cast<int>(m_cellTemplate->widget()->getViewHeight());
    }
}

// The open inventory, crafting list or vendor: the items of the shown tab turning in their cells
// (larger under the pointer, black while a stack is negative), the title, the coins of the
// player's bag, each stack's count, the names (in their text colour) of crafting results and
// vendor wares with the vendor's price, the scroll buttons and tabs (cyan under the pointer; the
// shown tab's frame green), and the stack the pointer carries. Its text is laid out in pixels: the
// title (12 high, stroke 3) at 15, 25; the coins (10 high, stroke 2) from 30 left of the right
// edge, 40 apart, 10 above the bottom (platinum at x 15); the counts and prices beside the cell's
// bottom right (6 and 37 on, 10 on for prices); the names 20 right of the cell and 50 down. The
// coins' colours are RGBA: copper (0.8, 0.5, 0), silver (0.7, 0.7, 0.7), gold (1, 0.9, 0) and
// platinum (0.5, 0.2, 1).
void InventoryWidget::draw()
{
    if (!m_inventory)
        return;
    updateRows();
    m_hoveredCell = plasma::Vector<2, int>(-1, -1);
    plasma::Vector<2, float> position = getContentPosition();
    if (m_tabIndex < (int)m_inventory->m_inventoryTabs.size()) {
        m_game->m_device->SetRenderState(D3DRS_ZENABLE, TRUE);
        plasma::Vector<4, float> lightPositions[16];
        plasma::Vector<3, float> lightColors[16];
        for (int i = 0; i < 16; i++) {
            lightPositions[i] = plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f);
            lightColors[i] = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
        }
        m_game->m_shader->apply();
        m_game->m_shader->setPointLights(lightPositions, lightColors);
        int columns = (int)((viewWidth() - InventoryGridLeft) / (m_cellWidth + InventoryCellGap));
        int rows = (int)((getViewHeight() - InventoryGridTop) / (m_cellHeight + InventoryCellGap));
        int first = columns * m_scrollRows[m_tabIndex];
        int end = rows * columns + first;
        if (m_tabIndex < (int)m_inventory->m_inventoryTabs.size() &&
            end > (int)m_inventory->m_inventoryTabs[m_tabIndex].size())
            end = (int)m_inventory->m_inventoryTabs[m_tabIndex].size();
        for (int i = first; i < end; i++) {
            int index = i - columns * m_scrollRows[m_tabIndex];
            int x = (int)((m_cellWidth + InventoryCellGap) * (index % columns) + (position[0] + InventoryGridLeft));
            int y = (int)((m_cellHeight + InventoryCellGap) * (index / columns) + (position[1] + InventoryGridTop));
            float scale = 0.03f;
            plasma::Vector<2, float> pointer = m_engine->getPointer();
            if (pointer[0] >= x && m_cellWidth + x > pointer[0] && pointer[1] >= y && m_cellHeight + y > pointer[1]) {
                m_hoveredCell = plasma::Vector<2, int>(m_tabIndex, i);
                scale *= 1.5f;
            }
            if ((m_tabIndex < (int)m_inventory->m_inventoryTabs.size() &&
                 m_inventory->m_inventoryTabs[m_tabIndex][i].quantity != 0) ||
                m_viewMode == InventoryViewCrafting || m_viewMode == InventoryViewVendor) {
                m_game->m_shader->setMaterialColor(m_inventory->m_inventoryTabs[m_tabIndex][i].quantity >= 0
                                                       ? plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f)
                                                       : plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f));
                Item *item = &m_inventory->m_inventoryTabs[m_tabIndex][i].item;
                if (item->category == ItemCategoryRing)
                    scale *= 0.75f;
                int half = m_cellHeight / 2;
                m_game->drawItem((float)(x + half), (float)(y + half), m_game->m_previewRotation, scale, item, 0.0f);
            }
        }
        m_game->m_device->SetRenderState(D3DRS_ZENABLE, FALSE);

        // The title.
        std::wstringstream text;
        switch (m_viewMode) {
        case InventoryViewBag:
            switch (m_tabIndex) {
            case InventoryTabEquipment:
                text << L"Equipment" << std::endl;
                break;
            case InventoryTabGeneral:
                text << L"Items" << std::endl;
                break;
            case InventoryTabMaterials:
                text << L"Ingredients" << std::endl;
                break;
            }
            break;
        case InventoryViewEquipment:
            text << L"Equipment" << std::endl;
            break;
        case InventoryViewCrafting:
            switch (m_tabIndex) {
            case CraftingTabWeapons:
                text << L"Weapons" << std::endl;
                break;
            case CraftingTabArmor:
                text << L"Armor" << std::endl;
                break;
            case CraftingTabAmulets:
                text << L"Amulets" << std::endl;
                break;
            case CraftingTabCooking:
                text << L"Cooking" << std::endl;
                break;
            case CraftingTabAlchemy:
                text << L"Alchemy" << std::endl;
                break;
            default:
                text << L"Formulas" << std::endl;
                break;
            }
            break;
        case InventoryViewVendor:
            text << L"Vendor" << std::endl;
            break;
        }
        m_engine->getFontEngine()->renderText(L"resource1.dat", text.str(), 0.0f, 0.0f, 15.0f, 25.0f, 12.0f, 3.0f,
                                              plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f).m_elements,
                                              plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f).m_elements,
                                              plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
                                              plasma::TEXT_FLAGS_NONE, -1.0f, true);
        m_engine->getFontEngine()->renderText(L"resource1.dat", text.str(), 0.0f, 0.0f, 15.0f, 25.0f, 12.0f, 0.0f,
                                              plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f).m_elements,
                                              plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
                                              plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
                                              plasma::TEXT_FLAGS_NONE, -1.0f, true);

        // The bag's coins: copper, silver and gold right-aligned at the bottom right, the platinum coins
        // at the bottom left.
        if (m_viewMode == InventoryViewBag) {
            int copper = m_inventory->m_copperBalance % 100;
            int silver = m_inventory->m_copperBalance / 100 % 100;
            int gold = m_inventory->m_copperBalance / 100 / 100;
            int x = (int)(getContentWidth() - 30.0f);
            int y = (int)(getContentHeight() - 10.0f);
            text.str(L"");
            text << copper << " C ";
            m_engine->getFontEngine()->renderText(L"resource1.dat", text.str(), 0.0f, 0.0f, (float)x, (float)y, 10.0f,
                                                  2.0f, plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f).m_elements,
                                                  plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f).m_elements,
                                                  plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
                                                  (1 << plasma::TEXT_RIGHT), -1.0f, true);
            m_engine->getFontEngine()->renderText(L"resource1.dat", text.str(), 0.0f, 0.0f, (float)x, (float)y, 10.0f,
                                                  0.0f, plasma::Vector<4, float>(0.8f, 0.5f, 0.0f, 1.0f).m_elements,
                                                  plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
                                                  plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
                                                  (1 << plasma::TEXT_RIGHT), -1.0f, true);
            x -= 40;
            text.str(L"");
            text << silver << " S ";
            m_engine->getFontEngine()->renderText(L"resource1.dat", text.str(), 0.0f, 0.0f, (float)x, (float)y, 10.0f,
                                                  2.0f, plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f).m_elements,
                                                  plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f).m_elements,
                                                  plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
                                                  (1 << plasma::TEXT_RIGHT), -1.0f, true);
            m_engine->getFontEngine()->renderText(L"resource1.dat", text.str(), 0.0f, 0.0f, (float)x, (float)y, 10.0f,
                                                  0.0f, plasma::Vector<4, float>(0.7f, 0.7f, 0.7f, 1.0f).m_elements,
                                                  plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
                                                  plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
                                                  (1 << plasma::TEXT_RIGHT), -1.0f, true);
            x -= 40;
            text.str(L"");
            text << gold << " G ";
            m_engine->getFontEngine()->renderText(L"resource1.dat", text.str(), 0.0f, 0.0f, (float)x, (float)y, 10.0f,
                                                  2.0f, plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f).m_elements,
                                                  plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f).m_elements,
                                                  plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
                                                  (1 << plasma::TEXT_RIGHT), -1.0f, true);
            m_engine->getFontEngine()->renderText(L"resource1.dat", text.str(), 0.0f, 0.0f, (float)x, (float)y, 10.0f,
                                                  0.0f, plasma::Vector<4, float>(1.0f, 0.9f, 0.0f, 1.0f).m_elements,
                                                  plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
                                                  plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
                                                  (1 << plasma::TEXT_RIGHT), -1.0f, true);
            int platinumY = (int)(getContentHeight() - 10.0f);
            text.str(L"");
            text << m_inventory->m_platinumCoins << "  Platinum Coins ";
            m_engine->getFontEngine()->renderText(
                L"resource1.dat", text.str(), 0.0f, 0.0f, 15.0f, (float)platinumY, 10.0f, 2.0f,
                plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f).m_elements,
                plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f).m_elements,
                plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, plasma::TEXT_FLAGS_NONE, -1.0f, true);
            m_engine->getFontEngine()->renderText(
                L"resource1.dat", text.str(), 0.0f, 0.0f, 15.0f, (float)platinumY, 10.0f, 0.0f,
                plasma::Vector<4, float>(0.5f, 0.2f, 1.0f, 1.0f).m_elements,
                plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
                plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, plasma::TEXT_FLAGS_NONE, -1.0f, true);
        }

        // The texts of the visible stacks.
        for (int i = first; i < end; i++) {
            int index = i - columns * m_scrollRows[m_tabIndex];
            int row = index / columns;
            int column = index % columns;
            ItemStack &stack = m_inventory->m_inventoryTabs[m_tabIndex][i];
            // A stack's count.
            if (stack.item.category && stack.item.isStackable() && stack.quantity > 0) {
                std::wstringstream count;
                count << stack.quantity << std::endl;
                m_engine->getFontEngine()->renderText(
                    L"resource1.dat", count.str(), 0.0f, 0.0f,
                    (float)((m_cellWidth + InventoryCellGap) * column + 6 + m_cellWidth),
                    (float)((m_cellHeight + InventoryCellGap) * row + m_cellHeight + 37), 10.0f, 2.0f,
                    plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f).m_elements,
                    plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f).m_elements,
                    plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, (1 << plasma::TEXT_RIGHT), -1.0f,
                    true);
                m_engine->getFontEngine()->renderText(
                    L"resource1.dat", count.str(), 0.0f, 0.0f,
                    (float)((m_cellWidth + InventoryCellGap) * column + 6 + m_cellWidth),
                    (float)((m_cellHeight + InventoryCellGap) * row + m_cellHeight + 37), 10.0f, 0.0f,
                    plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f).m_elements,
                    plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
                    plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, (1 << plasma::TEXT_RIGHT), -1.0f,
                    true);
            }
            // A crafting result: its count, name and power.
            if (stack.item.category && m_viewMode == InventoryViewCrafting) {
                std::wstringstream text;
                if (stack.quantity > 0) {
                    text << stack.quantity << std::endl;
                    m_engine->getFontEngine()->renderText(
                        L"resource1.dat", text.str(), 0.0f, 0.0f,
                        (float)((m_cellWidth + InventoryCellGap) * column + 6 + m_cellWidth),
                        (float)((m_cellHeight + InventoryCellGap) * row + m_cellHeight + 37), 10.0f, 2.0f,
                        plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f).m_elements,
                        plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f).m_elements,
                        plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, (1 << plasma::TEXT_RIGHT), -1.0f,
                        true);
                    m_engine->getFontEngine()->renderText(
                        L"resource1.dat", text.str(), 0.0f, 0.0f,
                        (float)((m_cellWidth + InventoryCellGap) * column + 6 + m_cellWidth),
                        (float)((m_cellHeight + InventoryCellGap) * row + m_cellHeight + 37), 10.0f, 0.0f,
                        plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f).m_elements,
                        plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
                        plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, (1 << plasma::TEXT_RIGHT), -1.0f,
                        true);
                }
                text.str(L"");
                text << m_game->m_world.createItemName(stack.item);
                if (stack.item.scalesWithLevel())
                    text << " +" << stack.item.power();
                m_engine->getFontEngine()->renderText(
                    L"resource1.dat", text.str(), 0.0f, 0.0f,
                    (float)((m_cellWidth + InventoryCellGap) * column + 20 + m_cellHeight),
                    (float)((m_cellHeight + InventoryCellGap) * row + 50), 10.0f, 2.0f,
                    plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f).m_elements,
                    plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f).m_elements,
                    plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, (1 << plasma::TEXT_WRAP),
                    (float)(m_cellWidth - m_cellHeight - 20), true);
                m_engine->getFontEngine()->renderText(
                    L"resource1.dat", text.str(), 0.0f, 0.0f,
                    (float)((m_cellWidth + InventoryCellGap) * column + 20 + m_cellHeight),
                    (float)((m_cellHeight + InventoryCellGap) * row + 50), 10.0f, 0.0f,
                    stack.item.textColor().m_elements, plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
                    plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, (1 << plasma::TEXT_WRAP),
                    (float)(m_cellWidth - m_cellHeight - 20), true);
            }
            // A vendor's ware: its name and power and its price in copper, silver and gold, right-aligned in
            // the cell.
            if (stack.item.category && m_viewMode == InventoryViewVendor) {
                std::wstringstream text;
                text << m_game->m_world.createItemName(stack.item);
                if (stack.item.scalesWithLevel())
                    text << " +" << stack.item.power();
                m_engine->getFontEngine()->renderText(
                    L"resource1.dat", text.str(), 0.0f, 0.0f,
                    (float)((m_cellWidth + InventoryCellGap) * column + 20 + m_cellHeight),
                    (float)((m_cellHeight + InventoryCellGap) * row + 50), 10.0f, 2.0f,
                    plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f).m_elements,
                    plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f).m_elements,
                    plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, (1 << plasma::TEXT_WRAP),
                    (float)(m_cellWidth - m_cellHeight - 20), true);
                m_engine->getFontEngine()->renderText(
                    L"resource1.dat", text.str(), 0.0f, 0.0f,
                    (float)((m_cellWidth + InventoryCellGap) * column + 20 + m_cellHeight),
                    (float)((m_cellHeight + InventoryCellGap) * row + 50), 10.0f, 0.0f,
                    stack.item.textColor().m_elements, plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
                    plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, (1 << plasma::TEXT_WRAP),
                    (float)(m_cellWidth - m_cellHeight - 20), true);
                int price = stack.item.price();
                int copper = price % 100;
                int silver = price / 100 % 100;
                int gold = price / 100 / 100;
                int x = m_cellWidth + 10 + (m_cellWidth + InventoryCellGap) * column;
                if (copper) {
                    text.str(L"");
                    text << copper << " C ";
                    m_engine->getFontEngine()->renderText(
                        L"resource1.dat", text.str(), 0.0f, 0.0f, (float)x,
                        (float)((m_cellHeight + InventoryCellGap) * row + m_cellHeight + 37), 10.0f, 2.0f,
                        plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f).m_elements,
                        plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f).m_elements,
                        plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, (1 << plasma::TEXT_RIGHT), -1.0f,
                        true);
                    m_engine->getFontEngine()->renderText(
                        L"resource1.dat", text.str(), 0.0f, 0.0f, (float)x,
                        (float)((m_cellHeight + InventoryCellGap) * row + m_cellHeight + 37), 10.0f, 0.0f,
                        plasma::Vector<4, float>(0.8f, 0.5f, 0.0f, 1.0f).m_elements,
                        plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
                        plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, (1 << plasma::TEXT_RIGHT), -1.0f,
                        true);
                    x -= 40;
                }
                if (silver) {
                    text.str(L"");
                    text << silver << " S ";
                    m_engine->getFontEngine()->renderText(
                        L"resource1.dat", text.str(), 0.0f, 0.0f, (float)x,
                        (float)((m_cellHeight + InventoryCellGap) * row + m_cellHeight + 37), 10.0f, 2.0f,
                        plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f).m_elements,
                        plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f).m_elements,
                        plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, (1 << plasma::TEXT_RIGHT), -1.0f,
                        true);
                    m_engine->getFontEngine()->renderText(
                        L"resource1.dat", text.str(), 0.0f, 0.0f, (float)x,
                        (float)((m_cellHeight + InventoryCellGap) * row + m_cellHeight + 37), 10.0f, 0.0f,
                        plasma::Vector<4, float>(0.7f, 0.7f, 0.7f, 1.0f).m_elements,
                        plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
                        plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, (1 << plasma::TEXT_RIGHT), -1.0f,
                        true);
                    x -= 40;
                }
                if (gold) {
                    text.str(L"");
                    text << gold << " G ";
                    m_engine->getFontEngine()->renderText(
                        L"resource1.dat", text.str(), 0.0f, 0.0f, (float)x,
                        (float)((m_cellHeight + InventoryCellGap) * row + m_cellHeight + 37), 10.0f, 2.0f,
                        plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f).m_elements,
                        plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f).m_elements,
                        plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, (1 << plasma::TEXT_RIGHT), -1.0f,
                        true);
                    m_engine->getFontEngine()->renderText(
                        L"resource1.dat", text.str(), 0.0f, 0.0f, (float)x,
                        (float)((m_cellHeight + InventoryCellGap) * row + m_cellHeight + 37), 10.0f, 0.0f,
                        plasma::Vector<4, float>(1.0f, 0.9f, 0.0f, 1.0f).m_elements,
                        plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
                        plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, (1 << plasma::TEXT_RIGHT), -1.0f,
                        true);
                }
            }
        }

        // The scroll buttons, cyan under the pointer.
        if (m_upButton)
            m_upButton->display()->setFillColor(m_upButton->widget()->isPointerOver()
                                                    ? plasma::Vector<4, float>(0.0f, 1.0f, 1.0f, 1.0f)
                                                    : plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f));
        if (m_downButton)
            m_downButton->display()->setFillColor(m_downButton->widget()->isPointerOver()
                                                      ? plasma::Vector<4, float>(0.0f, 1.0f, 1.0f, 1.0f)
                                                      : plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f));
        if (m_scrollButton)
            m_scrollButton->display()->setFillColor(m_scrollButton->widget()->isPointerOver()
                                                        ? plasma::Vector<4, float>(0.0f, 1.0f, 1.0f, 1.0f)
                                                        : plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f));
    }

    // The tabs: grey unless shown or under the pointer; the frame cyan under the pointer, green on the
    // shown tab.
    int index = 0;
    for (std::list<plasma::Node *>::iterator it = m_tabNodes.begin(); it != m_tabNodes.end(); it++) {
        (*it)->display()->setFillColor(index == m_tabIndex || (*it)->widget()->isPointerOver()
                                           ? plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f)
                                           : plasma::Vector<4, float>(0.8f, 0.8f, 0.8f, 1.0f));
        (*it)->findNode(L"frame")->display()->setFillColor(
            (*it)->widget()->isPointerOver()
                ? plasma::Vector<4, float>(0.0f, 1.0f, 1.0f, 1.0f)
                : (index == m_tabIndex ? plasma::Vector<4, float>(0.0f, 1.0f, 0.0f, 1.0f)
                                       : plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f)));
        if ((*it)->widget()->isPointerOver())
            m_hoveredTab = index;
        index++;
    }

    // The stack the pointer carries.
    if (m_game->m_player->m_inventory.m_cursor.quantity)
        m_game->drawItem(m_engine->getPointer()[0], m_engine->getPointer()[1], m_game->m_previewRotation, 0.05f,
                         &m_game->m_player->m_inventory.m_cursor.item, 0.0f);
}

bool InventoryWidget::contains(const plasma::Vector<2, float> &point)
{
    return point[0] >= 0.0f && point[1] >= 0.0f && point[0] < viewWidth() && point[1] < getViewHeight();
}

void InventoryWidget::scrollDown()
{
    if (m_inventory) {
        updateRows();
        if (m_tabIndex < static_cast<int>(m_scrollRows.size()) && m_scrollRows[m_tabIndex] < m_rowCount - 1) {
            ++m_scrollRows[m_tabIndex];
            updateSelection(false);
            updateScrollButton();
        }
    }
}

void InventoryWidget::selectHoveredTab()
{
    if (m_inventory && m_hoveredTab >= 0 && m_tabIndex != m_hoveredTab) {
        std::cout << "tab: " << m_tabIndex << " " << m_hoveredTab << std::endl;
        m_tabIndex = m_hoveredTab;
        updateRows();
        // The code indexes the tab before checking the scroll-row bounds.
        if (m_inventory->m_inventoryTabs[m_tabIndex].empty() || m_tabIndex < 0 ||
            m_tabIndex >= static_cast<int>(m_scrollRows.size())) {
            m_selectedCell = plasma::Vector<2, int>(-1, -1);
        } else {
            int columns = static_cast<int>((viewWidth() - InventoryGridLeft) / (m_cellWidth + InventoryCellGap));
            m_selectedCell = plasma::Vector<2, int>(m_tabIndex, columns * m_scrollRows[m_tabIndex]);
        }
        updateSelection(false);
        updateScrollButton();
        m_game->playSound(SoundMenuTab);
        m_game->onChangeInventoryTab(this);
    }
}

void InventoryWidget::dragScrollButton()
{
    if (m_inventory && m_engine->isButtonDown(plasma::MOUSE_BUTTON_LEFT) && m_rowCount > 0) {
        int trackHeight = static_cast<int>(getViewHeight() - 2 * ScrollTrackTop);
        int buttonHeight = trackHeight / m_rowCount;
        float top = m_scrollButton->widget()->getViewPosition()[1];
        int y = static_cast<int>(m_engine->getPointerDelta()[1] + top);
        if (y < ScrollTrackTop)
            y = ScrollTrackTop;
        int range = trackHeight - buttonHeight;
        if (y > range + ScrollTrackTop)
            y = range + ScrollTrackTop;
        std::cout << trackHeight << " " << buttonHeight << std::endl;
        if (range > 0)
            m_scrollRows[m_tabIndex] = (m_rowCount - 1) * (y - ScrollTrackTop) / range;
        if (m_scrollRows[m_tabIndex] < 0)
            m_scrollRows[m_tabIndex] = 0;
        if (m_scrollRows[m_tabIndex] >= m_rowCount)
            m_scrollRows[m_tabIndex] = m_rowCount - 1;
        updateSelection(false);
        m_scrollButton->m_widget->moveTo(m_scrollButton->widget()->getViewPosition()[0], y, true);
    }
}

// The selection frame starts at 10, 10, the arrows 30 from the right edge (10 from the top, 30
// from the bottom), in pixels.
void InventoryWidget::resized()
{
    if (m_cellTemplate) {
        updateRows();
        int columns = static_cast<int>((viewWidth() - InventoryGridLeft) / (m_cellWidth + InventoryCellGap));
        int rows = static_cast<int>((getViewHeight() - InventoryGridTop) / (m_cellHeight + InventoryCellGap));
        std::list<plasma::Node *> oldCells;
        for (std::list<plasma::Node *>::iterator it = m_node->m_parent->m_children.begin();
             it != m_node->m_parent->m_children.end(); ++it) {
            if ((*it)->m_shape == m_cellTemplate->m_shape)
                oldCells.push_back(*it);
        }
        for (std::list<plasma::Node *>::iterator it = oldCells.begin(); it != oldCells.end(); ++it)
            m_engine->deleteNode(*it);
        for (int x = 0; x < columns; ++x) {
            for (int y = 0; y < rows; ++y) {
                plasma::Node *cell = m_cellTemplate->instance(0);
                cell->setTransformation(cell->m_transformation->clone());
                m_node->m_parent->insertChild(cell, m_node);
                cell->m_widget->moveTo(x * (m_cellWidth + InventoryCellGap) + InventoryGridLeft,
                                       y * (m_cellHeight + InventoryCellGap) + InventoryGridTop, true);
            }
        }
        if (m_selectionFrame) {
            if (m_tabIndex < static_cast<int>(m_scrollRows.size())) {
                m_selectionFrame->m_widget->moveTo(10.0f, 10.0f, true);
                m_selectionFrame->setVisible(true);
                int index = m_selectedCell[1] - columns * m_scrollRows[m_tabIndex];
                // This assumes a nonzero column count.
                m_selectionFrame->m_widget->moveTo(
                    index % columns * (m_cellWidth + InventoryCellGap) + InventoryGridLeft,
                    index / columns * (m_cellHeight + InventoryCellGap) + InventoryGridTop, true);
            } else {
                m_selectionFrame->setVisible(false);
            }
        }
        if (m_upButton) {
            plasma::Widget *widget = m_upButton->m_widget;
            widget->moveTo(viewWidth() - 30.0f, 10.0f, true);
        }
        if (m_downButton) {
            plasma::Widget *widget = m_downButton->m_widget;
            widget->moveTo(viewWidth() - 30.0f, getViewHeight() - 30.0f, true);
        }
        updateScrollButton();
    }
}

void InventoryWidget::scrollUp()
{
    if (m_inventory) {
        updateRows();
        if (m_tabIndex < static_cast<int>(m_scrollRows.size()) && m_scrollRows[m_tabIndex] > 0) {
            --m_scrollRows[m_tabIndex];
            updateSelection(false);
            updateScrollButton();
        }
    }
}

// The tabs sit 3 pixels above the widget, from x 4, 2 apart.
void InventoryWidget::setTabNodes(const std::list<plasma::Node *> &tabNodes)
{
    m_tabNodes = tabNodes;
    std::list<plasma::Node *> oldTabs;
    for (std::list<plasma::Node *>::iterator it = m_node->m_children.begin(); it != m_node->m_children.end(); ++it) {
        if (*it != m_upButton && *it != m_downButton && *it != m_selectionFrame && *it != m_scrollButton)
            oldTabs.push_back(*it);
    }
    for (std::list<plasma::Node *>::iterator it = oldTabs.begin(); it != oldTabs.end(); ++it)
        m_engine->deleteNode(*it);
    int x = 4;
    for (std::list<plasma::Node *>::const_iterator it = tabNodes.begin(); it != tabNodes.end(); ++it) {
        m_node->addChild(*it);
        plasma::Widget *widget = (*it)->m_widget;
        widget->moveTo(x, -widget->getViewHeight() - 3.0f, true);
        x = static_cast<int>((*it)->widget()->viewWidth() + 2.0f + x);
        (*it)->m_widget->connect(plasma::WIDGET_EVENT_POINTER_PRESSED, this, &InventoryWidget::selectHoveredTab, true,
                                 true);
    }
}

void InventoryWidget::updateRows()
{
    if (m_inventory) {
        int columns = static_cast<int>((viewWidth() - InventoryGridLeft) / (m_cellWidth + InventoryCellGap));
        int rows = static_cast<int>((getViewHeight() - InventoryGridTop) / (m_cellHeight + InventoryCellGap));
        m_scrollRows.resize(m_inventory->m_inventoryTabs.size(), 0);
        if (!m_inventory->m_inventoryTabs.empty() &&
            m_tabIndex < static_cast<int>(m_inventory->m_inventoryTabs.size())) {
            if (m_inventory->m_inventoryTabs[m_tabIndex].empty()) {
                m_rowCount = 0;
                m_scrollRows[m_tabIndex] = 0;
            } else {
                // The code does not guard against a zero column count.
                m_rowCount =
                    static_cast<int>((m_inventory->m_inventoryTabs[m_tabIndex].size() - 1) / columns - rows + 2);
                if (m_rowCount < 1)
                    m_rowCount = 1;
                if (m_scrollRows[m_tabIndex] > m_rowCount - 1)
                    m_scrollRows[m_tabIndex] = m_rowCount - 1;
            }
        } else {
            m_rowCount = 1;
        }
    }
}

// The thumb is 28 pixels from the right edge.
void InventoryWidget::updateScrollButton()
{
    if (m_inventory) {
        viewWidth();
        if (m_scrollButton && m_rowCount && !m_scrollRows.empty()) {
            int trackHeight = static_cast<int>(getViewHeight() - 2 * ScrollTrackTop);
            int buttonHeight = trackHeight / m_rowCount;
            if (m_rowCount > 1) {
                plasma::Widget *widget = m_scrollButton->m_widget;
                widget->setViewRect(viewWidth() - 28.0f,
                                    m_scrollRows[m_tabIndex] * (trackHeight - buttonHeight) / (m_rowCount - 1) +
                                        ScrollTrackTop,
                                    widget->viewWidth(), buttonHeight, true);
            } else {
                plasma::Widget *widget = m_scrollButton->m_widget;
                widget->setViewRect(viewWidth() - 28.0f, ScrollTrackTop, widget->viewWidth(), buttonHeight, true);
            }
        }
    }
}

void InventoryWidget::updateSelection(bool select)
{
    if (m_inventory) {
        if (select) {
            if (m_hoveredCell[1] < 0)
                return;
            m_selectedCell = m_hoveredCell;
            m_game->playSound(SoundMenuSelect);
        }
        if (m_selectionFrame) {
            int columns = static_cast<int>((viewWidth() - InventoryGridLeft) / (m_cellWidth + InventoryCellGap));
            int rows = static_cast<int>((getViewHeight() - InventoryGridTop) / (m_cellHeight + InventoryCellGap));
            int first = columns * m_scrollRows[m_tabIndex];
            int item = m_selectedCell[1];
            if (item < 0 || item < first || item >= first + columns * rows) {
                m_selectionFrame->setVisible(false);
            } else {
                m_selectionFrame->setVisible(true);
                m_selectionFrame->m_widget->moveTo((m_selectedCell[1] - columns * m_scrollRows[m_tabIndex]) % columns *
                                                           (m_cellWidth + InventoryCellGap) +
                                                       InventoryGridLeft,
                                                   (m_selectedCell[1] - columns * m_scrollRows[m_tabIndex]) / columns *
                                                           (m_cellHeight + InventoryCellGap) +
                                                       InventoryGridTop,
                                                   true);
            }
        }
    }
}
}
