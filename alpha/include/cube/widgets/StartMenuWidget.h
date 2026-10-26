// StartMenuWidget displays title and selection screens.
#pragma once
#include "plasma/Widget.h"
namespace cube {
class GameController;
// Entries named by the labels drawn in this widget.
enum StartMenuButton {
    StartMenuPlay = 0,
    StartMenuOptions = 1,
    StartMenuExit = 2
};
class StartMenuWidget : public plasma::Widget {
public:
    StartMenuWidget(plasma::Engine *engine, plasma::Node *node, GameController *game);
    virtual void draw();
    int m_hoveredButton; // -1 when no entry is under the pointer
    GameController *m_game;
};
}
