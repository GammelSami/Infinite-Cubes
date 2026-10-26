// See cube/InventoryWidget.h.
#include "plasma/TextIds.h"
#include "gfx/RendererIds.h"
#include <algorithm>
#include <list>
#include <sstream>
#include <array>
#include <set>
#include <vector>
#include "cube/widgets/InventoryWidget.h"
#include "cube/Creature.h"
#include "cube/Game.h"
#include "cube/Speech.h"
#include "cube/World.h"
#include "gfx/D3D11Renderer.h"
#include "plasma/D3D11Engine.h"
#include "plasma/Display.h"
#include "plasma/Engine.h"
#include "plasma/Node.h"
#include "plasma/Widget.h"
#include "cube/CreatureIds.h"
#include "cube/InventoryWidgetIds.h"
#include "cube/ServerUpdateIds.h"

#include "cube/InterfaceIds.h"
#include "plasma/EditFilter.h"

namespace cube {

InventoryWidget::InventoryWidget(Game *game, int type, plasma::Node *node, plasma::Node *cellTemplate,
                                 plasma::Node *upButton, plasma::Node *downButton, plasma::Node *scrollButton,
                                 plasma::Node *leftButton, plasma::Node *rightButton, plasma::Node *selectionFrame)
    : BaseWidget(game->m_engine, node, L""),
      m_tabs(nullptr),
      m_region(0, 0),
      m_game(game),
      m_cellTemplate(cellTemplate),
      m_selectionFrame(selectionFrame),
      m_upButton(upButton),
      m_downButton(downButton),
      m_scrollButton(scrollButton),
      m_leftButton(leftButton),
      m_rightButton(rightButton),
      m_hoveredCell(-1, -1),
      m_selectedCell(0, 0),
      m_type(static_cast<InventoryViewMode>(type)),
      m_unused2a0(false),
      m_rowCount(1),
      m_cellWidth(40),
      m_cellHeight(40),
      m_hoveredTab(-1),
      m_tabIndex(0)
{
    m_tabParent = m_engine->createNode(nullptr, nullptr, nullptr, node, L"");
    m_cellParent = m_engine->createNode(nullptr, nullptr, nullptr, node, L"");

    // The templates are copied under node with their own transformation and display, and their buttons connected.
    if (m_selectionFrame) {
        m_selectionFrame = m_selectionFrame->instance(node);
        m_selectionFrame->makeTransformationUnique();
        m_selectionFrame->makeDisplayUnique();
        m_selectionFrame->display()->m_visibility.value() = 0;
    }
    if (m_upButton) {
        m_upButton = m_upButton->instance(node);
        m_upButton->makeTransformationUnique();
        m_upButton->makeDisplayUnique();
        m_upButton->connectWidget(L"upbutton", plasma::WIDGET_EVENT_POINTER_PRESSED, this, &InventoryWidget::scrollUp,
                                  true);
    }
    if (m_downButton) {
        m_downButton = m_downButton->instance(node);
        m_downButton->makeTransformationUnique();
        m_downButton->makeDisplayUnique();
        m_downButton->connectWidget(L"downbutton", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                    &InventoryWidget::scrollDown, true);
    }
    if (m_leftButton) {
        m_leftButton = m_leftButton->instance(node);
        m_leftButton->makeTransformationUnique();
        m_leftButton->makeDisplayUnique();
        m_leftButton->connectWidget(L"leftbutton", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                    &InventoryWidget::selectPreviousRegion, true);
    }
    if (m_rightButton) {
        m_rightButton = m_rightButton->instance(node);
        m_rightButton->makeTransformationUnique();
        m_rightButton->makeDisplayUnique();
        m_rightButton->connectWidget(L"rightbutton", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                     &InventoryWidget::selectNextRegion, true);
    }
    if (m_scrollButton) {
        m_scrollButton = m_scrollButton->instance(node);
        m_scrollButton->makeTransformationUnique();
        m_scrollButton->makeDisplayUnique();
        m_scrollButton->connectWidget(L"scrollbutton", plasma::WIDGET_EVENT_POINTER_MOVED, this,
                                      &InventoryWidget::dragScrollButton, true);
    }
    if (m_cellTemplate) {
        m_cellWidth = (int)m_cellTemplate->widget()->viewWidth();
        m_cellHeight = (int)m_cellTemplate->widget()->getViewHeight();
    }
    setFont(L"resource1.dat");
}

void InventoryWidget::draw()
{
    // The region buttons show while the first two tabs of the equipment inventory could go further.
    if (m_leftButton) {
        m_leftButton->display()->m_visibility.value() =
            (m_tabIndex == 0 || m_tabIndex == 1) && !m_regions.empty() && m_region != m_regions.front();
    }
    if (m_rightButton) {
        m_rightButton->display()->m_visibility.value() =
            (m_tabIndex == 0 || m_tabIndex == 1) && !m_regions.empty() && m_region != m_regions.back();
    }
    m_unused2a0 = false;
    if (!m_tabs)
        return;

    m_hoveredCell = plasma::Vector<2, int>(-1, -1);
    plasma::Vector<2, float> position = getContentPosition();
    for (std::vector<plasma::Node *>::iterator it = m_cellNodes.begin(); it != m_cellNodes.end(); ++it)
        (*it)->display()->m_fillColor.value() = plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 0.25f);
    // Window units per widget unit.
    plasma::Matrix<float> matrix;
    m_node->getMatrix(matrix);
    float scale = matrix.m_elements[0];
    float itemScale = scale * 0.03f;

