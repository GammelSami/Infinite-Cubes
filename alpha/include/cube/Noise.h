// Value noise uses a pseudo-random grid, a row stride of 57 and cosine interpolation.
#pragma once
#include <math.h>

namespace cube {

// Pseudo-random value from negative one to one for a grid point.
inline double gridValue(int x, int y)
{
    // UB: signed shifts and products retain 32-bit wraparound.
    int n = x + y * 57;
    n = (n << 13) ^ n;
    return 1.0 - ((n * (n * n * 60493 + 19990303) + 1376312589) & 0x7fffffff) / 1073741824.0;
}

// Between a (at 0) and b (at 1), eased by a cosine.
inline double cosineInterpolate(double a, double b, double x)
{
    double f = (1.0 - cos(x * 3.1415927)) * 0.5;
    return a * (1.0 - f) + b * f;
}

// The noise at a point: the grid values of the four corners around it, interpolated (the
// coordinates are truncated toward zero).
inline float noise(double x, double y)
{
    double x0 = (int)x;
    double y0 = (int)y;
    double lowerLeftValue = gridValue((int)x0, (int)y0);
    double lowerRightValue = gridValue((int)(x0 + 1.0), (int)y0);
    double upperLeftValue = gridValue((int)x0, (int)(y0 + 1.0));
    double upperRightValue = gridValue((int)(x0 + 1.0), (int)(y0 + 1.0));
    double lowerRowValue = cosineInterpolate(lowerLeftValue, lowerRightValue, x - x0);
    double upperRowValue = cosineInterpolate(upperLeftValue, upperRightValue, x - x0);
    return (float)cosineInterpolate(lowerRowValue, upperRowValue, y - y0);
}

// Fraction of the distance to a target covered by steps, each moving rate of the remaining
// distance.
inline float blendFactor(int steps, float rate)
{
    double value = 0.0;
    double fraction = rate;
    for (int i = 0; i < steps; i++)
        value += (1.0 - value) * fraction;
    return (float)value;
}
// Moves value towards target by blendFactor(steps, rate).
template<typename T> void blend(T &value, const T &target, int steps, float rate)
{
    float factor = blendFactor(steps, rate);
    value = value * (1.0f - factor) + target * factor;
}
template<> void blend(float &value, const float &target, int steps, float rate);

}
