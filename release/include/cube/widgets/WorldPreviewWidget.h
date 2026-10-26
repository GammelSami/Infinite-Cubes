// WorldPreviewWidget renders a rotating world model with its name, explored percentage and seed; displays "New world"
// when no world is selected.
#pragma once
#include "cube/widgets/BaseWidget.h"

namespace plasma {
class Node;
}

namespace cube {

class Game;
struct WorldInfo;

class WorldPreviewWidget : public BaseWidget {
public:
    WorldPreviewWidget(Game *game, plasma::Node *node);

    virtual void draw();

    WorldInfo *m_worldInfo;

protected:
    Game *m_game; // Game and its renderer used by draw.
    float m_rotation;
};

}
