// WorldPreviewWidget draws saved-world list entries.
#include "cube/DefaultBlocks.h"
#include "cube/widgets/WorldPreviewWidget.h"
#include "cube/Animation.h"
#include "cube/Creature.h"
#include "cube/CubeShader.h"
#include "cube/GameController.h"
#include "cube/Sprite.h"
#include "cube/WorldInfo.h"
#include "plasma/Engine.h"
#include "plasma/FontEngine.h"
#include "plasma/Font.h"
#include "plasma/Node.h"
#include "plasma/StringConversion.h"
#include "plasma/Transformation.h"
#include <sstream>
namespace cube {

WorldPreviewWidget::WorldPreviewWidget(plasma::Engine *engine, plasma::Node *node, GameController *game)
    : Widget(engine, node),
      m_game(game)
{
    m_worldInfo = 0;
    m_rotation = 0.0f;
}
// A world list entry: "New world" for the empty slot; otherwise the world's preview model turning
// while the pointer is over the entry (easing back to rest otherwise), the selected character
// standing beside it when the player's last world is this one, and the name, explored area and
// seed. Its numbers are hand-set: the model's turn (0.05 degrees per millisecond under the
// pointer, eased back at 0.01 per millisecond; at rest 45 degrees), its scale 0.05, the light
// ((0.5, 0.6, 3.5), back and ambient (0.2, 0.3, 0.4) and 0.4; a fog scale of 1e9, no fog), the
// previews' perspective (45 degrees, depth terms 1.0001 and -0.10001) placed 100, 110 pixels into
// the entry (the character -100 beside it, walking as in CharacterPreviewWidget::draw), and the
// text in pixels: "New world" centred at 30 (14 high, stroke 3, wrapped at 180), the name at 200,
// 30 (14 high), the explored area at 200, 55 and the seed at 200, 80 (10).
void WorldPreviewWidget::draw()
{
    if (!m_worldInfo) {
        int x = (int)(viewWidth() * 0.5f);
        std::wstringstream text;
        text << L"New world";
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

    if (m_worldInfo->m_preview) {
        m_game->m_device->SetRenderState(D3DRS_ZENABLE, TRUE);
        plasma::Vector<4, float> lightPositions[PointLightSourceCount];
        plasma::Vector<3, float> lightColors[PointLightSourceCount];
        for (int i = 0; i < PointLightSourceCount; i++) {
            lightPositions[i] = plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f);
            lightColors[i] = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
        }
        if (isPointerOver()) {
            m_rotation = m_rotation - m_engine->getFrameTime() * 0.05f;
        } else {
            float factor = 0.0f;
            blend(factor, 1.0f, m_engine->getFrameTime(), 0.01f);
            m_rotation = interpolateAngle(m_rotation, 0.0f, factor);
        }
        m_game->m_shader->apply();
        m_game->m_shader->setPointLights(lightPositions, lightColors);
        m_game->m_shader->setAlpha(1.0f);
        m_game->m_shader->setShininess(0.0f);
        m_game->m_shader->setMaterialColor(plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f));
        m_game->m_shader->setCameraPosition(plasma::Vector<3, float>(0.0f, 0.0f, 0.0f));
        m_game->m_shader->setLight(
            plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f), plasma::Vector<4, float>(0.2f, 0.3f, 0.4f, 1.0f),
            plasma::Vector<3, float>(0.5f, 0.6f, 3.5f).normalized(), plasma::Vector<4, float>(0.4f, 0.4f, 0.4f, 1.0f));
        m_game->m_shader->setFogScale(1.0e9f);
        plasma::Vector<2, float> pivot = m_node->m_matrix * m_node->transformation()->m_pivot.value();

