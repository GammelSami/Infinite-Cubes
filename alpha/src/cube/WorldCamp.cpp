// Generates camps of zone objects.
#include "cube/DefaultBlocks.h"
#include "cube/World.h"
#include "cube/Zone.h"
#include <stdlib.h>

namespace cube {
// A camp needs a river distance of at least 1 / RiverDistanceScale (anonymous: this file's own).
namespace {
enum {
    RiverDistanceScale = 50
};
}

// A camp at `position`, away from rivers: a campfire on the first spot of a 3 x 3 block square
// from `position` where it settles, then a tent, table, stool or sleeping mat at each corner 3.5
// blocks off, where they settle. False if the place is by a river or the campfire fits nowhere.
// The objects' sizes are in blocks (x, y, height).
bool World::placeCamp(Zone *zone, const plasma::Vector<3, Fixed> &position)
{
    float river = 1.0f - riverDistance((int)position[0], (int)position[1], zone) * RiverDistanceScale;
    if (river >= 0.0f)
        return false;

    Zone::Object campfire;
    campfire.type = StaticObjectCampfire;
    campfire.size = plasma::Vector<3, float>(2.4f, 2.4f, 0.5f);
    campfire.rotation = rand() % 4;
    for (int x = 0; x < 3; x++) {
        for (int y = 0; y < 3; y++) {
            campfire.position = position + plasma::Vector<3, Fixed>(x, y, 0);
            if (settleObject(campfire, zone, true)) {
                zone->m_objects.push_back(campfire);

                const Fixed corner = 3.5;
                for (int cx = 0; cx < 14; cx += 7) {
                    for (int cy = 0; cy < 14; cy += 7) {
                        Zone::Object object;
                        switch (rand() % 4) {
                        case 1:
                            object.type = StaticObjectStool;
                            object.size = plasma::Vector<3, float>(1.0f, 1.0f, 0.5f);
                            break;
                        case 2:
                            object.type = StaticObjectTable;
                            object.size = plasma::Vector<3, float>(3.0f, 3.0f, 1.0f);
                            break;
                        case 3:
                            object.type = StaticObjectSleepingMat;
                            object.size = plasma::Vector<3, float>(2.0f, 2.0f, 0.1f);
                            break;
                        default:
                            object.type = StaticObjectTent;
                            object.size = plasma::Vector<3, float>(4.0f, 4.0f, 3.0f);
                            break;
                        }
                        object.rotation = rand() % 4;
                        object.position =
                            position + plasma::Vector<3, Fixed>(Fixed(cx) - corner, Fixed(cy) - corner, 0);
                        if (settleObject(object, zone, true))
                            zone->m_objects.push_back(object);
                    }
                }
                return true;
            }
        }
    }
    return false;
}

} // namespace cube
