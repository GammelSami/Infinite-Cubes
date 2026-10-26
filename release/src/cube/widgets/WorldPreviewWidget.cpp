// See cube/WorldPreviewWidget.h.
#include "plasma/TextIds.h"
#include "gfx/RendererIds.h"
#include <math.h>
#include <sstream>
#include <string>
#include "cube/widgets/WorldPreviewWidget.h"
#include "cube/Noise.h"
#include "cube/Sprite.h"
#include "cube/WorldInfo.h"
#include "gfx/D3D11Renderer.h"
#include "plasma/Engine.h"
#include "plasma/Node.h"
#include "cube/Game.h"
#include "cube/World.h"
#include "plasma/D3D11Engine.h"
#include "plasma/EditFilter.h"

namespace cube {

WorldPreviewWidget::WorldPreviewWidget(Game *game, plasma::Node *node)
    : BaseWidget(game->m_engine, node, L""),
      m_worldInfo(nullptr),
      m_game(game),
      m_rotation(0.0f)
{
    setFont(L"resource1.dat");
}

namespace {
// The preview model turns this many degrees per millisecond while the pointer is over the entry, and eases back at
// this blend rate otherwise.
const float PreviewTurnRate = 0.05f;
const float PreviewReturnRate = 0.01f;
}

// The model is drawn 110 units (times the node's scale) right of and below the widget's pivot.
void WorldPreviewWidget::draw()
{
    setTextSize(15.0f);
    setTextStrokeRadius(3.0f);
    setTextColor(plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f));
    setTextStrokeColor(plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f));
    if (!m_worldInfo) {
        int x = (int)(getContentWidth() * 0.5f);
        std::wstringstream stream;
        stream << L"New world";
        setTextFlags((1 << plasma::TEXT_CENTER) | (1 << plasma::TEXT_MIDDLE));
        renderText(stream.str(), (float)x, 30.0f);
    } else {
        if (m_worldInfo->m_preview) {
            // The model turns while the pointer is over the entry and eases back otherwise.
            if (isPointerOver())
                m_rotation = m_rotation - m_engine->getFrameTime() * PreviewTurnRate;
            else
                m_rotation =
                    interpolateAngle(m_rotation, 0.0f, blendFactor(m_engine->getFrameTime(), PreviewReturnRate) + 0.0f);
            plasma::Matrix<float> matrix;
            m_node->getMatrix(matrix);
            plasma::Vector<2, float> origin = matrix * m_node->transformation()->m_pivot.value();
            float scale = matrix.m_elements[0] * 0.008f;

            // The model tilted by -120 degrees about x, turned about z (45 degrees at rest), scaled (0.008 of the
            // node's scale) and centred, 17 units away; a perspective of 45 degrees (the half angle pi / 8), mirrored
            // in x, with the depth terms of a near plane at 0.1 and a far plane at 1000.
            plasma::Matrix<float> model;
            model.identity();
            model.translate(0.0f, 0.0f, 17.0f);
            model.rotateX(-120.0f);
            model.rotateZ(m_rotation + 45.0f);
            model.scale(scale, scale, scale);
            model.translate(m_worldInfo->m_preview->m_size[0] * -0.5f, m_worldInfo->m_preview->m_size[1] * -0.5f,
                            m_worldInfo->m_preview->m_size[2] * -0.5f);

            float focal = 1.0f / tanf(0.39269909f);
            plasma::Matrix<float> projection;
            for (int i = 0; i < 16; i++)
                projection.m_elements[i] = 0.0f;
            projection.m_elements[0] =
                -(focal / ((float)m_engine->viewportWidth() / (float)m_engine->viewportHeight()));
            projection.m_elements[5] = focal;
            projection.m_elements[10] = 1.0001f;
            projection.m_elements[11] = 1.0f;
            projection.m_elements[14] = -0.10001f;
            float placeY = (matrix.m_elements[0] * 110.0f + origin[1] - m_engine->viewportHeight() * 0.5f) /
                           m_engine->viewportHeight() * -2.0f;
            float placeX = (matrix.m_elements[0] * 110.0f + origin[0] - m_engine->viewportWidth() * 0.5f) /
                           m_engine->viewportWidth();
            plasma::Matrix<float> placement;
            placement.identity();
            placement.translate(placeX + placeX, placeY, 0.0f);
            placement *= projection;
            projection = placement;
            plasma::Matrix<float> identity;
            identity.identity();
            m_game->m_renderer->activateMode(gfx::RendererMode_OpaqueFront);
            m_game->m_renderer->drawSprite(m_worldInfo->m_preview, identity, model, projection,
                                           plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f));
        }
        m_engine->applyRenderState();

        std::wstringstream stream;
        stream << WidenString(m_worldInfo->m_name);
        setTextColor(plasma::Vector<4, float>(0.25f, 1.0f, 1.0f, 1.0f));
        setTextFlags(0);
        renderText(stream.str(), 200.0f, 30.0f);
        stream.str(std::wstring(L""));
        stream << L"Explored: " << (float)m_worldInfo->m_exploredArea << L" km\x00b2";
        setTextSize(12.0f);
        setTextColor(plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f));
        renderText(stream.str(), 200.0f, 55.0f);
        stream.str(std::wstring(L""));
        stream << L"Seed: " << m_worldInfo->m_seed;
        setTextColor(plasma::Vector<4, float>(0.5f, 0.5f, 1.0f, 1.0f));
        renderText(stream.str(), 200.0f, 80.0f);
    }
}

}
