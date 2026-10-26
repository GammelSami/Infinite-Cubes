// See cube/ControlsWidget.h.
#include "plasma/TextIds.h"
#include <algorithm>
#include <string>
#include "cube/widgets/ControlsWidget.h"
#include "cube/Game.h"
#include "cube/World.h"
#include "plasma/D3D11Engine.h"
#include "plasma/Display.h"
#include "plasma/Engine.h"
#include "plasma/Node.h"
#include "plasma/Transformation.h"
#include "plasma/Widget.h"
#include "plasma/EditFilter.h"

namespace cube {

static const float LIST_TOP = 35.0f;
static const float LIST_BOTTOM_MARGIN = 150.0f;
static const int ROW_HEIGHT = 30;

ControlsWidget::ControlsWidget(Game *game, plasma::Node *node, plasma::Node *button, plasma::Node *upButton,
                               plasma::Node *downButton, plasma::Node *scrollButton)
    : BaseWidget(game->m_engine, node, L""),
      m_game(game),
      m_upButton(upButton),
      m_downButton(downButton),
      m_scrollButton(scrollButton),
      m_scrollPosition(0),
      m_hoveredIndex(-1),
      m_selectedIndex(-1)
{
    m_defaultsButton = button->clone(node);
    m_okButton = button->clone(node);
    m_cancelButton = button->clone(node);
    m_defaultsButton->widget()->resizeTo(140.0f, 15.0f, true);
    m_defaultsButton->setText(L"Defaults");
    m_okButton->widget()->resizeTo(140.0f, 15.0f, true);
    m_okButton->setText(L"OK");
    m_cancelButton->widget()->resizeTo(140.0f, 15.0f, true);
    m_cancelButton->setText(L"Cancel");
    m_defaultsButton->widget()->connect(plasma::WIDGET_EVENT_POINTER_RELEASED, this, &ControlsWidget::resetToDefaults,
                                        true, true);
    m_okButton->widget()->connect(plasma::WIDGET_EVENT_POINTER_RELEASED, this, &ControlsWidget::accept, true, true);
    m_cancelButton->widget()->connect(plasma::WIDGET_EVENT_POINTER_RELEASED, this, &ControlsWidget::cancel, true, true);

    // The scroll buttons become this panel's own copies (own transformation and display).
    if (m_upButton) {
        m_upButton = m_upButton->instance(node);
        m_upButton->setTransformation(m_upButton->transformation()->clone());
        m_upButton->setDisplay(m_upButton->display()->clone());
        m_upButton->connectWidget(L"upbutton", plasma::WIDGET_EVENT_POINTER_PRESSED, this, &ControlsWidget::scrollUp,
                                  true);
    }
    if (m_downButton) {
        m_downButton = m_downButton->instance(node);
        m_downButton->setTransformation(m_downButton->transformation()->clone());
        m_downButton->setDisplay(m_downButton->display()->clone());
        m_downButton->connectWidget(L"downbutton", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                    &ControlsWidget::scrollDown, true);
    }
    if (m_scrollButton) {
        m_scrollButton = m_scrollButton->instance(node);
        m_scrollButton->setTransformation(m_scrollButton->transformation()->clone());
        m_scrollButton->setDisplay(m_scrollButton->display()->clone());
        m_scrollButton->connectWidget(L"scrollbutton", plasma::WIDGET_EVENT_POINTER_MOVED, this,
                                      &ControlsWidget::dragScrollButton, true);
    }
    setFont(L"resource1.dat");
}

void ControlsWidget::resetSelectedIndex()
{
    m_selectedIndex = -1;
}

void ControlsWidget::draw()
{
    // The buttons' centres, for the interface's button list.
    plasma::Vector<2, float> minimum;
    plasma::Vector<2, float> maximum;
    m_defaultsButton->getShapeBounds(minimum, maximum);
    m_game->m_interface.m_buttonCenters.push_back(plasma::Vector<2, int>((maximum + minimum) * 0.5f));
    m_okButton->getShapeBounds(minimum, maximum);
    m_game->m_interface.m_buttonCenters.push_back(plasma::Vector<2, int>((maximum + minimum) * 0.5f));
    m_cancelButton->getShapeBounds(minimum, maximum);
    m_game->m_interface.m_buttonCenters.push_back(plasma::Vector<2, int>((maximum + minimum) * 0.5f));

    // The visible rows.
    float y = 27.0f;
    int first = m_scrollPosition;
    int last = first + (int)(getContentHeight() - 120.0f) / ROW_HEIGHT;
    if (first < 0)
        first = 0;
    if (last < 0)
        last = 0;
    if (first > Controls::ButtonCount)
        first = Controls::ButtonCount;
    if (last > Controls::ButtonCount)
        last = Controls::ButtonCount;
    plasma::Vector<2, float> pointer = getPointerOffset();
    m_hoveredIndex = -1;
    setTextColor(plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f));
    setTextStrokeColor(plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f));
    setTextSize(12.0f);
    setTextStrokeRadius(3.0f);
    if (m_game->m_gamepadActive)
        m_selectedIndex = -1;
    for (int i = first; i < last; i++) {
        plasma::Vector<4, float> color;
        if (i == m_selectedIndex) {
            // Waiting for the new key.
            setTextColor(plasma::Vector<4, float>(1.0f, 0.35f, 0.35f, 1.0f));
            setTextFlags(0);
            renderText(Controls::buttonName(i), 20.0f, y);
            setTextFlags(1 << plasma::TEXT_CENTER);
            renderText(L"Press a key", (getContentWidth() - 250.0f) * 0.5f + 240.0f, y);
        } else {
            // Unbound actions green, the row under the pointer cyan.
            color = plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f);
            if (!m_controls.buttons[i].code && !m_controls.buttons[i].device)
                color = plasma::Vector<4, float>(0.2f, 1.0f, 0.2f, 1.0f);
            if (m_selectedIndex < 0 && !m_game->m_gamepadActive && pointer[0] >= 0.0f &&
                getContentWidth() - 40.0f > pointer[0] && pointer[1] >= y - 20.0f && y + 10.0f > pointer[1]) {
                m_hoveredIndex = i;
                color = plasma::Vector<4, float>(0.2f, 1.0f, 1.0f, 1.0f);
            }
            setTextColor(color);
            setTextFlags(0);
            renderText(Controls::buttonName(i), 20.0f, y);
            setTextFlags(1 << plasma::TEXT_CENTER);
            renderText(m_game->m_keyNames.getFullName(m_controls.buttons[i]),
                       (getContentWidth() - 250.0f) * 0.5f + 240.0f, y);
        }
        y += ROW_HEIGHT;
    }

    // Scroll buttons under the pointer are drawn cyan.
    if (m_upButton)
        m_upButton->display()->m_fillColor.value() = m_upButton->widget()->isPointerOver()
                                                         ? plasma::Vector<4, float>(0.0f, 1.0f, 1.0f, 1.0f)
                                                         : plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f);
    if (m_downButton)
        m_downButton->display()->m_fillColor.value() = m_downButton->widget()->isPointerOver()
                                                           ? plasma::Vector<4, float>(0.0f, 1.0f, 1.0f, 1.0f)
                                                           : plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f);
    if (m_scrollButton)
        m_scrollButton->display()->m_fillColor.value() = m_scrollButton->widget()->isPointerOver()
                                                             ? plasma::Vector<4, float>(0.0f, 1.0f, 1.0f, 1.0f)
                                                             : plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f);
}

