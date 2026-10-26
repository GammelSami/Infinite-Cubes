#pragma once

// The world's line-of-sight queries, shared by World.cpp (cameraRay, rayDistance, canSee) and World/AI.cpp (canWalk,
// rayLength, combat). Not in World.h on purpose (see WorldLayout.h).

namespace cube {

// The block-crossing walks: the step bound before the nearest crossing is found, the squared direction component below
// which an axis is ignored, and the crossing limit.
const float RayStepBound = 10.0f;
const float RayAxisEpsilon = 0.000001f;
const int RayMaxCrossings = 500;

// A target's stealth shrinks sight and attack ranges to no less than this fraction (World::canSee; squared in the AI's
// attack range).
const float MinStealthRangeFactor = 0.1f;

}
