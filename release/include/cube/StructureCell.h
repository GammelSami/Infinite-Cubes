// StructureCell describes a structure's room kind, room part and storey height. Corridor uses a similar record.
// WorldMap and the zone generator share this header.
#pragma once
#include "cube/WorldIds.h"

namespace cube {

struct StructureCell {
    StructureCell()
        : kind(StructureCell_None),
          style(0),
          side(0)
    {}
    StructureCell(unsigned char kind, unsigned char roomPart, unsigned char style, unsigned char side)
        : kind(kind),
          roomPart(roomPart),
          style(style),
          side(side)
    {}

    unsigned char kind;     // Values 9..12 select the map site's marker colour.
    unsigned char roomPart; // RoomPart discriminator, not a bit mask.
    unsigned char style;    // Value 1 selects a 20-block storey height instead of 16.
    unsigned char side;
};

}
