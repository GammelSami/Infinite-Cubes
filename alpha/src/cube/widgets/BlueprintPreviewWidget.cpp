// BlueprintPreviewWidget displays a crafting formula and its requirements.
#include "cube/widgets/BlueprintPreviewWidget.h"
#include "cube/DefaultBlocks.h"
#include "cube/Creature.h"
#include "cube/CubeShader.h"
#include "cube/GameController.h"
#include "cube/WorldBlocks.h"
#include "cube/Sprite.h"
#include "cube/SpriteIds.h"
#include "cube/Zone.h"
#include "plasma/Display.h"
#include "plasma/Engine.h"
#include "plasma/FontEngine.h"
#include "plasma/Font.h"
#include "plasma/Node.h"
#include "plasma/Transformation.h"
#include <math.h>
#include <sstream>
namespace cube {
// The result's and the ingredients' presentation (anonymous: this file's own).
namespace {
enum {
    ResultTilt = -120
}; // degrees about x
}

BlueprintPreviewWidget::BlueprintPreviewWidget(plasma::Engine *engine, plasma::Node *node, GameController *game,
                                               plasma::Node *itemShadow, plasma::Node *craftButton,
                                               plasma::Node *craftBar)
    : Widget(engine, node),
      m_resultItemRotation(ResultTilt, 0.0f, 0.0f),
      m_game(game),
      m_itemShadow(itemShadow),
      m_craftButton(craftButton),
      m_craftBar(craftBar),
      m_craftProgressFill(0),
      m_hoveredIngredientIndex(-1),
      m_craftButtonHovered(false)
{
    if (m_craftButton)
        m_craftButton->setParent(node);
    if (m_craftBar) {
        m_craftBar->setParent(node);
        m_craftProgressFill = m_craftBar->findNode(L"bar");
    }
}

// The crafting panel: the formula's result turning at the top (larger under the pointer, an
// unknown item's sprite while the formula is not known), the ingredients in a grid of 45 units
// below, the result's name (with its power) or "Unknown", under each ingredient how many the
// player holds of how many it needs (green when enough), the workstation the formula needs when
// none is near (water under or around the player, or one of the station objects within 4 blocks in
// the 3 x 3 cells around the player), and the craft button: cyan under the pointer when crafting
// is possible, green otherwise, white when something is missing. Its layout is in pixels: the
// result's hover box 120 wide from y 50 to 170; the ingredients' 40-pixel cells 45 apart, 10 in
// from the left and 130 up from the bottom, their counts 50 and 55 on (10 high); the name centred
// at 30 (12 high, stroke 3), "Unknown" at 15, 65 (15 high); a missing workstation's message at 15,
// 40 up from the bottom (9 high, red: RGBA 1, 0.25, 0.25). The previews' projection is a
// perspective of 45 degrees (focal 1 / tan(pi / 8)) with depth terms 1.0001 and -0.10001 (0.1 to
// 1000), shifted to the item's place; the light (0.7, 0.5, -1) with back and ambient colours (0.2,
// 0.2, 0.4) and 0.4, the material 0.9 opaque. The wet ground under the player is sampled a tenth
// of a block below its feet.
void BlueprintPreviewWidget::draw()
{
    Sprite *sprite = m_game->m_world.m_spriteManager.itemSprite(&m_formula.result);
    if (!m_game->m_player->knowsFormula(m_formula.result))
        sprite = m_game->m_world.m_spriteManager.sprite(SpriteUnknown);
    if (!sprite)
        return;
    plasma::Vector<2, float> position = getContentPosition();

    m_game->m_device->SetRenderState(D3DRS_ZENABLE, TRUE);
    plasma::Vector<4, float> lightPositions[16];
    plasma::Vector<3, float> lightColors[16];
    for (int i = 0; i < 16; i++) {
        lightPositions[i] = plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f);
        lightColors[i] = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
    }
    m_game->m_shader->apply();
    m_game->m_shader->setPointLights(lightPositions, lightColors);
    m_game->m_shader->setMaterialColor(plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 0.9f));
    m_game->m_shader->setLight(
        plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f), plasma::Vector<4, float>(0.2f, 0.2f, 0.4f, 1.0f),
        plasma::Vector<3, float>(0.7f, 0.5f, -1.0f).normalized(), plasma::Vector<4, float>(0.4f, 0.4f, 0.4f, 1.0f));

    // The result, 110 units below the top, scaled to 0.06 of its largest side.
    float itemX = position[0] + viewWidth() * 0.5f;
    float itemY = position[1] + 110.0f;
    int size = std::max(sprite->sizeX(), std::max(sprite->sizeY(), sprite->sizeZ()));
    float scale = 0.06f / size;
    plasma::Matrix<float> view;
    view.identity();
    m_resultItemRotation[2] += m_engine->getFrameTime() * 0.02f;
    m_hoveredIngredientIndex = -1;
    m_hoveredItem.category = ItemCategoryNone;
    if (getPointerOffset()[0] > getContentWidth() * 0.5f - 60.0f && getPointerOffset()[1] > 50.0f &&
        getContentWidth() * 0.5f + 60.0f > getPointerOffset()[0] && 170.0f > getPointerOffset()[1]) {
        m_hoveredItem = m_formula.result;
        scale *= 1.5f;
    }
    plasma::Matrix<float> model;
    model.identity();
    model.translate(0.0f, 0.0f, 1.0f);
    model.rotateX(m_resultItemRotation[0]);
    model.rotateY(m_resultItemRotation[1]);
    model.rotateZ(m_resultItemRotation[2]);
    model.scale(scale, scale, scale);
    model.translate(sprite->sizeX() * -0.5f, sprite->sizeY() * -0.5f, sprite->sizeZ() * -0.5f);
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
    placement.translate((itemX - m_engine->viewportWidth() * 0.5f) / m_engine->viewportWidth() * 2.0f,
                        (itemY - m_engine->viewportHeight() * 0.5f) / m_engine->viewportHeight() * -2.0f, 0.0f);
    projection = placement * projection;
    m_game->m_shader->setMatrices(model, view, projection);
    sprite->draw();

    // The ingredients.
    int columns = (int)((viewWidth() - 10.0f) / 45.0f);
    getViewHeight();
    position[1] += getContentHeight() - 130.0f;
    if (m_game->m_player->knowsFormula(m_formula.result)) {
        for (int i = 0; i < (int)m_formula.ingredients.size(); i++) {
            float iconScale = 0.025f;
            int x = (int)(45 * (i % columns) + (position[0] + 10.0f));
            int y = (int)(45 * (i / columns) + (position[1] + 10.0f));
            plasma::Vector<2, float> pointer = m_engine->getPointer();
            if (pointer[0] >= x && x + 40 > pointer[0] && pointer[1] >= y && y + 40 > pointer[1]) {
                m_hoveredIngredientIndex = i;
                m_hoveredItem = m_formula.ingredients[i].item;
                iconScale = 0.0375f;
            }
            m_game->m_shader->setMaterialColor(plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f));
            m_game->drawItem((float)(x + 20), (float)(y + 20), m_game->m_previewRotation, iconScale,
                             &m_formula.ingredients[i].item, 0.0f);
        }
    }
    m_game->m_device->SetRenderState(D3DRS_ZENABLE, FALSE);

    std::wstringstream text;
    bool missingIngredients = false;
    if (m_game->m_player->knowsFormula(m_formula.result)) {
        text << m_game->m_world.createItemName(m_formula.result);
        if (m_formula.result.scalesWithLevel())
            text << L" +" << m_formula.result.power();
        m_engine->getFontEngine()->renderText(L"resource1.dat", text.str(), 0.0f, 0.0f, getContentWidth() * 0.5f, 30.0f,
                                              12.0f, 3.0f, plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f).m_elements,
                                              plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f).m_elements,
                                              plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
                                              (1 << plasma::TEXT_CENTER) | (1 << plasma::TEXT_WRAP), getContentWidth(),
                                              true);
        m_engine->getFontEngine()->renderText(L"resource1.dat", text.str(), 0.0f, 0.0f, getContentWidth() * 0.5f, 30.0f,
                                              12.0f, 0.0f, plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f).m_elements,
                                              plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
                                              plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
                                              (1 << plasma::TEXT_CENTER) | (1 << plasma::TEXT_WRAP), getContentWidth(),
                                              true);

        // How many of each ingredient the player holds of how many the formula needs.
        for (int i = 0; i < (int)m_formula.ingredients.size(); i++) {
            int x = 50 + 45 * (i % columns);
            int needed = m_formula.ingredients[i].quantity;
            int y = (int)(getContentHeight() - 130.0f + 45 * (i / columns) + 55.0f);
            int held = 0;
            Creature *player = m_game->m_player;
            for (int tab = 0; tab < (int)player->m_inventory.m_inventoryTabs.size(); tab++) {
                for (int slot = 0; slot < (int)player->m_inventory.m_inventoryTabs[tab].size(); slot++) {
                    if (player->m_inventory.m_inventoryTabs[tab][slot].quantity &&
                        m_formula.ingredients[i].item == player->m_inventory.m_inventoryTabs[tab][slot].item)
                        held += player->m_inventory.m_inventoryTabs[tab][slot].quantity;
                }
            }
            plasma::Vector<4, float> color(0.0f, 1.0f, 0.0f, 1.0f);
            if (held < needed) {
                color = plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f);
                missingIngredients = true;
            }
            std::wstringstream count;
            count << held << L"/" << needed << std::endl;
            m_engine->getFontEngine()->renderText(L"resource1.dat", count.str(), 0.0f, 0.0f, (float)x, (float)y, 10.0f,
                                                  2.0f, plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f).m_elements,
                                                  plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f).m_elements,
                                                  plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
                                                  (1 << plasma::TEXT_RIGHT), -1.0f, true);
            m_engine->getFontEngine()->renderText(
                L"resource1.dat", count.str(), 0.0f, 0.0f, (float)x, (float)y, 10.0f, 0.0f, color.m_elements,
                plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
                plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, (1 << plasma::TEXT_RIGHT), -1.0f, true);
        }
    } else {
        text << L"Unknown";
        m_engine->getFontEngine()->renderText(L"resource1.dat", text.str(), 0.0f, 0.0f, 15.0f, 65.0f, 15.0f, 3.0f,
                                              plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f).m_elements,
                                              plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f).m_elements,
                                              plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
                                              plasma::TEXT_FLAGS_NONE, -1.0f, true);
        m_engine->getFontEngine()->renderText(L"resource1.dat", text.str(), 0.0f, 0.0f, 15.0f, 65.0f, 15.0f, 0.0f,
                                              plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f).m_elements,
                                              plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
                                              plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
                                              plasma::TEXT_FLAGS_NONE, -1.0f, true);
    }

    // The workstation the formula needs: water, or a station object within 4 blocks among the objects
    // of the 3 x 3 cells (8 x 8 blocks) around the player. the code writes each case out with its
    // message.
    bool missingWorkstation = false;
    switch (m_formula.workstationRequired) {
    case WorkstationWater: {
        // The player is in water or stands on wet ground.
        Creature *player = m_game->m_player;
        if (!(player->m_entityData.flags & (1 << CreatureFlagInWater)) &&
            m_game->m_world
                    .getBlock(player->m_entityData.position[0], player->m_entityData.position[1],
                              player->m_entityData.position[2] - Fixed(player->m_entityData.appearance.size[2] * 0.5f) -
                                  Fixed(0.1),
                              0)
                    .blockType() != BlockWet) {
            missingWorkstation = true;
            std::wstring message = L"Requires water";
            plasma::Vector<2, float> textPosition(15.0f, getViewHeight() - 40.0f);
            m_engine->getFontEngine()->renderText(
                L"resource1.dat", message, 0.0f, 0.0f, textPosition[0], textPosition[1], 9.0f, 3.0f,
                plasma::Vector<4, float>(1.0f, 0.25f, 0.25f, 1.0f).m_elements,
                plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f).m_elements,
                plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, (1 << plasma::TEXT_WRAP),
                viewWidth() - m_craftButton->widget()->viewWidth() - 40.0f - 10.0f, true);
            m_engine->getFontEngine()->renderText(
                L"resource1.dat", message, 0.0f, 0.0f, textPosition[0], textPosition[1], 9.0f, 0.0f,
                plasma::Vector<4, float>(1.0f, 0.25f, 0.25f, 1.0f).m_elements,
                plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
                plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, (1 << plasma::TEXT_WRAP),
                viewWidth() - m_craftButton->widget()->viewWidth() - 40.0f - 10.0f, true);
        }
        break;
    }
    case WorkstationSpinningWheel: {
        Creature *player = m_game->m_player;
        int cellX = (int)player->m_entityData.position[0] / 8;
        int cellY = (int)player->m_entityData.position[1] / 8;
        missingWorkstation = true;
        for (int x = cellX - 1; x <= cellX + 1; x++) {
            for (int y = cellY - 1; y <= cellY + 1; y++) {
                std::list<Zone::CellRecord> *objects = m_game->m_world.getCellObjects(x, y, 0);
                if (!objects)
                    continue;
                for (std::list<Zone::CellRecord>::iterator it = objects->begin(); it != objects->end(); ++it) {
                    if (it->object->type == StaticObjectSpinningWheel &&
                        plasma::Vector<3, float>(it->object->position - player->m_entityData.position).squaredLength() <
                            16.0f) {
                        missingWorkstation = false;
                        goto spinningWheelSearched;
                    }
                }
            }
        }
    spinningWheelSearched:
        if (missingWorkstation) {
            std::wstring message = L"Requires Spinning Wheel";
            plasma::Vector<2, float> textPosition(15.0f, getViewHeight() - 40.0f);
            m_engine->getFontEngine()->renderText(
                L"resource1.dat", message, 0.0f, 0.0f, textPosition[0], textPosition[1], 9.0f, 3.0f,
                plasma::Vector<4, float>(1.0f, 0.25f, 0.25f, 1.0f).m_elements,
                plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f).m_elements,
                plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, (1 << plasma::TEXT_WRAP),
                viewWidth() - m_craftButton->widget()->viewWidth() - 40.0f - 10.0f, true);
            m_engine->getFontEngine()->renderText(
                L"resource1.dat", message, 0.0f, 0.0f, textPosition[0], textPosition[1], 9.0f, 0.0f,
                plasma::Vector<4, float>(1.0f, 0.25f, 0.25f, 1.0f).m_elements,
                plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
                plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, (1 << plasma::TEXT_WRAP),
                viewWidth() - m_craftButton->widget()->viewWidth() - 40.0f - 10.0f, true);
        }
        break;
    }
    case WorkstationLoom: {
        Creature *player = m_game->m_player;
        int cellX = (int)player->m_entityData.position[0] / 8;
        int cellY = (int)player->m_entityData.position[1] / 8;
        missingWorkstation = true;
        for (int x = cellX - 1; x <= cellX + 1; x++) {
            for (int y = cellY - 1; y <= cellY + 1; y++) {
                std::list<Zone::CellRecord> *objects = m_game->m_world.getCellObjects(x, y, 0);
                if (!objects)
                    continue;
                for (std::list<Zone::CellRecord>::iterator it = objects->begin(); it != objects->end(); ++it) {
                    if (it->object->type == StaticObjectLoom &&
                        plasma::Vector<3, float>(it->object->position - player->m_entityData.position).squaredLength() <
                            16.0f) {
                        missingWorkstation = false;
                        goto loomSearched;
                    }
                }
            }
        }
    loomSearched:
        if (missingWorkstation) {
            std::wstring message = L"Requires Loom";
            plasma::Vector<2, float> textPosition(15.0f, getViewHeight() - 40.0f);
            m_engine->getFontEngine()->renderText(
                L"resource1.dat", message, 0.0f, 0.0f, textPosition[0], textPosition[1], 9.0f, 3.0f,
                plasma::Vector<4, float>(1.0f, 0.25f, 0.25f, 1.0f).m_elements,
                plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f).m_elements,
                plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, (1 << plasma::TEXT_WRAP),
                viewWidth() - m_craftButton->widget()->viewWidth() - 40.0f - 10.0f, true);
            m_engine->getFontEngine()->renderText(
                L"resource1.dat", message, 0.0f, 0.0f, textPosition[0], textPosition[1], 9.0f, 0.0f,
                plasma::Vector<4, float>(1.0f, 0.25f, 0.25f, 1.0f).m_elements,
                plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
                plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, (1 << plasma::TEXT_WRAP),
                viewWidth() - m_craftButton->widget()->viewWidth() - 40.0f - 10.0f, true);
        }
        break;
    }
    case WorkstationFurnace: {
        Creature *player = m_game->m_player;
        int cellX = (int)player->m_entityData.position[0] / 8;
        int cellY = (int)player->m_entityData.position[1] / 8;
        missingWorkstation = true;
        for (int x = cellX - 1; x <= cellX + 1; x++) {
            for (int y = cellY - 1; y <= cellY + 1; y++) {
                std::list<Zone::CellRecord> *objects = m_game->m_world.getCellObjects(x, y, 0);
                if (!objects)
                    continue;
                for (std::list<Zone::CellRecord>::iterator it = objects->begin(); it != objects->end(); ++it) {
                    if (it->object->type == StaticObjectFurnace &&
                        plasma::Vector<3, float>(it->object->position - player->m_entityData.position).squaredLength() <
                            16.0f) {
                        missingWorkstation = false;
                        goto furnaceSearched;
                    }
                }
            }
        }
    furnaceSearched:
        if (missingWorkstation) {
            std::wstring message = L"Requires Furnace";
            plasma::Vector<2, float> textPosition(15.0f, getViewHeight() - 40.0f);
            m_engine->getFontEngine()->renderText(
                L"resource1.dat", message, 0.0f, 0.0f, textPosition[0], textPosition[1], 9.0f, 3.0f,
                plasma::Vector<4, float>(1.0f, 0.25f, 0.25f, 1.0f).m_elements,
                plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f).m_elements,
                plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, (1 << plasma::TEXT_WRAP),
                viewWidth() - m_craftButton->widget()->viewWidth() - 40.0f - 10.0f, true);
            m_engine->getFontEngine()->renderText(
                L"resource1.dat", message, 0.0f, 0.0f, textPosition[0], textPosition[1], 9.0f, 0.0f,
                plasma::Vector<4, float>(1.0f, 0.25f, 0.25f, 1.0f).m_elements,
                plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
                plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, (1 << plasma::TEXT_WRAP),
                viewWidth() - m_craftButton->widget()->viewWidth() - 40.0f - 10.0f, true);
        }
        break;
    }
    case WorkstationAnvil: {
        Creature *player = m_game->m_player;
        int cellX = (int)player->m_entityData.position[0] / 8;
        int cellY = (int)player->m_entityData.position[1] / 8;
        missingWorkstation = true;
        for (int x = cellX - 1; x <= cellX + 1; x++) {
            for (int y = cellY - 1; y <= cellY + 1; y++) {
                std::list<Zone::CellRecord> *objects = m_game->m_world.getCellObjects(x, y, 0);
                if (!objects)
                    continue;
                for (std::list<Zone::CellRecord>::iterator it = objects->begin(); it != objects->end(); ++it) {
                    if (it->object->type == StaticObjectAnvil &&
                        plasma::Vector<3, float>(it->object->position - player->m_entityData.position).squaredLength() <
                            16.0f) {
                        missingWorkstation = false;
                        goto anvilSearched;
                    }
                }
            }
        }
    anvilSearched:
        if (missingWorkstation) {
            std::wstring message = L"Requires Anvil";
            plasma::Vector<2, float> textPosition(15.0f, getViewHeight() - 40.0f);
            m_engine->getFontEngine()->renderText(
                L"resource1.dat", message, 0.0f, 0.0f, textPosition[0], textPosition[1], 9.0f, 3.0f,
                plasma::Vector<4, float>(1.0f, 0.25f, 0.25f, 1.0f).m_elements,
                plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f).m_elements,
                plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, (1 << plasma::TEXT_WRAP),
                viewWidth() - m_craftButton->widget()->viewWidth() - 40.0f - 10.0f, true);
            m_engine->getFontEngine()->renderText(
                L"resource1.dat", message, 0.0f, 0.0f, textPosition[0], textPosition[1], 9.0f, 0.0f,
                plasma::Vector<4, float>(1.0f, 0.25f, 0.25f, 1.0f).m_elements,
                plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
                plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, (1 << plasma::TEXT_WRAP),
                viewWidth() - m_craftButton->widget()->viewWidth() - 40.0f - 10.0f, true);
        }
        break;
    }
    case WorkstationSaw: {
        Creature *player = m_game->m_player;
        int cellX = (int)player->m_entityData.position[0] / 8;
        int cellY = (int)player->m_entityData.position[1] / 8;
        missingWorkstation = true;
        for (int x = cellX - 1; x <= cellX + 1; x++) {
            for (int y = cellY - 1; y <= cellY + 1; y++) {
                std::list<Zone::CellRecord> *objects = m_game->m_world.getCellObjects(x, y, 0);
                if (!objects)
                    continue;
                for (std::list<Zone::CellRecord>::iterator it = objects->begin(); it != objects->end(); ++it) {
                    if (it->object->type == StaticObjectSaw &&
                        plasma::Vector<3, float>(it->object->position - player->m_entityData.position).squaredLength() <
                            16.0f) {
                        missingWorkstation = false;
                        goto sawSearched;
                    }
                }
            }
        }
    sawSearched:
        if (missingWorkstation) {
            std::wstring message = L"Requires Saw";
            plasma::Vector<2, float> textPosition(15.0f, getViewHeight() - 40.0f);
            m_engine->getFontEngine()->renderText(
                L"resource1.dat", message, 0.0f, 0.0f, textPosition[0], textPosition[1], 9.0f, 3.0f,
                plasma::Vector<4, float>(1.0f, 0.25f, 0.25f, 1.0f).m_elements,
                plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f).m_elements,
                plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, (1 << plasma::TEXT_WRAP),
                viewWidth() - m_craftButton->widget()->viewWidth() - 40.0f - 10.0f, true);
            m_engine->getFontEngine()->renderText(
                L"resource1.dat", message, 0.0f, 0.0f, textPosition[0], textPosition[1], 9.0f, 0.0f,
                plasma::Vector<4, float>(1.0f, 0.25f, 0.25f, 1.0f).m_elements,
                plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
                plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, (1 << plasma::TEXT_WRAP),
                viewWidth() - m_craftButton->widget()->viewWidth() - 40.0f - 10.0f, true);
        }
        break;
    }
    case WorkstationWorkbench: {
        Creature *player = m_game->m_player;
        int cellX = (int)player->m_entityData.position[0] / 8;
        int cellY = (int)player->m_entityData.position[1] / 8;
        missingWorkstation = true;
        for (int x = cellX - 1; x <= cellX + 1; x++) {
            for (int y = cellY - 1; y <= cellY + 1; y++) {
                std::list<Zone::CellRecord> *objects = m_game->m_world.getCellObjects(x, y, 0);
                if (!objects)
                    continue;
                for (std::list<Zone::CellRecord>::iterator it = objects->begin(); it != objects->end(); ++it) {
                    if (it->object->type == StaticObjectWorkbench &&
                        plasma::Vector<3, float>(it->object->position - player->m_entityData.position).squaredLength() <
                            16.0f) {
                        missingWorkstation = false;
                        goto workbenchSearched;
                    }
                }
            }
        }
    workbenchSearched:
        if (missingWorkstation) {
            std::wstring message = L"Requires Workbench";
            plasma::Vector<2, float> textPosition(15.0f, getViewHeight() - 40.0f);
            m_engine->getFontEngine()->renderText(
                L"resource1.dat", message, 0.0f, 0.0f, textPosition[0], textPosition[1], 9.0f, 3.0f,
                plasma::Vector<4, float>(1.0f, 0.25f, 0.25f, 1.0f).m_elements,
                plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f).m_elements,
                plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, (1 << plasma::TEXT_WRAP),
                viewWidth() - m_craftButton->widget()->viewWidth() - 40.0f - 10.0f, true);
            m_engine->getFontEngine()->renderText(
                L"resource1.dat", message, 0.0f, 0.0f, textPosition[0], textPosition[1], 9.0f, 0.0f,
                plasma::Vector<4, float>(1.0f, 0.25f, 0.25f, 1.0f).m_elements,
                plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
                plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, (1 << plasma::TEXT_WRAP),
                viewWidth() - m_craftButton->widget()->viewWidth() - 40.0f - 10.0f, true);
        }
        break;
    }
    case WorkstationCampfire: {
        Creature *player = m_game->m_player;
        int cellX = (int)player->m_entityData.position[0] / 8;
        int cellY = (int)player->m_entityData.position[1] / 8;
        missingWorkstation = true;
        for (int x = cellX - 1; x <= cellX + 1; x++) {
            for (int y = cellY - 1; y <= cellY + 1; y++) {
                std::list<Zone::CellRecord> *objects = m_game->m_world.getCellObjects(x, y, 0);
                if (!objects)
                    continue;
                for (std::list<Zone::CellRecord>::iterator it = objects->begin(); it != objects->end(); ++it) {
                    if (it->object->type == StaticObjectCampfire &&
                        plasma::Vector<3, float>(it->object->position - player->m_entityData.position).squaredLength() <
                            16.0f) {
                        missingWorkstation = false;
                        goto campfireSearched;
                    }
                }
            }
        }
    campfireSearched:
        if (missingWorkstation) {
            std::wstring message = L"Requires Campfire";
            plasma::Vector<2, float> textPosition(15.0f, getViewHeight() - 40.0f);
            m_engine->getFontEngine()->renderText(
                L"resource1.dat", message, 0.0f, 0.0f, textPosition[0], textPosition[1], 9.0f, 3.0f,
                plasma::Vector<4, float>(1.0f, 0.25f, 0.25f, 1.0f).m_elements,
                plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f).m_elements,
                plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, (1 << plasma::TEXT_WRAP),
                viewWidth() - m_craftButton->widget()->viewWidth() - 40.0f - 10.0f, true);
            m_engine->getFontEngine()->renderText(
                L"resource1.dat", message, 0.0f, 0.0f, textPosition[0], textPosition[1], 9.0f, 0.0f,
                plasma::Vector<4, float>(1.0f, 0.25f, 0.25f, 1.0f).m_elements,
                plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
                plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, (1 << plasma::TEXT_WRAP),
                viewWidth() - m_craftButton->widget()->viewWidth() - 40.0f - 10.0f, true);
        }
        break;
    }
    }

    // The craft button; the progress bar shows while crafting.
    if (m_craftButton) {
        if (!missingWorkstation && !missingIngredients && m_game->m_player->knowsFormula(m_formula.result)) {
            if (m_craftButton->widget()->isPointerOver()) {
                m_craftButton->findNode(L"frame")->display()->setFillColor(
                    plasma::Vector<4, float>(0.0f, 1.0f, 1.0f, 1.0f));
                m_craftButtonHovered = true;
            } else {
                m_craftButton->findNode(L"frame")->display()->setFillColor(
                    plasma::Vector<4, float>(0.0f, 1.0f, 0.0f, 1.0f));
                m_craftButtonHovered = false;
            }
        } else {
            m_craftButton->findNode(L"frame")->display()->setFillColor(
                plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f));
            m_craftButtonHovered = false;
        }
    }
    if (m_craftBar)
        m_craftBar->setVisible(m_game->m_craftingProgress >= 0.0f);
}