    setTextSize(12.0f);
    setTextStrokeRadius(3.0f);
    setTextColor(plasma::Vector<4, float>(1.0f, 0.8f, 0.5f, 1.0f));
    setTextStrokeColor(plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f));
    setTextFlags(0);
    if (m_tabIndex < (int)m_tabs->size()) {
        int columns = (int)((getContentWidth() - 10.0f) / (m_cellWidth + 5));
        int rows = (int)((getContentHeight() - 40.0f) / (m_cellHeight + 5));
        int first = columns * m_scrollRows[m_tabIndex];
        int end = first + columns * rows;
        int last = end;
        if (m_tabIndex < (int)m_tabs->size() && end > (int)(*m_tabs)[m_tabIndex].size())
            last = (*m_tabs)[m_tabIndex].size();
        if (m_type == InventoryViewBag && (m_tabIndex == InventoryTabEquipment || m_tabIndex == InventoryTabSpecial)) {
            // Only the items of m_region are shown: they come first.
            for (int i = first; i < last; i++) {
                ItemStack &stack = (*m_tabs)[m_tabIndex][i];
                if (stack.quantity && stack.item.region != m_region) {
                    last = i;
                    break;
                }
            }
        }

        // The items, turning slowly; the one under the pointer is larger.
        m_game->m_renderer->activateMode(gfx::RendererMode_OpaqueFront);
        for (int i = first; i < end; i++) {
            int index = i - columns * m_scrollRows[m_tabIndex];
            int x = (int)((index % columns * (m_cellWidth + 5) + 10) * scale + position[0]);
            int y = (int)((index / columns * (m_cellHeight + 5) + 40) * scale + position[1]);
            float half = (m_cellHeight / 2) * scale;
            m_game->m_interface.m_buttonCenters.push_back(plasma::Vector<2, int>((int)(x + half), (int)(y + half)));
            if (i < last) {
                float size = itemScale;
                plasma::Vector<2, float> pointer = m_engine->getPointer();
                if (pointer[0] >= x && m_cellWidth * scale + x > pointer[0] && pointer[1] >= y &&
                    m_cellHeight * scale + y > pointer[1]) {
                    m_hoveredCell = plasma::Vector<2, int>(m_tabIndex, i);
                    size *= 1.5f;
                }
                if ((m_tabIndex < (int)m_tabs->size() && (*m_tabs)[m_tabIndex][i].quantity) ||
                    m_type == InventoryViewCrafting || m_type == InventoryViewVendor) {
                    if (((*m_tabs)[m_tabIndex][i].item).category == ItemCategoryRing)
                        size *= 0.75f;
                    plasma::Vector<3, float> rotation(225.0f, 0.0f,
                                                      m_engine->getCaretClock() % 10000 * 0.0001f * 360.0f);
                    float center = (m_cellHeight / 2) * scale;
                    m_game->m_renderer->drawItem(&((*m_tabs)[m_tabIndex][i].item), x + center, y + center, rotation,
                                                 size, 0.0f, true);
                }
            }
        }
        m_engine->applyRenderState();

        // The title.
        std::wstringstream title;
        switch (m_type) {
        case InventoryViewBag:
            switch (m_tabIndex) {
            case InventoryTabEquipment:
                title << L"Equipment" << std::endl;
                break;
            case InventoryTabConsumables:
                title << L"Items" << std::endl;
                break;
            case InventoryTabCrafting:
                title << L"Ingredients" << std::endl;
                break;
            case InventoryTabPets:
                title << L"Pets" << std::endl;
                break;
            case InventoryTabSpecial:
                title << L"Specials" << std::endl;
                break;
            case InventoryTabArtifacts:
                title << L"Artifacts" << std::endl;
                break;
            }
            break;
        case InventoryViewEquipment:
            title << L"Equipment" << std::endl;
            break;
        case InventoryViewCrafting:
            title << L"Crafting: ";
            switch (m_tabIndex) {
            case CraftingTabWeapons:
                title << L"Weapons" << std::endl;
                break;
            case CraftingTabArmor:
                title << L"Armor" << std::endl;
                break;
            case CraftingTabAmulets:
                title << L"Amulets" << std::endl;
                break;
            case CraftingTabCooking:
                title << L"Cooking" << std::endl;
                break;
            case CraftingTabAlchemy:
                title << L"Alchemy" << std::endl;
                break;
            case CraftingTabIngredients:
                title << L"Ingredients" << std::endl;
                break;
            case CraftingTabRepair:
                title << L"Repair" << std::endl;
                break;
            default:
                title << L"Formulas" << std::endl;
                break;
            }
            break;
        case InventoryViewVendor:
            title << L"Vendor" << std::endl;
            break;
        }
        renderText(title.str(), 15.0f, 25.0f);

        // The region's name under the first two tabs of the equipment.
        if (m_type == InventoryViewBag && (m_tabIndex == InventoryTabEquipment || m_tabIndex == InventoryTabSpecial)) {
            title.str(L"");
            setTextColor(plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f));
            setTextFlags(1 << plasma::TEXT_CENTER);
            // An integer hash of the region seeds its generated name.
            int n = m_region[0] + 57 * m_region[1];
            n = (n << 13) ^ n;
            int seed = 100000 - ((n * (n * n * 60493 + 19990303) + 1376312589) & 0x7fffffff) / 21474;
            title << m_game->m_speech.getText(m_game->m_speech.getLandName(m_game->m_world->landBiome(m_region)),
                                              L"named", randomName(seed, RaceNone));
            renderText(title.str(), (getContentWidth() + 15.0f - 60.0f) * 0.5f, getContentHeight() - 12.0f);
        }

        // The texts of the visible items.
        for (int i = first; i < last; i++) {
            int column = (i - columns * m_scrollRows[m_tabIndex]) % columns;
            int row = (i - columns * m_scrollRows[m_tabIndex]) / columns;
            float right = (float)(m_cellWidth + column * (m_cellWidth + 5) + 6);
            if (m_type != InventoryViewVendor && ((*m_tabs)[m_tabIndex][i]).item.category) {
                // Stackable categories (itemIsStackable's) count their items.
                unsigned char category = ((*m_tabs)[m_tabIndex][i]).item.category;
                if ((category == ItemCategoryConsumable || category == ItemCategoryCube ||
                     category == ItemCategoryMaterial || category == ItemCategoryCoin ||
                     category == ItemCategoryPlatinumCoin || category == ItemCategoryQuest ||
                     category == ItemCategoryElixir) &&
                    ((*m_tabs)[m_tabIndex][i]).quantity > 0) {
                    std::wstringstream count;
                }
            }
            if (((*m_tabs)[m_tabIndex][i]).item.category && m_type == InventoryViewCrafting) {
                std::wstringstream text;
                if (((*m_tabs)[m_tabIndex][i]).quantity > 0 && m_tabIndex == CraftingTabRepair) {
                    text << L"+";
                    setTextFlags(1 << plasma::TEXT_RIGHT);
                    setTextColor(plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f));
                    renderText(text.str(), (float)(m_cellWidth + 6 + column * (m_cellWidth + 5)),
                               (float)(m_cellHeight + 37 + row * (m_cellHeight + 5)));
                }
                text.str(L"");
                if (m_tabIndex == CraftingTabRepair &&
                    m_game->m_equippedCraftingItems.find(i) != m_game->m_equippedCraftingItems.end())
                    text << L"[E] ";
                text << m_game->m_speech.createItemName((*m_tabs)[m_tabIndex][i].item);
                Item &item = (*m_tabs)[m_tabIndex][i].item;
                if ((item.category >= ItemCategoryWeapon && item.category <= ItemCategoryRing ||
                     item.category == ItemCategoryEyePatch) &&
                    item.modifier % (10 * (item.rarity + 1)) == 0)
                    text << L"+";
                if ((*m_tabs)[m_tabIndex][i].quantity > 0 && m_tabIndex != CraftingTabRepair)
                    text << L" (" << (*m_tabs)[m_tabIndex][i].quantity << L")";
                Creature *creature = m_game->m_world->m_localCreature;
                if ((*m_tabs)[m_tabIndex][i].item.fitsClass(creature->m_entityData.classType))
                    setTextColor((*m_tabs)[m_tabIndex][i].item.textColor(creature->m_entityData.region));
                else
                    setTextColor(plasma::Vector<4, float>(1.0f, 0.25f, 0.25f, 1.0f));
                setTextFlags(1 << plasma::TEXT_WRAP);
                setTextWrapWidth((float)(m_cellWidth - m_cellHeight - 20));
                renderText(text.str(), (float)(m_cellHeight + 20 + column * (m_cellWidth + 5)),
                           (float)(m_cellHeight + row * (m_cellHeight + 5) + 13));
            }
            if ((*m_tabs)[m_tabIndex][i].item.category && m_type == InventoryViewBag) {
                std::wstringstream text;
                text << m_game->m_speech.createItemName((*m_tabs)[m_tabIndex][i].item);
                Item &item = (*m_tabs)[m_tabIndex][i].item;
                if ((item.category >= ItemCategoryWeapon && item.category <= ItemCategoryRing ||
                     item.category == ItemCategoryEyePatch) &&
                    item.modifier % (10 * (item.rarity + 1)) == 0)
                    text << L"+";
                unsigned char category = (*m_tabs)[m_tabIndex][i].item.category;
                if (category == ItemCategoryConsumable || category == ItemCategoryCube ||
                    category == ItemCategoryMaterial || category == ItemCategoryCoin ||
                    category == ItemCategoryPlatinumCoin || category == ItemCategoryQuest ||
                    category == ItemCategoryElixir)
                    text << L" (" << (*m_tabs)[m_tabIndex][i].quantity << L")";
                Creature *creature = m_game->m_world->m_localCreature;
                if ((*m_tabs)[m_tabIndex][i].item.fitsClass(creature->m_entityData.classType))
                    setTextColor((*m_tabs)[m_tabIndex][i].item.textColor(creature->m_entityData.region));
                else
                    setTextColor(plasma::Vector<4, float>(1.0f, 0.25f, 0.25f, 1.0f));
                setTextFlags(1 << plasma::TEXT_WRAP);
                setTextWrapWidth((float)(m_cellWidth - m_cellHeight - 20));
                renderText(text.str(), (float)(m_cellHeight + 20 + column * (m_cellWidth + 5)),
                           (float)(m_cellHeight + row * (m_cellHeight + 5) + 10));
                // With the vendor open, what the item sells for (not for tabs 1 and 5).
                plasma::Display *vendorDisplay =
                    m_game->m_interface.m_inGame.m_vendorInventoryWidget->m_node->display();
                if (vendorDisplay->m_visibility.value() && ((m_tabIndex - InventoryTabSpecial) & ~4) != 0) {
                    int price = (*m_tabs)[m_tabIndex][i].item.sellPrice();
                    if (price) {
                        text.str(L"");
                        text << price << " \xae";
                        setTextColor(plasma::Vector<4, float>(1.0f, 0.9f, 0.0f, 1.0f));
                        setTextFlags(1 << plasma::TEXT_RIGHT);
                        renderText(text.str(), right, (float)(m_cellHeight + 37 + row * (m_cellHeight + 5)));
                    }
                }
            }
            if ((*m_tabs)[m_tabIndex][i].item.category && (*m_tabs)[m_tabIndex][i].quantity &&
                m_type == InventoryViewVendor) {
                std::wstringstream text;
                text << m_game->m_speech.createItemName((*m_tabs)[m_tabIndex][i].item);
                Item &item = (*m_tabs)[m_tabIndex][i].item;
                if ((item.category >= ItemCategoryWeapon && item.category <= ItemCategoryRing ||
                     item.category == ItemCategoryEyePatch) &&
                    item.modifier % (10 * (item.rarity + 1)) == 0)
                    text << L"+";
                if ((*m_tabs)[m_tabIndex][i].quantity > 0)
                    text << L" (" << (*m_tabs)[m_tabIndex][i].quantity << L")";
                Creature *creature = m_game->m_world->m_localCreature;
                if ((*m_tabs)[m_tabIndex][i].item.fitsClass(creature->m_entityData.classType))
                    setTextColor((*m_tabs)[m_tabIndex][i].item.textColor(creature->m_entityData.region));
                else
                    setTextColor(plasma::Vector<4, float>(1.0f, 0.25f, 0.25f, 1.0f));
                setTextFlags(1 << plasma::TEXT_WRAP);
                setTextWrapWidth((float)(m_cellWidth - m_cellHeight - 20));
                renderText(text.str(), (float)(m_cellHeight + 20 + column * (m_cellWidth + 5)),
                           (float)(m_cellHeight + row * (m_cellHeight + 5) + 10));
                // The vendor's price: the buying price on the first tab, else the selling price.
                int priceX = column * (m_cellWidth + 5) + m_cellWidth + 10;
                int price;
                if (m_tabIndex == 0)
                    price = (*m_tabs)[0][i].item.price();
                else
                    price = (*m_tabs)[m_tabIndex][i].item.sellPrice();
                if (price) {
                    text.str(L"");
                    text << price << " \xae";
                    setTextColor(plasma::Vector<4, float>(1.0f, 0.9f, 0.0f, 1.0f));
                    setTextFlags(1 << plasma::TEXT_RIGHT);
                    renderText(text.str(), (float)priceX, (float)(m_cellHeight + 37 + row * (m_cellHeight + 5)));
                    plasma::Vector<2, float> minimum;
                    plasma::Vector<2, float> maximum;
                    measureText(text.str(), minimum, maximum);
                }
            }
        }

        // The buttons light up under the pointer.
        if (m_upButton)
            m_upButton->display()->m_fillColor.value() = m_upButton->widget()->isPointerOver()
                                                             ? plasma::Vector<4, float>(0.0f, 1.0f, 1.0f, 1.0f)
                                                             : plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f);
        if (m_downButton)
            m_downButton->display()->m_fillColor.value() = m_downButton->widget()->isPointerOver()
                                                               ? plasma::Vector<4, float>(0.0f, 1.0f, 1.0f, 1.0f)
                                                               : plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f);
        if (m_scrollButton)
            m_scrollButton->display()->m_fillColor.value() = m_scrollButton->widget()->isPointerOver()
                                                                 ? plasma::Vector<4, float>(0.0f, 1.0f, 1.0f, 1.0f)
                                                                 : plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f);
        if (m_leftButton)
            m_leftButton->display()->m_fillColor.value() = m_leftButton->widget()->isPointerOver()
                                                               ? plasma::Vector<4, float>(0.0f, 1.0f, 1.0f, 1.0f)
                                                               : plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f);
        if (m_rightButton)
            m_rightButton->display()->m_fillColor.value() = m_rightButton->widget()->isPointerOver()
                                                                ? plasma::Vector<4, float>(0.0f, 1.0f, 1.0f, 1.0f)
                                                                : plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f);
    }

    // The tabs: the current one and the one under the pointer are highlighted; m_hoveredTab records the latter.
    int tab = 0;
    for (std::list<plasma::Node *>::iterator it = m_tabNodes.begin(); it != m_tabNodes.end(); ++it) {
        (*it)->display()->m_fillColor.value() = tab == m_tabIndex || (*it)->widget()->isPointerOver()
                                                    ? plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f)
                                                    : plasma::Vector<4, float>(0.8f, 0.8f, 0.8f, 1.0f);
        (*it)->findNode(L"frame")->display()->m_fillColor.value() =
            (*it)->widget()->isPointerOver() ? plasma::Vector<4, float>(0.0f, 1.0f, 1.0f, 1.0f)
            : tab == m_tabIndex              ? plasma::Vector<4, float>(0.0f, 1.0f, 0.0f, 1.0f)
                                             : plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f);
        if ((*it)->widget()->isPointerOver())
            m_hoveredTab = tab;
        tab++;
    }

    // The item being dragged follows the pointer.
    Creature *creature = m_game->m_world->m_localCreature;
    if (creature->m_inventory.m_cursorCount) {
        m_game->m_renderer->activateMode(gfx::RendererMode_OpaqueFront);
        plasma::Vector<3, float> rotation(225.0f, 0.0f, m_engine->getCaretClock() % 10000 * 0.0001f * 360.0f);
        m_game->m_renderer->drawItem(&m_game->m_world->m_localCreature->m_inventory.m_cursorItem,
                                     m_engine->getPointer()[0], m_engine->getPointer()[1], rotation, 0.05f, 0.0f, true);
        m_engine->applyRenderState();
    }
}

