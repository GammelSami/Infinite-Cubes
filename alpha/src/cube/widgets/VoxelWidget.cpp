// VoxelWidget edits spirit cubes in equipment.
#include "cube/widgets/VoxelWidget.h"
#include "cube/DefaultBlocks.h"
#include "cube/GameController.h"
#include "cube/Creature.h"
#include "cube/CubeShader.h"
#include "cube/ItemIds.h"
#include "cube/StatCurves.h"
#include "plasma/Engine.h"
#include "plasma/FontEngine.h"
#include "plasma/Font.h"
#include "plasma/Node.h"
#include "plasma/Transformation.h"
#include <math.h>
#include <sstream>
namespace cube {

// The editor turns the item 90 degrees about z and -90 about x at first; its cube sprite is a
// single white voxel.
VoxelWidget::VoxelWidget(plasma::Engine *engine, plasma::Node *node, GameController *game)
    : Widget(engine, node),
      m_item(0),
      m_rotation(-90.0f, 0.0f, 90.0f),
      m_selectedUpgradeIndex(-1),
      m_game(game),
      m_voxelPlacementPosition(0, 0, 0),
      m_pointerOnVoxel(false),
      m_hoveredUpgradeIndex(-1),
      m_holdingPlacedUpgrade(false),
      m_upgradePickupLevel(0),
      m_voxelSprite(game->m_device, false)
{
    m_voxelSprite.resize(1, 1, 1);
    m_voxelSprite.voxel(0, 0, 0) = plasma::Vector<3, unsigned char>(255, 255, 255);
    m_voxelSprite.updateMesh();
}

// A click on the item's model (placeUpgrade): with an upgrade selected, a spirit cube of its
// material is placed at the voxel under the pointer and one such upgrade is used up from the
// inventory; a held cube is moved there; otherwise the cube there is picked up. A click outside
// the model puts a held cube back.
void VoxelWidget::placeUpgrade(int material, int level)
{
    if (m_pointerOnVoxel && m_item) {
        if (m_selectedUpgradeIndex >= 0 && m_selectedUpgradeIndex < (int)m_availableCustomizations.size()) {
            if (m_item->spiritCount < Item::SpiritCapacity) {
                m_item->addSpirit(m_voxelPlacementPosition[0], m_voxelPlacementPosition[1], m_voxelPlacementPosition[2],
                                  m_availableCustomizations[m_selectedUpgradeIndex].item.material,
                                  m_availableCustomizations[m_selectedUpgradeIndex].item.level);
                for (std::vector<std::vector<ItemStack>>::iterator tab =
                         m_game->m_player->m_inventory.m_inventoryTabs.begin();
                     tab != m_game->m_player->m_inventory.m_inventoryTabs.end(); ++tab) {
                    for (std::vector<ItemStack>::iterator stack = tab->begin(); stack != tab->end(); ++stack) {
                        if (stack->quantity != 0 &&
                            stack->item.category == m_availableCustomizations[m_selectedUpgradeIndex].item.category &&
                            stack->item.id == m_availableCustomizations[m_selectedUpgradeIndex].item.id &&
                            stack->item.material == m_availableCustomizations[m_selectedUpgradeIndex].item.material &&
                            stack->item.power() == m_availableCustomizations[m_selectedUpgradeIndex].item.power()) {
                            stack->removeOne();
                            return;
                        }
                    }
                }
            }
        } else if (m_holdingPlacedUpgrade) {
            m_item->removeSpirit(m_upgradePickupLocation[0], m_upgradePickupLocation[1], m_upgradePickupLocation[2]);
            m_item->addSpirit(m_voxelPlacementPosition[0], m_voxelPlacementPosition[1], m_voxelPlacementPosition[2],
                              m_upgradePickupMaterial, m_upgradePickupLevel);
            m_holdingPlacedUpgrade = false;
        } else {
            for (int i = 0; i < m_item->spiritCount; i++) {
                plasma::Vector<3, signed char> position((signed char)m_voxelPlacementPosition[0],
                                                        (signed char)m_voxelPlacementPosition[1],
                                                        (signed char)m_voxelPlacementPosition[2]);
                if (!(m_item->spirits[i].position != position)) {
                    m_holdingPlacedUpgrade = true;
                    m_upgradePickupMaterial = m_item->spirits[i].material;
                    m_upgradePickupLevel = m_item->spirits[i].level;
                    m_upgradePickupLocation = m_voxelPlacementPosition;
                    return;
                }
            }
        }
    } else {
        if (!m_engine->getPointerNode() && m_holdingPlacedUpgrade) {
            m_item->removeSpirit(m_upgradePickupLocation[0], m_upgradePickupLocation[1], m_upgradePickupLocation[2]);
            m_holdingPlacedUpgrade = false;
        }
        m_selectedUpgradeIndex = -1;
        m_holdingPlacedUpgrade = false;
    }
}

// The weapon customization screen: the upgrades the player owns for the item (cubes of its
// material, and the spirit cubes of materials 128..131 whose power lies within 10 below the
// item's), the item's model turning in the middle with the voxel under the pointer found by
// walking the pointer's ray through the model, the upgrade a click would place (in front of the
// face hit, or along the ray), the placed spirits (the one under the pointer pulsing), the list of
// upgrades (eight per row; the hovered or selected one larger), "Upgrades n/max", the title, and
// the owned count of each upgrade. Its layout is in pixels: the model centred 60% down the widget
// at a quarter of its largest side, in the previews' perspective (45 degrees, depth terms 1.0001
// and -0.10001); the upgrades in 40-pixel cells 45 apart, eight a row, from 20, 40 in (their
// counts at 60, 80); "Upgrades n/max" 20 in from the bottom left (10 high, stroke 2), the title at
// 15, 25 (12 high, stroke 3). The spirit under the pointer pulses (0.01 radians per millisecond of
// the caret clock, by a fifth). The ray's boundary search starts from 10 voxels, more than any
// step within the model.
void VoxelWidget::draw()
{
    m_selectedUpgradeItem = Item();
    m_availableCustomizations.clear();
    if (m_item) {
        Item upgrade;
        upgrade.material = m_item->material;
        Creature *player = m_game->m_player;
        upgrade.category = ItemCategoryMaterial;
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
            m_availableCustomizations.push_back(stack);
        }
        int minimumPower = m_item->power() - 10;
        int maximumPower = m_item->power();
        if (minimumPower < 1)
            minimumPower = 1;
        for (int power = minimumPower; power <= maximumPower; power++) {
            for (int material = ItemMaterialFireSpirit; material < (ItemMaterialWindSpirit + 1); material++) {
                int spiritCount = 0;
                for (int tab = 0; tab < (int)m_game->m_player->m_inventory.m_inventoryTabs.size(); tab++) {
                    for (std::vector<ItemStack>::iterator it =
                             m_game->m_player->m_inventory.m_inventoryTabs[tab].begin();
                         it != m_game->m_player->m_inventory.m_inventoryTabs[tab].end(); ++it) {
                        if (it->item.category == ItemCategoryMaterial && it->item.id == MaterialItemSpirit &&
                            it->item.material == material && it->item.power() == power)
                            spiritCount += it->quantity;
                    }
                }
                if (spiritCount) {
                    ItemStack stack;
                    stack.quantity = spiritCount;
                    stack.item.category = ItemCategoryMaterial;
                    stack.item.id = MaterialItemSpirit;
                    stack.item.material = material;
                    stack.item.level = (short)levelFromBonus(power * 0.01f);
                    m_availableCustomizations.push_back(stack);
                }
            }
        }

        // The upgrade that a click would place.
        if (m_hoveredUpgradeIndex < 0 &&
            (m_holdingPlacedUpgrade ||
             (m_selectedUpgradeIndex >= 0 && m_selectedUpgradeIndex < (int)m_availableCustomizations.size()))) {
            if (m_holdingPlacedUpgrade) {
                m_selectedUpgradeItem.category = ItemCategoryMaterial;
                m_selectedUpgradeItem.id =
                    m_upgradePickupMaterial >= ItemMaterialFireSpirit ? MaterialItemSpirit : MaterialItemCube;
                m_selectedUpgradeItem.level = m_upgradePickupLevel;
                m_selectedUpgradeItem.material = m_upgradePickupMaterial;
            } else {
                m_selectedUpgradeItem = m_availableCustomizations[m_selectedUpgradeIndex].item;
            }
        }
        plasma::Vector<2, float> pivot = m_node->transformation()->m_pivot.value();
        if (m_selectedUpgradeIndex >= (int)m_availableCustomizations.size())
            m_selectedUpgradeIndex = (int)m_availableCustomizations.size() - 1;

        Sprite *sprite = m_game->m_world.m_spriteManager.itemSprite(m_item);
        if (sprite) {
            m_game->m_device->SetRenderState(D3DRS_ZENABLE, TRUE);
            plasma::Vector<4, float> lightPositions[PointLightSourceCount];
            plasma::Vector<3, float> lightColors[PointLightSourceCount];
            for (int i = 0; i < PointLightSourceCount; i++) {
                lightPositions[i] = plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f);
                lightColors[i] = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
            }
            m_game->m_shader->apply();
            m_game->m_shader->setPointLights(lightPositions, lightColors);
            m_game->m_shader->setMaterialColor(plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f));
            m_game->m_shader->setAlpha(1.0f);

