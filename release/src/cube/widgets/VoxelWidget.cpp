// See cube/VoxelWidget.h.
#include "plasma/TextIds.h"
#include "gfx/RendererIds.h"
#include <math.h>
#include <sstream>
#include "cube/widgets/VoxelWidget.h"
#include "cube/Sprite.h"
#include "cube/SpriteManager.h"
#include "gfx/D3D11Renderer.h"
#include "cube/Creature.h"
#include "cube/Game.h"
#include "cube/World.h"
#include "plasma/D3D11Engine.h"
#include "plasma/Engine.h"
#include "plasma/Node.h"
#include "cube/ItemIds.h"
#include "cube/SpriteIds.h"
#include "plasma/EditFilter.h"

namespace cube {

VoxelWidget::VoxelWidget(Game *game, plasma::Node *node)
    : BaseWidget(game->m_engine, node, L""),
      m_rotation(-90.0f, 0.0f, 90.0f),
      m_selectedUpgradeIndex(-1),
      m_game(game),
      m_selectedItemIndex(0),
      m_hoveredItemIndex(-1),
      m_voxelPlacementPosition(0, 0, 0),
      m_pointerOnVoxel(false),
      m_hoveredUpgradeIndex(-1),
      m_holdingPlacedUpgrade(false)
{
    setFont(L"resource1.dat");
}

void VoxelWidget::placeUpgrade(unsigned char material)
{
    Item *item = selectedItem();
    if (m_pointerOnVoxel && item) {
        if (m_selectedUpgradeIndex >= 0 && m_selectedUpgradeIndex < (int)m_availableCustomizations.size()) {
            // Place a new spirit of the selected upgrade's material, then use up one of that upgrade from the player's
            // inventory.
            if (item->spiritCount < 32) {
                item->addSpirit(m_voxelPlacementPosition[0], m_voxelPlacementPosition[1], m_voxelPlacementPosition[2],
                                m_availableCustomizations[m_selectedUpgradeIndex].item.material);
                for (std::vector<ItemStack> &tab : m_game->m_world->m_localCreature->m_inventory.m_inventoryTabs) {
                    for (ItemStack &stack : tab) {
                        if (stack.quantity && stack.item == m_availableCustomizations[m_selectedUpgradeIndex].item) {
                            if (--stack.quantity <= 0) {
                                stack.quantity = 0;
                                stack.item.category = ItemCategoryNone;
                                stack.item.id = NoneCleared;
                            }
                            return;
                        }
                    }
                }
            }
        } else if (m_holdingPlacedUpgrade) {
            // Move the held spirit to the voxel.
            item->removeSpirit(m_upgradePickupLocation[0], m_upgradePickupLocation[1], m_upgradePickupLocation[2]);
            item->addSpirit(m_voxelPlacementPosition[0], m_voxelPlacementPosition[1], m_voxelPlacementPosition[2],
                            m_upgradePickupMaterial);
            m_holdingPlacedUpgrade = false;
        } else {
            // Pick up the spirit at the voxel, if there is one.
            plasma::Vector<3, signed char> position(m_voxelPlacementPosition[0], m_voxelPlacementPosition[1],
                                                    m_voxelPlacementPosition[2]);
            for (int i = 0; i < item->spiritCount; i++) {
                if (!(item->spirits[i].position != position)) {
                    m_holdingPlacedUpgrade = true;
                    m_upgradePickupMaterial = item->spirits[i].material;
                    m_upgradePickupLocation = m_voxelPlacementPosition;
                    return;
                }
            }
        }
    } else {
        // A click outside the item puts a held spirit back.
        if (!m_engine->getPointerNode() && m_holdingPlacedUpgrade) {
            item->removeSpirit(m_upgradePickupLocation[0], m_upgradePickupLocation[1], m_upgradePickupLocation[2]);
            m_holdingPlacedUpgrade = false;
        }
        m_selectedUpgradeIndex = -1;
        m_holdingPlacedUpgrade = false;
    }
}

void VoxelWidget::draw()
{
    setTextSize(12.0f);
    setTextStrokeRadius(3.0f);
    setTextColor(plasma::Vector<4, float>(1.0f, 0.8f, 0.5f, 1.0f));
    setTextStrokeColor(plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f));
    setTextFlags(0);
    renderText(L"Weapon Customization", 15.0f, 25.0f);
    setTextColor(plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f));
    m_selectedUpgradeItem = Item();
    m_availableCustomizations.clear();
    m_game->m_renderer->activateMode(gfx::RendererMode_OpaqueFront);

    // The two equipped weapons at the bottom right, turning; the one under the pointer is drawn larger and becomes
    // m_hoveredItemIndex.
    plasma::Matrix<float> matrix;
    m_node->getMatrix(matrix);
    float scale = matrix.m_elements[0];
    plasma::Vector<2, float> origin = matrix * m_node->transformation()->m_pivot.value();
    int offset = 0;
    m_hoveredItemIndex = -1;
    if (m_game->m_world->m_localCreature->m_entityData.equipment.weaponRight.category) {
        float y = getViewHeight() + origin[1] - scale * 80.0f;
        float x = viewWidth() + origin[0] - scale * 60.0f;
        float itemScale = scale * 0.075f;
        if (scale * 50.0f > (float)fabs(x - m_engine->getPointer()[0]) &&
            scale * 50.0f > (float)fabs(y - m_engine->getPointer()[1])) {
            itemScale *= 1.25f;
            m_hoveredItemIndex = 0;
        }
        plasma::Vector<3, float> rotation(225.0f, 0.0f, (float)(m_engine->getCaretClock() % 10000) * 0.0001f * 360.0f);
        m_game->m_renderer->drawItem(&m_game->m_world->m_localCreature->m_entityData.equipment.weaponRight, x, y,
                                     rotation, itemScale, -0.05f, true);
        offset = (int)(scale * 100.0f + 0.0f);
        if (!selectedItem())
            m_selectedItemIndex = 0;
    }
    if (m_game->m_world->m_localCreature->m_entityData.equipment.weaponLeft.category) {
        float y = getViewHeight() + origin[1] - scale * 80.0f;
        float x = viewWidth() + origin[0] - scale * 60.0f - offset;
        float itemScale = scale * 0.075f;
        if (scale * 50.0f > (float)fabs(x - m_engine->getPointer()[0]) &&
            scale * 50.0f > (float)fabs(y - m_engine->getPointer()[1])) {
            itemScale *= 1.25f;
            m_hoveredItemIndex = 1;
        }
        plasma::Vector<3, float> rotation(225.0f, 0.0f, (float)(m_engine->getCaretClock() % 10000) * 0.0001f * 360.0f);
        m_game->m_renderer->drawItem(&m_game->m_world->m_localCreature->m_entityData.equipment.weaponLeft, x, y,
                                     rotation, itemScale, -0.05f, true);
        if (!selectedItem())
            m_selectedItemIndex = 1;
    }

    Item *item = selectedItem();
    if (item) {
        // The upgrades the player owns for this weapon: the cubes of the weapon's material (category 11, id 10), then
        // the four spirit cubes (id 14, materials 128..131).
        Creature *player = m_game->m_world->m_localCreature;
        Item upgrade;
        upgrade.category = ItemCategoryMaterial;
        upgrade.material = item->material;
        upgrade.id = MaterialItemCube;
        int count = 0;
        for (int tab = 0; tab < (int)player->m_inventory.m_inventoryTabs.size(); tab++) {
            for (ItemStack &stack : player->m_inventory.m_inventoryTabs[tab]) {
                if (stack.item == upgrade)
                    count += stack.quantity;
            }
        }
        if (count) {
            ItemStack stack;
            stack.quantity = count;
            stack.item = upgrade;
            stack.item.material = item->material;
            m_availableCustomizations.push_back(stack);
        }
        for (int material = ItemMaterialFireSpirit; material < ItemMaterialWind + 1; material++) {
            int spiritCount = 0;
            Creature *creature = m_game->m_world->m_localCreature;
            for (int tab = 0; tab < (int)creature->m_inventory.m_inventoryTabs.size(); tab++) {
                for (ItemStack &stack : creature->m_inventory.m_inventoryTabs[tab]) {
                    if (stack.item.category == ItemCategoryMaterial && stack.item.id == MaterialItemSpirit &&
                        stack.item.material == material)
                        spiritCount += stack.quantity;
                }
            }
            if (spiritCount) {
                ItemStack stack;
                stack.quantity = spiritCount;
                stack.item.category = ItemCategoryMaterial;
                stack.item.id = MaterialItemSpirit;
                stack.item.material = material;
                m_availableCustomizations.push_back(stack);
            }
        }

        // The upgrade that a click would place.
        if (m_hoveredUpgradeIndex < 0) {
            if (m_holdingPlacedUpgrade) {
                m_selectedUpgradeItem.category = ItemCategoryMaterial;
                m_selectedUpgradeItem.id =
                    m_upgradePickupMaterial < ItemMaterialFireSpirit ? MaterialItemCube : MaterialItemSpirit;
                m_selectedUpgradeItem.material = m_upgradePickupMaterial;
            } else if (m_selectedUpgradeIndex >= 0 && m_selectedUpgradeIndex < (int)m_availableCustomizations.size()) {
                m_selectedUpgradeItem = m_availableCustomizations[m_selectedUpgradeIndex].item;
            }
        }
        plasma::Vector<2, float> pivot = m_node->transformation()->m_pivot.value();
        if (m_selectedUpgradeIndex >= (int)m_availableCustomizations.size())
            m_selectedUpgradeIndex = (int)m_availableCustomizations.size() - 1;

        Sprite *sprite = m_game->m_world->m_spriteManager->itemSprite(item);
        if (sprite) {
            // The weapon's voxels in perspective, centred in the widget.
            float centerY = getViewHeight() * 0.6f;
            plasma::Vector<2, float> center = matrix * pivot;
            center[0] += viewWidth() * 0.5f;
            center[1] += centerY;
            int largest = sprite->m_size[0];
            if (sprite->m_size[1] > largest)
                largest = sprite->m_size[1];
            if (sprite->m_size[2] > largest)
                largest = sprite->m_size[2];
            plasma::Matrix<float> view;
            view.identity();
            plasma::Matrix<float> model;
            model.identity();
            model.translate(0.0f, 0.0f, 1.0f);
            model.rotateX(m_rotation[0]);
            model.rotateY(m_rotation[1]);
            model.rotateZ(m_rotation[2]);
            model.scale(0.35f, 0.35f, 0.35f);
            model.scale(1.0f / largest, 1.0f / largest, 1.0f / largest);
            model.translate(sprite->m_size[0] * -0.5f, sprite->m_size[1] * -0.5f, sprite->m_size[2] * -0.5f);
            float width = (float)m_engine->viewportWidth();
            float height = (float)m_engine->viewportHeight();
            float focal = 1.0f / tanf(0.39269909f);
            plasma::Matrix<float> projection;
            for (int i = 0; i < 16; i++)
                projection.m_elements[i] = 0.0f;
            projection.m_elements[0] = -(focal / (width / height));
            projection.m_elements[5] = focal;
            projection.m_elements[10] = 1.0001f;
            projection.m_elements[11] = 1.0f;
            projection.m_elements[14] = -0.10001f;
            plasma::Matrix<float> placement;
            placement.identity();
            float placeX = (center[0] - width * 0.5f) / width;
            placement.translate(placeX + placeX, (center[1] - height * 0.5f) / height * -2.0f, 0.0f);
            plasma::Matrix<float> inverse = placement;
            inverse *= projection;
            projection = inverse;
            inverse *= view;
            inverse *= model;
            inverse.invert();

            // The pointer's ray in voxel space, from the near to the far plane.
            float ndcX = m_engine->getPointer()[0] / width + m_engine->getPointer()[0] / width - 1.0f;
            float ndcY = 1.0f - (m_engine->getPointer()[1] / height + m_engine->getPointer()[1] / height);
            float w = 1.0f / (inverse.m_elements[7] * ndcY + inverse.m_elements[3] * ndcX +
                              inverse.m_elements[11] * 0.0f + inverse.m_elements[15]);
            plasma::Vector<3, float> start(w * (inverse.m_elements[4] * ndcY + inverse.m_elements[0] * ndcX +
                                                inverse.m_elements[8] * 0.0f + inverse.m_elements[12]),
                                           w * (inverse.m_elements[5] * ndcY + inverse.m_elements[1] * ndcX +
                                                inverse.m_elements[9] * 0.0f + inverse.m_elements[13]),
                                           w * (inverse.m_elements[6] * ndcY + inverse.m_elements[2] * ndcX +
                                                inverse.m_elements[10] * 0.0f + inverse.m_elements[14]));
            w = 1.0f / (ndcY * inverse.m_elements[7] + ndcX * inverse.m_elements[3] + inverse.m_elements[11] +
                        inverse.m_elements[15]);
            plasma::Vector<3, float> direction(w * (ndcY * inverse.m_elements[4] + ndcX * inverse.m_elements[0] +
                                                    inverse.m_elements[8] + inverse.m_elements[12]) -
                                                   start[0],
                                               w * (ndcY * inverse.m_elements[5] + ndcX * inverse.m_elements[1] +
                                                    inverse.m_elements[9] + inverse.m_elements[13]) -
                                                   start[1],
                                               w * (ndcY * inverse.m_elements[6] + ndcX * inverse.m_elements[2] +
                                                    inverse.m_elements[10] + inverse.m_elements[14]) -
                                                   start[2]);
            float length =
                sqrtf(direction[0] * direction[0] + direction[1] * direction[1] + direction[2] * direction[2]);
            direction[0] *= 1.0f / length;
            direction[1] *= 1.0f / length;
            direction[2] *= 1.0f / length;

            // Walk the voxels along the ray (at most 1000 units) until a solid voxel or a placed spirit;
            // m_pointerOnVoxel tells whether one was hit.
            float distance = 0.0f;
            plasma::Vector<3, int> voxel((int)start[0], (int)start[1], (int)start[2]);
            if (start[0] < 0.0f)
                voxel[0]--;
            if (start[1] < 0.0f)
                voxel[1]--;
            if (start[2] < 0.0f)
                voxel[2]--;
            plasma::Vector<3, int> face(0, 0, 0);
            m_pointerOnVoxel = false;
            while (distance < 1000.0f) {
                const plasma::Vector<3, unsigned char> *color = &Sprite::s_emptyVoxel;
                if (voxel[0] >= 0 && voxel[1] >= 0 && voxel[2] >= 0 && voxel[0] < sprite->m_size[0] &&
                    voxel[1] < sprite->m_size[1] && voxel[2] < sprite->m_size[2])
                    color = &sprite->m_voxels[voxel[0] + sprite->m_size[0] * (voxel[1] + sprite->m_size[1] * voxel[2])];
                if (!sprite->isEmpty(color, false))
                    m_pointerOnVoxel = true;
                for (int i = 0; i < item->spiritCount; i++) {
                    if (m_holdingPlacedUpgrade &&
                        !(item->spirits[i].position != plasma::Vector<3, signed char>(m_upgradePickupLocation[0],
                                                                                      m_upgradePickupLocation[1],
                                                                                      m_upgradePickupLocation[2])))
                        continue;
                    if (!(item->spirits[i].position != plasma::Vector<3, signed char>(voxel[0], voxel[1], voxel[2]))) {
                        // The pointer is on a placed spirit: show it.
                        m_pointerOnVoxel = true;
                        m_selectedUpgradeItem.category = ItemCategoryMaterial;
                        m_selectedUpgradeItem.id = MaterialItemSpirit;
                        m_selectedUpgradeItem.material = item->spirits[i].material;
                        m_selectedUpgradeItem.rarity = ItemRarityBlue;
                        goto found;
                    }
                }
                if (m_pointerOnVoxel)
                    break;
                {
                    // The nearest voxel boundary along the ray.
                    float step = 10.0f;
                    int axis = 0;
                    if (direction[0] != 0.0f) {
                        float d = ((float)(direction[0] > 0.0f ? voxel[0] + 1 : voxel[0]) -
                                   (start[0] + direction[0] * distance)) /
                                  direction[0];
                        if (d < 10.0f)
                            step = d;
                    }
                    if (direction[1] != 0.0f) {
                        float d = ((float)(direction[1] > 0.0f ? voxel[1] + 1 : voxel[1]) -
                                   (start[1] + direction[1] * distance)) /
                                  direction[1];
                        if (step > d) {
                            axis = 1;
                            step = d;
                        }
                    }
                    if (direction[2] != 0.0f) {
                        float d = ((float)(direction[2] > 0.0f ? voxel[2] + 1 : voxel[2]) -
                                   (start[2] + direction[2] * distance)) /
                                  direction[2];
                        if (step > d) {
                            axis = 2;
                            step = d;
                        }
                    }
                    face = plasma::Vector<3, int>(0, 0, 0);
                    if (direction[axis] > 0.0f) {
                        voxel[axis]++;
                        face[axis] = -1;
                    } else {
                        voxel[axis]--;
                        face[axis] = 1;
                    }
                    distance += step;
                }
            }
        found:
            m_voxelPlacementPosition = voxel;
            m_game->m_renderer->drawSprite(sprite, model, view, projection,
                                           plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f));

            // The upgrade to place, at the free voxel in front of the hit face (or far along the ray when nothing is
            // hit).
            if (m_hoveredUpgradeIndex < 0 &&
                (m_holdingPlacedUpgrade ||
                 (m_selectedUpgradeIndex >= 0 && m_selectedUpgradeIndex < (int)m_availableCustomizations.size()))) {
                plasma::Matrix<float> cube = model;
                if (m_pointerOnVoxel) {
                    m_voxelPlacementPosition += face;
                    cube.translate((float)m_voxelPlacementPosition[0], (float)m_voxelPlacementPosition[1],
                                   (float)m_voxelPlacementPosition[2]);
                } else {
                    cube.translate(direction[0] * 100.0f + start[0], direction[1] * 100.0f + start[1],
                                   direction[2] * 100.0f + start[2]);
                }
                plasma::Vector<4, float> white(1.0f, 1.0f, 1.0f, 1.0f);
                Sprite *spirit;
                int material;
                if (m_holdingPlacedUpgrade) {
                    spirit = m_game->m_world->m_spriteManager->sprite(SpriteCube);
                    material = m_upgradePickupMaterial;
                } else {
                    spirit = m_game->m_world->m_spriteManager->sprite(SpriteCube);
                    material = m_availableCustomizations[m_selectedUpgradeIndex].item.material;
                }
                m_game->m_renderer->drawSprite(spirit, cube, view, projection, materialColor(material, white, 0.0f));
            }

            // The placed spirits (the hovered one pulsing, the held one not at its old place).
            for (int i = 0; i < item->spiritCount; i++) {
                Spirit &placed = item->spirits[i];
                if (m_holdingPlacedUpgrade &&
                    !(placed.position != plasma::Vector<3, signed char>(m_upgradePickupLocation[0],
                                                                        m_upgradePickupLocation[1],
                                                                        m_upgradePickupLocation[2])))
                    continue;
                plasma::Matrix<float> cube = model;
                cube.translate(placed.position[0], placed.position[1], placed.position[2]);
                if (m_pointerOnVoxel &&
                    !(placed.position != plasma::Vector<3, signed char>(m_voxelPlacementPosition[0],
                                                                        m_voxelPlacementPosition[1],
                                                                        m_voxelPlacementPosition[2]))) {
                    cube.translate(0.5f, 0.5f, 0.5f);
                    cube.scale(1.1f, 1.1f, 1.1f);
                    cube.translate(-0.5f, -0.5f, -0.5f);
                    float pulse = cosf(m_engine->getCaretClock() * 0.01f) * 0.2f + 1.0f;
                    plasma::Vector<4, float> tint = plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f) * pulse;
                    m_game->m_renderer->drawSprite(m_game->m_world->m_spriteManager->sprite(SpriteCube), cube, view,
                                                   projection, materialColor(placed.material, tint, 0.0f));
                } else {
                    plasma::Vector<4, float> white(1.0f, 1.0f, 1.0f, 1.0f);
                    m_game->m_renderer->drawSprite(m_game->m_world->m_spriteManager->sprite(SpriteCube), cube, view,
                                                   projection, materialColor(placed.material, white, 0.0f));
                }
            }
        }

        // The upgrade list: eight per row, the hovered or selected one larger.
        plasma::Vector<2, float> listOrigin = matrix * pivot;
        m_hoveredUpgradeIndex = -1;
        for (int i = 0; i < (int)m_availableCustomizations.size(); i++) {
            Item shown(m_availableCustomizations[i].item);
            int x = (int)(45 * (i % 8) + (listOrigin[0] + 20.0f));
            int y = (int)(45 * (i / 8) + (listOrigin[1] + 40.0f));
            float itemScale = 0.1f;
            if (m_engine->getPointer()[0] >= x && x + 40 > m_engine->getPointer()[0] &&
                m_engine->getPointer()[1] >= y && y + 40 > m_engine->getPointer()[1]) {
                m_hoveredUpgradeIndex = i;
                m_previewUpgradeItem = shown;
            }
            if (m_hoveredUpgradeIndex == i || m_selectedUpgradeIndex == i)
                itemScale = 0.15f;
            plasma::Vector<3, float> rotation(225.0f, 0.0f,
                                              (float)(m_engine->getCaretClock() % 10000) * 0.0001f * 360.0f);
            m_game->m_renderer->drawItem(&shown, (float)(x + 20), (float)(y + 20), rotation, itemScale, 0.0f, true);
        }
        m_engine->applyRenderState();
        std::wstringstream stream;
        stream << L"Upgrades " << (int)item->spiritCount << "/" << item->maxSpiritCount() << std::endl;
        renderText(stream.str(), 20.0f, getContentHeight() - 20.0f);
    }
    m_engine->applyRenderState();

    // The owned count of each upgrade, under its icon.
    setTextFlags(1 << plasma::TEXT_RIGHT);
    for (int i = 0; i < (int)m_availableCustomizations.size(); i++) {
        std::wstringstream stream;
        stream << m_availableCustomizations[i].quantity << std::endl;
        renderText(stream.str(), (float)(45 * (i % 8) + 60), (float)(45 * (i / 8) + 80));
    }
}

void VoxelWidget::cancelUpgrade()
{
    m_selectedUpgradeIndex = -1;
    m_holdingPlacedUpgrade = false;
}

void VoxelWidget::selectUpgrade()
{
    m_selectedUpgradeIndex = m_selectedUpgradeIndex == m_hoveredUpgradeIndex ? -1 : m_hoveredUpgradeIndex;
    m_game->playSound(SoundMenuSelect);
}

void VoxelWidget::selectItem()
{
    m_selectedItemIndex = m_hoveredItemIndex;
    m_game->playSound(SoundMenuSelect);
}

Item *VoxelWidget::selectedItem()
{
    // An empty slot holds category 0.
    if (m_selectedItemIndex == 1) {
        Item *item = &m_game->m_world->m_localCreature->m_entityData.equipment.weaponLeft;
        return item->category ? item : nullptr;
    } else if (m_selectedItemIndex == 0) {
        return m_game->m_world->m_localCreature->m_entityData.equipment.weaponRight.category
                   ? &m_game->m_world->m_localCreature->m_entityData.equipment.weaponRight
                   : nullptr;
    }
    return nullptr;
}

}
