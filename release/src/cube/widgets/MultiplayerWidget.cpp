// See cube/MultiplayerWidget.h.
#include "plasma/TextIds.h"
#include <algorithm>
#include "cube/widgets/MultiplayerWidget.h"
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

MultiplayerWidget::MultiplayerWidget(Game *game, plasma::Node *node, plasma::Node *button, plasma::Node *upButton,
                                     plasma::Node *downButton, plasma::Node *scrollBar)
    : BaseWidget(game->m_engine, node, L""),
      m_game(game),
      m_checkGame(true),
      m_upButton(upButton),
      m_downButton(downButton),
      m_scrollBar(scrollBar),
      m_scrollPosition(0),
      m_selectedPlayerIndex(-1),
      m_joinHovered(false)
{
    m_leaveButton = button->clone(node);
    m_leaveButton->widget()->resizeTo(140.0f, 15.0f, true);
    m_leaveButton->setText(L"Leave");

    // The scroll buttons and bar get their own transformation and display.
    if (m_upButton) {
        m_upButton = m_upButton->instance(node);
        m_upButton->setTransformation(m_upButton->transformation()->clone());
        m_upButton->setDisplay(m_upButton->display()->clone());
        m_upButton->connectWidget(L"upbutton", plasma::WIDGET_EVENT_POINTER_PRESSED, this, &MultiplayerWidget::scrollUp,
                                  true);
    }
    if (m_downButton) {
        m_downButton = m_downButton->instance(node);
        m_downButton->setTransformation(m_downButton->transformation()->clone());
        m_downButton->setDisplay(m_downButton->display()->clone());
        m_downButton->connectWidget(L"downbutton", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                    &MultiplayerWidget::scrollDown, true);
    }
    if (m_scrollBar) {
        m_scrollBar = m_scrollBar->instance(node);
        m_scrollBar->setTransformation(m_scrollBar->transformation()->clone());
        m_scrollBar->setDisplay(m_scrollBar->display()->clone());
        m_scrollBar->connectWidget(L"scrollbutton", plasma::WIDGET_EVENT_POINTER_MOVED, this,
                                   &MultiplayerWidget::dragScrollButton, true);
    }
    connect(plasma::WIDGET_EVENT_POINTER_PRESSED, this, &MultiplayerWidget::inviteOrJoin, true, true);
    setFont(L"resource1.dat");
}

