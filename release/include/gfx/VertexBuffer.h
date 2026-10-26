// gfx::VertexBuffer: the API-neutral handle of a vertex buffer that gfx::Graphics creates (Graphics.h);
// gfx::D3D11VertexBuffer (D3D11Graphics.h) is the only implementation. Also gfx::Vertex, the vertex format of every
// buffer Graphics creates.
#pragma once
#include "plasma/Vector.h"

namespace gfx {

// Sprite vertex format.
struct Vertex {
    // Initialises the vertex attributes.
    Vertex(const plasma::Vector<3, int> &position, const plasma::Vector<3, int> &normal,
           const plasma::Vector<4, float> &color)
        : position((unsigned char)position[0], (unsigned char)position[1], (unsigned char)position[2]),
          normal(1),
          color((int)(color[0] * 255.0f), (int)(color[1] * 255.0f), (int)(color[2] * 255.0f), (int)(color[3] * 255.0f))
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

    plasma::Vector<3, unsigned char> position; // POSITION x, y, z
    unsigned char normal;                      // POSITION w, the face index 0..5
    plasma::Vector<4, unsigned char> color;    // COLOR, RGBA
};

class VertexBuffer {
public:
    virtual ~VertexBuffer() {}

    virtual bool bind() { return true; }

    virtual void prepare() {}
};

}
