// SystemWidget displays the system menu.
#pragma once
#include "plasma/Widget.h"
namespace cube {
class GameController;
// Entries named by the labels drawn in this widget.
enum SystemMenuButton {
    SystemMenuOptions = 0,
    SystemMenuStartMenu = 1,
    SystemMenuExit = 2
};
class SystemWidget : public plasma::Widget {
public:
    SystemWidget(plasma::Engine *engine, plasma::Node *node, GameController *game);
    virtual void draw();
    int m_hoveredButton; // -1 when no entry is under the pointer
    GameController *m_game;
};
}