void MultiplayerWidget::draw()
{
    // The visible rows, 30 units apart from y 62.
    int visibleRows = (int)(getContentHeight() - 120.0f) / 30;
    int first = m_scrollPosition;
    int last = first + visibleRows;
    if (first < 0)
        first = 0;
    if (last < 0)
        last = 0;
    if (first > (int)m_friends.size())
        first = m_friends.size();
    if (last > (int)m_friends.size())
        last = m_friends.size();
    plasma::Vector<2, float> pointer = getPointerOffset();

    setTextStrokeColor(plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f));
    setTextSize(12.0f);
    setTextStrokeRadius(3.0f);
    setTextColor(plasma::Vector<4, float>(1.0f, 0.8f, 0.5f, 1.0f));
    setTextFlags(0);
    renderText(L"Friends", 20.0f, 27.0f);
    setTextColor(plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f));
    float y = 62.0f;
    m_selectedPlayerIndex = -1;
    m_leaveButton->display()->m_visibility.value() = 0;
    setTextWrapWidth(100.0f);
    setTextFlags(1 << plasma::TEXT_WRAP);
    m_joinHovered = false;
    AppId_t appID = 0;
    if (m_checkGame)
        appID = SteamUtils()->GetAppID();
    for (int i = first; i < last; i++) {
        // The name's colour, the invite action's and the join action's.
        plasma::Vector<4, float> nameColor(1.0f, 1.0f, 1.0f, 1.0f);
        plasma::Vector<4, float> inviteColor(1.0f, 1.0f, 1.0f, 1.0f);
        plasma::Vector<4, float> joinColor(0.6f, 0.6f, 0.6f, 1.0f);
        if (!m_friends[i].first.IsValid())
            continue;
        bool invited = m_game->m_host.isInvited(m_friends[i].first);
        bool joined = m_game->m_host.hasJoined(m_friends[i].first);
        bool invitedBy = m_game->m_client.isInvitedBy(m_friends[i].first);
        bool isHost = m_game->m_client.isHost(m_friends[i].first);
        if (invitedBy) {
            nameColor = plasma::Vector<4, float>(0.3f, 1.0f, 0.2f, 1.0f);
            joinColor = plasma::Vector<4, float>(0.3f, 1.0f, 0.2f, 1.0f);
        }
        if (invited)
            nameColor = plasma::Vector<4, float>(0.3f, 1.0f, 0.2f, 1.0f);
        if (joined)
            nameColor = plasma::Vector<4, float>(0.5f, 0.2f, 1.0f, 1.0f);
        bool hovered = false;
        FriendGameInfo_t info;
        // Whether the friend plays this game (always, without the check).
        bool playing = true;
        if (m_checkGame) {
            playing = SteamFriends()->GetFriendGamePlayed(m_friends[i].first, &info);
            if (playing && info.m_gameID.AppID() != appID)
                playing = false;
        }
        if (!playing)
            nameColor = plasma::Vector<4, float>(0.5f, 0.5f, 0.5f, 1.0f);
        if (playing && pointer[0] >= 0.0f && getContentWidth() - 40.0f > pointer[0] && pointer[1] >= y - 20.0f &&
            y + 10.0f > pointer[1]) {
            hovered = true;
            m_selectedPlayerIndex = i;
            if (getContentWidth() - 100.0f > pointer[0]) {
                m_joinHovered = false;
                inviteColor = plasma::Vector<4, float>(0.2f, 1.0f, 1.0f, 1.0f);
                if (isHost)
                    joinColor = plasma::Vector<4, float>(1.0f, 0.25f, 0.25f, 1.0f);
            } else {
                m_joinHovered = true;
                if (isHost)
                    joinColor = plasma::Vector<4, float>(1.0f, 0.5f, 0.5f, 1.0f);
                else if (invitedBy)
                    joinColor = plasma::Vector<4, float>(0.2f, 1.0f, 1.0f, 1.0f);
            }
        }
        setTextColor(nameColor);
        setTextFlags(0);
        std::wstring name = m_friends[i].second;
        if (isHost)
            name += L" (host)";
        if (joined)
            name += L" (joined)";
        else if (invited)
            name += L" (invited)";
        renderText(name, 20.0f, y);
        setTextFlags(1 << plasma::TEXT_RIGHT);
        if (hovered) {
            std::wstring invite;
            if (invited) {
                invite = L"uninvite";
                inviteColor = plasma::Vector<4, float>(1.0f, 0.25f, 0.25f, 1.0f);
            } else {
                invite = L"invite";
            }
            setTextColor(inviteColor);
            renderText(invite, getContentWidth() - 130.0f, y);
        }
        if (hovered || invitedBy) {
            setTextColor(joinColor);
            std::wstring join;
            if (isHost)
                join = L"leave";
            else
                join = L"join";
            renderText(join, getContentWidth() - 50.0f, y);
        }
        y += 30.0f;
    }

    // The scroll buttons light up under the pointer.
    if (m_upButton)
        m_upButton->display()->m_fillColor.value() = m_upButton->widget()->isPointerOver()
                                                         ? plasma::Vector<4, float>(0.0f, 1.0f, 1.0f, 1.0f)
                                                         : plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f);
    if (m_downButton)
        m_downButton->display()->m_fillColor.value() = m_downButton->widget()->isPointerOver()
                                                           ? plasma::Vector<4, float>(0.0f, 1.0f, 1.0f, 1.0f)
                                                           : plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f);
    if (m_scrollBar)
        m_scrollBar->display()->m_fillColor.value() = m_scrollBar->widget()->isPointerOver()
                                                          ? plasma::Vector<4, float>(0.0f, 1.0f, 1.0f, 1.0f)
                                                          : plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f);
}

