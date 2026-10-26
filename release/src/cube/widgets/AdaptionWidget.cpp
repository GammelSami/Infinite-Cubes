// It shows the item and the upgraded copy's description side by side, an "Upgrade" and a "Goodbye!" button and the
// cost. See cube/AdaptionWidget.h.
#include "plasma/TextIds.h"
#include "gfx/RendererIds.h"
#include <sstream>
#include "cube/widgets/AdaptionWidget.h"
#include "cube/Creature.h"
#include "cube/Game.h"
#include "cube/Item.h"
#include "cube/SpriteManager.h"
#include "cube/World.h"
#include "gfx/D3D11Renderer.h"
#include "plasma/D3D11Engine.h"
#include "plasma/Display.h"
#include "plasma/Node.h"

namespace cube {

AdaptionWidget::AdaptionWidget(Game *game, plasma::Node *node, plasma::Node *itemFrame, plasma::Node *rightArrow)
    : BaseWidget(game->m_engine, node, L""),
      m_item(nullptr),
      m_game(game),
      m_itemFrame(itemFrame),
      m_rightArrow(rightArrow)
{
    // Show the item frame and the arrow under this widget's node, each with its own transformation and display.
    if (m_itemFrame) {
        m_itemFrame = m_itemFrame->instance(node);
        m_itemFrame->setTransformation(m_itemFrame->transformation()->clone());
        m_itemFrame->setDisplay(m_itemFrame->display()->clone());
    }
    if (m_rightArrow) {
        m_rightArrow = m_rightArrow->instance(node);
        m_rightArrow->setTransformation(m_rightArrow->transformation()->clone());
        m_rightArrow->setDisplay(m_rightArrow->display()->clone());
    }
}

int AdaptionWidget::getCost()
{
    if (!m_item)
        return 0;
    float cost = m_item->price();
    return cost;
}

void AdaptionWidget::draw()
{
    // The node's scale on the screen (element 0 of its accumulated matrix).
    plasma::Matrix<float> matrix;
    m_node->getMatrix(matrix);
    if (m_item) {
        // The item's model, turning in the item frame.
        if (m_game->m_world->m_spriteManager->itemSprite(m_item)) {
            m_game->m_renderer->activateMode(gfx::RendererMode_OpaqueFront);
            plasma::Vector<2, float> center =
                m_itemFrame->widget()->getContentPosition() + m_itemFrame->widget()->screenSize() * 0.5f;
            m_game->m_renderer->drawItem(
                m_item, center[0], center[1],
                plasma::Vector<3, float>(225.0f, 0.0f, m_engine->getCaretClock() % 10000 * 0.0001f * 360.0f),
                matrix[0] * 0.06f, 0.0f, true);
            m_engine->applyRenderState();
        }
        // The item's description on the left, the upgraded item's on the right, compared with the item.
        m_game->writeItemInfo(this, m_item, 1, 15, 180, 1.0f, 220, true, false, false, nullptr, nullptr);
        Item upgraded = *m_item;
        m_game->writeItemInfo(this, &upgraded, 1, (int)(getContentWidth() * 0.5f + 20.0f), 180, 1.0f, 220, true, false,
                              false, nullptr, m_item);
        m_rightArrow->display()->m_visibility.value() = 1;
    } else {
        m_rightArrow->display()->m_visibility.value() = 0;
    }

    setFont(L"resource1.dat");
    setTextSize(12.0f);
    setTextStrokeRadius(3.0f);
    setTextStrokeColor(plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f));
    setTextFlags(0);
    setTextColor(plasma::Vector<4, float>(1.0f, 0.8f, 0.5f, 1.0f));
    std::wstringstream text;
    text << L"Upgrade";
    renderText(text.str(), 15.0f, 25.0f);

    // The buttons: cyan under the pointer (the upgrade only when affordable), the upgrade button grey without an item.
    text.str(L"");
    text << "Upgrade";
    setTextFlags(1 << plasma::TEXT_CENTER);
    setTextColor(!m_item                  ? plasma::Vector<4, float>(0.7f, 0.7f, 0.7f, 1.0f)
                 : isPointerOverUpgrade() ? plasma::Vector<4, float>(0.0f, 1.0f, 1.0f, 1.0f)
                                          : plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f));
    renderText(text.str(), getContentWidth() / 3.0f, getContentHeight() - 20.0f);
    text.str(L"");
    text << "Goodbye!";
    setTextColor(isPointerOverGoodbye() ? plasma::Vector<4, float>(0.0f, 1.0f, 1.0f, 1.0f)
                                        : plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f));
    renderText(text.str(), getContentWidth() * 2.0f / 3.0f, getContentHeight() - 20.0f);

    int y = (int)(getContentHeight() - 50.0f);
    if (m_item) {
        if (getCost() > m_game->m_world->m_localCreature->m_inventory.m_gold) {
            text.str(L"");
            text << "Not enough money.";
            setTextFlags(0);
            setTextColor(plasma::Vector<4, float>(1.0f, 0.0f, 0.0f, 1.0f));
            renderText(text.str(), 15.0f, (float)(y - 20));
        }
        int price = getCost();
        int costY = (int)(getContentHeight() - 50.0f);
        text.str(L"");
        text << "COST: ";
        setTextFlags(0);
        setTextColor(plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f));
        plasma::Vector<2, float> end = renderText(text.str(), 15.0f, (float)costY);
        text.str(L"");
        text << price << " GOLD ";
        setTextColor(plasma::Vector<4, float>(1.0f, 0.9f, 0.0f, 1.0f));
        renderText(text.str(), end[0], (float)costY);
    }
}

bool AdaptionWidget::isPointerOverGoodbye()
{
    // The right half of the bottom 30 units, in the node's scale.
    plasma::Vector<2, float> pointer = getPointerOffset();
    plasma::Matrix<float> matrix;
    m_node->getMatrix(matrix);
    float scale = matrix[0];
    return pointer[1] > (getContentHeight() - 30.0f) * scale && getContentHeight() * scale > pointer[1] &&
           pointer[0] > getContentWidth() * 0.5f * scale && getContentWidth() * scale > pointer[0];
}

bool AdaptionWidget::isPointerOverUpgrade()
{
    // Only an item the local creature can afford can be upgraded.
    if (!m_item || getCost() > m_game->m_world->m_localCreature->m_inventory.m_gold)
        return false;
    // The left half of the bottom 30 units, in the node's scale.
    plasma::Vector<2, float> pointer = getPointerOffset();
    plasma::Matrix<float> matrix;
    m_node->getMatrix(matrix);
    float scale = matrix[0];
    return pointer[1] > (getContentHeight() - 30.0f) * scale && getContentHeight() * scale > pointer[1] &&
           getContentWidth() * 0.5f * scale > pointer[0] && pointer[0] > 0.0f;
}

void AdaptionWidget::resized()
{
    // The item frame centred at the top, the arrow between the two descriptions.
    if (m_itemFrame) {
        plasma::Widget *frame = m_itemFrame->widget();
        frame->moveTo((getContentWidth() - frame->viewWidth()) * 0.5f, 50.0f, true);
    }
    if (m_rightArrow) {
        plasma::Widget *arrow = m_rightArrow->widget();
        arrow->moveTo(getContentWidth() * 0.5f - 10.0f, 230.0f, true);
    }
}

}