void InventoryWidget::selectNextRegion()
{
    m_game->playSound(SoundMenuTab);
    for (int i = 0; i < (int)m_regions.size(); i++) {
        plasma::Vector<2, int> region(m_regions[i][0], m_regions[i][1]);
        if (m_region == region) {
            m_region = m_regions[(i + 1) % m_regions.size()];
            break;
        }
    }
    m_game->updateSorting();
    updateRows();
    updateScrollButton();
}

void InventoryWidget::scrollDown()
{
    if (m_tabs) {
        updateRows();
        if (m_tabIndex < (int)m_scrollRows.size()) {
            int &row = m_scrollRows[m_tabIndex];
            if (row < m_rowCount - 1) {
                row++;
                m_game->playSound(SoundMenuTab);
                updateSelection(false);
                updateScrollButton();
            }
        }
    }
}

void InventoryWidget::selectNextTab()
{
    if (++m_tabIndex >= (int)m_tabNodes.size())
        m_tabIndex = 0;
    updateRows();
    // Select the tab's first visible item, if any.
    if ((*m_tabs)[m_tabIndex].empty() || m_tabIndex < 0 || m_tabIndex >= (int)m_scrollRows.size()) {
        m_selectedCell = plasma::Vector<2, int>(-1, -1);
    } else {
        int columns = (int)((getContentWidth() - 10.0f) / (m_cellWidth + 5));
        m_selectedCell = plasma::Vector<2, int>(m_tabIndex, columns * m_scrollRows[m_tabIndex]);
    }
    updateSelection(false);
    updateScrollButton();
    m_game->playSound(SoundMenuTab);
    m_game->onChangeInventoryTab(this);
}