void MultiplayerWidget::dragScrollButton()
{
    if (m_engine->isButtonDown(0)) {
        // The track runs from y 60 over the panel height less 210; 30 units per row.
        int trackHeight = (int)(getContentHeight() - 150.0f - 60.0f);
        int buttonHeight = (int)m_scrollBar->widget()->getViewHeight();
        // The bar follows the pointer: its position in the panel plus the pointer's movement.
        float y = (getPointerMovement() + m_scrollBar->widget()->getOffsetInParent())[1];
        int visibleRows = trackHeight / 30;
        int range = trackHeight - buttonHeight;
        float top = 60.0f > y ? 60.0f : y;
        float position = (float)(range + 60) < top ? (float)(range + 60) : top;
        if (range > 0)
            m_scrollPosition = (int)((m_friends.size() - visibleRows - 1) * (position - 35.0f) / range);
        if (m_scrollPosition < 0)
            m_scrollPosition = 0;
        if ((size_t)m_scrollPosition >= m_friends.size() - visibleRows - 1)
            m_scrollPosition = m_friends.size() - visibleRows - 1;
        if (m_scrollPosition < 0)
            m_scrollPosition = 0;
        m_scrollBar->widget()->moveTo(m_scrollBar->widget()->getOffsetInParent()[0], position, true);
    }
}

void MultiplayerWidget::inviteOrJoin()
{
    int index = m_selectedPlayerIndex;
    if (index >= 0 && index < (int)m_friends.size()) {
        CSteamID steamId = m_friends[index].first;
        if (m_joinHovered) {
            if (m_game->m_client.isHost(steamId))
                m_game->m_client.disconnect();
            else if (m_game->m_client.isInvitedBy(steamId))
                m_game->m_client.connect(steamId);
        } else if (m_game->m_host.isInvited(steamId) || m_game->m_host.hasJoined(steamId)) {
            m_game->m_host.uninvite(steamId);
        } else {
            m_game->m_host.invite(steamId);
        }
    }
}

void MultiplayerWidget::resized()
{
    // The Leave button centred at the bottom, the scroll buttons on the right.
    plasma::Widget *leaveWidget = m_leaveButton->widget();
    float width = viewWidth() - 40.0f;
    float y = getViewHeight() - leaveWidget->getViewHeight() - 20.0f;
    leaveWidget->moveTo((width - leaveWidget->viewWidth()) * 0.5f + 20.0f, y, true);
    updateScrollbarPosition();
    if (m_upButton) {
        plasma::Widget *upWidget = m_upButton->widget();
        upWidget->moveTo(getContentWidth() - 30.0f, 35.0f, true);
    }
    if (m_downButton) {
        plasma::Widget *downWidget = m_downButton->widget();
        downWidget->moveTo(getContentWidth() - 30.0f, getContentHeight() - 140.0f, true);
    }
}

void MultiplayerWidget::scrollDown()
{
    int visibleRows = (int)(getContentHeight() - 150.0f - 60.0f) / 30;
    if (m_friends.size() <= (size_t)visibleRows) {
        m_scrollPosition = 0;
    } else {
        m_scrollPosition++;
        if ((size_t)m_scrollPosition >= m_friends.size() - visibleRows - 1)
            m_scrollPosition = m_friends.size() - visibleRows - 1;
        if (m_scrollPosition < 0)
            m_scrollPosition = 0;
        updateScrollbarPosition();
    }
}

void MultiplayerWidget::scrollUp()
{
    m_scrollPosition--;
    if (m_scrollPosition < 0)
        m_scrollPosition = 0;
    updateScrollbarPosition();
}

void MultiplayerWidget::updateScrollbarPosition()
{
    if (m_scrollBar) {
        int trackHeight = (int)(getContentHeight() - 150.0f - 60.0f);
        int visibleRows = trackHeight / 30;
        if (m_friends.size() <= (size_t)(visibleRows + 1)) {
            plasma::Widget *scrollWidget = m_scrollBar->widget();
            scrollWidget->setViewRect(getContentWidth() - 28.0f, 60.0f, scrollWidget->getContentWidth(), trackHeight,
                                      true);
        } else {
            // A bar as tall as the visible share of the list, placed by the scroll position.
            int barHeight = visibleRows * trackHeight / m_friends.size();
            plasma::Widget *scrollWidget = m_scrollBar->widget();
            scrollWidget->setViewRect(
                getContentWidth() - 28.0f,
                (trackHeight - barHeight) * m_scrollPosition / (m_friends.size() - visibleRows - 1) + 60,
                scrollWidget->getContentWidth(), barHeight, true);
        }
    }
}

}
