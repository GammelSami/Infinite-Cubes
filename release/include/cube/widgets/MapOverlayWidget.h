// MapOverlayWidget draws map markers and records the hovered marker's item or region for the preview widget.
#pragma once
#include <string>
#include "plasma/Vector.h"
#include "plasma/Widget.h"
#include "cube/Item.h"

namespace plasma {
class Node;
}

namespace cube {

class Game;

class MapOverlayWidget : public plasma::Widget {
public:
    MapOverlayWidget(Game *game, plasma::Node *node);

    // Collects the markers of the map regions around the map position (the discovered places, missions, areas, flight
    // points, creatures and treasures), finds the one under the pointer and draws them from far to near.
    virtual void draw();

    friend class Game;

protected:
    Game *m_game; // Used by draw.
    Item m_hoveredItem;
    // Region coordinates.
    plasma::Vector<2, int> m_hoveredRegion;
    unsigned char m_unused258[0x8]; // No access in the program.
};

}
