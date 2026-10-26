// PreviewWidget draws item and skill descriptions.
#include "cube/DefaultBlocks.h"
#include "cube/widgets/PreviewWidget.h"
#include <sstream>
#include "cube/CubeShader.h"
#include "cube/GameController.h"
#include "cube/Creature.h"
#include "plasma/Engine.h"
#include "plasma/FontEngine.h"
#include "plasma/Font.h"
#include "plasma/Node.h"
#include "plasma/Transformation.h"
namespace cube {

PreviewWidget::PreviewWidget(plasma::Engine *engine, plasma::Node *node, GameController *game)
    : Widget(engine, node),
      m_item(0),
      m_itemInfoFlag(false),
      m_skill(-1),
      m_skillLevel(0),
      m_specialization(-1),
      m_game(game)
{}

// The preview beside the inventory: without an item the hovered skill's or specialization's text;
// with one its model (turning), the stars of its rarity, its description, the price while a
// vendor's offer is hovered, and the descriptions of the items it would replace. Its layout is in
// pixels from the widget's pivot: the texts 14, 25 in (the skills' 275 wide, the item's 280), the
// model 150, 150 in at scale 0.1 (depth offset -0.05), the price from 290, 280 leftwards, 40 a
// coin (10 high, stroke 2, in the coins' RGBA colours), "Currently equipped" at -190, 20 (8 high,
// stroke 3), the equipped items' descriptions from -240, 40 (230 wide), 130 apart.
void PreviewWidget::draw()
{
    m_node->findNode(L"star1")->setVisible(false);
    m_node->findNode(L"star2")->setVisible(false);
    m_node->findNode(L"star3")->setVisible(false);
    m_node->findNode(L"star4")->setVisible(false);
    if (!m_item) {
        if (m_specialization >= 0) {
            plasma::Vector<2, float> pivot = m_node->transformation()->m_pivot.value();
            m_engine->beginTextDrawing(m_node);
            m_game->writeSpecializationInfo(m_game->m_player->m_entityData.classType, m_specialization,
                                            (int)(pivot[0] + 14.0f), (int)(pivot[1] + 25.0f), 1.0f, 275);
            m_engine->endTextDrawing(m_node);
        }
        if (m_skill >= 0) {
            plasma::Vector<2, float> pivot = m_node->transformation()->m_pivot.value();
            m_engine->beginTextDrawing(m_node);
            m_game->writeSkillInfo(m_skill, m_skillLevel, (int)(pivot[0] + 14.0f), (int)(pivot[1] + 25.0f), 1.0f, 275);
            m_engine->endTextDrawing(m_node);
        }
        return;
    }
    switch (m_item->rarity) {
    case ItemRarityGreen:
        m_node->findNode(L"star1")->setVisible(true);
        m_node->findNode(L"star2")->setVisible(false);
        m_node->findNode(L"star3")->setVisible(false);
        m_node->findNode(L"star4")->setVisible(false);
        break;
    case ItemRarityBlue:
        m_node->findNode(L"star1")->setVisible(false);
        m_node->findNode(L"star2")->setVisible(true);
        m_node->findNode(L"star3")->setVisible(false);
        m_node->findNode(L"star4")->setVisible(false);
        break;
    case ItemRarityPurple:
        m_node->findNode(L"star1")->setVisible(false);
        m_node->findNode(L"star2")->setVisible(false);
        m_node->findNode(L"star3")->setVisible(true);
        m_node->findNode(L"star4")->setVisible(false);
        break;
    case ItemRarityYellow:
        m_node->findNode(L"star1")->setVisible(false);
        m_node->findNode(L"star2")->setVisible(false);
        m_node->findNode(L"star3")->setVisible(false);
        m_node->findNode(L"star4")->setVisible(true);
        break;
    default:
        m_node->findNode(L"star1")->setVisible(false);
        m_node->findNode(L"star2")->setVisible(false);
        m_node->findNode(L"star3")->setVisible(false);
        m_node->findNode(L"star4")->setVisible(false);
        break;
    }
    if (m_game->m_world.m_spriteManager.itemSprite(m_item)) {
        m_game->m_device->SetRenderState(D3DRS_ZENABLE, TRUE);
        m_game->m_device->SetRenderState(D3DRS_ZFUNC, D3DCMP_LESS);
        plasma::Vector<4, float> lightPositions[PointLightSourceCount];
        plasma::Vector<3, float> lightColors[PointLightSourceCount];
        for (int i = 0; i < PointLightSourceCount; i++) {
            lightPositions[i] = plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f);
            lightColors[i] = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
        }
        m_game->m_shader->apply();
        m_game->m_shader->setPointLights(lightPositions, lightColors);
        m_game->m_shader->setMaterialColor(plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f));
        plasma::Vector<2, float> center = m_node->m_matrix.transform(m_node->transformation()->m_pivot.value());
        m_game->drawItem(center[0] + 150.0f, center[1] + 150.0f, m_game->m_previewRotation, 0.1f, m_item, -0.05f);
    }
    m_game->m_device->SetRenderState(D3DRS_ZENABLE, FALSE);
    plasma::Vector<2, float> pivot = m_node->transformation()->m_pivot.value();
    m_game->writeItemInfo(m_item, (int)(pivot[0] + 14.0f), (int)(pivot[1] + 25.0f), 1.0f, 280, true, true, false);
    if (m_game->hasHoveredTradeItem()) {
        // The price in coins, right to left: 100 copper make a silver, 100 silver a gold; coins of none
        // are left out.
        int price = m_item->price();
        int copper = price % 100;
        int silver = price / 100 % 100;
        int gold = price / 100 / 100;
        std::wstringstream text;
        int x = (int)(pivot[0] + 290.0f);
        int y = (int)(pivot[1] + 280.0f);
        if (copper) {
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
        }
        if (silver) {
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
        }
        if (gold) {
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
        }
    }
    if (!m_equippedItems.empty()) {
        std::wstring caption(L"Currently equipped");
        m_engine->getFontEngine()->renderText(
            L"resource1.dat", caption, 0.0f, 0.0f, pivot[0] - 190.0f, pivot[1] + 20.0f, 8.0f, 3.0f,
            plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f).m_elements,
            plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f).m_elements,
            plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, plasma::TEXT_FLAGS_NONE, -1.0f, true);
        m_engine->getFontEngine()->renderText(
            L"resource1.dat", caption, 0.0f, 0.0f, pivot[0] - 190.0f, pivot[1] + 20.0f, 8.0f, 0.0f,
            plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f).m_elements,
            plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
            plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, plasma::TEXT_FLAGS_NONE, -1.0f, true);
        pivot[1] += 10.0f;
        for (std::list<Item *>::iterator it = m_equippedItems.begin(); it != m_equippedItems.end(); ++it) {
            m_game->writeItemInfo(*it, (int)(pivot[0] - 240.0f), (int)(pivot[1] + 30.0f), 1.0f, 230, true, true,
                                  m_itemInfoFlag);
            pivot[1] += 130.0f;
        }
    }
}
}
