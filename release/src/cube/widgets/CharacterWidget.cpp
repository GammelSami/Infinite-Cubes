// See cube/CharacterWidget.h.
#include <cmath>
#include <algorithm>
#include <sstream>
#include "cube/widgets/CharacterWidget.h"
#include "cube/Creature.h"
#include "cube/Game.h"
#include "cube/World.h"
#include "plasma/D3D11Engine.h"
#include "plasma/Engine.h"
#include "plasma/Node.h"
#include "cube/CreatureIds.h"
#include "plasma/EditFilter.h"

namespace cube {

CharacterWidget::CharacterWidget(Game *game, plasma::Node *node)
    : BaseWidget(game->m_engine, node, L""),
      m_game(game)
{
    setFont(L"resource1.dat");
}

void CharacterWidget::draw()
{
    // Computes the matrix without using the result.
    plasma::Matrix<float> matrix;
    m_node->getMatrix(matrix);

    // The player's name.
    setTextSize(16.0f);
    setTextStrokeRadius(4.0f);
    setTextStrokeColor(plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f));
    setTextColor(plasma::Vector<4, float>(1.0f, 0.8f, 0.2f, 1.0f));
    renderText(WidenString(std::string(m_game->m_world->m_localCreature->m_entityData.name)), 0.0f, 0.0f);

