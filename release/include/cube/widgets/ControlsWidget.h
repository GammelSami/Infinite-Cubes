// ControlsWidget edits a copy of the key bindings in a scrollable action list. Defaults restores bindings, OK applies
// them through Game::SetControls and Cancel hides the panel.
#pragma once
#include "cube/widgets/BaseWidget.h"
#include "cube/Controls.h"

namespace plasma {
class Node;
}

namespace cube {

class Game;

class ControlsWidget : public BaseWidget {
public:
    ControlsWidget(Game *game, plasma::Node *node, plasma::Node *button, plasma::Node *upButton,
                   plasma::Node *downButton, plasma::Node *scrollButton);

    // Draws the visible rows from m_scrollPosition, tracks the row under the pointer (m_hoveredIndex) and the row
    // being rebound (m_selectedIndex).
    virtual void draw();
    // Lays out the three buttons at the bottom and the scroll buttons at the right, then the scroll bar
    // (updateScrollbarPosition).
    virtual void resized();

    void resetSelectedIndex();
    // Binds the action being rebound (m_selectedIndex, if any) to button, first clearing button from every action, and
    // clears m_hoveredIndex.
    void rebindSelected(const Controls::Button &button);
    // Button handlers hide the panel, restore defaults or apply the edited controls.
    void cancel();
    void resetToDefaults(); // Restores the default Controls bindings.
    void dragScrollButton();
    void accept(); // Applies m_controls with Game::SetControls, then hides the panel.
    void scrollDown();
    void scrollUp();
    void selectHoveredIndex();
    // Places the scroll button at m_scrollPosition.
    void updateScrollbarPosition();

    Game *m_game; // Used by accept
    Controls m_controls;
    plasma::Node *m_okButton;       // Cloned button drawn by the widget.
    plasma::Node *m_cancelButton;   // Cloned button drawn by the widget.
    plasma::Node *m_defaultsButton; // Cloned button drawn by the widget.
    plasma::Node *m_upButton;       // Instanced button node.
    plasma::Node *m_downButton;     // Instanced button node.
    plasma::Node *m_scrollButton;   // Instanced button node.
    int m_scrollPosition;           // Initially 0.
    int m_hoveredIndex;             // -1; draw
    int m_selectedIndex;            // -1; resetSelectedIndex
};

}
