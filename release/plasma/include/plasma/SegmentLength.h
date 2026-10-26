#pragma once

// Squared lengths below this are treated as a zero-length segment when stroking or laying out a path (Drawing and
// TextShape).

namespace plasma {

const float MinimumSegmentLengthSquared = 1.0e-20f;

} // namespace plasma
