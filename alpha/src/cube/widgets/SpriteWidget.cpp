// SpriteWidget draws a voxel-model preview.
#include "cube/widgets/SpriteWidget.h"
#include "cube/CubeShader.h"
#include "cube/DefaultBlocks.h"
#include "cube/GameController.h"
#include "cube/Sprite.h"
#include "plasma/Node.h"
#include "plasma/Transformation.h"
namespace cube {

SpriteWidget::SpriteWidget(plasma::Engine *engine, plasma::Node *node, GameController *game)
    : Widget(engine, node),
      m_game(game)
{
    m_sprite = 0;
    m_item = 0;
}
// The item or sprite shown at the centre of the widget (moved by the node's pivot), turning with
// the previews' rotation; the sprite is scaled to its largest dimension (an item at 0.0006 of the
// widget's width).
void SpriteWidget::draw()
{
    if (!m_sprite && !m_item)
        return;
    m_game->m_device->SetRenderState(D3DRS_ZENABLE, TRUE);
    plasma::Vector<4, float> lightPositions[16];
    plasma::Vector<3, float> lightColors[16];
    for (int i = 0; i < 16; i++) {
        lightPositions[i] = plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f);
        lightColors[i] = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
    }
    m_game->m_shader->apply();
    m_game->m_shader->setPointLights(lightPositions, lightColors);
    m_game->m_shader->setAlpha(1.0f);
    m_game->m_shader->setShininess(0.0f);
    m_game->m_shader->setMaterialColor(plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f));
    float x = getContentWidth() * 0.5f;
    float y = getContentHeight() * 0.5f;
    plasma::Vector<2, float> pivot = transformPoint(m_node->m_matrix, m_node->transformation()->m_pivot.value());
    x += pivot[0];
    y += pivot[1];
    if (m_item) {
        if (m_item->category != ItemCategoryNone)
            m_game->drawItem(x, y, m_game->m_previewRotation, getContentWidth() * 0.0006f, m_item, 0.0f);
    } else {
        int size = std::max(m_sprite->sizeY(), m_sprite->sizeX());
        size = std::max(m_sprite->sizeZ(), size);
        m_game->drawSpriteModel(x, y, m_game->m_previewRotation, getContentWidth() * 0.0006f / (float)size, m_sprite,
                                0.0f);
    }
    m_game->m_device->SetRenderState(D3DRS_ZENABLE, FALSE);
}

bool SpriteWidget::contains(const plasma::Vector<2, float> &point)
{
    return point[0] >= 0.0f && point[1] >= 0.0f && point[0] < viewWidth() && point[1] < getViewHeight();
}
}
