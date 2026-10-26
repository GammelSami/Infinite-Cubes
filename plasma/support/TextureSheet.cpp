// Texture-sheet support implementation.
#include "plasma/Matrix.h"

template<> void plasma::Matrix<float>::scale(float x, float y, float z)
{
    if (x != 1.0f) {
        for (int i = 0; i < 4; ++i)
            m_elements[i] *= x;
    }
    if (y != 1.0f) {
        for (int i = 4; i < 8; ++i)
            m_elements[i] *= y;
    }
    if (z != 1.0f) {
        for (int i = 8; i < 12; ++i)
            m_elements[i] *= z;
    }
}
