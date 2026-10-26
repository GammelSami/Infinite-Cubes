// Animation helpers shared by the creature renderer and the previews.
#pragma once
#include <math.h>
#include "plasma/Vector.h"

namespace cube {
// Repeats a millisecond's blend rather than scaling its rate by elapsed time.
inline void blend(float &value, const float &target, int elapsed, float rate)
{
    for (int i = 0; i < elapsed; ++i)
        value += (target - value) * rate;
}

// Blends each vector component by the same rule.
inline void blend(plasma::Vector<3, float> &value, const plasma::Vector<3, float> &target, int elapsed, float rate)
{
    for (int i = 0; i < elapsed; ++i)
        value += (target - value) * rate;
}

// Turns by the signed sine of the angle difference, clamped before asin. opposite angles produce
// no turn.
inline float interpolateAngle(float angle, float target, float factor)
{
    float a = angle / 180.0f * 3.141592653589793;
    plasma::Vector<2, double> directionA(cosf(a), sinf(a));
    float b = target / 180.0f * 3.141592653589793;
    plasma::Vector<2, double> directionB(cosf(b), sinf(b));
    float sine = directionB[1] * directionA[0] - directionB[0] * directionA[1];
    if (sine > 1.0f)
        sine = 1.0f;
    if (sine < -1.0f)
        sine = -1.0f;
    float delta = asin(sine) * (factor * 180.0 / 3.141592653589793);
    return angle + delta;
}
}