// In pixels: the item frame at y 90, the craft button 20 from the bottom right, the bar beside it
// (20 in, 10 apart).
void BlueprintPreviewWidget::resized()
{
    if (m_itemShadow) {
        if (m_itemShadow->m_parent != m_node->m_parent)
            m_node->m_parent->insertChild(m_itemShadow, m_node);
        plasma::Widget *widget = m_itemShadow->m_widget;
        widget->moveTo((viewWidth() - widget->viewWidth()) * 0.5f, 90.0f, true);
    }
    if (m_craftButton) {
        plasma::Widget *widget = m_craftButton->m_widget;
        widget->moveTo(viewWidth() - widget->viewWidth() - 20.0f, getViewHeight() - widget->getViewHeight() - 20.0f,
                       true);
    }
    if (m_craftBar && m_craftButton) {
        plasma::Widget *button = m_craftButton->m_widget;
        plasma::Widget *bar = m_craftBar->m_widget;
        bar->setViewRect(20.0f, getViewHeight() - button->getViewHeight() - 20.0f,
                         viewWidth() - button->viewWidth() - 40.0f - 10.0f, button->getViewHeight(), true);
    }
}

// The fill leaves 4 pixels at either end of the bar.
void BlueprintPreviewWidget::setProgress(float progress)
{
    if (m_craftProgressFill) {
        if (progress < 0.0f)
            progress = 0.0f;
        m_craftProgressFill->m_transformation->getDeformation().identity();
        plasma::Widget *bar = m_craftBar->m_widget;
        plasma::Widget *fill = m_craftProgressFill->m_widget;
        plasma::Matrix<float> &deformation = m_craftProgressFill->m_transformation->getDeformation();
        deformation.scale((bar->viewWidth() - 8.0f) * progress / fill->getContentWidth(), 1.0f);
        m_craftProgressFill->m_transformation->attributesChanged(plasma::ATTRIBUTES_CHANGED_ALL);
    }
}
}
