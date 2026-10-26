// See cube/HelpWidget.h.
#include <string>
#include "cube/Creature.h"
#include "cube/Game.h"
#include "cube/widgets/HelpWidget.h"
#include "cube/Speech.h"
#include "cube/World.h"
#include "plasma/D3D11Engine.h"
#include "plasma/Node.h"
#include "plasma/Transformation.h"
#include "cube/CreatureIds.h"
#include "plasma/EditFilter.h"

namespace cube {

HelpWidget::HelpWidget(Game *game, plasma::Node *node)
    : BaseWidget(game->m_engine, node, L""),
      m_game(game)
{
    setFont(L"resource1.dat");
}

void HelpWidget::draw()
{
    // Rows 20 apart: the key in cyan at the left, the action at 180, its cost at 320.
    plasma::Matrix<float> matrix;
    m_node->getMatrix(matrix);
    setTextSize(12.0f);
    setTextStrokeRadius(3.0f);
    setTextStrokeColor(plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f));
    bool alternative = m_game->m_controlStates[Controls::ButtonClassSkill];
    int attackSkill = m_game->m_world->m_localCreature->nextAttackSkill(alternative);
    setTextColor(plasma::Vector<4, float>(0.25f, 1.0f, 1.0f, 1.0f));
    renderText(m_game->m_keyNames.getFullName(m_game->m_controls.buttons[Controls::ButtonAttack]), 0.0f, 0.0f);
    setTextColor(plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f));
    static const plasma::Vector<4, float> staminaColor(0.6f, 1.0f, 1.0f, 1.0f);
    static const plasma::Vector<4, float> manaColor(0.6f, 0.6f, 1.0f, 1.0f);
    static const plasma::Vector<4, float> chargeColor(1.0f, 0.5f, 1.0f, 1.0f);
    renderText(m_game->m_speech.getText(m_game->m_speech.skillKey(attackSkill), L"default", L""), 180.0f, 0.0f);
    if (attackSkill == AbilityShuriken && !m_game->m_world->m_localCreature->findBuff(BuffNinjutsu)) {
        setTextColor(staminaColor);
        renderText(L"[25 STAMINA]", 320.0f, 0.0f);
    }

    // The special attack: charged skills are held.
    setTextColor(plasma::Vector<4, float>(0.25f, 1.0f, 1.0f, 1.0f));
    if (attackSkill == AbilityBlock || attackSkill == AbilityChargedShot || attackSkill == AbilitySalvo ||
        attackSkill == AbilityChargedBoomerang || attackSkill == AbilityCrush || attackSkill == AbilitySwirl) {
        renderText(L"HOLD " + m_game->m_keyNames.getFullName(m_game->m_controls.buttons[Controls::ButtonSpecialAttack]),
                   180.0f, 20.0f);
    } else {
        renderText(m_game->m_keyNames.getFullName(m_game->m_controls.buttons[Controls::ButtonSpecialAttack]), 0.0f,
                   20.0f);
    }
    setTextColor(plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f));
    int specialSkill = m_game->m_world->m_localCreature->nextSpecialAttackSkill(alternative);
    renderText(m_game->m_speech.getText(m_game->m_speech.skillKey(specialSkill), L"default", L""), 180.0f, 20.0f);
    switch (specialSkill) {
    case AbilityPerforate:
    case AbilityAmbush:
    case AbilityKick:
        setTextColor(manaColor);
        renderText(L"[ALL MP]", 320.0f, 20.0f);
        break;
    case AbilityShuriken:
        if (!m_game->m_world->m_localCreature->findBuff(BuffNinjutsu)) {
            setTextColor(staminaColor);
            renderText(L"[25 STAMINA]", 320.0f, 20.0f);
        }
        break;
    case AbilityIntercept: {
        setTextColor(manaColor);
        int end = (int)renderText(L"[ALL MP]", 320.0f, 20.0f)[0];
        if (!m_game->m_world->m_localCreature->findBuff(BuffNinjutsu)) {
            setTextColor(staminaColor);
            renderText(L"[25 STAMINA]", (float)(end + 20), 20.0f);
        }
        break;
    }
    case AbilityBlock:
    case AbilityChargedShot:
    case AbilitySalvo:
    case AbilityChargedBoomerang:
    case AbilityCrush:
    case AbilitySwirl:
        setTextColor(chargeColor);
        renderText(L"[CHARGE]", 320.0f, 20.0f);
        break;
    }

    // The class skill (by class and specialization).
    setTextColor(plasma::Vector<4, float>(0.25f, 1.0f, 1.0f, 1.0f));
    renderText(L"Hold " + m_game->m_keyNames.getFullName(m_game->m_controls.buttons[Controls::ButtonClassSkill]), 0.0f,
               40.0f);
    setTextColor(plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f));
    Creature *creature = m_game->m_world->m_localCreature;
    switch (creature->m_entityData.classType) {
    case ClassWarrior:
        renderText(m_game->m_speech.getText(m_game->m_speech.skillKey(AbilityCyclone), L"default", L""), 180.0f, 40.0f);
        setTextColor(staminaColor);
        renderText(L"[STAMINA]", 320.0f, 40.0f);
        break;
    case ClassRanger:
        if (creature->m_entityData.specialization == SpecializationScout) {
            renderText(m_game->m_speech.getText(m_game->m_speech.skillKey(AbilitySprint), L"default", L""), 180.0f,
                       40.0f);
            setTextColor(staminaColor);
            renderText(L"[STAMINA]", 320.0f, 40.0f);
        } else {
            renderText(m_game->m_speech.getText(m_game->m_speech.skillKey(AbilitySneak), L"default", L""), 180.0f,
                       40.0f);
        }
        break;
    case ClassMage:
        renderText(m_game->m_speech.getText(m_game->m_speech.skillKey(AbilityFloat), L"default", L""), 180.0f, 40.0f);
        break;
    case ClassRogue:
        if (creature->m_entityData.specialization == SpecializationNinja) {
            renderText(m_game->m_speech.getText(m_game->m_speech.skillKey(AbilitySprint), L"default", L""), 180.0f,
                       40.0f);
            setTextColor(staminaColor);
            renderText(L"[STAMINA]", 320.0f, 40.0f);
        } else {
            renderText(m_game->m_speech.getText(m_game->m_speech.skillKey(AbilitySneak), L"default", L""), 180.0f,
                       40.0f);
        }
        break;
    }

    // Dodging.
    int y = 60;
    setTextColor(plasma::Vector<4, float>(0.25f, 1.0f, 1.0f, 1.0f));
    renderText(L"Move + " + m_game->m_keyNames.getFullName(m_game->m_controls.buttons[Controls::ButtonDodge]), 0.0f,
               60.0f);
    setTextColor(plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f));
    renderText(L"Dodge", 180.0f, 60.0f);
    setTextColor(staminaColor);
    renderText(L"[25 STAMINA]", 320.0f, 60.0f);

    // The class's tapped skill.
    creature = m_game->m_world->m_localCreature;
    switch (creature->m_entityData.classType) {
    case ClassWarrior:
        y = 80;
        setTextColor(plasma::Vector<4, float>(0.25f, 1.0f, 1.0f, 1.0f));
        renderText(L"Tap " + m_game->m_keyNames.getFullName(m_game->m_controls.buttons[Controls::ButtonDodge]), 0.0f,
                   80.0f);
        setTextColor(plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f));
        renderText(m_game->m_speech.getText(m_game->m_speech.skillKey(AbilitySmash), L"default", L""), 180.0f, 80.0f);
        setTextColor(staminaColor);
        renderText(L"[100 STAMINA]", 320.0f, 80.0f);
        break;
    case ClassRanger:
        y = 80;
        setTextColor(plasma::Vector<4, float>(0.25f, 1.0f, 1.0f, 1.0f));
        renderText(L"Tap " + m_game->m_keyNames.getFullName(m_game->m_controls.buttons[Controls::ButtonDodge]), 0.0f,
                   80.0f);
        setTextColor(plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f));
        if (m_game->m_world->m_localCreature->m_entityData.specialization == SpecializationNinja) {
            renderText(m_game->m_speech.getText(m_game->m_speech.skillKey(AbilityRangerKick), L"default", L""), 180.0f,
                       80.0f);
            setTextColor(staminaColor);
            renderText(L"[50 STAMINA]", 320.0f, 80.0f);
        } else {
            renderText(m_game->m_speech.getText(m_game->m_speech.skillKey(AbilityRetreat), L"default", L""), 180.0f,
                       80.0f);
            setTextColor(staminaColor);
            renderText(L"[100 STAMINA]", 320.0f, 80.0f);
        }
        break;
    case ClassMage:
        y = 80;
        setTextColor(plasma::Vector<4, float>(0.25f, 1.0f, 1.0f, 1.0f));
        renderText(L"Tap " + m_game->m_keyNames.getFullName(m_game->m_controls.buttons[Controls::ButtonDodge]), 0.0f,
                   80.0f);
        setTextColor(plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f));
        renderText(m_game->m_speech.getText(m_game->m_speech.skillKey(AbilityTeleport), L"default", L""), 180.0f,
                   80.0f);
        setTextColor(manaColor);
        renderText(L"[100 MANA]", 320.0f, 80.0f);
        break;
    case ClassRogue:
        if (!creature->m_entityData.specialization) {
            y = 80;
            setTextColor(plasma::Vector<4, float>(0.25f, 1.0f, 1.0f, 1.0f));
            renderText(L"Tap " + m_game->m_keyNames.getFullName(m_game->m_controls.buttons[Controls::ButtonDodge]),
                       0.0f, 80.0f);
            setTextColor(plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f));
            renderText(m_game->m_speech.getText(m_game->m_speech.skillKey(AbilityThrowPoisonVial), L"default", L""),
                       180.0f, 80.0f);
            setTextColor(staminaColor);
            renderText(L"[100 STAMINA]", 320.0f, 80.0f);
        }
        break;
    }

    // The pet.
    setTextColor(plasma::Vector<4, float>(0.25f, 1.0f, 1.0f, 1.0f));
    renderText(m_game->m_keyNames.getFullName(m_game->m_controls.buttons[Controls::ButtonCallPet]), 0.0f,
               (float)(y + 20));
    setTextColor(plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f));
    renderText(L"Call/Ride Pet", 180.0f, (float)(y + 20));
}

}