        // The model tilted by -120 degrees about x, turned about z, scaled and centred, 17 units away.
        plasma::Matrix<float> model;
        model.identity();
        model.translate(0.0f, 0.0f, 17.0f);
        model.rotateX(-120.0f);
        model.rotateZ(m_rotation + 45.0f);
        model.scale(0.05f, 0.05f, 0.05f);
        model.translate(m_worldInfo->m_preview->sizeX() * -0.5f, m_worldInfo->m_preview->sizeY() * -0.5f,
                        m_worldInfo->m_preview->sizeZ() * -0.5f);

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
        placement.translate(
            (pivot[0] + 100.0f - m_engine->viewportWidth() * 0.5f) / m_engine->viewportWidth() * 2.0f,
            (pivot[1] + 110.0f - m_engine->viewportHeight() * 0.5f) / m_engine->viewportHeight() * -2.0f, 0.0f);
        projection = placement * projection;
        plasma::Matrix<float> identity;
        identity.identity();
        m_game->m_shader->setMatrices(identity, model, projection);
        m_worldInfo->m_preview->draw();
    }

    // The selected character, posed as if walking, where the player last was in this world.
    Creature::PlayerData *playerData = m_game->m_player->m_playerData;
    if (m_worldInfo->m_seed == playerData->lastWorld.first && m_worldInfo->m_name == playerData->lastWorld.second) {
        Creature *creature = m_game->selectedCharacter();
        creature->m_entityData.binaryToggles = Creature::BinaryToggles();
        creature->m_entityData.flags = 0;
        creature->m_entityData.rollTime = 0;
        creature->m_entityData.stunTime = 0;
        creature->m_entityData.currentAbility = AbilityNone;
        creature->m_walkPhase += m_engine->getFrameTime() * 0.0075f;
        creature->m_drawnPosition = creature->m_entityData.position;
        creature->m_drawnRotation[2] = 0.0f;
        creature->m_walkAmplitude = 0.2f;
        blend(creature->m_entityData.acceleration, plasma::Vector<3, float>(0.0f, 8.0f, 0.0f), m_engine->getFrameTime(),
              0.01f);
        blend(creature->m_entityData.velocity, plasma::Vector<3, float>(0.0f, 3.0f, 0.0f), m_engine->getFrameTime(),
              0.01f);

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
        view.translate(0.0f, 0.0f, (float)-creature->m_entityData.position[2]);

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
        placement.translate(
            (pivot[0] - 100.0f - m_engine->viewportWidth() * 0.5f) / m_engine->viewportWidth() * 2.0f,
            (pivot[1] + 100.0f - m_engine->viewportHeight() * 0.5f) / m_engine->viewportHeight() * -2.0f, 0.0f);
        projection = placement * projection;
        creature->m_animationState.addSprites(
            m_game->m_shader, view, projection, &m_game->m_world.m_spriteManager, m_engine->getFrameTime(),
            plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f), -creature->m_entityData.position[0],
            -creature->m_entityData.position[1], 0, 0);
    }
    m_game->m_device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    m_game->m_device->SetRenderState(D3DRS_ZENABLE, FALSE);

    std::wstringstream text;
    text << widenString(m_worldInfo->m_name);
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
    text.str(L"");
    text << L"Explored: " << (float)m_worldInfo->m_exploredArea << L" km\x00b2";
    m_engine->getFontEngine()->renderText(L"resource1.dat", text.str(), 0.0f, 2.0f, 200.0f, 55.0f, 10.0f, 3.0f,
                                          plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f).m_elements,
                                          plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f).m_elements,
                                          plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
                                          plasma::TEXT_FLAGS_NONE, -1.0f, true);
    m_engine->getFontEngine()->renderText(L"resource1.dat", text.str(), 0.0f, 2.0f, 200.0f, 55.0f, 10.0f, 0.0f,
                                          plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f).m_elements,
                                          plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
                                          plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
                                          plasma::TEXT_FLAGS_NONE, -1.0f, true);
    text.str(L"");
    text << L"Seed: " << m_worldInfo->m_seed;
    m_engine->getFontEngine()->renderText(L"resource1.dat", text.str(), 0.0f, 2.0f, 200.0f, 80.0f, 10.0f, 3.0f,
                                          plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f).m_elements,
                                          plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f).m_elements,
                                          plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
                                          plasma::TEXT_FLAGS_NONE, -1.0f, true);
    m_engine->getFontEngine()->renderText(L"resource1.dat", text.str(), 0.0f, 2.0f, 200.0f, 80.0f, 10.0f, 0.0f,
                                          plasma::Vector<4, float>(0.5f, 0.5f, 1.0f, 1.0f).m_elements,
                                          plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
                                          plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
                                          plasma::TEXT_FLAGS_NONE, -1.0f, true);
}

bool WorldPreviewWidget::contains(const plasma::Vector<2, float> &point)
{
    return point[0] >= 0.0f && point[1] >= 0.0f && point[0] < viewWidth() && point[1] < getViewHeight();
}
}

// Widens each byte to one character without code-page conversion. Shared by game and engine.
std::wstring widenString(const std::string &text)
{
    return std::wstring(text.begin(), text.end());
}