            // The item's voxels in perspective, in the middle of the widget.
            float centerX = viewWidth() * 0.5f;
            float centerY = getViewHeight() * 0.6f;
            plasma::Vector<2, float> center = m_node->m_matrix.transform(pivot);
            center[0] += centerX;
            center[1] += centerY;
            int largest = sprite->sizeX();
            if (sprite->sizeY() > largest)
                largest = sprite->sizeY();
            if (sprite->sizeZ() > largest)
                largest = sprite->sizeZ();
            plasma::Matrix<float> view;
            view.identity();
            plasma::Matrix<float> model;
            model.identity();
            model.translate(0.0f, 0.0f, 1.0f);
            model.rotateX(m_rotation[0]);
            model.rotateY(m_rotation[1]);
            model.rotateZ(m_rotation[2]);
            model.scale(0.25f, 0.25f, 0.25f);
            model.scale(1.0f / largest, 1.0f / largest, 1.0f / largest);
            model.translate(sprite->sizeX() * -0.5f, sprite->sizeY() * -0.5f, sprite->sizeZ() * -0.5f);
            float width = (float)m_engine->viewportWidth();
            float height = (float)m_engine->viewportHeight();
            float focal = 1.0f / tan(0.39269909f);
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
            placement.translate((center[0] - width * 0.5f) / width * 2.0f, (center[1] - height * 0.5f) / height * -2.0f,
                                0.0f);
            projection = placement * projection;
            plasma::Matrix<float> inverse = projection * view * model;
            inverse.invert();

