// See cube/CharacterPreviewWidget.h.
#include "plasma/TextIds.h"
#include <sstream>
#include "cube/widgets/CharacterPreviewWidget.h"
#include "cube/Creature.h"
#include "cube/Game.h"
#include "cube/World.h"
#include "cube/World.h"
#include "gfx/D3D11Renderer.h"
#include "plasma/D3D11Engine.h"
#include "plasma/Engine.h"
#include "plasma/Node.h"
#include "cube/CreatureIds.h"
#include "plasma/EditFilter.h"

namespace cube {

CharacterPreviewWidget::CharacterPreviewWidget(Game *game, plasma::Node *node)
    : BaseWidget(game->m_engine, node, L""),
      m_creature(nullptr),
      m_game(game)
{
    setFont(L"resource1.dat");
}

void CharacterPreviewWidget::draw()
{
    setTextSize(15.0f);
    setTextStrokeRadius(3.0f);
    setTextColor(plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f));
    setTextStrokeColor(plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f));
    if (!m_creature) {
        int x = (int)(getContentWidth() * 0.5f);
        std::wstringstream text;
        text << L"New character";
        setTextFlags((1 << plasma::TEXT_CENTER) | (1 << plasma::TEXT_MIDDLE));
        renderText(text.str(), (float)x, 30.0f);
        return;
    }

    // Pose the creature: standing still, its animation clock running; it turns and leans forward while the pointer is
    // over the entry and eases back otherwise.
    m_creature->m_entityData.rollTime = 0.0f;
    m_creature->m_entityData.stunTime = 0.0f;
    m_creature->m_entityData.binaryToggles = Creature::EntityData::BinaryToggles();
    m_creature->m_entityData.flags = CreatureFlagNone;
    m_creature->m_walkPhase += m_engine->getFrameTime() * 0.0075f;
    m_creature->m_entityData.currentAbility = AbilityNone;
    if (isPointerOver()) {
        m_creature->m_drawnRotation[2] -= m_engine->getFrameTime() * 0.05f;
        m_creature->m_walkAmplitude = 0.2f;
        blend(m_creature->m_entityData.acceleration, plasma::Vector<3, float>(0.0f, 8.0f, 0.0f),
              m_engine->getFrameTime(), 0.01f);
        blend(m_creature->m_entityData.velocity, plasma::Vector<3, float>(0.0f, 3.0f, 0.0f), m_engine->getFrameTime(),
              0.01f);
    } else {
        m_creature->m_drawnRotation[2] =
            interpolateAngle(m_creature->m_drawnRotation[2], 0.0f, blendFactor(m_engine->getFrameTime(), 0.01f) + 0.0f);
        m_creature->m_walkAmplitude +=
            blendFactor(m_engine->getFrameTime(), 0.01f) * (0.0f - m_creature->m_walkAmplitude);
        blend(m_creature->m_entityData.acceleration, plasma::Vector<3, float>(0.0f, 0.0f, 0.0f),
              m_engine->getFrameTime(), 0.01f);
        blend(m_creature->m_entityData.velocity, plasma::Vector<3, float>(0.0f, 0.0f, 0.0f), m_engine->getFrameTime(),
              0.01f);
    }

    // The model, left of the texts, at the node's pivot in window units.
    plasma::Matrix<float> matrix;
    m_node->getMatrix(matrix);
    plasma::Vector<2, float> pivot = matrix.transform(m_node->transformation()->m_pivot.value());
    m_game->m_renderer->drawCreature(&m_creature->m_animationState, matrix[0] * 100.0f + pivot[0],
                                     matrix[0] * 110.0f + pivot[1], plasma::Vector<3, float>(-95.0f, 0.0f, 45.0f),
                                     matrix[0]);
    m_engine->applyRenderState();

    std::wstringstream text;
    text << WidenString(std::string(m_creature->m_entityData.name));
    setTextSize(12.0f);
    setTextColor(plasma::Vector<4, float>(0.25f, 1.0f, 1.0f, 1.0f));
    setTextFlags(0);
    renderText(text.str(), 200.0f, 30.0f);

    // Level, race and class.
    text.str(L"");
    text << L"LV " << m_creature->m_entityData.level;
    text << " ";
    switch (m_creature->m_entityData.race) {
    case RaceHumanMale:
    case RaceHumanFemale:
        text << "Human";
        break;
    case RaceElfMale:
    case RaceElfFemale:
        text << "Elf";
        break;
    case RaceOrcMale:
    case RaceOrcFemale:
        text << "Orc";
        break;
    case RaceDwarfMale:
    case RaceDwarfFemale:
        text << "Dwarf";
        break;
    case RaceGoblinMale:
    case RaceGoblinFemale:
        text << "Goblin";
        break;
    case RaceLizardmanMale:
    case RaceLizardmanFemale:
        text << "Lizardman";
        break;
    case RaceUndeadMale:
    case RaceUndeadFemale:
        text << "Undead";
        break;
    }
    text << " ";
    switch (m_creature->m_entityData.classType) {
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
    setTextColor(plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f));
    renderText(text.str(), 200.0f, 55.0f);

    // Specialization (0 or 1 per class).
    text.str(L"");
    text << L"Specialization: ";
    switch (m_creature->m_entityData.classType) {
    case ClassWarrior:
        if (m_creature->m_entityData.specialization == SpecializationBerserker)
            text << "Berserker";
        if (m_creature->m_entityData.specialization == SpecializationGuardian)
            text << "Guardian";
        break;
    case ClassRanger:
        if (m_creature->m_entityData.specialization == SpecializationSniper)
            text << "Sniper";
        if (m_creature->m_entityData.specialization == SpecializationScout)
            text << "Scout";
        break;
    case ClassMage:
        if (m_creature->m_entityData.specialization == SpecializationFire)
            text << "Fire Mage";
        if (m_creature->m_entityData.specialization == SpecializationWater)
            text << "Water Mage";
        break;
    case ClassRogue:
        if (m_creature->m_entityData.specialization == SpecializationAssassin)
            text << "Assassin";
        if (m_creature->m_entityData.specialization == SpecializationNinja)
            text << "Ninja";
        break;
    }
    setTextColor(plasma::Vector<4, float>(0.5f, 0.5f, 1.0f, 1.0f));
    renderText(text.str(), 200.0f, 75.0f);
}

}