void InventoryWidget::selectHoveredTab()
{
    if (m_tabs) {
        int tab = m_hoveredTab;
        if (tab >= 0 && tab < (int)m_tabs->size() && m_tabIndex != tab) {
            m_tabIndex = tab;
            updateRows();
            // Select the tab's first visible item, if any.
            if ((*m_tabs)[m_tabIndex].empty() || m_tabIndex < 0 || m_tabIndex >= (int)m_scrollRows.size()) {
                m_selectedCell = plasma::Vector<2, int>(-1, -1);
            } else {
                int columns = (int)((getContentWidth() - 10.0f) / (m_cellWidth + 5));
                m_selectedCell = plasma::Vector<2, int>(m_tabIndex, columns * m_scrollRows[m_tabIndex]);
            }
            updateSelection(false);
            updateScrollButton();
            m_game->playSound(SoundMenuTab);
            m_game->onChangeInventoryTab(this);
        }
    }
}

void InventoryWidget::dragScrollButton()
{
    if (m_tabs && m_engine->isButtonDown(0) && m_rowCount > 0) {
        // The track runs from y 35 over the panel height less 70.
        int trackHeight = (int)(getContentHeight() - 70.0f);
        int buttonHeight = trackHeight / m_rowCount;
        float y = (getPointerMovement() + m_scrollButton->widget()->getOffsetInParent())[1];
        int range = trackHeight - buttonHeight;
        float top = 35.0f > y ? 35.0f : y;
        float position = (float)(range + 35) < top ? (float)(range + 35) : top;
        if (range > 0)
            m_scrollRows[m_tabIndex] = (int)((m_rowCount - 1) * (position - 35.0f) / range);
        if (m_scrollRows[m_tabIndex] < 0)
            m_scrollRows[m_tabIndex] = 0;
        if (m_scrollRows[m_tabIndex] >= m_rowCount)
            m_scrollRows[m_tabIndex] = m_rowCount - 1;
        updateSelection(false);
        m_scrollButton->widget()->moveTo(m_scrollButton->widget()->getOffsetInParent()[0], position, true);
    }
}