            // The pointer's ray in voxel space, from the near to the far plane.
            float x = m_engine->getPointer()[0] / (float)m_engine->viewportWidth() * 2.0f - 1.0f;
            float y = 1.0f - m_engine->getPointer()[1] / (float)m_engine->viewportHeight() * 2.0f;
            float w = 1.0f / (inverse.m_elements[7] * y + inverse.m_elements[3] * x + inverse.m_elements[11] * 0.0f +
                              inverse.m_elements[15]);
            plasma::Vector<3, float> start(w * (inverse.m_elements[4] * y + inverse.m_elements[0] * x +
                                                inverse.m_elements[8] * 0.0f + inverse.m_elements[12]),
                                           w * (inverse.m_elements[5] * y + inverse.m_elements[1] * x +
                                                inverse.m_elements[9] * 0.0f + inverse.m_elements[13]),
                                           w * (inverse.m_elements[6] * y + inverse.m_elements[2] * x +
                                                inverse.m_elements[10] * 0.0f + inverse.m_elements[14]));
            x = m_engine->getPointer()[0] / (float)m_engine->viewportWidth() * 2.0f - 1.0f;
            y = 1.0f - m_engine->getPointer()[1] / (float)m_engine->viewportHeight() * 2.0f;
            w = 1.0f / (y * inverse.m_elements[7] + x * inverse.m_elements[3] + inverse.m_elements[11] +
                        inverse.m_elements[15]);
            plasma::Vector<3, float> direction(w * (y * inverse.m_elements[4] + x * inverse.m_elements[0] +
                                                    inverse.m_elements[8] + inverse.m_elements[12]) -
                                                   start[0],
                                               w * (y * inverse.m_elements[5] + x * inverse.m_elements[1] +
                                                    inverse.m_elements[9] + inverse.m_elements[13]) -
                                                   start[1],
                                               w * (y * inverse.m_elements[6] + x * inverse.m_elements[2] +
                                                    inverse.m_elements[10] + inverse.m_elements[14]) -
                                                   start[2]);
            float length =
                sqrt(direction[0] * direction[0] + direction[1] * direction[1] + direction[2] * direction[2]);
            direction[0] *= 1.0f / length;
            direction[1] *= 1.0f / length;
            direction[2] *= 1.0f / length;

