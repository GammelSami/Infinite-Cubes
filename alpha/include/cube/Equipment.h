// Equipment holds thirteen items.
#pragma once
#include "cube/Item.h"

namespace cube {

struct Equipment {
    // The implicit constructor constructs each Item in member order.
    Item head;
    Item neck;
    Item chest;
    Item feet;
    Item hands;
    Item shoulder;
    Item weaponLeft;
    Item weaponRight;
    Item ringLeft;
    Item ringRight;
    Item lamp;    // category 24
    Item special; // category 23, e.g. hang glider
    Item pet;     // categories 19 (pet) and 20 (pet food)

    // Compares items, starting with pet, lamp and special slots; used to test entity-data changes.
    bool operator!=(const Equipment &other) const
    {
        return pet != other.pet || lamp != other.lamp || special != other.special || head != other.head ||
               neck != other.neck || chest != other.chest || feet != other.feet || hands != other.hands ||
               shoulder != other.shoulder || weaponLeft != other.weaponLeft || weaponRight != other.weaponRight ||
               ringLeft != other.ringLeft || ringRight != other.ringRight;
    }
};

}
