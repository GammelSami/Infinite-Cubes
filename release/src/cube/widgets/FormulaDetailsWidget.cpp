// See cube/FormulaDetailsWidget.h.
#include "plasma/TextIds.h"
#include "cube/ZoneIds.h"
#include "gfx/RendererIds.h"
#include <algorithm>
#include <sstream>
#include "cube/widgets/FormulaDetailsWidget.h"
#include "cube/Game.h"
#include "cube/SpriteManager.h"
#include "cube/World.h"
#include "cube/Zone.h"
#include "gfx/D3D11Renderer.h"
#include "plasma/Display.h"
#include "plasma/D3D11Engine.h"
#include "plasma/Node.h"
#include "cube/CreatureIds.h"
#include "plasma/EditFilter.h"

namespace cube {

FormulaDetailsWidget::FormulaDetailsWidget(Game *game, plasma::Node *node, plasma::Node *itemShadow,
                                           plasma::Node *craftButton, plasma::Node *craftBar)
    : BaseWidget(game->m_engine, node, L""),
      m_resultItemRotation(-120.0f, 0.0f, 0.0f),
      m_formulaCell(-1, -1),
      m_game(game),
      m_itemShadow(itemShadow),
      m_craftButton(craftButton),
      m_craftBar(craftBar),
      m_craftBarBar(nullptr),
      m_hoveredIngredientIndex(-1),
      m_craftButtonHovered(false)
{
    if (m_craftButton)
        m_craftButton->setParent(node);
    if (m_craftBar) {
        m_craftBar->setParent(node);
        m_craftBarBar = m_craftBar->findNode(L"bar");
    }
    if (m_itemShadow)
        m_itemShadow->setParent(node);
    setFont(L"resource1.dat");
}

void FormulaDetailsWidget::draw()
{
    if (!m_game->m_world->m_spriteManager->itemSprite(&m_formula.result))
        return;

    // The result item, turning, at the top centre; larger while the pointer is over it.
    plasma::Vector<2, float> position = getContentPosition();
    plasma::Matrix<float> matrix;
    m_node->getMatrix(matrix);
    plasma::Vector<2, float> itemPosition = position + plasma::Vector<2, float>(viewWidth() * 0.5f, matrix[0] * 110.0f);
    float itemX = itemPosition[0];
    float itemY = itemPosition[1];
    float itemScale = 0.1f;
    m_resultItemRotation[2] += m_engine->getFrameTime() * 0.02f;
    m_hoveredIngredientIndex = -1;
    m_hoveredItem.category = ItemCategoryNone;
    if (getPointerOffset()[0] > getContentWidth() * 0.5f - 60.0f && getPointerOffset()[1] > 50.0f &&
        getContentWidth() * 0.5f + 60.0f > getPointerOffset()[0] && 170.0f > getPointerOffset()[1]) {
        m_hoveredItem = m_formula.result;
        itemScale = 0.15f;
    }
    m_game->m_renderer->activateMode(gfx::RendererMode_OpaqueFront);
    m_game->m_renderer->drawItem(&m_formula.result, itemX, itemY, m_resultItemRotation, matrix[0] * itemScale, 0.0f,
                                 false);

    // The ingredients in a grid (70 x 45 units per cell) at the bottom.
    int columns = (int)((viewWidth() - 10.0f) / 45.0f);
    getViewHeight();
    position[1] += (getContentHeight() - 130.0f) * matrix[0];
    for (int i = 0; i < (int)m_formula.ingredients.size(); i++) {
        int x = (int)((20 + 70 * (i % columns)) * matrix[0] + position[0]);
        int y = (int)((10 + 45 * (i / columns)) * matrix[0] + position[1]);
        float iconScale = 0.025f;
        plasma::Vector<2, float> pointer = m_engine->getPointer();
        if (pointer[0] >= x && x + matrix[0] * 40.0f > pointer[0] && pointer[1] >= y &&
            y + matrix[0] * 40.0f > pointer[1]) {
            m_hoveredIngredientIndex = i;
            m_hoveredItem = m_formula.ingredients[i].item;
            iconScale = 0.0375f;
        }
        plasma::Vector<3, float> rotation(225.0f, 0.0f, (m_engine->getCaretClock() % 10000) * 0.0001f * 360.0f);
        m_game->m_renderer->drawItem(&m_formula.ingredients[i].item, matrix[0] * 20.0f + x, matrix[0] * 20.0f + y,
                                     rotation, matrix[0] * iconScale, 0.0f, true);
        m_game->m_interface.m_buttonCenters.push_back(
            plasma::Vector<2, int>((int)(matrix[0] * 20.0f + x), (int)(matrix[0] * 20.0f + y)));
    }
    m_engine->applyRenderState();

    // The result's name in its rarity colour.
    std::wstringstream text;
    setTextSize(12.0f);
    setTextStrokeRadius(3.0f);
    setTextStrokeColor(plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f));
    setTextFlags((1 << plasma::TEXT_CENTER) | (1 << plasma::TEXT_WRAP));
    setTextWrapWidth(getContentWidth() - 10.0f);
    bool missingIngredients = false;
    text << m_game->m_speech.createItemName(m_formula.result);
    setTextColor(m_formula.result.textColor(m_game->m_world->m_localCreature->m_entityData.region));
    renderText(text.str(), getContentWidth() * 0.5f, 30.0f);
    setTextFlags(1 << plasma::TEXT_CENTER);

