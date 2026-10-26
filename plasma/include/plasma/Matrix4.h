#pragma once

namespace plasma {

// The editor's 4x4 float matrix, stored column-major with translation in m[12..14].
struct Matrix4 {
    void identity()
    {
        m[0] = 1.0f;
        m[1] = 0.0f;
        m[2] = 0.0f;
        m[3] = 0.0f;
        m[4] = 0.0f;
        m[5] = 1.0f;
        m[6] = 0.0f;
        m[7] = 0.0f;
        m[8] = 0.0f;
        m[9] = 0.0f;
        m[10] = 1.0f;
        m[11] = 0.0f;
        m[12] = 0.0f;
        m[13] = 0.0f;
        m[14] = 0.0f;
        m[15] = 1.0f;
    }
    void scale(float x, float y);
    void translate(float x, float y)
    {
        m[12] = m[0] * x + m[4] * y + m[12];
        m[13] = m[1] * x + m[5] * y + m[13];
        m[14] = m[2] * x + m[6] * y + m[14];
        m[15] = m[3] * x + m[7] * y + m[15];
    }

    float m[16];
};

}
