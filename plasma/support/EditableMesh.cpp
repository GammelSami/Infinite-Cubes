// GLU edge-flag callback used by Drawing::Tessellate. The callback is empty.
#include "plasma/Drawing.h"
#include "plasma/GLDrawing.h"
#include "plasma/QuadMesh.h"
#include "plasma/Node.h"
#include "plasma/GLEngine.h"

namespace plasma {

void __stdcall tessellationEdgeFlag(unsigned char) {}

}