void InventoryWidget::initialize()
{
    plasma::Widget::initialize();
    resized();
}

void InventoryWidget::resized()
{
    if (m_cellTemplate) {
        updateRows();
        int columns = (int)((getContentWidth() - 10.0f) / (m_cellWidth + 5));
        int rows = (int)((getContentHeight() - 40.0f) / (m_cellHeight + 5));
        m_cellParent->destroyChildren();
        m_cellNodes.clear();
        for (int x = 0; x < columns; x++) {
            for (int y = 0; y < rows; y++) {
                plasma::Node *cell = m_cellTemplate->instance(m_cellParent);
                cell->makeTransformationUnique();
                cell->makeDisplayUnique();
                cell->widget()->moveTo(x * (m_cellWidth + 5) + 10, y * (m_cellHeight + 5) + 40, true);
                m_cellNodes.push_back(cell);
            }
        }
        if (m_selectionFrame) {
            if (m_tabIndex < (int)m_scrollRows.size()) {
                m_selectionFrame->widget()->moveTo(10.0f, 10.0f, true);
                m_selectionFrame->display()->m_visibility.value() = 1;
                int index = m_selectedCell[1] - columns * m_scrollRows[m_tabIndex];
                m_selectionFrame->widget()->moveTo(index % columns * (m_cellWidth + 5) + 10,
                                                   index / columns * (m_cellHeight + 5) + 40, true);
            } else {
                m_selectionFrame->display()->m_visibility.value() = 0;
            }
        }
        if (m_upButton) {
            plasma::Widget *upWidget = m_upButton->widget();
            upWidget->moveTo(getContentWidth() - 30.0f, 10.0f, true);
        }
        if (m_downButton) {
            plasma::Widget *downWidget = m_downButton->widget();
            downWidget->moveTo(getContentWidth() - 30.0f, getContentHeight() - 30.0f, true);
        }
        if (m_leftButton) {
            plasma::Widget *leftWidget = m_leftButton->widget();
            leftWidget->moveTo(15.0f, getContentHeight() - 25.0f, true);
        }
        if (m_rightButton) {
            plasma::Widget *rightWidget = m_rightButton->widget();
            rightWidget->moveTo(getContentWidth() - 60.0f, getContentHeight() - 25.0f, true);
        }
        updateScrollButton();
    }
}

