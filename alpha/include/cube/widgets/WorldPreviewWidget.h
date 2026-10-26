// WorldPreviewWidget displays a saved-world list entry.
#pragma once
#include "plasma/Widget.h"
namespace cube {
class GameController;
struct WorldInfo;
class WorldPreviewWidget : public plasma::Widget {
public:
    WorldPreviewWidget(plasma::Engine *engine, plasma::Node *node, GameController *game);
    virtual void draw();
    virtual bool contains(const plasma::Vector<2, float> &point);
    WorldInfo *m_worldInfo;
    float m_rotation;
    GameController *m_game;
};
}
