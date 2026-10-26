// CharacterWidget draws character equipment and statistics.
#include "cube/widgets/CharacterWidget.h"
#include "cube/DefaultBlocks.h"
#include "cube/Creature.h"
#include "cube/CreatureIds.h"
#include "cube/GameController.h"
#include "cube/StatCurves.h"
#include "plasma/Engine.h"
#include "plasma/FontEngine.h"
#include "plasma/Font.h"
#include "plasma/StringConversion.h"
#include <math.h>
#include <sstream>
namespace cube {

CharacterWidget::CharacterWidget(plasma::Engine *engine, plasma::Node *node, GameController *game)
    : Widget(engine, node)
{
    m_game = game;
}

// The character sheet: the player's name, level, class and specialization, then power, HP, armour,
// resistance, critical chance and tempo (percentages), regeneration (a percentage of the level's
// base, with the raw value beside it), and the weapon and armour ratings: the equipment's item
// power against the player's level (both hands, halved with one weapon; the four armour pieces
// with the chest twice, over five), coloured from grey (below 0.8) through white, green, blue and
// purple to yellow (1.8 and more; the steps between at 1.1, 1.2 and 1.5). Its layout is in pixels:
// from 10, 20, the name (12 high, stroke 3, wrapped at 180) and the class line (12), then rows 16
// apart (20 between groups) of 10-pixel text, the values at x 150 (the raw regeneration at 200);
// colours RGBA. The regeneration shown is 100 plus ten times the player's, divided by the level
// curve 2^((level - 1) / 4).
void CharacterWidget::draw()
{
    plasma::Vector<2, int> position(10, 20);
    std::wstringstream text;
    float armorRating = 0.0f;
    Creature *player = m_game->m_player;
    text << widenString(std::string(player->m_entityData.name));
    m_engine->getFontEngine()->renderText(
        L"resource1.dat", text.str(), 0.0f, 2.0f, (float)position[0], (float)position[1], 12.0f, 3.0f,
        plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f).m_elements,
        plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f).m_elements,
        plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, (1 << plasma::TEXT_WRAP), 180.0f, true);
    m_engine->getFontEngine()->renderText(
        L"resource1.dat", text.str(), 0.0f, 2.0f, (float)position[0], (float)position[1], 12.0f, 0.0f,
        plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f).m_elements,
        plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
        plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, (1 << plasma::TEXT_WRAP), 180.0f, true);
    position[1] += 16;

    text.str(L"");
    text << L"LVL " << player->m_entityData.level;
    switch (player->m_entityData.classType) {
    case ClassWarrior:
        text << " Warrior";
        if (player->m_entityData.specialization == SpecializationBerserker)
            text << " | Berserker";
        else if (player->m_entityData.specialization == SpecializationGuardian)
            text << " | Guardian";
        break;
    case ClassRanger:
        text << " Ranger";
        if (player->m_entityData.specialization == SpecializationSniper)
            text << " | Sniper";
        else if (player->m_entityData.specialization == SpecializationScout)
            text << " | Scout";
        break;
    case ClassRogue:
        text << " Rogue";
        if (player->m_entityData.specialization == SpecializationAssassin)
            text << " | Assassin";
        else if (player->m_entityData.specialization == SpecializationNinja)
            text << " | Ninja";
        break;
    case ClassMage:
        text << " Mage";
        if (player->m_entityData.specialization == SpecializationFire)
            text << " | Fire";
        else if (player->m_entityData.specialization == SpecializationWater)
            text << " | Water";
        break;
    }
    m_engine->getFontEngine()->renderText(
        L"resource1.dat", text.str(), 0.0f, 2.0f, (float)position[0], (float)position[1], 12.0f, 3.0f,
        plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f).m_elements,
        plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f).m_elements,
        plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, plasma::TEXT_FLAGS_NONE, -1.0f, true);
    m_engine->getFontEngine()->renderText(
        L"resource1.dat", text.str(), 0.0f, 2.0f, (float)position[0], (float)position[1], 12.0f, 0.0f,
        plasma::Vector<4, float>(1.0f, 0.5f, 0.0f, 1.0f).m_elements,
        plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
        plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, plasma::TEXT_FLAGS_NONE, -1.0f, true);
    position[1] += 20;

    text.str(L"");
    text << L"Power";
    m_engine->getFontEngine()->renderText(
        L"resource1.dat", text.str(), 0.0f, 2.0f, (float)position[0], (float)position[1], 10.0f, 3.0f,
        plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f).m_elements,
        plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f).m_elements,
        plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, plasma::TEXT_FLAGS_NONE, -1.0f, true);
    m_engine->getFontEngine()->renderText(
        L"resource1.dat", text.str(), 0.0f, 2.0f, (float)position[0], (float)position[1], 10.0f, 0.0f,
        plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f).m_elements,
        plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
        plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, plasma::TEXT_FLAGS_NONE, -1.0f, true);
    text.str(L"");
    text << player->power();
    m_engine->getFontEngine()->renderText(
        L"resource1.dat", text.str(), 0.0f, 2.0f, (float)(position[0] + 150), (float)position[1], 10.0f, 3.0f,
        plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f).m_elements,
        plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f).m_elements,
        plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, plasma::TEXT_FLAGS_NONE, -1.0f, true);
    m_engine->getFontEngine()->renderText(
        L"resource1.dat", text.str(), 0.0f, 2.0f, (float)(position[0] + 150), (float)position[1], 10.0f, 0.0f,
        plasma::Vector<4, float>(0.3f, 0.6f, 1.0f, 1.0f).m_elements,
        plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
        plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, plasma::TEXT_FLAGS_NONE, -1.0f, true);
    position[1] += 20;
    text.str(L"");
    text << L"HP";
    m_engine->getFontEngine()->renderText(
        L"resource1.dat", text.str(), 0.0f, 2.0f, (float)position[0], (float)position[1], 10.0f, 3.0f,
        plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f).m_elements,
        plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f).m_elements,
        plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, plasma::TEXT_FLAGS_NONE, -1.0f, true);
    m_engine->getFontEngine()->renderText(
        L"resource1.dat", text.str(), 0.0f, 2.0f, (float)position[0], (float)position[1], 10.0f, 0.0f,
        plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f).m_elements,
        plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
        plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, plasma::TEXT_FLAGS_NONE, -1.0f, true);
    text.str(L"");
    text << roundTenth(player->maxHP());
    m_engine->getFontEngine()->renderText(
        L"resource1.dat", text.str(), 0.0f, 2.0f, (float)(position[0] + 150), (float)position[1], 10.0f, 3.0f,
        plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f).m_elements,
        plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f).m_elements,
        plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, plasma::TEXT_FLAGS_NONE, -1.0f, true);
    m_engine->getFontEngine()->renderText(
        L"resource1.dat", text.str(), 0.0f, 2.0f, (float)(position[0] + 150), (float)position[1], 10.0f, 0.0f,
        plasma::Vector<4, float>(0.3f, 0.6f, 1.0f, 1.0f).m_elements,
        plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
        plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, plasma::TEXT_FLAGS_NONE, -1.0f, true);
    position[1] += 16;
    text.str(L"");
    text << L"ARMOR";
    m_engine->getFontEngine()->renderText(
        L"resource1.dat", text.str(), 0.0f, 2.0f, (float)position[0], (float)position[1], 10.0f, 3.0f,
        plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f).m_elements,
        plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f).m_elements,
        plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, plasma::TEXT_FLAGS_NONE, -1.0f, true);
    m_engine->getFontEngine()->renderText(
        L"resource1.dat", text.str(), 0.0f, 2.0f, (float)position[0], (float)position[1], 10.0f, 0.0f,
        plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f).m_elements,
        plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
        plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, plasma::TEXT_FLAGS_NONE, -1.0f, true);
    text.str(L"");
    text << roundTenth(player->armor());
    m_engine->getFontEngine()->renderText(
        L"resource1.dat", text.str(), 0.0f, 2.0f, (float)(position[0] + 150), (float)position[1], 10.0f, 3.0f,
        plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f).m_elements,
        plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f).m_elements,
        plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, plasma::TEXT_FLAGS_NONE, -1.0f, true);
    m_engine->getFontEngine()->renderText(
        L"resource1.dat", text.str(), 0.0f, 2.0f, (float)(position[0] + 150), (float)position[1], 10.0f, 0.0f,
        plasma::Vector<4, float>(0.3f, 0.6f, 1.0f, 1.0f).m_elements,
        plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
        plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, plasma::TEXT_FLAGS_NONE, -1.0f, true);
    position[1] += 16;
    text.str(L"");
    text << L"RESI";
    m_engine->getFontEngine()->renderText(
        L"resource1.dat", text.str(), 0.0f, 2.0f, (float)position[0], (float)position[1], 10.0f, 3.0f,
        plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f).m_elements,
        plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f).m_elements,
        plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, plasma::TEXT_FLAGS_NONE, -1.0f, true);
    m_engine->getFontEngine()->renderText(
        L"resource1.dat", text.str(), 0.0f, 2.0f, (float)position[0], (float)position[1], 10.0f, 0.0f,
        plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f).m_elements,
        plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
        plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, plasma::TEXT_FLAGS_NONE, -1.0f, true);
    text.str(L"");
    text << roundTenth(player->resistance());
    m_engine->getFontEngine()->renderText(
        L"resource1.dat", text.str(), 0.0f, 2.0f, (float)(position[0] + 150), (float)position[1], 10.0f, 3.0f,
        plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f).m_elements,
        plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f).m_elements,
        plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, plasma::TEXT_FLAGS_NONE, -1.0f, true);
    m_engine->getFontEngine()->renderText(
        L"resource1.dat", text.str(), 0.0f, 2.0f, (float)(position[0] + 150), (float)position[1], 10.0f, 0.0f,
        plasma::Vector<4, float>(0.3f, 0.6f, 1.0f, 1.0f).m_elements,
        plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
        plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, plasma::TEXT_FLAGS_NONE, -1.0f, true);
    position[1] += 16;
    text.str(L"");
    text << L"CRIT";
    m_engine->getFontEngine()->renderText(
        L"resource1.dat", text.str(), 0.0f, 2.0f, (float)position[0], (float)position[1], 10.0f, 3.0f,
        plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f).m_elements,
        plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f).m_elements,
        plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, plasma::TEXT_FLAGS_NONE, -1.0f, true);
    m_engine->getFontEngine()->renderText(
        L"resource1.dat", text.str(), 0.0f, 2.0f, (float)position[0], (float)position[1], 10.0f, 0.0f,
        plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f).m_elements,
        plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
        plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, plasma::TEXT_FLAGS_NONE, -1.0f, true);
    text.str(L"");
    text << roundTenth(m_game->m_player->criticalChance() * 100.0f) << L"%";
    m_engine->getFontEngine()->renderText(
        L"resource1.dat", text.str(), 0.0f, 2.0f, (float)(position[0] + 150), (float)position[1], 10.0f, 3.0f,
        plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f).m_elements,
        plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f).m_elements,
        plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, plasma::TEXT_FLAGS_NONE, -1.0f, true);
    m_engine->getFontEngine()->renderText(
        L"resource1.dat", text.str(), 0.0f, 2.0f, (float)(position[0] + 150), (float)position[1], 10.0f, 0.0f,
        plasma::Vector<4, float>(0.3f, 0.6f, 1.0f, 1.0f).m_elements,
        plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
        plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, plasma::TEXT_FLAGS_NONE, -1.0f, true);
    position[1] += 16;
    text.str(L"");
    text << L"TEMPO";
    m_engine->getFontEngine()->renderText(
        L"resource1.dat", text.str(), 0.0f, 2.0f, (float)position[0], (float)position[1], 10.0f, 3.0f,
        plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f).m_elements,
        plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f).m_elements,
        plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, plasma::TEXT_FLAGS_NONE, -1.0f, true);
    m_engine->getFontEngine()->renderText(
        L"resource1.dat", text.str(), 0.0f, 2.0f, (float)position[0], (float)position[1], 10.0f, 0.0f,
        plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f).m_elements,
        plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
        plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, plasma::TEXT_FLAGS_NONE, -1.0f, true);
    text.str(L"");
    text << roundTenth(m_game->m_player->tempo() * 100.0f) << L"%";
    m_engine->getFontEngine()->renderText(
        L"resource1.dat", text.str(), 0.0f, 2.0f, (float)(position[0] + 150), (float)position[1], 10.0f, 3.0f,
        plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f).m_elements,
        plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f).m_elements,
        plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, plasma::TEXT_FLAGS_NONE, -1.0f, true);
    m_engine->getFontEngine()->renderText(
        L"resource1.dat", text.str(), 0.0f, 2.0f, (float)(position[0] + 150), (float)position[1], 10.0f, 0.0f,
        plasma::Vector<4, float>(0.3f, 0.6f, 1.0f, 1.0f).m_elements,
        plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
        plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, plasma::TEXT_FLAGS_NONE, -1.0f, true);
    position[1] += 16;
    text.str(L"");
    text << L"REG";
    m_engine->getFontEngine()->renderText(
        L"resource1.dat", text.str(), 0.0f, 2.0f, (float)position[0], (float)position[1], 10.0f, 3.0f,
        plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f).m_elements,
        plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f).m_elements,
        plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, plasma::TEXT_FLAGS_NONE, -1.0f, true);
    m_engine->getFontEngine()->renderText(
        L"resource1.dat", text.str(), 0.0f, 2.0f, (float)position[0], (float)position[1], 10.0f, 0.0f,
        plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f).m_elements,
        plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
        plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, plasma::TEXT_FLAGS_NONE, -1.0f, true);
    text.str(L"");
    text << roundTenth(m_game->m_player->regeneration() * 10.0f /
                           pow(2.0f, (m_game->m_player->m_entityData.level - 1) * 0.25f) +
                       100.0f)
         << L"%";
    m_engine->getFontEngine()->renderText(
        L"resource1.dat", text.str(), 0.0f, 2.0f, (float)(position[0] + 150), (float)position[1], 10.0f, 3.0f,
        plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f).m_elements,
        plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f).m_elements,
        plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, plasma::TEXT_FLAGS_NONE, -1.0f, true);
    m_engine->getFontEngine()->renderText(
        L"resource1.dat", text.str(), 0.0f, 2.0f, (float)(position[0] + 150), (float)position[1], 10.0f, 0.0f,
        plasma::Vector<4, float>(0.3f, 0.6f, 1.0f, 1.0f).m_elements,
        plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
        plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, plasma::TEXT_FLAGS_NONE, -1.0f, true);
    text.str(L"");
    text << L"(" << roundTenth(m_game->m_player->regeneration()) << L")";
    m_engine->getFontEngine()->renderText(
        L"resource1.dat", text.str(), 0.0f, 2.0f, (float)(position[0] + 200), (float)position[1], 10.0f, 3.0f,
        plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f).m_elements,
        plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f).m_elements,
        plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, plasma::TEXT_FLAGS_NONE, -1.0f, true);
    m_engine->getFontEngine()->renderText(
        L"resource1.dat", text.str(), 0.0f, 2.0f, (float)(position[0] + 200), (float)position[1], 10.0f, 0.0f,
        plasma::Vector<4, float>(0.3f, 0.6f, 1.0f, 1.0f).m_elements,
        plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
        plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, plasma::TEXT_FLAGS_NONE, -1.0f, true);
    position[1] += 16;

    text.str(L"");
    text << L"Weapon Rating";
    m_engine->getFontEngine()->renderText(
        L"resource1.dat", text.str(), 0.0f, 2.0f, (float)position[0], (float)position[1], 10.0f, 3.0f,
        plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f).m_elements,
        plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f).m_elements,
        plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, plasma::TEXT_FLAGS_NONE, -1.0f, true);
    m_engine->getFontEngine()->renderText(
        L"resource1.dat", text.str(), 0.0f, 2.0f, (float)position[0], (float)position[1], 10.0f, 0.0f,
        plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f).m_elements,
        plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
        plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, plasma::TEXT_FLAGS_NONE, -1.0f, true);
    // Both weapons' power against the level, halved without a two-handed weapon.
    float weaponRating = 0.0f;
    Equipment &equipment = m_game->m_player->m_entityData.equipment;
    if (equipment.weaponLeft.category == ItemCategoryWeapon)
        weaponRating += itemPowerScale(equipment.weaponLeft.level, equipment.weaponLeft.rarity) /
                        itemPowerScale((float)m_game->m_player->m_entityData.level, ItemRarityWhite);
    if (equipment.weaponRight.category == ItemCategoryWeapon)
        weaponRating += itemPowerScale(equipment.weaponRight.level, equipment.weaponRight.rarity) /
                        itemPowerScale((float)m_game->m_player->m_entityData.level, ItemRarityWhite);
    if (!m_game->m_player->hasTwoHandedWeapon())
        weaponRating *= 0.5f;
    plasma::Vector<4, float> weaponRatingColor(1.0f, 1.0f, 1.0f, 1.0f);
    if (weaponRating < 0.8f)
        weaponRatingColor = plasma::Vector<4, float>(0.7f, 0.7f, 0.7f, 1.0f);
    else if (weaponRating < 1.1f)
        weaponRatingColor = plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f);
    else if (weaponRating < 1.2f)
        weaponRatingColor = plasma::Vector<4, float>(0.0f, 1.0f, 0.0f, 1.0f);
    else if (weaponRating < 1.5f)
        weaponRatingColor = plasma::Vector<4, float>(0.25f, 0.25f, 1.0f, 1.0f);
    else if (weaponRating < 1.8f)
        weaponRatingColor = plasma::Vector<4, float>(0.5f, 0.0f, 1.0f, 1.0f);
    else
        weaponRatingColor = plasma::Vector<4, float>(1.0f, 1.0f, 0.0f, 1.0f);
    text.str(L"");
    text << (int)(weaponRating * 100.0f + 0.5f) << L"%";
    m_engine->getFontEngine()->renderText(
        L"resource1.dat", text.str(), 0.0f, 2.0f, (float)(position[0] + 150), (float)position[1], 10.0f, 3.0f,
        plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f).m_elements,
        plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f).m_elements,
        plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, plasma::TEXT_FLAGS_NONE, -1.0f, true);
    m_engine->getFontEngine()->renderText(
        L"resource1.dat", text.str(), 0.0f, 2.0f, (float)(position[0] + 150), (float)position[1], 10.0f, 0.0f,
        weaponRatingColor.m_elements, plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
        plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, plasma::TEXT_FLAGS_NONE, -1.0f, true);
    position[1] += 16;

    text.str(L"");
    text << L"Armor Rating";
    m_engine->getFontEngine()->renderText(
        L"resource1.dat", text.str(), 0.0f, 2.0f, (float)position[0], (float)position[1], 10.0f, 3.0f,
        plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f).m_elements,
        plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f).m_elements,
        plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, plasma::TEXT_FLAGS_NONE, -1.0f, true);
    m_engine->getFontEngine()->renderText(
        L"resource1.dat", text.str(), 0.0f, 2.0f, (float)position[0], (float)position[1], 10.0f, 0.0f,
        plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f).m_elements,
        plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
        plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, plasma::TEXT_FLAGS_NONE, -1.0f, true);
    // The armour pieces' power against the level: shoulders, the chest twice, hands and feet, over
    // five.
    if (equipment.shoulder.category == ItemCategoryShoulder)
        armorRating += itemPowerScale(equipment.shoulder.level, equipment.shoulder.rarity) /
                       itemPowerScale((float)m_game->m_player->m_entityData.level, ItemRarityWhite);
    if (equipment.chest.category == ItemCategoryChest)
        armorRating += itemPowerScale(equipment.chest.level, equipment.chest.rarity) * 2.0f /
                       itemPowerScale((float)m_game->m_player->m_entityData.level, ItemRarityWhite);
    if (equipment.hands.category == ItemCategoryHands)
        armorRating += itemPowerScale(equipment.hands.level, equipment.hands.rarity) /
                       itemPowerScale((float)m_game->m_player->m_entityData.level, ItemRarityWhite);
    if (equipment.feet.category == ItemCategoryFeet)
        armorRating += itemPowerScale(equipment.feet.level, equipment.feet.rarity) /
                       itemPowerScale((float)m_game->m_player->m_entityData.level, ItemRarityWhite);
    armorRating /= 5.0f;
    plasma::Vector<4, float> armorRatingColor(1.0f, 1.0f, 1.0f, 1.0f);
    if (armorRating < 0.8f)
        armorRatingColor = plasma::Vector<4, float>(0.7f, 0.7f, 0.7f, 1.0f);
    else if (armorRating < 1.1f)
        armorRatingColor = plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f);
    else if (armorRating < 1.2f)
        armorRatingColor = plasma::Vector<4, float>(0.0f, 1.0f, 0.0f, 1.0f);
    else if (armorRating < 1.5f)
        armorRatingColor = plasma::Vector<4, float>(0.25f, 0.25f, 1.0f, 1.0f);
    else if (armorRating < 1.8f)
        armorRatingColor = plasma::Vector<4, float>(0.5f, 0.0f, 1.0f, 1.0f);
    else
        armorRatingColor = plasma::Vector<4, float>(1.0f, 1.0f, 0.0f, 1.0f);
    text.str(L"");
    text << (int)(armorRating * 100.0f + 0.5f) << L"%";
    m_engine->getFontEngine()->renderText(
        L"resource1.dat", text.str(), 0.0f, 2.0f, (float)(position[0] + 150), (float)position[1], 10.0f, 3.0f,
        plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f).m_elements,
        plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f).m_elements,
        plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, plasma::TEXT_FLAGS_NONE, -1.0f, true);
    m_engine->getFontEngine()->renderText(
        L"resource1.dat", text.str(), 0.0f, 2.0f, (float)(position[0] + 150), (float)position[1], 10.0f, 0.0f,
        armorRatingColor.m_elements, plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
        plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, plasma::TEXT_FLAGS_NONE, -1.0f, true);
}

// Rounds to one decimal, halves away from zero (the item descriptions' statistics); a negative
// value that rounds to zero gives 0, not -0.
float roundTenth(float value)
{
    if (value < 0.0f) {
        float rounded = -roundTenth(-value);
        if (rounded == 0.0f)
            rounded = 0.0f;
        return rounded;
    }
    return (int)(value * 10.0f + 0.5f) * 0.1f;
}
}
