// It shows the inventory item chosen by Game::handleButtonDown in the item frame with its description, an "Identify"
// and a "Goodbye!" button and the cost. See cube/EnchantWidget.h.
#include "plasma/TextIds.h"
#include "gfx/RendererIds.h"
#include <algorithm>
#include <sstream>
#include "cube/Creature.h"
#include "cube/widgets/EnchantWidget.h"
#include "cube/Game.h"
#include "cube/Item.h"
#include "cube/World.h"
#include "gfx/D3D11Renderer.h"
#include "plasma/D3D11Engine.h"
#include "plasma/Display.h"
#include "plasma/Node.h"
#include "plasma/EditFilter.h"

namespace cube {

EnchantWidget::EnchantWidget(Game *game, plasma::Node *node, plasma::Node *itemFrame)
    : BaseWidget(game->m_engine, node, L""),
      m_inventoryTabIndex(-1),
      m_inventoryItemIndex(-1),
      m_canIdentify(1),
      m_game(game),
      m_itemFrame(itemFrame)
{
    // Show the item frame under this widget's node, with its own transformation and display.
    if (m_itemFrame) {
        m_itemFrame = m_itemFrame->instance(node);
        m_itemFrame->setTransformation(m_itemFrame->transformation()->clone());
        m_itemFrame->setDisplay(m_itemFrame->display()->clone());
    }
    setFont(L"resource1.dat");
}

void EnchantWidget::draw()
{
    Item *item = getItem();
    setTextSize(12.0f);
    setTextStrokeRadius(3.0f);
    setTextStrokeColor(plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f));
    setTextFlags(0);
    if (item) {
        // The item's model, turning in the item frame, and its description.
        plasma::Matrix<float> matrix;
        m_node->getMatrix(matrix);
        m_game->m_renderer->activateMode(gfx::RendererMode_OpaqueFront);
        plasma::Vector<2, float> center =
            m_itemFrame->widget()->getContentPosition() + m_itemFrame->widget()->screenSize() * 0.5f;
        m_game->m_renderer->drawItem(
            item, center[0], center[1],
            plasma::Vector<3, float>(225.0f, 0.0f, m_engine->getCaretClock() % 10000 * 0.0001f * 360.0f),
            matrix[0] * 0.06f, 0.0f, true);
        m_engine->applyRenderState();
        m_game->writeItemInfo(this, item, 1, 15, 180, 1.0f, 300, true, false, false, nullptr, nullptr);
    }

    std::wstringstream text;
    text << L"Identification";
    setTextColor(plasma::Vector<4, float>(1.0f, 0.8f, 0.5f, 1.0f));
    renderText(text.str(), 15.0f, 25.0f);

    // The buttons: cyan under the pointer, "Identify" grey without an item.
    text.str(L"");
    setTextFlags(1 << plasma::TEXT_CENTER);
    text << "Identify";
    setTextColor(!item                     ? plasma::Vector<4, float>(0.7f, 0.7f, 0.7f, 1.0f)
                 : isPointerOverIdentify() ? plasma::Vector<4, float>(0.0f, 1.0f, 1.0f, 1.0f)
                                           : plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f));
    renderText(text.str(), getContentWidth() / 3.0f, getContentHeight() - 20.0f);
    m_game->m_interface.addButtonCenter(this, (int)(getContentWidth() / 3.0f), (int)(getContentHeight() - 20.0f));
    text.str(L"");
    text << "Goodbye!";
    setTextColor(isPointerOverGoodbye() ? plasma::Vector<4, float>(0.0f, 1.0f, 1.0f, 1.0f)
                                        : plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f));
    renderText(text.str(), getContentWidth() * 2.0f / 3.0f, getContentHeight() - 20.0f);
    m_game->m_interface.addButtonCenter(this, (int)(getContentWidth() * 2.0f / 3.0f),
                                        (int)(getContentHeight() - 20.0f));

    if (item) {
        int cost = getCost();
        int y = (int)(getContentHeight() - 50.0f);
        text.str(L"");
        text << "COST: ";
        setTextFlags(0);
        setTextColor(plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f));
        plasma::Vector<2, float> end = renderText(text.str(), 15.0f, (float)y);
        text.str(L"");
        text << cost << " GOLD ";
        setTextColor(plasma::Vector<4, float>(1.0f, 0.9f, 0.0f, 1.0f));
        renderText(text.str(), end[0], (float)y);
    }
}

int EnchantWidget::getCost()
{
    Item *item = getItem();
    if (!item)
        return 0;
    return (std::max)(1, item->price() / 2);
}

Item *EnchantWidget::getItem()
{
    Creature *creature = m_game->m_world->m_localCreature;
    if (creature->m_inventory.m_inventoryTabs.empty())
        return nullptr;
    if (m_inventoryTabIndex < 0 || m_inventoryTabIndex >= (int)creature->m_inventory.m_inventoryTabs.size())
        return nullptr;
    if (m_inventoryItemIndex < 0)
        return nullptr;
    if (m_inventoryItemIndex >= (int)creature->m_inventory.m_inventoryTabs[m_inventoryTabIndex].size() ||
        !creature->m_inventory.m_inventoryTabs[m_inventoryTabIndex][m_inventoryItemIndex].quantity)
        return nullptr;
    Item *item = &creature->m_inventory.m_inventoryTabs[m_inventoryTabIndex][m_inventoryItemIndex].item;
    if (item->category != ItemCategoryLeftovers)
        return nullptr;
    return item;
}

bool EnchantWidget::isPointerOverGoodbye()
{
    // The right half of the bottom 30 units.
    plasma::Vector<2, float> pointer = getPointerOffset();
    return pointer[1] > getContentHeight() - 30.0f && getContentHeight() > pointer[1] &&
           pointer[0] > getContentWidth() * 0.5f && getContentWidth() > pointer[0];
}

bool EnchantWidget::isPointerOverIdentify()
{
    if (!getItem())
        return false;
    // The left half of the bottom 30 units.
    plasma::Vector<2, float> pointer = getPointerOffset();
    return pointer[1] > getContentHeight() - 30.0f && getContentHeight() > pointer[1] &&
           getContentWidth() * 0.5f > pointer[0] && pointer[0] > 0.0f;
}

bool EnchantWidget::isUnidentifiedItem()
{
    return getItem() && getItem()->category == ItemCategoryLeftovers;
}

void EnchantWidget::resized()
{
    // The item frame centred at the top.
    if (m_itemFrame) {
        plasma::Widget *frame = m_itemFrame->widget();
        frame->moveTo((getContentWidth() - frame->viewWidth()) * 0.5f, 50.0f, true);
    }
}

}
