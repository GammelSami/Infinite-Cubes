// CharacterPreviewWidget draws a saved-character list entry.
#include "cube/DefaultBlocks.h"
#include "cube/widgets/CharacterPreviewWidget.h"
#include "cube/Animation.h"
#include "cube/Creature.h"
#include "cube/CreatureIds.h"
#include "cube/CubeShader.h"
#include "cube/GameController.h"
#include "cube/RaceIds.h"
#include "plasma/Engine.h"
#include "plasma/FontEngine.h"
#include "plasma/Font.h"
#include "plasma/Node.h"
#include "plasma/StringConversion.h"
#include "plasma/Transformation.h"
#include <sstream>
namespace cube {

CharacterPreviewWidget::CharacterPreviewWidget(plasma::Engine *engine, plasma::Node *node, GameController *game)
    : Widget(engine, node),
      m_game(game)
{
    m_creature = 0;
}

// A character list entry: "New character" for the empty slot; otherwise the character walking in
// place (turning while the pointer is over the entry, easing back to rest otherwise) with its
// name, level, race, class and specialization. Its numbers are hand-set: the walk (the phase
// 0.0075 per millisecond, the amplitude 0.2, the acceleration and velocity eased to 8 and 3 blocks
// per second forward at a rate of 0.01 per millisecond, the turn 0.05 degrees per millisecond
// under the pointer), the light ((0.5, 1, 1.5), back and ambient (0.2, 0.3, 0.4) and 0.4; a fog
// scale of 1e9, no fog), the view and the previews' perspective (45 degrees, depth terms 1.0001
// and -0.10001) placed 100, 110 pixels into the entry, and the text in pixels: "New character"
// centred at 30 (14 high, stroke 3, wrapped at 180), the name at 200, 30 (14 high), the summary at
// 200, 55 (12) and the specialization at 200, 75 (10), in RGBA colours.
void CharacterPreviewWidget::draw()
{
    if (!m_creature) {
        int x = (int)(viewWidth() * 0.5f);
        std::wstringstream text;
        text << L"New character";
        m_engine->getFontEngine()->renderText(L"resource1.dat", text.str(), 0.0f, 2.0f, (float)x, 30.0f, 14.0f, 3.0f,
                                              plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f).m_elements,
                                              plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f).m_elements,
                                              plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
                                              (1 << plasma::TEXT_CENTER) | (1 << plasma::TEXT_MIDDLE), 180.0f, true);
        m_engine->getFontEngine()->renderText(L"resource1.dat", text.str(), 0.0f, 2.0f, (float)x, 30.0f, 14.0f, 0.0f,
                                              plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f).m_elements,
                                              plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
                                              plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
                                              (1 << plasma::TEXT_CENTER) | (1 << plasma::TEXT_MIDDLE), 180.0f, true);
        return;
    }

    // Standing still with the walk cycle running.
    m_creature->m_entityData.rollTime = 0;
    m_creature->m_entityData.stunTime = 0;
    m_creature->m_entityData.binaryToggles = Creature::BinaryToggles();
    m_creature->m_entityData.flags = 0;
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
        float factor = 0.0f;
        blend(factor, 1.0f, m_engine->getFrameTime(), 0.01f);
        m_creature->m_drawnRotation[2] = interpolateAngle(m_creature->m_drawnRotation[2], 0.0f, factor);
        blend(m_creature->m_walkAmplitude, 0.0f, m_engine->getFrameTime(), 0.01f);
        blend(m_creature->m_entityData.acceleration, plasma::Vector<3, float>(0.0f, 0.0f, 0.0f),
              m_engine->getFrameTime(), 0.01f);
        blend(m_creature->m_entityData.velocity, plasma::Vector<3, float>(0.0f, 0.0f, 0.0f), m_engine->getFrameTime(),
              0.01f);
    }

    m_game->m_device->SetRenderState(D3DRS_ZENABLE, TRUE);
    plasma::Vector<4, float> lightPositions[PointLightSourceCount];
    plasma::Vector<3, float> lightColors[PointLightSourceCount];
    for (int i = 0; i < PointLightSourceCount; i++) {
        lightPositions[i] = plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f);
        lightColors[i] = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
    }
    m_game->m_shader->apply();
    m_game->m_shader->setPointLights(lightPositions, lightColors);
    m_game->m_shader->setAlpha(1.0f);
    m_game->m_shader->setShininess(0.0f);
    m_game->m_shader->setMaterialColor(plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f));
    m_game->m_shader->setCameraPosition(plasma::Vector<3, float>(0.0f, 0.0f, 0.0f));
    m_game->m_shader->setLight(
        plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f), plasma::Vector<4, float>(0.2f, 0.3f, 0.4f, 1.0f),
        plasma::Vector<3, float>(0.5f, 1.0f, 1.5f).normalized(), plasma::Vector<4, float>(0.4f, 0.4f, 0.4f, 1.0f));
    m_game->m_shader->setFogScale(1.0e9f);
    plasma::Vector<2, float> pivot = m_node->m_matrix * m_node->transformation()->m_pivot.value();

    // Seen from 17 units, tilted by -95 degrees and turned by 45, at the creature's height; addSprites
    // places it by its x and y.
    plasma::Matrix<float> view;
    view.identity();
    view.translate(0.0f, 0.0f, 17.0f);
    view.rotateX(-95.0f);
    view.rotateZ(45.0f);
    view.translate(0.0f, 0.0f, (float)-m_creature->m_entityData.position[2]);

    // A 45 degree perspective moved to the entry's place in the window.
    float focal = 1.0f / tan(0.39269909f);
    plasma::Matrix<float> projection;
    for (int i = 0; i < 16; i++)
        projection.m_elements[i] = 0.0f;
    projection.m_elements[0] = -(focal / ((float)m_engine->viewportWidth() / (float)m_engine->viewportHeight()));
    projection.m_elements[5] = focal;
    projection.m_elements[10] = 1.0001f;
    projection.m_elements[11] = 1.0f;
    projection.m_elements[14] = -0.10001f;
    plasma::Matrix<float> placement;
    placement.identity();
    placement.translate((pivot[0] + 100.0f - m_engine->viewportWidth() * 0.5f) / m_engine->viewportWidth() * 2.0f,
                        (pivot[1] + 110.0f - m_engine->viewportHeight() * 0.5f) / m_engine->viewportHeight() * -2.0f,
                        0.0f);
    projection = placement * projection;
    m_creature->m_animationState.addSprites(m_game->m_shader, view, projection, &m_game->m_world.m_spriteManager,
                                            m_engine->getFrameTime(), plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f),
                                            -m_creature->m_entityData.position[0],
                                            -m_creature->m_entityData.position[1], 0, 0);
    m_game->m_device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    m_game->m_device->SetRenderState(D3DRS_ZENABLE, FALSE);

    std::wstringstream text;
    text << widenString(std::string(m_creature->m_entityData.name));
    m_engine->getFontEngine()->renderText(L"resource1.dat", text.str(), 0.0f, 2.0f, 200.0f, 30.0f, 14.0f, 3.0f,
                                          plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f).m_elements,
                                          plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f).m_elements,
                                          plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
                                          (1 << plasma::TEXT_WRAP), 180.0f, true);
    m_engine->getFontEngine()->renderText(L"resource1.dat", text.str(), 0.0f, 2.0f, 200.0f, 30.0f, 14.0f, 0.0f,
                                          plasma::Vector<4, float>(0.25f, 1.0f, 1.0f, 1.0f).m_elements,
                                          plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
                                          plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
                                          (1 << plasma::TEXT_WRAP), 180.0f, true);

    // Level, race and class.
    text.str(L"");
    text << L"LVL " << m_creature->m_entityData.level;
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
    case ClassRogue:
        text << "Rogue";
        break;
    case ClassMage:
        text << "Mage";
        break;
    }
    m_engine->getFontEngine()->renderText(L"resource1.dat", text.str(), 0.0f, 2.0f, 200.0f, 55.0f, 12.0f, 3.0f,
                                          plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f).m_elements,
                                          plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f).m_elements,
                                          plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
                                          plasma::TEXT_FLAGS_NONE, -1.0f, true);
    m_engine->getFontEngine()->renderText(L"resource1.dat", text.str(), 0.0f, 2.0f, 200.0f, 55.0f, 12.0f, 0.0f,
                                          plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f).m_elements,
                                          plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
                                          plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
                                          plasma::TEXT_FLAGS_NONE, -1.0f, true);

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
    m_engine->getFontEngine()->renderText(L"resource1.dat", text.str(), 0.0f, 2.0f, 200.0f, 75.0f, 10.0f, 3.0f,
                                          plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f).m_elements,
                                          plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f).m_elements,
                                          plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
                                          plasma::TEXT_FLAGS_NONE, -1.0f, true);
    m_engine->getFontEngine()->renderText(L"resource1.dat", text.str(), 0.0f, 2.0f, 200.0f, 75.0f, 10.0f, 0.0f,
                                          plasma::Vector<4, float>(0.5f, 0.5f, 1.0f, 1.0f).m_elements,
                                          plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
                                          plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
                                          plasma::TEXT_FLAGS_NONE, -1.0f, true);
}

bool CharacterPreviewWidget::contains(const plasma::Vector<2, float> &point)
{
    return point[0] >= 0.0f && point[1] >= 0.0f && point[0] < viewWidth() && point[1] < getViewHeight();
}
}