            // Walk the voxels along the ray (at most 1000 units) until a solid voxel or a placed spirit.
            float distance = 0.0f;
            plasma::Vector<3, int> voxel(start);
            if (start[0] < 0.0f)
                voxel[0]--;
            if (start[1] < 0.0f)
                voxel[1]--;
            if (start[2] < 0.0f)
                voxel[2]--;
            plasma::Vector<3, int> face(0, 0, 0);
            m_pointerOnVoxel = false;
            while (distance < 1000.0f) {
                if (!sprite->isEmpty(&sprite->voxel(voxel[0], voxel[1], voxel[2]), false))
                    m_pointerOnVoxel = true;
                for (int i = 0; i < m_item->spiritCount; i++) {
                    if (m_holdingPlacedUpgrade &&
                        !(m_item->spirits[i].position != plasma::Vector<3, signed char>(m_upgradePickupLocation[0],
                                                                                        m_upgradePickupLocation[1],
                                                                                        m_upgradePickupLocation[2])))
                        continue;
                    if (!(m_item->spirits[i].position !=
                          plasma::Vector<3, signed char>(voxel[0], voxel[1], voxel[2]))) {
                        // The pointer is on a placed spirit: show it.
                        m_pointerOnVoxel = true;
                        m_selectedUpgradeItem.category = ItemCategoryMaterial;
                        m_selectedUpgradeItem.id = MaterialItemSpirit;
                        m_selectedUpgradeItem.material = m_item->spirits[i].material;
                        m_selectedUpgradeItem.level = m_item->spirits[i].level;
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
            m_game->m_shader->setMatrices(model, view, projection);
            sprite->draw();

            // The upgrade to place, at the free voxel in front of the hit face (or 100 units along the ray
            // when nothing is hit).
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
                m_game->m_shader->setMaterialColor(materialColor(
                    m_holdingPlacedUpgrade ? m_upgradePickupMaterial
                                           : m_availableCustomizations[m_selectedUpgradeIndex].item.material,
                    white, 0.0f));
                m_game->m_shader->setMatrices(cube, view, projection);
                m_voxelSprite.draw();
            }

            // The placed spirits (the one under the pointer pulsing, the held one not at its old place).
            for (int i = 0; i < m_item->spiritCount; i++) {
                Spirit &placed = m_item->spirits[i];
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
                    float pulse = cos(m_engine->getCaretClock() * 0.01f) * 0.2f + 1.0f;
                    plasma::Vector<4, float> tint = plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f) * pulse;
                    m_game->m_shader->setMaterialColor(materialColor(placed.material, tint, 0.0f));
                } else {
                    plasma::Vector<4, float> white(1.0f, 1.0f, 1.0f, 1.0f);
                    m_game->m_shader->setMaterialColor(materialColor(placed.material, white, 0.0f));
                }
                m_game->m_shader->setMatrices(cube, view, projection);
                m_voxelSprite.draw();
            }
        }

        // The upgrade list: eight per row, the hovered or selected one larger.
        plasma::Vector<2, float> listOrigin = m_node->m_matrix.transform(pivot);
        m_hoveredUpgradeIndex = -1;
        for (int i = 0; i < (int)m_availableCustomizations.size(); i++) {
            Item shown(m_availableCustomizations[i].item);
            float itemScale = 0.02f;
            int x = (int)(45 * (i % 8) + (listOrigin[0] + 20.0f));
            int y = (int)(45 * (i / 8) + (listOrigin[1] + 40.0f));
            if (m_engine->getPointer()[0] >= x && x + 40 > m_engine->getPointer()[0] &&
                m_engine->getPointer()[1] >= y && y + 40 > m_engine->getPointer()[1]) {
                m_hoveredUpgradeIndex = i;
                m_previewUpgradeItem = shown;
            }
            if (m_hoveredUpgradeIndex == i || m_selectedUpgradeIndex == i)
                itemScale = 0.03f;
            m_game->m_shader->setMaterialColor(plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f));
            m_game->drawItem((float)(x + 20), (float)(y + 20), m_game->m_previewRotation, itemScale, &shown, 0.0f);
        }
        m_game->m_device->SetRenderState(D3DRS_ZENABLE, FALSE);
        std::wstringstream text;
        text << L"Upgrades " << m_item->spiritCount << "/" << m_item->maxSpiritCount() << std::endl;
        m_engine->getFontEngine()->renderText(
            L"resource1.dat", text.str(), 0.0f, 0.0f, 20.0f, getContentHeight() - 20.0f, 10.0f, 2.0f,
            plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f).m_elements,
            plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f).m_elements,
            plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, plasma::TEXT_FLAGS_NONE, -1.0f, true);
        m_engine->getFontEngine()->renderText(
            L"resource1.dat", text.str(), 0.0f, 0.0f, 20.0f, getContentHeight() - 20.0f, 10.0f, 0.0f,
            plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f).m_elements,
            plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
            plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, plasma::TEXT_FLAGS_NONE, -1.0f, true);
    }

    m_engine->getFontEngine()->renderText(L"resource1.dat", L"Weapon Customization", 0.0f, 0.0f, 15.0f, 25.0f, 12.0f,
                                          3.0f, plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f).m_elements,
                                          plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f).m_elements,
                                          plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
                                          plasma::TEXT_FLAGS_NONE, -1.0f, true);
    m_engine->getFontEngine()->renderText(L"resource1.dat", L"Weapon Customization", 0.0f, 0.0f, 15.0f, 25.0f, 12.0f,
                                          0.0f, plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f).m_elements,
                                          plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
                                          plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
                                          plasma::TEXT_FLAGS_NONE, -1.0f, true);

    // The owned count of each upgrade, under its icon; cyan for the selected one.
    for (int i = 0; i < (int)m_availableCustomizations.size(); i++) {
        int x = 60 + 45 * (i % 8);
        int y = 80 + 45 * (i / 8);
        plasma::Vector<4, float> color(1.0f, 1.0f, 1.0f, 1.0f);
        if (i == m_selectedUpgradeIndex)
            color = plasma::Vector<4, float>(0.0f, 1.0f, 1.0f, 1.0f);
        std::wstringstream count;
        count << m_availableCustomizations[i].quantity << std::endl;
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
}

void VoxelWidget::cancelUpgrade()
{
    m_selectedUpgradeIndex = -1;
    m_holdingPlacedUpgrade = false;
}

void VoxelWidget::selectUpgrade()
{
    m_selectedUpgradeIndex = m_selectedUpgradeIndex == m_hoveredUpgradeIndex ? -1 : m_hoveredUpgradeIndex;
}
}
