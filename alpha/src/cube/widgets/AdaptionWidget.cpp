// AdaptionWidget shows equipment adaptation controls.
#include "cube/DefaultBlocks.h"
#include "cube/widgets/AdaptionWidget.h"
#include "cube/GameController.h"
#include "cube/Creature.h"
#include "plasma/Node.h"
#include "plasma/Transformation.h"
#include "plasma/Display.h"
#include "plasma/FontEngine.h"
#include "plasma/Font.h"
#include "plasma/Engine.h"
#include "cube/Sprite.h"
#include "cube/CubeShader.h"
#include <sstream>
namespace cube {

AdaptionWidget::AdaptionWidget(plasma::Engine *engine, plasma::Node *node, GameController *game,
                               plasma::Node *itemFrame, plasma::Node *rightArrow)
    : Widget(engine, node),
      m_item(0),
      m_inventoryTabIndex(-1),
      m_inventoryItemIndex(-1),
      m_game(game),
      m_itemFrame(itemFrame),
      m_rightArrow(rightArrow)
{
    if (m_itemFrame) {
        m_itemFrame = m_itemFrame->instance(node);
        m_itemFrame->setTransformation(m_itemFrame->m_transformation->clone());
        m_itemFrame->setDisplay(m_itemFrame->m_display->clone());
    }
    if (m_rightArrow) {
        m_rightArrow = m_rightArrow->instance(node);
        m_rightArrow->setTransformation(m_rightArrow->m_transformation->clone());
        m_rightArrow->setDisplay(m_rightArrow->m_display->clone());
    }
}

// The action buttons' hit strips occupy the bottom thirty pixels.
bool AdaptionWidget::isPointerOverGoodbye()
{
    plasma::Vector<2, float> pointer = getPointerOffset();
    return pointer[1] > getContentHeight() - 30.0f && pointer[1] < getContentHeight() &&
           pointer[0] > getContentWidth() * 0.5f && pointer[0] < getContentWidth();
}

bool AdaptionWidget::isPointerOverUpgrade()
{
    if (!getItem())
        return false;
    plasma::Vector<2, float> pointer = getPointerOffset();
    return pointer[1] > getContentHeight() - 30.0f && pointer[1] < getContentHeight() &&
           pointer[0] < getContentWidth() * 0.5f && pointer[0] > 0.0f;
}

// In pixels: the item frame centred at y 50, the arrow 10 left of the centre at y 230.
void AdaptionWidget::resized()
{
    if (m_itemFrame) {
        plasma::Widget *widget = m_itemFrame->m_widget;
        widget->moveTo((getContentWidth() - widget->viewWidth()) * 0.5f, 50.0f, true);
    }
    if (m_rightArrow) {
        plasma::Widget *widget = m_rightArrow->m_widget;
        widget->moveTo(getContentWidth() * 0.5f - 10.0f, 230.0f, true);
    }
}
}

namespace cube {
// The platinum coins it costs to adapt the item to the player's level: for each level it lacks, 2
// * 2^((rarity - 1 + its level) / 4). The exponent keeps the item's own level, so every step costs
// the same.
int AdaptionWidget::getCost()
{
    Item *item = getItem();
    if (!item)
        return 0;
    int cost = 0;
    for (int level = item->level; level < m_game->m_player->m_entityData.level; level++)
        cost += pow(2.0f, (item->rarity - 1 + item->level) * 0.25f) * 2.0f;
    return cost;
}

Item *AdaptionWidget::getItem()
{
    Item *item;
    if (m_item) {
        item = m_item;
    } else {
        Creature *player = m_game->m_player;
        if (player->m_inventory.m_inventoryTabs.empty())
            return 0;
        if (m_inventoryTabIndex < 0 || m_inventoryTabIndex >= (int)player->m_inventory.m_inventoryTabs.size())
            return 0;
        if (m_inventoryItemIndex < 0)
            return 0;
        std::vector<ItemStack> &tab = player->m_inventory.m_inventoryTabs[m_inventoryTabIndex];
        if (m_inventoryItemIndex >= (int)tab.size())
            return 0;
        if (!tab[m_inventoryItemIndex].quantity)
            return 0;
        item = &tab[m_inventoryItemIndex].item;
    }
    if (item->category != ItemCategoryWeapon && item->category != ItemCategoryShoulder &&
        item->category != ItemCategoryHands && item->category != ItemCategoryChest &&
        item->category != ItemCategoryFeet && item->category != ItemCategoryNeck && item->category != ItemCategoryRing)
        return 0;
    return item->flags & (1 << ItemFlagAdapted) ? 0 : item;
}

// The item turning in the item frame with its description, the adapted item's description beside
// it, the Adapt and Goodbye buttons (cyan under the pointer; Adapt grey without an item) and the
// cost in platinum coins. Its layout is in pixels: the item at scale 0.06, its description at 15,
// 180 (300 wide), the adapted one 50 further right; the title at 15, 25 (12 high, stroke 3); Adapt
// and Goodbye at a third and two thirds of the width, 20 up from the bottom; the cost 50 up from
// the bottom, the amount at 220 (10 high, stroke 2.5) and its label at 15 (stroke 2); colours RGBA
// (grey 0.7 without an item, cyan under the pointer, platinum purple).
void AdaptionWidget::draw()
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
        Item adapted = *item;
        adapted.level = m_game->m_player->m_entityData.level;
        m_game->writeItemInfo(&adapted, (int)(getContentWidth() * 0.5f + 50.0f), 180, 1.0f, 300, false, false, false);
        m_rightArrow->setVisible(true);
    } else {
        m_rightArrow->setVisible(false);
    }

    std::wstringstream text;
    text << L"Adaption";
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
    text << "Adapt";
    m_engine->getFontEngine()->renderText(
        L"resource1.dat", text.str(), 0.0f, 0.0f, getContentWidth() / 3.0f, getContentHeight() - 20.0f, 12.0f, 3.0f,
        plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f).m_elements,
        plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f).m_elements,
        plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, (1 << plasma::TEXT_CENTER), -1.0f, true);
    const float *upgradeColor = !item ? plasma::Vector<4, float>(0.7f, 0.7f, 0.7f, 1.0f).m_elements
                                      : (isPointerOverUpgrade() ? plasma::Vector<4, float>(0.0f, 1.0f, 1.0f, 1.0f)
                                                                : plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f))
                                            .m_elements;
    m_engine->getFontEngine()->renderText(
        L"resource1.dat", text.str(), 0.0f, 0.0f, getContentWidth() / 3.0f, getContentHeight() - 20.0f, 12.0f, 0.0f,
        upgradeColor, plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
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

    int y = (int)(getContentHeight() - 50.0f);
    text.str(L"");
    text << getCost() << " Platinum Coins ";
    m_engine->getFontEngine()->renderText(L"resource1.dat", text.str(), 0.0f, 0.0f, 220.0f, (float)y, 10.0f, 2.5f,
                                          plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f).m_elements,
                                          plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f).m_elements,
                                          plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
                                          (1 << plasma::TEXT_RIGHT), -1.0f, true);
    m_engine->getFontEngine()->renderText(L"resource1.dat", text.str(), 0.0f, 0.0f, 220.0f, (float)y, 10.0f, 0.0f,
                                          plasma::Vector<4, float>(0.5f, 0.2f, 1.0f, 1.0f).m_elements,
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