    // "have/need" under each ingredient: green when the inventory holds enough, white otherwise.
    for (int i = 0; i < (int)m_formula.ingredients.size(); i++) {
        int row = i / columns;
        int needed = m_formula.ingredients[i].quantity;
        int x = 40 + 70 * (i % columns);
        int y = (int)(getContentHeight() - 130.0f + 45 * row + 55.0f);
        int held = 0;
        for (int tab = 0; tab < (int)m_game->m_world->m_localCreature->m_inventory.m_inventoryTabs.size(); tab++) {
            for (int slot = 0; slot < (int)m_game->m_world->m_localCreature->m_inventory.m_inventoryTabs[tab].size();
                 slot++) {
                if (m_game->m_world->m_localCreature->m_inventory.m_inventoryTabs[tab][slot].quantity &&
                    m_formula.ingredients[i].item ==
                        m_game->m_world->m_localCreature->m_inventory.m_inventoryTabs[tab][slot].item)
                    held += m_game->m_world->m_localCreature->m_inventory.m_inventoryTabs[tab][slot].quantity;
            }
        }
        plasma::Vector<4, float> color(0.0f, 1.0f, 0.0f, 1.0f);
        if (held < needed) {
            missingIngredients = true;
            color = plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f);
        }
        std::wstringstream count;
        count << held << L"/" << needed << std::endl;
        setTextColor(color);
        renderText(count.str(), (float)x, (float)y);
    }

    // The workstation the formula needs, if it is not near the player: each case searches the zones within 10 blocks
    // for the workstation's static entity (within 4 blocks) and says what is missing.
    bool missingWorkstation = false;
    switch (m_formula.workstationRequired) {
    case WorkstationWater: {
        // Water: the player stands in it or above a wet block.
        Creature *player = m_game->m_world->m_localCreature;
        if (!(player->m_entityData.flags & CreatureFlagInWater) &&
            m_game->m_world
                    ->getBlockAtDot(player->m_entityData.position[0], player->m_entityData.position[1],
                                    player->m_entityData.position[2] - player->m_entityData.appearance.size[2] * 0.5f -
                                        0.1)
                    .type != BlockWet) {
            missingWorkstation = true;
            std::wstring message = L"Requires water";
            setTextFlags(1 << plasma::TEXT_WRAP);
            setTextColor(plasma::Vector<4, float>(1.0f, 0.25f, 0.25f, 1.0f));
            renderText(message, 15.0f, getContentHeight() - 40.0f);
        }
        break;
    }
    case WorkstationSpinningWheel: {
        const plasma::Vector<3, Fixed> &position = m_game->m_world->m_localCreature->m_entityData.position;
        int minX = floorDivide((int)position[0] - 10, BlocksPerZone);
        int maxX = floorDivide((int)position[0] + 10, BlocksPerZone);
        int minY = floorDivide((int)position[1] - 10, BlocksPerZone);
        int maxY = floorDivide((int)position[1] + 10, BlocksPerZone);
        missingWorkstation = true;
        for (int x = minX; x <= maxX; x++) {
            for (int y = minY; y <= maxY; y++) {
                Zone *zone = m_game->m_world->zone(x, y);
                if (!zone)
                    continue;
                for (StaticEntity &entity : zone->m_staticEntities) {
                    if (entity.type != StaticEntitySpinningwheel)
                        continue;
                    plasma::Vector<3, float> offset(entity.position -
                                                    m_game->m_world->m_localCreature->m_entityData.position);
                    if (offset.squaredLength() < 16.0f) {
                        missingWorkstation = false;
                        goto searchedSpinningWheel;
                    }
                }
            }
        }
    searchedSpinningWheel:
        if (missingWorkstation) {
            std::wstring message = L"Requires Spinning Wheel";
            setTextFlags(1 << plasma::TEXT_WRAP);
            setTextColor(plasma::Vector<4, float>(1.0f, 0.25f, 0.25f, 1.0f));
            renderText(message, 15.0f, getContentHeight() - 40.0f);
        }
        break;
    }
    case WorkstationLoom: {
        const plasma::Vector<3, Fixed> &position = m_game->m_world->m_localCreature->m_entityData.position;
        int minX = floorDivide((int)position[0] - 10, BlocksPerZone);
        int maxX = floorDivide((int)position[0] + 10, BlocksPerZone);
        int minY = floorDivide((int)position[1] - 10, BlocksPerZone);
        int maxY = floorDivide((int)position[1] + 10, BlocksPerZone);
        missingWorkstation = true;
        for (int x = minX; x <= maxX; x++) {
            for (int y = minY; y <= maxY; y++) {
                Zone *zone = m_game->m_world->zone(x, y);
                if (!zone)
                    continue;
                for (StaticEntity &entity : zone->m_staticEntities) {
                    if (entity.type != StaticEntityLoom)
                        continue;
                    plasma::Vector<3, float> offset(entity.position -
                                                    m_game->m_world->m_localCreature->m_entityData.position);
                    if (offset.squaredLength() < 16.0f) {
                        missingWorkstation = false;
                        goto searchedLoom;
                    }
                }
            }
        }
    searchedLoom:
        if (missingWorkstation) {
            std::wstring message = L"Requires Loom";
            setTextFlags(1 << plasma::TEXT_WRAP);
            setTextColor(plasma::Vector<4, float>(1.0f, 0.25f, 0.25f, 1.0f));
            renderText(message, 15.0f, getContentHeight() - 40.0f);
        }
        break;
    }
    case WorkstationFurnace: {
        const plasma::Vector<3, Fixed> &position = m_game->m_world->m_localCreature->m_entityData.position;
        int minX = floorDivide((int)position[0] - 10, BlocksPerZone);
        int maxX = floorDivide((int)position[0] + 10, BlocksPerZone);
        int minY = floorDivide((int)position[1] - 10, BlocksPerZone);
        int maxY = floorDivide((int)position[1] + 10, BlocksPerZone);
        missingWorkstation = true;
        for (int x = minX; x <= maxX; x++) {
            for (int y = minY; y <= maxY; y++) {
                Zone *zone = m_game->m_world->zone(x, y);
                if (!zone)
                    continue;
                for (StaticEntity &entity : zone->m_staticEntities) {
                    if (entity.type != StaticEntityFurnace)
                        continue;
                    plasma::Vector<3, float> offset(entity.position -
                                                    m_game->m_world->m_localCreature->m_entityData.position);
                    if (offset.squaredLength() < 16.0f) {
                        missingWorkstation = false;
                        goto searchedFurnace;
                    }
                }
            }
        }
    searchedFurnace:
        if (missingWorkstation) {
            std::wstring message = L"Requires Furnace";
            setTextFlags(1 << plasma::TEXT_WRAP);
            setTextColor(plasma::Vector<4, float>(1.0f, 0.25f, 0.25f, 1.0f));
            renderText(message, 15.0f, getContentHeight() - 40.0f);
        }
        break;
    }
    case WorkstationAnvil: {
        const plasma::Vector<3, Fixed> &position = m_game->m_world->m_localCreature->m_entityData.position;
        int minX = floorDivide((int)position[0] - 10, BlocksPerZone);
        int maxX = floorDivide((int)position[0] + 10, BlocksPerZone);
        int minY = floorDivide((int)position[1] - 10, BlocksPerZone);
        int maxY = floorDivide((int)position[1] + 10, BlocksPerZone);
        missingWorkstation = true;
        for (int x = minX; x <= maxX; x++) {
            for (int y = minY; y <= maxY; y++) {
                Zone *zone = m_game->m_world->zone(x, y);
                if (!zone)
                    continue;
                for (StaticEntity &entity : zone->m_staticEntities) {
                    if (entity.type != StaticEntityAnvil)
                        continue;
                    plasma::Vector<3, float> offset(entity.position -
                                                    m_game->m_world->m_localCreature->m_entityData.position);
                    if (offset.squaredLength() < 16.0f) {
                        missingWorkstation = false;
                        goto searchedAnvil;
                    }
                }
            }
        }
    searchedAnvil:
        if (missingWorkstation) {
            std::wstring message = L"Requires Anvil";
            setTextFlags(1 << plasma::TEXT_WRAP);
            setTextColor(plasma::Vector<4, float>(1.0f, 0.25f, 0.25f, 1.0f));
            renderText(message, 15.0f, getContentHeight() - 40.0f);
        }
        break;
    }
    case WorkstationSaw: {
        const plasma::Vector<3, Fixed> &position = m_game->m_world->m_localCreature->m_entityData.position;
        int minX = floorDivide((int)position[0] - 10, BlocksPerZone);
        int maxX = floorDivide((int)position[0] + 10, BlocksPerZone);
        int minY = floorDivide((int)position[1] - 10, BlocksPerZone);
        int maxY = floorDivide((int)position[1] + 10, BlocksPerZone);
        missingWorkstation = true;
        for (int x = minX; x <= maxX; x++) {
            for (int y = minY; y <= maxY; y++) {
                Zone *zone = m_game->m_world->zone(x, y);
                if (!zone)
                    continue;
                for (StaticEntity &entity : zone->m_staticEntities) {
                    if (entity.type != StaticEntitySawbench)
                        continue;
                    plasma::Vector<3, float> offset(entity.position -
                                                    m_game->m_world->m_localCreature->m_entityData.position);
                    if (offset.squaredLength() < 16.0f) {
                        missingWorkstation = false;
                        goto searchedSaw;
                    }
                }
            }
        }
    searchedSaw:
        if (missingWorkstation) {
            std::wstring message = L"Requires Saw";
            setTextFlags(1 << plasma::TEXT_WRAP);
            setTextColor(plasma::Vector<4, float>(1.0f, 0.25f, 0.25f, 1.0f));
            renderText(message, 15.0f, getContentHeight() - 40.0f);
        }
        break;
    }
    case WorkstationWorkbench: {
        const plasma::Vector<3, Fixed> &position = m_game->m_world->m_localCreature->m_entityData.position;
        int minX = floorDivide((int)position[0] - 10, BlocksPerZone);
        int maxX = floorDivide((int)position[0] + 10, BlocksPerZone);
        int minY = floorDivide((int)position[1] - 10, BlocksPerZone);
        int maxY = floorDivide((int)position[1] + 10, BlocksPerZone);
        missingWorkstation = true;
        for (int x = minX; x <= maxX; x++) {
            for (int y = minY; y <= maxY; y++) {
                Zone *zone = m_game->m_world->zone(x, y);
                if (!zone)
                    continue;
                for (StaticEntity &entity : zone->m_staticEntities) {
                    if (entity.type != StaticEntityWorkbench)
                        continue;
                    plasma::Vector<3, float> offset(entity.position -
                                                    m_game->m_world->m_localCreature->m_entityData.position);
                    if (offset.squaredLength() < 16.0f) {
                        missingWorkstation = false;
                        goto searchedWorkbench;
                    }
                }
            }
        }
    searchedWorkbench:
        if (missingWorkstation) {
            std::wstring message = L"Requires Workbench";
            setTextFlags(1 << plasma::TEXT_WRAP);
            setTextColor(plasma::Vector<4, float>(1.0f, 0.25f, 0.25f, 1.0f));
            renderText(message, 15.0f, getContentHeight() - 40.0f);
        }
        break;
    }
    case WorkstationCampfire: {
        // A campfire entity within 4 blocks, or a campfire creature within 5 blocks.
        World *world = m_game->m_world;
        int minX = floorDivide((int)world->m_localCreature->m_entityData.position[0] - 10, BlocksPerZone);
        int maxX = floorDivide((int)world->m_localCreature->m_entityData.position[0] + 10, BlocksPerZone);
        int minY = floorDivide((int)world->m_localCreature->m_entityData.position[1] - 10, BlocksPerZone);
        int maxY = floorDivide((int)world->m_localCreature->m_entityData.position[1] + 10, BlocksPerZone);
        missingWorkstation = true;
        for (int x = minX; x <= maxX; x++) {
            for (int y = minY; y <= maxY; y++) {
                Zone *zone = world->zone(x, y);
                if (!zone)
                    continue;
                for (StaticEntity &entity : zone->m_staticEntities) {
                    if (entity.type != StaticEntityCampfire)
                        continue;
                    plasma::Vector<3, float> offset(entity.position - world->m_localCreature->m_entityData.position);
                    if (offset.squaredLength() < 16.0f) {
                        missingWorkstation = false;
                        goto searchedCampfire;
                    }
                }
            }
        }
    searchedCampfire:
        for (Creature *creature : world->m_creatures) {
            if (creature->m_entityData.race != RaceCampfire)
                continue;
            plasma::Vector<3, float> offset(creature->m_entityData.position -
                                            world->m_localCreature->m_entityData.position);
            if (offset.squaredLength() < 25.0f) {
                missingWorkstation = false;
                break;
            }
        }
        if (missingWorkstation) {
            std::wstring message = L"Requires Campfire";
            setTextFlags(1 << plasma::TEXT_WRAP);
            setTextColor(plasma::Vector<4, float>(1.0f, 0.25f, 0.25f, 1.0f));
            renderText(message, 15.0f, getContentHeight() - 40.0f);
        }
        break;
    }
    }

    // The craft button: cyan under the pointer when crafting is possible (Game crafts while it is held), green
    // otherwise, white when something is missing.
    if (m_craftButton) {
        plasma::Display *frame;
        if (!missingWorkstation && !missingIngredients) {
            if (m_craftButton->widget()->isPointerOver()) {
                frame = m_craftButton->findNode(L"frame")->display();
                frame->m_fillColor.value() = plasma::Vector<4, float>(0.0f, 1.0f, 1.0f, 1.0f);
                m_craftButtonHovered = true;
            } else {
                frame = m_craftButton->findNode(L"frame")->display();
                frame->m_fillColor.value() = plasma::Vector<4, float>(0.0f, 1.0f, 0.0f, 1.0f);
                m_craftButtonHovered = false;
            }
        } else {
            frame = m_craftButton->findNode(L"frame")->display();
            frame->m_fillColor.value() = plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f);
            m_craftButtonHovered = false;
        }
        plasma::Vector<2, float> center = m_craftButton->widget()->screenSize() * 0.5f;
        plasma::Vector<2, float> corner = m_craftButton->widget()->getContentPosition();
        m_game->m_interface.m_buttonCenters.push_back(
            plasma::Vector<2, int>((int)(corner[0] + center[0]), (int)(corner[1] + center[1])));
    }
    // The progress bar is visible while Game crafts.
    if (m_craftBar)
        m_craftBar->display()->m_visibility.value() = m_game->m_craftingProgress >= 0.0f;
}

