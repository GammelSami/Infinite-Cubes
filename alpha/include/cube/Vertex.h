#pragma once
#include <d3d9.h>
#include "plasma/Vector.h"
// The client mesh helpers use the default blocks.
#include "cube/DefaultBlocks.h"

namespace cube {
// Packed voxel vertex with a four-byte position and D3DCOLOR.
struct Vertex {
    Vertex(const plasma::Vector<3, int> &position, const plasma::Vector<3, int> &normal,
           const plasma::Vector<4, float> &color)
        : position((unsigned char)position[0], (unsigned char)position[1], (unsigned char)position[2]),
          normal(1),
          color(D3DCOLOR_ARGB((int)(color[3] * 255.0f), (int)(color[0] * 255.0f), (int)(color[1] * 255.0f),
                              (int)(color[2] * 255.0f)))
    {
        if (normal == plasma::Vector<3, int>(1, 0, 0))
            this->normal = 0;
        else if (normal == plasma::Vector<3, int>(-1, 0, 0))
            this->normal = 1;
        else if (normal == plasma::Vector<3, int>(0, 1, 0))
            this->normal = 2;
        else if (normal == plasma::Vector<3, int>(0, -1, 0))
            this->normal = 3;
        else if (normal == plasma::Vector<3, int>(0, 0, 1))
            this->normal = 4;
        else if (normal == plasma::Vector<3, int>(0, 0, -1))
            this->normal = 5;
    }
    plasma::Vector<3, unsigned char> position;
    unsigned char normal; // face direction index
    D3DCOLOR color;       // packed ARGB
};
}
