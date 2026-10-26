// Default air, water and solid blocks. Each translation unit including this header has its own internal copies.
#pragma once
#include "plasma/Vector.h"
#include "cube/Block.h"

namespace cube {

static const Block airCube(plasma::Vector<3, unsigned char>(255, 255, 255), BlockAir); // white, empty
static const Block waterCube(plasma::Vector<3, unsigned char>(255, 255, 255),
                             BlockWater | (1 << BlockFlagUnderwater));                     // white, water
static const Block solidCube(plasma::Vector<3, unsigned char>(200, 200, 200), BlockSolid); // grey, solid

}
