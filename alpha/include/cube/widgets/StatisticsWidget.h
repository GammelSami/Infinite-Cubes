// StatisticsWidget displays the unused statistics panel.
#pragma once
#include "plasma/Widget.h"
namespace cube {
class GameController;
class StatisticsWidget : public plasma::Widget {
public:
    StatisticsWidget(plasma::Engine *engine, plasma::Node *node, GameController *game);
    virtual void draw();
    GameController *m_game;
};
}
