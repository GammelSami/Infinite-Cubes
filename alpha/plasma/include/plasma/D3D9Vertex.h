#pragma once
#include "plasma/Vector.h"

namespace plasma {
// Interleaved shape vertex declaration for Direct3D 9.
struct D3D9Vertex {
    Vector<2, float> position;
    Vector<4, float> color;
    Vector<2, float> firstNormal;
    Vector<2, float> secondNormal;
    Vector<2, float> texture;
};
}
