#pragma once
#include "plasma/Vector.h"

// Squared distance from a point to a segment. A segment shorter than the squared tolerance 1e-20
// counts as its start.
inline float squaredDistanceToSegment(const plasma::Vector<2, float> &point, const plasma::Vector<2, float> &start,
                                      const plasma::Vector<2, float> &end)
{
    plasma::Vector<2, float> toPoint = point - start;
    plasma::Vector<2, float> segment = end - start;
    float lengthSquared = segment.squaredLength();
    if (lengthSquared < 1e-20f)
        return toPoint.squaredLength();
    float t = toPoint.dot(segment) / lengthSquared;
    if (t <= 0.0f)
        return toPoint.squaredLength();
    if (t >= 1.0f)
        return (point - end).squaredLength();
    return (toPoint - segment * t).squaredLength();
}
