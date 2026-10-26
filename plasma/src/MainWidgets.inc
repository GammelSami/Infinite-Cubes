// Widget helpers shared by platform entry points, included by WindowsUI.cpp.
#ifdef PLASMA_PORTABLE
#include "Module.h"
#include "plasma/TextShape.h"
#endif

ModuleWidgetItem::ModuleWidgetItem(const std::wstring &caption, const std::wstring &name, const std::wstring &shape,
                                   int left, int top, int width, int height)
    : m_caption(caption),
      m_name(name),
      m_shape(shape),
      m_left(left),
      m_top(top),
      m_width(width),
      m_height(height)
{}

void plasma::TextShape::setVerticalAlignment(int alignment)
{
    // Each case leaves the other alignment bit as it was.
    switch (alignment) {
    case ALIGN_TOP:
        m_flags &= ~TEXT_VERTICAL_MASK;
        break;
    case ALIGN_BOTTOM:
        m_flags = (m_flags & ~(1 << TEXT_MIDDLE)) | (1 << TEXT_BOTTOM);
        break;
    case ALIGN_MIDDLE:
        m_flags = (m_flags & ~(1 << TEXT_BOTTOM)) | (1 << TEXT_MIDDLE);
        break;
    }
}