void FormulaDetailsWidget::resized()
{
    if (m_itemShadow) {
        plasma::Widget *shadow = m_itemShadow->widget();
        shadow->moveTo((viewWidth() - shadow->viewWidth()) * 0.5f, 120.0f, true);
    }
    if (m_craftButton) {
        plasma::Widget *button = m_craftButton->widget();
        button->moveTo(viewWidth() - button->viewWidth() - 20.0f, getViewHeight() - button->getViewHeight() - 20.0f,
                       true);
    }
    if (m_craftBar && m_craftButton) {
        plasma::Widget *button = m_craftButton->widget();
        plasma::Widget *bar = m_craftBar->widget();
        bar->setViewRect(20.0f, getViewHeight() - button->getViewHeight() - 20.0f,
                         viewWidth() - button->viewWidth() - 40.0f - 10.0f, button->getViewHeight(), true);
    }
}

void FormulaDetailsWidget::setProgress(float progress)
{
    if (m_craftBarBar) {
        progress = (std::max)(progress, 0.0f);
        m_craftBarBar->transformation()->m_deformation.value().identity();
        m_craftBarBar->transformation()->m_deformation.value().scale(
            (m_craftBar->widget()->viewWidth() - 8.0f) * progress / m_craftBarBar->widget()->getContentWidth(), 1.0f);
        m_craftBarBar->transformation()->attributesChanged(true);
    }
}

}