void ControlsWidget::rebindSelected(const Controls::Button &button)
{
    if ((unsigned)m_selectedIndex <= Controls::ButtonZoomOut) {
        // A key can be bound once: clear it from every other action.
        for (int i = 0; i < Controls::ButtonCount; i++) {
            if (m_controls.buttons[i].device == button.device && m_controls.buttons[i].code == button.code) {
                m_controls.buttons[i].device = Controls::DeviceKeyboard;
                m_controls.buttons[i].code = 0;
            }
        }
        m_controls.buttons[m_selectedIndex] = button;
        m_hoveredIndex = -1;
    }
}

void ControlsWidget::cancel()
{
    m_node->display()->m_visibility.value() = 0;
}

void ControlsWidget::resetToDefaults()
{
    m_controls = Controls();
}

void ControlsWidget::dragScrollButton()
{
    if (m_engine->isButtonDown(0)) {
        int listHeight = (int)(getContentHeight() - LIST_BOTTOM_MARGIN - LIST_TOP);
        int buttonHeight = (int)m_scrollButton->widget()->getViewHeight();
        float pointerY = (getPointerMovement() + m_scrollButton->widget()->getOffsetInParent())[1];
        int visibleRows = listHeight / ROW_HEIGHT;
        int track = listHeight - buttonHeight;
        float top = LIST_TOP > pointerY ? LIST_TOP : pointerY;
        float y = (float)(track + (int)LIST_TOP) < top ? (float)(track + (int)LIST_TOP) : top;
        if (track > 0)
            m_scrollPosition = (int)((Controls::ButtonCount - 1 - visibleRows) * (y - LIST_TOP) / track);
        if (m_scrollPosition < 0)
            m_scrollPosition = 0;
        if (m_scrollPosition >= Controls::ButtonCount - visibleRows)
            m_scrollPosition = Controls::ButtonCount - 1 - visibleRows;
        m_scrollButton->widget()->moveTo(m_scrollButton->widget()->getOffsetInParent()[0], y, true);
    }
}

