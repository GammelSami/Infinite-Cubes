// EnchantWidget shows identification controls.
#include "cube/widgets/EnchantWidget.h"
#include "cube/Item.h"
#include "cube/GameController.h"
#include "cube/Creature.h"
#include "cube/DefaultBlocks.h"
#include "plasma/Node.h"
#include "plasma/Transformation.h"
#include "plasma/Display.h"
#include "plasma/FontEngine.h"
#include "plasma/Font.h"
#include "plasma/Engine.h"
#include "cube/CubeShader.h"
#include <sstream>
namespace cube {
// Identifying costs half the item's price, at least a copper (anonymous: this file's own).
namespace {
enum {
    IdentificationPriceDivisor = 2
};
enum {
    MinimumIdentificationCost = 1
};
}

EnchantWidget::EnchantWidget(plasma::Engine *engine, plasma::Node *node, GameController *game, plasma::Node *itemFrame)
    : Widget(engine, node),
      m_inventoryTabIndex(-1),
      m_inventoryItemIndex(-1),
      m_canIdentify(1),
      m_game(game),
      m_itemFrame(itemFrame)
{
    if (m_itemFrame) {
        m_itemFrame = m_itemFrame->instance(node);
        m_itemFrame->setTransformation(m_itemFrame->m_transformation->clone());
        m_itemFrame->setDisplay(m_itemFrame->m_display->clone());
    }
}

// The unidentified item turning in the item frame with its description, the Identify and Goodbye
// buttons (cyan under the pointer; Identify grey without an item) and the cost in gold, silver and
// copper coins. Its layout is in pixels: the item at scale 0.06, its description at 15, 180 (300
// wide); the title at 15, 25 (12 high, stroke 3); Identify and Goodbye at a third and two thirds
// of the width, 20 up from the bottom; the cost 50 up from the bottom, the copper, silver and gold
// amounts at 220, 180 and 140 (10 high, stroke 2); colours RGBA (grey 0.7 without an item, cyan
// under the pointer, the coins' colours).
void EnchantWidget::draw()
{
    Item *item = getItem();
    if (item) {
        plasma::Vector<2, float> pivot = m_node->m_matrix * m_node->transformation()->m_pivot.value();
        if (m_game->m_world.m_spriteManager.itemSprite(item)) {
            m_game->m_device->SetRenderState(D3DRS_ZENABLE, TRUE);
            plasma::Vector<4, float> lightPositions[PointLightSourceCount];
            plasma::Vector<3, float> lightColors[PointLightSourceCount];
            for (int i = 0; i < PointLightSourceCount; i++) {
                lightPositions[i] = plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f);
                lightColors[i] = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
            }
            m_game->m_shader->apply();
            m_game->m_shader->setPointLights(lightPositions, lightColors);
            m_game->m_shader->setMaterialColor(plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f));
            m_game->drawItem(
                pivot[0] + m_itemFrame->widget()->getViewPosition()[0] + m_itemFrame->widget()->viewWidth() * 0.5f,
                pivot[1] + m_itemFrame->widget()->getViewPosition()[1] + m_itemFrame->widget()->getViewHeight() * 0.5f,
                m_game->m_previewRotation, 0.06f, item, 0.0f);
        }
        m_game->m_device->SetRenderState(D3DRS_ZENABLE, FALSE);
        m_game->writeItemInfo(item, 15, 180, 1.0f, 300, true, false, false);
    }

    std::wstringstream text;
    text << L"Identification";
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

    text.str(L"");
    text << "Identify";
    m_engine->getFontEngine()->renderText(
        L"resource1.dat", text.str(), 0.0f, 0.0f, getContentWidth() / 3.0f, getContentHeight() - 20.0f, 12.0f, 3.0f,
        plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f).m_elements,
        plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f).m_elements,
        plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, (1 << plasma::TEXT_CENTER), -1.0f, true);
    const float *identifyColor = !item ? plasma::Vector<4, float>(0.7f, 0.7f, 0.7f, 1.0f).m_elements
                                       : (isPointerOverIdentify() ? plasma::Vector<4, float>(0.0f, 1.0f, 1.0f, 1.0f)
                                                                  : plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f))
                                             .m_elements;
    m_engine->getFontEngine()->renderText(
        L"resource1.dat", text.str(), 0.0f, 0.0f, getContentWidth() / 3.0f, getContentHeight() - 20.0f, 12.0f, 0.0f,
        identifyColor, plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
        plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, (1 << plasma::TEXT_CENTER), -1.0f, true);

    text.str(L"");
    text << "Goodbye!";
    m_engine->getFontEngine()->renderText(
        L"resource1.dat", text.str(), 0.0f, 0.0f, getContentWidth() * 2.0f / 3.0f, getContentHeight() - 20.0f, 12.0f,
        3.0f, plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f).m_elements,
        plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f).m_elements,
        plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, (1 << plasma::TEXT_CENTER), -1.0f, true);
    const float *goodbyeColor = (isPointerOverGoodbye() ? plasma::Vector<4, float>(0.0f, 1.0f, 1.0f, 1.0f)
                                                        : plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f))
                                    .m_elements;
    m_engine->getFontEngine()->renderText(
        L"resource1.dat", text.str(), 0.0f, 0.0f, getContentWidth() * 2.0f / 3.0f, getContentHeight() - 20.0f, 12.0f,
        0.0f, goodbyeColor, plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
        plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, (1 << plasma::TEXT_CENTER), -1.0f, true);

    if (item) {
        // The cost in coins: 100 copper make a silver, 100 silver a gold.
        int cost = getCost();
        int copper = cost % 100;
        int silver = cost / 100 % 100;
        int gold = cost / 100 / 100;
        int y = (int)(getContentHeight() - 50.0f);
        text.str(L"");
        text << copper << " C ";
        m_engine->getFontEngine()->renderText(L"resource1.dat", text.str(), 0.0f, 0.0f, 220.0f, (float)y, 10.0f, 2.0f,
                                              plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f).m_elements,
                                              plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f).m_elements,
                                              plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
                                              (1 << plasma::TEXT_RIGHT), -1.0f, true);
        m_engine->getFontEngine()->renderText(L"resource1.dat", text.str(), 0.0f, 0.0f, 220.0f, (float)y, 10.0f, 0.0f,
                                              plasma::Vector<4, float>(0.8f, 0.5f, 0.0f, 1.0f).m_elements,
                                              plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
                                              plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
                                              (1 << plasma::TEXT_RIGHT), -1.0f, true);
        text.str(L"");
        text << silver << " S ";
        m_engine->getFontEngine()->renderText(L"resource1.dat", text.str(), 0.0f, 0.0f, 180.0f, (float)y, 10.0f, 2.0f,
                                              plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f).m_elements,
                                              plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f).m_elements,
                                              plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
                                              (1 << plasma::TEXT_RIGHT), -1.0f, true);
        m_engine->getFontEngine()->renderText(L"resource1.dat", text.str(), 0.0f, 0.0f, 180.0f, (float)y, 10.0f, 0.0f,
                                              plasma::Vector<4, float>(0.7f, 0.7f, 0.7f, 1.0f).m_elements,
                                              plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
                                              plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
                                              (1 << plasma::TEXT_RIGHT), -1.0f, true);
        text.str(L"");
        text << gold << " G ";
        m_engine->getFontEngine()->renderText(L"resource1.dat", text.str(), 0.0f, 0.0f, 140.0f, (float)y, 10.0f, 2.0f,
                                              plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f).m_elements,
                                              plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f).m_elements,
                                              plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
                                              (1 << plasma::TEXT_RIGHT), -1.0f, true);
        m_engine->getFontEngine()->renderText(L"resource1.dat", text.str(), 0.0f, 0.0f, 140.0f, (float)y, 10.0f, 0.0f,
                                              plasma::Vector<4, float>(1.0f, 0.9f, 0.0f, 1.0f).m_elements,
                                              plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
                                              plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
                                              (1 << plasma::TEXT_RIGHT), -1.0f, true);
        text.str(L"");
        text << "COST:";
        m_engine->getFontEngine()->renderText(L"resource1.dat", text.str(), 0.0f, 0.0f, 15.0f, (float)y, 10.0f, 2.0f,
                                              plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f).m_elements,
                                              plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f).m_elements,
                                              plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
                                              plasma::TEXT_FLAGS_NONE, -1.0f, true);
        m_engine->getFontEngine()->renderText(L"resource1.dat", text.str(), 0.0f, 0.0f, 15.0f, (float)y, 10.0f, 0.0f,
                                              plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f).m_elements,
                                              plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
                                              plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
                                              plasma::TEXT_FLAGS_NONE, -1.0f, true);
    }
}

