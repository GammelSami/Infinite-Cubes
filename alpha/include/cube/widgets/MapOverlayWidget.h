// MapOverlayWidget displays labels over the world map.
#pragma once
#include "plasma/Widget.h"
namespace cube {
class GameController;
class MapOverlayWidget : public plasma::Widget {
public:
    MapOverlayWidget(plasma::Engine *engine, plasma::Node *node, GameController *game);
    virtual void draw();
    GameController *m_game;
};
}