void InventoryWidget::selectPreviousRegion()
{
    m_game->playSound(SoundMenuTab);
    for (int i = 0; i < (int)m_regions.size(); i++) {
        plasma::Vector<2, int> region(m_regions[i][0], m_regions[i][1]);
        if (m_region == region) {
            m_region = m_regions[(m_regions.size() - 1 + i) % m_regions.size()];
            break;
        }
    }
    updateRows();
    updateScrollButton();
}

void InventoryWidget::scrollUp()
{
    if (m_tabs) {
        updateRows();
        if (m_tabIndex < (int)m_scrollRows.size()) {
            int &row = m_scrollRows[m_tabIndex];
            if (row > 0) {
                row--;
                m_game->playSound(SoundMenuTab);
                updateSelection(false);
                updateScrollButton();
            }
        }
    }
}

void InventoryWidget::selectPreviousTab()
{
    if (--m_tabIndex < 0)
        m_tabIndex = (int)m_tabNodes.size() - 1;
    updateRows();
    // Select the tab's first visible item, if any.
    if ((*m_tabs)[m_tabIndex].empty() || m_tabIndex < 0 || m_tabIndex >= (int)m_scrollRows.size()) {
        m_selectedCell = plasma::Vector<2, int>(-1, -1);
    } else {
        int columns = (int)((getContentWidth() - 10.0f) / (m_cellWidth + 5));
        m_selectedCell = plasma::Vector<2, int>(m_tabIndex, columns * m_scrollRows[m_tabIndex]);
    }
    updateSelection(false);
    updateScrollButton();
    m_game->playSound(SoundMenuTab);
    m_game->onChangeInventoryTab(this);
}

