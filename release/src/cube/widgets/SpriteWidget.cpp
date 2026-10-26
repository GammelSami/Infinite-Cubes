// See cube/SpriteWidget.h.
#include "gfx/RendererIds.h"
#include "cube/Game.h"
#include "cube/Item.h"
#include "cube/Sprite.h"
#include "cube/widgets/SpriteWidget.h"
#include "cube/World.h"
#include "gfx/D3D11Renderer.h"
#include "plasma/D3D11Engine.h"
#include "plasma/Node.h"
#include "plasma/EditFilter.h"

namespace cube {

SpriteWidget::SpriteWidget(Game *game, plasma::Node *node)
    : BaseWidget(game->m_engine, node, L""),
      m_sprite(nullptr),
      m_unused1f0(0),
      m_game(game)
{
    m_flags |= (1 << plasma::WIDGET_RENDER_BEHIND_CHILDREN);
}

void SpriteWidget::draw()
{
    if (m_sprite || m_item.category) {
        m_game->m_renderer->activateMode(gfx::RendererMode_OpaqueFront);
        // The node's matrix is computed but not used.
        plasma::Matrix<float> matrix;
        m_node->getMatrix(matrix);
        plasma::Vector<2, float> center = getContentPosition() + screenSize() * 0.5f;
        if (m_item.category) {
            m_game->m_renderer->drawItem(
                &m_item, center[0], center[1],
                plasma::Vector<3, float>(225.0f, 0.0f, m_engine->getCaretClock() % 10000 * 0.0001f * 360.0f),
                screenSize()[0] * 0.0006f, 0.0f, true);
        } else {
            // Scaled to fit the sprite's largest dimension.
            int largest = m_sprite->m_size[0];
            if (m_sprite->m_size[1] > largest)
                largest = m_sprite->m_size[1];
            if (m_sprite->m_size[2] > largest)
                largest = m_sprite->m_size[2];
            m_game->m_renderer->drawSpriteModel(
                m_sprite, center[0], center[1],
                plasma::Vector<3, float>(225.0f, 0.0f, m_engine->getCaretClock() % 10000 * 0.0001f * 360.0f),
                screenSize()[0] * 0.0005f / largest, 0.0f);
        }
        m_engine->applyRenderState();
    }
}

}
