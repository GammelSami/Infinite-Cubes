// PreviewWidget is a tooltip for flight points, regional artifacts, items, skills and specializations. It sizes itself
// to the displayed text and can compare an item with equipped gear.
// Depending on what is set, Render shows a flight point ("Flight point", "Distance: ", "Flight time: "), a region's
// artifact, an item with its stats (and "Currently equipped:" with the equipped item), a skill's description and
// level, or a specialization's description, and resizes itself to the text. No destructor of its own.
#pragma once
#include "plasma/Vector.h"
#include "cube/widgets/BaseWidget.h"

namespace plasma {
class Node;
}

namespace cube {

class Game;
struct Item;
struct ZoneItem;

class PreviewWidget : public BaseWidget {
public:
    PreviewWidget(Game *game, plasma::Node *node);

    virtual void draw();

    // "Flight point", its distance from the player, and the flight time or the price.
    void renderFlightPoint();
    // The skill's "description:" text, with "skill:level" and "details:" when level > 0, laid out from (x, y) wrapping
    // at x + width.
    void renderSkill(int skill, int level, int x, int y, float scale, int width);
    // The description of the specializations of the class.
    void renderSpecialization(int classType, int x, int y, float scale, int width);
    // The artifact of m_artifactRegion.
    void renderArtifact();

    friend class Game;

protected:
    // Nothing examined sets it.
    ZoneItem *m_zoneItem;
    // The item to show; draw derives the shown copy with the player.
    Item *m_item;
    int m_count; // 1; passed to Game
    bool
        m_unusedItemInfoFlag; // false; passed to Game::writeItemInfo for the equipped item, whose parameter is not read; never set
    int m_skill;
    int m_skillLevel; // > 0 adds the "skill:level" text
    int m_specialization;
    Item *m_equippedItem; // shown under "Currently equipped:"
    plasma::Vector<2, int> m_flightPoint;
    plasma::Vector<2, int> m_artifactRegion;
    Game *m_game;
};

}
