#include "plasma/RenderSurface.h"

namespace plasma {

RenderSurface::RenderSurface()
    : m_linearFiltering(true),
      m_drawColor(1.0f, 1.0f, 1.0f, 1.0f),
      m_clearColor(0.0f, 0.0f, 0.0f, 0.0f),
      m_depthAttachment(true)
{}

RenderSurface::~RenderSurface() {}

}
