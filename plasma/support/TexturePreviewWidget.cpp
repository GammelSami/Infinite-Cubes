// Texture-preview support implementation.
#include "plasma/GenericShape.h"
#include "plasma/Drawing.h"
#include "TexturePreviewWidget.h"
#include "TextureSpaceWidget.h"
#include "TextureModule.h"
#include "DesignSheet.h"
#include "FormatModule.h"
#include "ShapeInterface.h"
#include <GL/glew.h>

using namespace plasma;

void TexturePreviewWidget::pointerReleasedAnywhere()
{
    if (m_checked) {
        Display *display = m_workspace->m_textureModule->m_selectionPanel->m_node->m_display;
        if (!display->visibility().value())
            setChecked(false, false);
    }
}
