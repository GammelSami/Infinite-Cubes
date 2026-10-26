// PDF reader helpers for display clipping and Matrix4 scaling.
#include "plasma/Display.h"
#include "plasma/MeshShape.h"
#include "plasma/Node.h"

namespace plasma {

struct DisplayClipAccess : Display {
    static void clip(Display *display)
    {
        DisplayClipAccess *access = static_cast<DisplayClipAccess *>(display);
        access->m_clipping.value() = DISPLAY_CLIPPING_ENABLED;
        access->m_flags |= (1 << DISPLAY_MODULATION);
    }
};

}

void clipToShape(plasma::Display *display)
{
    plasma::DisplayClipAccess::clip(display);
}

#include <windows.h>
#include "pdf/PDFReader.h"

void plasma::Matrix4::scale(float x, float y)
{
    for (int i = 0; i < 4; ++i) {
        m[i] *= x;
        m[4 + i] *= y;
    }
}
