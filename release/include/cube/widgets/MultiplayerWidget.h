// MultiplayerWidget lists Steam friends and offers invitations, joining and leaving. Pressing a row calls Host to
// invite or uninvite, or Client to join.
#pragma once
#include <string>
#include <utility>
#include <vector>
#include "cube/widgets/BaseWidget.h"
#include "cube/Controls.h"
#include "steam/steam_api.h"

namespace plasma {
class Node;
}

namespace cube {

class Game;

class MultiplayerWidget : public BaseWidget {
public:
    MultiplayerWidget(Game *game, plasma::Node *node, plasma::Node *button, plasma::Node *upButton,
                      plasma::Node *downButton, plasma::Node *scrollBar);

    // Draws the visible friends from m_scrollPosition with their state and actions, sets m_selectedPlayerIndex and
    // m_joinHovered for the row under the pointer.
    virtual void draw();
    // Lays out the Leave button and the scroll buttons, then the scroll bar (updateScrollbarPosition).
    virtual void resized();

    // Button and scrollbar handlers connected by the constructor.
    void dragScrollButton();
    // For the selected friend: with m_joinHovered false, uninvites an invited or joined friend, else invites
    // (cube::Host); with it set, joins the friend if they are the host or invited the player (cube::Client).
    void inviteOrJoin();
    void scrollDown();
    void scrollUp();
    // Places the scrollbar at m_scrollPosition.
    void updateScrollbarPosition();

    Game *m_game; // Used by inviteOrJoin
    // Checks the friend's game state.
    bool m_checkGame; // true
    // Friend's Steam ID and persona name.
    std::vector<std::pair<CSteamID, std::wstring>> m_friends; // Owned by the widget and deleted by its destructor.
    Controls m_controls;
    plasma::Node *m_leaveButton; // L"Leave" clone
    plasma::Node *m_upButton;    // Instanced button node.
    plasma::Node *m_downButton;  // Instanced button node.
    plasma::Node *m_scrollBar;   // Instanced button node.
    int m_scrollPosition;        // Initially 0.
    int m_selectedPlayerIndex;   // Initially -1.
    bool m_joinHovered;          // false
};

}