void InventoryWidget::setTabNodes(const std::list<plasma::Node *> &tabNodes)
{
    m_tabNodes = tabNodes;
    for (plasma::Node *tab : tabNodes) {
        tab->makeTransformationUnique();
        tab->makeDisplayUnique(L"frame");
    }
    // The tabs sit side by side above the widget, 2 units apart.
    m_tabParent->destroyChildren();
    int x = 4;
    for (std::list<plasma::Node *>::const_iterator it = tabNodes.begin(); it != tabNodes.end(); ++it) {
        m_tabParent->addChild(*it);
        plasma::Widget *tabWidget = (*it)->widget();
        tabWidget->moveTo(x, -tabWidget->getViewHeight() - 3.0f, true);
        x = (int)((*it)->widget()->viewWidth() + 2.0f + x);
        (*it)->widget()->connect(plasma::WIDGET_EVENT_POINTER_PRESSED, this, &InventoryWidget::selectHoveredTab, true,
                                 true);
    }
}

void InventoryWidget::collectRegions()
{
    if (m_type == InventoryViewBag) {
        // Set of two-coordinate cells compared lexicographically.
        std::set<std::array<int, 2>> regions;
        Creature *creature = m_game->m_world->m_localCreature;
        for (ItemStack &stack : creature->m_inventory.m_inventoryTabs[m_tabIndex]) {
            if (stack.quantity) {
                regions.insert(std::array<int, 2> {stack.item.region[0], stack.item.region[1]});
            }
        }
        // The player's own region, each element through the whole member chain.
        regions.insert(std::array<int, 2> {m_game->m_world->m_localCreature->m_entityData.region[0],
                                           m_game->m_world->m_localCreature->m_entityData.region[1]});
        m_regions.clear();
        for (const std::array<int, 2> &region : regions) {
            m_regions.push_back(plasma::Vector<2, int>(region[0], region[1]));
        }
        // Nearest to the player's region first (Chebyshev distance).
        std::sort(m_regions.begin(), m_regions.end(),
                  [this](const plasma::Vector<2, int> &a, const plasma::Vector<2, int> &b) {
                      // Compares each region's distance to the player's region through the member chain.
                      int aDistance = (std::max)(abs(a[0] - m_game->m_world->m_localCreature->m_entityData.region[0]),
                                                 abs(a[1] - m_game->m_world->m_localCreature->m_entityData.region[1]));
                      int bDistance = (std::max)(abs(b[0] - m_game->m_world->m_localCreature->m_entityData.region[0]),
                                                 abs(b[1] - m_game->m_world->m_localCreature->m_entityData.region[1]));
                      return aDistance < bDistance;
                  });
    }
}