    std::wstringstream text;
    setTextColor(plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f));
    Creature *creature = m_game->m_world->m_localCreature;
    text << L"LEVEL " << creature->m_entityData.level;
    renderText(text.str(), 0.0f, 24.0f);

    // Weapon level: the rarity of the weapons in use as stars (two weapons are averaged unless one is two-handed),
    // over five grey stars.
    text.str(L"");
    text << L"Weapon Level";
    setTextColor(plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f));
    renderText(text.str(), 0.0f, 48.0f);
    float weaponLevel = 0.0f;
    creature = m_game->m_world->m_localCreature;
    if (creature->m_entityData.equipment.weaponLeft.category == ItemCategoryWeapon)
        weaponLevel += creature->m_entityData.equipment.weaponLeft.getRarity(creature->m_entityData.region);
    creature = m_game->m_world->m_localCreature;
    if (creature->m_entityData.equipment.weaponRight.category == ItemCategoryWeapon)
        weaponLevel += creature->m_entityData.equipment.weaponRight.getRarity(creature->m_entityData.region);
    if (!m_game->m_world->m_localCreature->hasTwoHandedWeapon())
        weaponLevel *= 0.5f;
    text.str(L"");
    for (int i = 0; i < 5; i++)
        text << L"\xb1";
    setTextColor(plasma::Vector<4, float>(0.3f, 0.3f, 0.3f, 1.0f));
    renderText(text.str(), 240.0f, 48.0f);
    text.str(L"");
    for (int i = 0; roundf(weaponLevel) >= i; i++)
        text << L"\xb1";
    setTextColor(rarityColor((int)roundf(weaponLevel)));
    renderText(text.str(), 240.0f, 48.0f);

    // Armor level: shoulders, chest (counted twice), hands and feet.
    text.str(L"");
    text << L"Armor Level";
    setTextColor(plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f));
    renderText(text.str(), 0.0f, 68.0f);
    float armorLevel = 0.0f;
    creature = m_game->m_world->m_localCreature;
    if (creature->m_entityData.equipment.shoulder.category == ItemCategoryShoulder)
        armorLevel += creature->m_entityData.equipment.shoulder.getRarity(creature->m_entityData.region);
    creature = m_game->m_world->m_localCreature;
    if (creature->m_entityData.equipment.chest.category == ItemCategoryChest)
        armorLevel += 2 * creature->m_entityData.equipment.chest.getRarity(creature->m_entityData.region);
    creature = m_game->m_world->m_localCreature;
    if (creature->m_entityData.equipment.hands.category == ItemCategoryHands)
        armorLevel += creature->m_entityData.equipment.hands.getRarity(creature->m_entityData.region);
    creature = m_game->m_world->m_localCreature;
    if (creature->m_entityData.equipment.feet.category == ItemCategoryFeet)
        armorLevel += creature->m_entityData.equipment.feet.getRarity(creature->m_entityData.region);
    armorLevel /= 5.0f;
    text.str(L"");
    for (int i = 0; i < 5; i++)
        text << L"\xb1";
    setTextColor(plasma::Vector<4, float>(0.3f, 0.3f, 0.3f, 1.0f));
    renderText(text.str(), 240.0f, 68.0f);
    text.str(L"");
    for (int i = 0; roundf(armorLevel) >= i; i++)
        text << L"\xb1";
    setTextColor(rarityColor((int)roundf(armorLevel)));
    renderText(text.str(), 240.0f, 68.0f);

    // Class and specialization.
    setTextSize(12.0f);
    setTextStrokeRadius(3.0f);
    setTextColor(plasma::Vector<4, float>(0.6f, 0.6f, 0.7f, 1.0f));
    text.str(L"");
    text << "Class";
    renderText(text.str(), 0.0f, 88.0f);
    text.str(L"");
    switch (m_game->m_world->m_localCreature->m_entityData.classType) {
    case ClassWarrior:
        text << "Warrior";
        break;
    case ClassRanger:
        text << "Ranger";
        break;
    case ClassMage:
        text << "Mage";
        break;
    case ClassRogue:
        text << "Rogue";
        break;
    }
    renderText(text.str(), 240.0f, 88.0f);
    text.str(L"");
    text << "Specialization";
    renderText(text.str(), 0.0f, 104.0f);
    text.str(L"");
    text << m_game->m_speech.getText(
        m_game->m_speech.specializationKey(m_game->m_world->m_localCreature->m_entityData.classType,
                                           m_game->m_world->m_localCreature->m_entityData.specialization),
        L"default", L"");
    renderText(text.str(), 240.0f, 104.0f);

    // The stats, the rows in alternating colours.
    plasma::Vector<4, float> rowColor(0.6f, 1.0f, 0.6f, 1.0f);
    plasma::Vector<4, float> otherColor(1.0f, 1.0f, 0.6f, 1.0f);
    text.str(L"");
    text << L"HIT POINTS";
    setTextColor(rowColor);
    renderText(text.str(), 0.0f, 124.0f);
    text.str(L"");
    text << (int)m_game->m_world->m_localCreature->maxHP();
    renderText(text.str(), 240.0f, 124.0f);

    std::swap(rowColor, otherColor);
    if (m_game->m_world->m_localCreature->m_entityData.classType == ClassMage) {
        text.str(L"");
        text << L"SPELL POWER ";
        setTextColor(rowColor);
        renderText(text.str(), 0.0f, 140.0f);
        float power = m_game->m_world->m_localCreature->spellPower(true);
        text.str(L"");
        text << roundTenth(power);
        renderText(text.str(), 240.0f, 140.0f);
    } else {
        text.str(L"");
        text << L"ATTACK POWER ";
        setTextColor(rowColor);
        renderText(text.str(), 0.0f, 140.0f);
        float power = m_game->m_world->m_localCreature->attackPower(true);
        text.str(L"");
        text << roundTenth(power);
        renderText(text.str(), 240.0f, 140.0f);
    }

    std::swap(rowColor, otherColor);
    text.str(L"");
    text << L"ARMOR";
    setTextColor(rowColor);
    renderText(text.str(), 0.0f, 156.0f);
    text.str(L"");
    text << roundTenth(m_game->m_world->m_localCreature->armor());
    renderText(text.str(), 240.0f, 156.0f);

    std::swap(rowColor, otherColor);
    text.str(L"");
    text << L"RESISTANCE";
    setTextColor(rowColor);
    renderText(text.str(), 0.0f, 172.0f);
    text.str(L"");
    text << roundTenth(m_game->m_world->m_localCreature->resistance());
    renderText(text.str(), 240.0f, 172.0f);

    std::swap(rowColor, otherColor);
    text.str(L"");
    text << L"CRITICAL";
    setTextColor(rowColor);
    renderText(text.str(), 0.0f, 188.0f);
    text.str(L"");
    creature = m_game->m_world->m_localCreature;
    text << roundTenth(creature->critical(nullptr, creature->isCaster()) * 100.0f) << L"%";
    renderText(text.str(), 240.0f, 188.0f);

    std::swap(rowColor, otherColor);
    text.str(L"");
    text << L"HASTE";
    setTextColor(rowColor);
    renderText(text.str(), 0.0f, 204.0f);
    text.str(L"");
    text << roundTenth((m_game->m_world->m_localCreature->tempo() - 1.0f) * 100.0f) << L"%";
    renderText(text.str(), 240.0f, 204.0f);

    std::swap(rowColor, otherColor);
    text.str(L"");
    text << L"REGENERATION";
    setTextColor(rowColor);
    renderText(text.str(), 0.0f, 220.0f);
    text.str(L"");
    text << roundTenth(m_game->m_world->m_localCreature->regeneration() * 100.0f) << L"%";
    renderText(text.str(), 240.0f, 220.0f);

    std::swap(rowColor, otherColor);
    text.str(L"");
    text << L"MANA GENERATION";
    setTextColor(rowColor);
    renderText(text.str(), 0.0f, 236.0f);
    text.str(L"");
    text << roundTenth(m_game->m_world->m_localCreature->manaGeneration() * 100.0f) << L"%";
    renderText(text.str(), 240.0f, 236.0f);

    // The travel skills, with the artifact bonus ("+N", green) after the name.
    rowColor = plasma::Vector<4, float>(0.45f, 0.6f, 1.0f, 1.0f);
    otherColor = plasma::Vector<4, float>(0.6f, 0.45f, 1.0f, 1.0f);
    int y = 240;
    for (int skill = ArtifactEffect_ClimbingSpeed; skill < ArtifactEffect_LampRadius + 1; skill++) {
        int artifactLevel = m_game->m_world->m_localCreature->m_artifactSkills[skill];
        std::swap(rowColor, otherColor);
        y += 20;
        text.str(L"");
        switch (skill) {
        case ArtifactEffect_HangGlidingSpeed:
            text << L"Hang Gliding Speed";
            break;
        case ArtifactEffect_ClimbingSpeed:
            text << L"Climbing Speed";
            break;
        case ArtifactEffect_SailingSpeed:
            text << L"Sailing Speed";
            break;
        case ArtifactEffect_SwimmingSpeed:
            text << L"Swimming Speed";
            break;
        case ArtifactEffect_DivingStamina:
            text << L"Diving Skill";
            break;
        case ArtifactEffect_RidingSpeed:
            text << L"Riding Speed";
            break;
        case ArtifactEffect_LampRadius:
            text << L"Light Radius";
            break;
        }
        setTextColor(rowColor);
        float nameEnd = renderText(text.str(), 0.0f, (float)y)[0];
        if (artifactLevel) {
            setTextColor(plasma::Vector<4, float>(0.5f, 1.0f, 0.5f, 1.0f));
            text.str(L"");
            text << L" +" << artifactLevel;
            renderText(text.str(), nameEnd, (float)y);
            setTextColor(rowColor);
        }
        text.str(L"");
        switch (skill) {
        case ArtifactEffect_HangGlidingSpeed:
            text << round(m_game->m_world->m_localCreature->hangGlidingSpeed() * 100.0f) << L"%";
            break;
        case ArtifactEffect_ClimbingSpeed:
            text << round(m_game->m_world->m_localCreature->climbingSpeed() * 100.0f) << L"%";
            break;
        case ArtifactEffect_SailingSpeed:
            text << round(m_game->m_world->m_localCreature->sailingSpeed() * 100.0f) << L"%";
            break;
        case ArtifactEffect_SwimmingSpeed:
            text << round(m_game->m_world->m_localCreature->swimmingSpeed() * 100.0f) << L"%";
            break;
        case ArtifactEffect_DivingStamina:
            text << roundTenth(m_game->m_world->m_localCreature->divingSkill() * 100.0f) << L" stam/s";
            break;
        case ArtifactEffect_RidingSpeed:
            text << round(m_game->m_world->m_localCreature->ridingSpeed() * 100.0f) << L"%";
            break;
        case ArtifactEffect_LampRadius:
            text << roundTenth(m_game->m_world->m_localCreature->lampDiameter() * 0.5) << L" m";
            break;
        }
        renderText(text.str(), 240.0f, (float)y);
    }
}

}