void ControlsWidget::accept()
{
    m_game->applyControls(m_controls);
    m_node->display()->m_visibility.value() = 0;
}

void ControlsWidget::resized()
{
    // Defaults at the left, OK centred, Cancel at the right, 20 from the bottom.
    plasma::Widget *defaultsButton = m_defaultsButton->widget();
    float y = getViewHeight() - defaultsButton->getViewHeight() - 20.0f;
    defaultsButton->moveTo(20.0f, y, true);
    plasma::Widget *okButton = m_okButton->widget();
    float width = viewWidth() - 40.0f;
    y = getViewHeight() - okButton->getViewHeight() - 20.0f;
    okButton->moveTo((width - okButton->viewWidth()) * 0.5f + 20.0f, y, true);
    okButton = m_okButton->widget();
    plasma::Widget *cancelButton = m_cancelButton->widget();
    float right = viewWidth() - 20.0f;
    y = getViewHeight() - okButton->getViewHeight() - 20.0f;
    cancelButton->moveTo(right - cancelButton->viewWidth(), y, true);
    updateScrollbarPosition();
    if (m_upButton) {
        plasma::Widget *upButton = m_upButton->widget();
        upButton->moveTo(getContentWidth() - 30.0f, 10.0f, true);
    }
    if (m_downButton) {
        plasma::Widget *downButton = m_downButton->widget();
        float bottom = getContentHeight() - 140.0f;
        downButton->moveTo(getContentWidth() - 30.0f, bottom, true);
    }
}

void ControlsWidget::scrollDown()
{
    int visibleRows = (int)(getContentHeight() - LIST_BOTTOM_MARGIN - LIST_TOP) / ROW_HEIGHT;
    if (++m_scrollPosition >= Controls::ButtonCount - visibleRows)
        m_scrollPosition = Controls::ButtonCount - 1 - visibleRows;
    updateScrollbarPosition();
}

void ControlsWidget::scrollUp()
{
    if (--m_scrollPosition < 0)
        m_scrollPosition = 0;
    updateScrollbarPosition();
}

void ControlsWidget::selectHoveredIndex()
{
    m_selectedIndex = m_hoveredIndex;
}

void ControlsWidget::updateScrollbarPosition()
{
    if (m_scrollButton) {
        // The scroll button's height is the visible share of the list; its position follows m_scrollPosition.
        int listHeight = (int)(getContentHeight() - LIST_BOTTOM_MARGIN - LIST_TOP);
        int visibleRows = listHeight / ROW_HEIGHT;
        int buttonHeight = visibleRows * listHeight / Controls::ButtonCount;
        plasma::Widget *button = m_scrollButton->widget();
        button->setViewRect(
            getContentWidth() - 28.0f,
            (float)((listHeight - buttonHeight) * m_scrollPosition / (Controls::ButtonCount - 1 - visibleRows) +
                    (int)LIST_TOP),
            button->getContentWidth(), (float)buttonHeight, true);
    }
}

}