void InventoryWidget::updateRows()
{
    if (m_tabs) {
        collectRegions();
        m_game->updateSorting();
        int columns = (int)((getContentWidth() - 10.0f) / (m_cellWidth + 5));
        int rows = (int)((getContentHeight() - 40.0f) / (m_cellHeight + 5));
        if (columns >= 1) {
            m_scrollRows.resize(m_tabs->size(), 0);
            if (m_tabs->empty() || m_tabIndex >= (int)m_tabs->size()) {
                m_rowCount = 1;
            } else {
                if ((*m_tabs)[m_tabIndex].empty()) {
                    m_rowCount = 0;
                    m_scrollRows[m_tabIndex] = 0;
                } else {
                    int lastRow;
                    if (m_type == InventoryViewBag &&
                        (m_tabIndex == InventoryTabEquipment || m_tabIndex == InventoryTabSpecial)) {
                        // Only the items of the chosen region are shown.
                        int count = 0;
                        for (ItemStack &stack : (*m_tabs)[m_tabIndex]) {
                            if (stack.quantity && stack.item.region == m_region)
                                count++;
                        }
                        lastRow = (count - 1) / columns;
                    } else {
                        lastRow = ((*m_tabs)[m_tabIndex].size() - 1) / columns;
                    }
                    m_rowCount = lastRow - rows + 2;
                    if (m_rowCount < 1)
                        m_rowCount = 1;
                    if (m_scrollRows[m_tabIndex] > m_rowCount - 1)
                        m_scrollRows[m_tabIndex] = m_rowCount - 1;
                }
            }
        }
    }
}

void InventoryWidget::updateScrollButton()
{
    if (m_tabs) {
        getContentWidth();
        if (m_scrollButton && m_rowCount && !m_scrollRows.empty()) {
            int trackHeight = (int)(getContentHeight() - 70.0f);
            int buttonHeight = trackHeight / m_rowCount;
            plasma::Node *button = m_scrollButton;
            if (m_rowCount > 1) {
                plasma::Widget *scrollWidget = button->widget();
                scrollWidget->setViewRect(getContentWidth() - 28.0f,
                                          m_scrollRows[m_tabIndex] * (trackHeight - buttonHeight) / (m_rowCount - 1) +
                                              35,
                                          scrollWidget->getContentWidth(), buttonHeight, true);
            } else {
                plasma::Widget *scrollWidget = button->widget();
                scrollWidget->setViewRect(getContentWidth() - 28.0f, 35.0f, scrollWidget->getContentWidth(),
                                          buttonHeight, true);
            }
        }
    }
}

void InventoryWidget::updateSelection(bool select)
{
    if (m_tabs) {
        if (select) {
            if (m_hoveredCell[1] < 0)
                return;
            m_selectedCell = m_hoveredCell;
            m_game->playSound(SoundMenuSelect);
        }
        if (m_selectionFrame) {
            int columns = (int)((viewWidth() - 10.0f) / (m_cellWidth + 5));
            int rows = (int)((getViewHeight() - 40.0f) / (m_cellHeight + 5));
            int first = columns * m_scrollRows[m_tabIndex];
            int item = m_selectedCell[1];
            if (item < 0 || item < first || item >= first + columns * rows) {
                m_selectionFrame->display()->m_visibility.value() = 0;
            } else {
                m_selectionFrame->display()->m_visibility.value() = 1;
                m_selectionFrame->widget()->moveTo(
                    (m_selectedCell[1] - columns * m_scrollRows[m_tabIndex]) % columns * (m_cellWidth + 5) + 10,
                    (m_selectedCell[1] - columns * m_scrollRows[m_tabIndex]) / columns * (m_cellHeight + 5) + 40, true);
            }
        }
    }
}

}