int EnchantWidget::getCost()
{
    Item *item = getItem();
    if (!item)
        return 0;
    return item->price() / IdentificationPriceDivisor < MinimumIdentificationCost
               ? MinimumIdentificationCost
               : item->price() / IdentificationPriceDivisor;
}

// The item of the selected inventory stack, when the stack is not empty and the item is
// unidentified (category 14); otherwise null.
Item *EnchantWidget::getItem()
{
    Creature *player = m_game->m_player;
    if (player->m_inventory.m_inventoryTabs.empty() || m_inventoryTabIndex < 0 ||
        m_inventoryTabIndex >= (int)player->m_inventory.m_inventoryTabs.size() || m_inventoryItemIndex < 0 ||
        m_inventoryItemIndex >= (int)player->m_inventory.m_inventoryTabs[m_inventoryTabIndex].size() ||
        player->m_inventory.m_inventoryTabs[m_inventoryTabIndex][m_inventoryItemIndex].quantity == 0)
        return 0;
    Item *item = &player->m_inventory.m_inventoryTabs[m_inventoryTabIndex][m_inventoryItemIndex].item;
    return item->category == ItemCategoryLeftovers ? item : 0;
}

// The action buttons' hit strips are the bottom 30 pixels.
bool EnchantWidget::isPointerOverGoodbye()
{
    plasma::Vector<2, float> pointer = getPointerOffset();
    return pointer[1] > getContentHeight() - 30.0f && pointer[1] < getContentHeight() &&
           pointer[0] > getContentWidth() * 0.5f && pointer[0] < getContentWidth();
}

bool EnchantWidget::isPointerOverIdentify()
{
    if (!getItem())
        return false;
    plasma::Vector<2, float> pointer = getPointerOffset();
    return pointer[1] > getContentHeight() - 30.0f && pointer[1] < getContentHeight() &&
           pointer[0] < getContentWidth() * 0.5f && pointer[0] > 0.0f;
}

bool EnchantWidget::isUnidentifiedItem()
{
    return getItem() && getItem()->category == ItemCategoryLeftovers;
}

// In pixels: the item frame centred at y 50.
void EnchantWidget::resized()
{
    if (m_itemFrame) {
        plasma::Widget *widget = m_itemFrame->m_widget;
        widget->moveTo((getContentWidth() - widget->viewWidth()) * 0.5f, 50.0f, true);
    }
}
}
