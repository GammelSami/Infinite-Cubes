// SkillWidget displays skill levels, specialization and learning costs.
#include "cube/DefaultBlocks.h"
#include "cube/widgets/SkillWidget.h"
#include "cube/GameController.h"
#include "cube/Creature.h"
#include "cube/StatCurves.h"
#include "plasma/FontEngine.h"
#include "plasma/Font.h"
#include "plasma/Engine.h"
#include <sstream>
#include <string.h>
namespace cube {
// The class master's prices (anonymous: this file's own), times the item power of a white item of
// the player's level: a skill point given back, and a change of specialization.
namespace {
enum {
    SkillRefundCost = 10
};
enum {
    SpecializationChangeCost = 100
};
}

SkillWidget::SkillWidget(plasma::Engine *engine, plasma::Node *node, GameController *game)
    : Widget(engine, node),
      m_game(game)
{
    m_specialization = SpecializationFirst;
    memset(m_skillLevels, 0, sizeof(m_skillLevels));
}

// The skill screen's texts: the points spent of those the level gives, the cost of the changes in
// coins (with what prevents them), and the Learn and Cancel buttons while the shown levels differ
// from the player's (Learn grey when it cannot be paid or no trainer is near). Its layout is in
// pixels: the title at 15, 25 (12 high, stroke 3), the points at 15, 45 (10 high, stroke 2), the
// cost 65 up from the bottom with the copper, silver and gold amounts right-aligned at 220, 180
// and 140, the actions Learn and Cancel at a third and two thirds of the width, 20 up from the
// bottom (their hit strips are the bottom 30 pixels); colours RGBA.
void SkillWidget::draw()
{
    std::wstringstream text;
    text << L"Skills";
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
    int spent = 0;
    for (int i = 0; i < SkillCount; i++)
        spent += m_skillLevels[i];
    text << "Points: " << spent << "/"
         << m_game->m_player->m_entityData.level * SkillPointsPerLevel - SkillPointsPerLevel;
    m_engine->getFontEngine()->renderText(L"resource1.dat", text.str(), 0.0f, 0.0f, 15.0f, 45.0f, 10.0f, 2.0f,
                                          plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f).m_elements,
                                          plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f).m_elements,
                                          plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
                                          plasma::TEXT_FLAGS_NONE, -1.0f, true);
    m_engine->getFontEngine()->renderText(L"resource1.dat", text.str(), 0.0f, 0.0f, 15.0f, 45.0f, 10.0f, 0.0f,
                                          plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f).m_elements,
                                          plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
                                          plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
                                          plasma::TEXT_FLAGS_NONE, -1.0f, true);

    bool changed = false;
    Creature *player = m_game->m_player;
    for (int i = 0; i < SkillCount; i++) {
        if (m_skillLevels[i] != player->m_entityData.skillLevels[i]) {
            changed = true;
            break;
        }
    }
    bool modified = changed;
    if (m_specialization != player->m_entityData.specialization)
        modified = true;
    int cost = getCost();
    bool affordable = cost <= m_game->m_player->m_inventory.m_copperBalance;
    if (cost > 0) {
        // 100 copper make a silver, 100 silver a gold.
        int copper = cost % 100;
        int silver = cost / 100 % 100;
        int gold = cost / 100 / 100;
        int y = (int)(getContentHeight() - 65.0f);
        if (!affordable) {
            text.str(L"");
            text << "Not enough money.";
            m_engine->getFontEngine()->renderText(L"resource1.dat", text.str(), 0.0f, 0.0f, 15.0f, (float)y, 10.0f,
                                                  2.0f, plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f).m_elements,
                                                  plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f).m_elements,
                                                  plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
                                                  plasma::TEXT_FLAGS_NONE, -1.0f, true);
            m_engine->getFontEngine()->renderText(L"resource1.dat", text.str(), 0.0f, 0.0f, 15.0f, (float)y, 10.0f,
                                                  0.0f, plasma::Vector<4, float>(1.0f, 0.25f, 0.25f, 1.0f).m_elements,
                                                  plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
                                                  plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
                                                  plasma::TEXT_FLAGS_NONE, -1.0f, true);
        } else if (!m_game->isClassMasterNearby()) {
            text.str(L"");
            text << "Requires class trainer.";
            m_engine->getFontEngine()->renderText(L"resource1.dat", text.str(), 0.0f, 0.0f, 15.0f, (float)y, 10.0f,
                                                  2.0f, plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f).m_elements,
                                                  plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f).m_elements,
                                                  plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
                                                  plasma::TEXT_FLAGS_NONE, -1.0f, true);
            m_engine->getFontEngine()->renderText(L"resource1.dat", text.str(), 0.0f, 0.0f, 15.0f, (float)y, 10.0f,
                                                  0.0f, plasma::Vector<4, float>(1.0f, 0.25f, 0.25f, 1.0f).m_elements,
                                                  plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
                                                  plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
                                                  plasma::TEXT_FLAGS_NONE, -1.0f, true);
            affordable = false;
        }
        y += 20;
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
    if (modified) {
        text.str(L"");
        text << "Learn";
        m_engine->getFontEngine()->renderText(
            L"resource1.dat", text.str(), 0.0f, 0.0f, getContentWidth() / 3.0f, getContentHeight() - 20.0f, 12.0f, 3.0f,
            plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f).m_elements,
            plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f).m_elements,
            plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, (1 << plasma::TEXT_CENTER), -1.0f, true);
        // Cyan under the pointer, grey while the cost cannot be paid.
        m_engine->getFontEngine()->renderText(
            L"resource1.dat", text.str(), 0.0f, 0.0f, getContentWidth() / 3.0f, getContentHeight() - 20.0f, 12.0f, 0.0f,
            (affordable ? (isPointerOverLearn() ? plasma::Vector<4, float>(0.0f, 1.0f, 1.0f, 1.0f)
                                                : plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f))
                        : plasma::Vector<4, float>(0.5f, 0.5f, 0.5f, 1.0f))
                .m_elements,
            plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
            plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, (1 << plasma::TEXT_CENTER), -1.0f, true);
        text.str(L"");
        text << "Cancel";
        m_engine->getFontEngine()->renderText(
            L"resource1.dat", text.str(), 0.0f, 0.0f, getContentWidth() * 2.0f / 3.0f, getContentHeight() - 20.0f,
            12.0f, 3.0f, plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f).m_elements,
            plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f).m_elements,
            plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, (1 << plasma::TEXT_CENTER), -1.0f, true);
        m_engine->getFontEngine()->renderText(L"resource1.dat", text.str(), 0.0f, 0.0f, getContentWidth() * 2.0f / 3.0f,
                                              getContentHeight() - 20.0f, 12.0f, 0.0f,
                                              (isPointerOverCancel() ? plasma::Vector<4, float>(0.0f, 1.0f, 1.0f, 1.0f)
                                                                     : plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f))
                                                  .m_elements,
                                              plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
                                              plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
                                              (1 << plasma::TEXT_CENTER), -1.0f, true);
    }
}

