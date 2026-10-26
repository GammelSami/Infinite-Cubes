// Simplex noise and smoothing helpers. The noise implementation is a C++ port of Stefan Gustavson's SimplexNoise.java
// (2012), using its skew factors, gradients and 2D algorithm.
#pragma once
#include "plasma/Vector.h"
#include "cube/Fixed.h"

namespace cube {

// 2D simplex noise: (sqrt(3) - 1) / 2 and (3 - sqrt(3)) / 6. 4D simplex noise: (sqrt(5) - 1) / 4 and (5 - sqrt(5)) /
// 20.
static const double F2 = 0.5 * (sqrt(3.0) - 1.0);
static const double G2 = (3.0 - sqrt(3.0)) / 6.0;
static const double F4 = (sqrt(5.0) - 1.0) / 4.0;
static const double G4 = (5.0 - sqrt(5.0)) / 20.0;

// Gradient tables for 3D and 4D simplex noise.
static plasma::Vector<3, double> grad3[] = {
    plasma::Vector<3, double>(1, 1, 0),   plasma::Vector<3, double>(-1, 1, 0),  plasma::Vector<3, double>(1, -1, 0),
    plasma::Vector<3, double>(-1, -1, 0), plasma::Vector<3, double>(1, 0, 1),   plasma::Vector<3, double>(-1, 0, 1),
    plasma::Vector<3, double>(1, 0, -1),  plasma::Vector<3, double>(-1, 0, -1), plasma::Vector<3, double>(0, 1, 1),
    plasma::Vector<3, double>(0, -1, 1),  plasma::Vector<3, double>(0, 1, -1),  plasma::Vector<3, double>(0, -1, -1)};
static plasma::Vector<4, double> grad4[] = {
    plasma::Vector<4, double>(0, 1, 1, 1),   plasma::Vector<4, double>(0, 1, 1, -1),
    plasma::Vector<4, double>(0, 1, -1, 1),  plasma::Vector<4, double>(0, 1, -1, -1),
    plasma::Vector<4, double>(0, -1, 1, 1),  plasma::Vector<4, double>(0, -1, 1, -1),
    plasma::Vector<4, double>(0, -1, -1, 1), plasma::Vector<4, double>(0, -1, -1, -1),
    plasma::Vector<4, double>(1, 0, 1, 1),   plasma::Vector<4, double>(1, 0, 1, -1),
    plasma::Vector<4, double>(1, 0, -1, 1),  plasma::Vector<4, double>(1, 0, -1, -1),
    plasma::Vector<4, double>(-1, 0, 1, 1),  plasma::Vector<4, double>(-1, 0, 1, -1),
    plasma::Vector<4, double>(-1, 0, -1, 1), plasma::Vector<4, double>(-1, 0, -1, -1),
    plasma::Vector<4, double>(1, 1, 0, 1),   plasma::Vector<4, double>(1, 1, 0, -1),
    plasma::Vector<4, double>(1, -1, 0, 1),  plasma::Vector<4, double>(1, -1, 0, -1),
    plasma::Vector<4, double>(-1, 1, 0, 1),  plasma::Vector<4, double>(-1, 1, 0, -1),
    plasma::Vector<4, double>(-1, -1, 0, 1), plasma::Vector<4, double>(-1, -1, 0, -1),
    plasma::Vector<4, double>(1, 1, 1, 0),   plasma::Vector<4, double>(1, 1, -1, 0),
    plasma::Vector<4, double>(1, -1, 1, 0),  plasma::Vector<4, double>(1, -1, -1, 0),
    plasma::Vector<4, double>(-1, 1, 1, 0),  plasma::Vector<4, double>(-1, 1, -1, 0),
    plasma::Vector<4, double>(-1, -1, 1, 0), plasma::Vector<4, double>(-1, -1, -1, 0)};
// Defined in Common.cpp.
extern unsigned char perm[512];
extern unsigned char permMod12[512];

// One xorshift step (13, 17, 5), used to hash generator counters and seeds.
inline unsigned int hash(unsigned int x)
{
    unsigned int a = x;
    a ^= a << 13;
    a ^= a >> 17;
    a ^= a << 5;
    return a;
}

// Gustavson's fastfloor and 2D dot.
inline int fastfloor(double x)
{
    int xi = (int)x;
    return x < xi ? xi - 1 : xi;
}

inline double dot(const plasma::Vector<3, double> &g, double x, double y)
{
    return g[0] * x + g[1] * y;
}

// Two-dimensional simplex noise at (0.32 * x, 0.32 * y), approximately -1..1, narrowed to float. It does not modify
// state.
inline float noise(double xin, double yin)
{
    xin *= 0.32;
    yin *= 0.32;
    double n0, n1, n2;
    double s = (xin + yin) * F2;
    int i = fastfloor(xin + s);
    int j = fastfloor(yin + s);
    double t = (i + j) * G2;
    double X0 = i - t;
    double Y0 = j - t;
    double x0 = xin - X0;
    double y0 = yin - Y0;
    int i1, j1;
    if (x0 > y0) {
        i1 = 1;
        j1 = 0;
    } else {
        i1 = 0;
        j1 = 1;
    }
    double x1 = x0 - i1 + G2;
    double y1 = y0 - j1 + G2;
    double x2 = x0 - 1.0 + 2.0 * G2;
    double y2 = y0 - 1.0 + 2.0 * G2;
    int ii = i & 255;
    int jj = j & 255;
    int gi0 = permMod12[ii + perm[jj]];
    int gi1 = permMod12[ii + i1 + perm[jj + j1]];
    int gi2 = permMod12[ii + 1 + perm[jj + 1]];
    double t0 = 0.5 - x0 * x0 - y0 * y0;
    if (t0 < 0)
        n0 = 0.0;
    else {
        t0 *= t0;
        n0 = t0 * t0 * dot(grad3[gi0], x0, y0);
    }
    double t1 = 0.5 - x1 * x1 - y1 * y1;
    if (t1 < 0)
        n1 = 0.0;
    else {
        t1 *= t1;
        n1 = t1 * t1 * dot(grad3[gi1], x1, y1);
    }
    double t2 = 0.5 - x2 * x2 - y2 * y2;
    if (t2 < 0)
        n2 = 0.0;
    else {
        t2 *= t2;
        n2 = t2 * t2 * dot(grad3[gi2], x2, y2);
    }
    return 70.0 * (n0 + n1 + n2);
}

// Frame-rate independent smoothing: the fraction to move towards a target after frameTime, using 100 steps per second
// with each step moving rate * 0.02 of the remaining distance. Returns zero for less than one step.
float blendFactor(double frameTime, float rate);
// Moves an angle in degrees towards target by factor along the shorter direction, using the sine of the difference;
// chooses target when the angles are nearly opposite.
float interpolateAngle(float angle, float target, float factor);
// value moved towards target by blendFactor(frameTime, rate).
template<typename T> void blend(T &value, const T &target, double frameTime, float rate)
{
    value += (target - value) * blendFactor(frameTime, rate);
}
template<> inline void blend(float &value, const float &target, double frameTime, float rate)
{
    float factor = blendFactor(frameTime, rate);
    value += (target - value) * factor;
}
// A coordinate moves by its difference times the factor, truncated (Game::update lifts the character with it in its
// debug cruise).
template<> inline void blend(Fixed &value, const Fixed &target, double frameTime, float rate)
{
    float factor = blendFactor(frameTime, rate);
    value += (target - value) * factor;
}

// Divides value by divisor, rounding down for a positive divisor. Used to convert block, zone, region and map
// coordinates.
inline int floorDivide(int value, int divisor)
{
    return value >= 0 ? value / divisor : (value + 1) / divisor - 1;
}

}