bool SkillWidget::isPointerOverCancel()
{
    bool changed = false;
    Creature *player = m_game->m_player;
    for (int i = 0; i < SkillCount; i++) {
        if (m_skillLevels[i] != player->m_entityData.skillLevels[i]) {
            changed = true;
            break;
        }
    }
    if (m_specialization == player->m_entityData.specialization && !changed)
        return false;
    plasma::Vector<2, float> pointer = getPointerOffset();
    return pointer[1] > getContentHeight() - 30.0f && pointer[1] < getContentHeight() &&
           pointer[0] > getContentWidth() * 0.5f && pointer[0] < getContentWidth();
}

bool SkillWidget::isPointerOverLearn()
{
    bool changed = false;
    Creature *player = m_game->m_player;
    for (int i = 0; i < SkillCount; i++) {
        if (m_skillLevels[i] != player->m_entityData.skillLevels[i]) {
            changed = true;
            break;
        }
    }
    if (m_specialization != player->m_entityData.specialization || changed) {
        int cost = getCost();
        if (cost <= m_game->m_player->m_inventory.m_copperBalance && (cost <= 0 || m_game->isClassMasterNearby())) {
            plasma::Vector<2, float> pointer = getPointerOffset();
            return pointer[1] > getContentHeight() - 30.0f && pointer[1] < getContentHeight() &&
                   pointer[0] < getContentWidth() * 0.5f && pointer[0] > 0.0f;
        }
    }
    return false;
}

int SkillWidget::getCost()
{
    float cost = 0.0f;
    for (int i = 0; i < SkillCount; i++) {
        Creature *player = m_game->m_player;
        if (m_skillLevels[i] < player->m_entityData.skillLevels[i])
            cost += itemPowerScale((float)player->m_entityData.level, ItemRarityWhite) *
                    (float)(player->m_entityData.skillLevels[i] - m_skillLevels[i]) * SkillRefundCost;
    }
    if (m_specialization != m_game->m_player->m_entityData.specialization && m_game->m_player->m_entityData.level > 1)
        cost += itemPowerScale((float)m_game->m_player->m_entityData.level, ItemRarityWhite) * SpecializationChangeCost;
    return (int)cost;
}
}
