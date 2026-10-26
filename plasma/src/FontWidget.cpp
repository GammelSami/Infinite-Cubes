#include "FontWidget.h"
#include "TextModule.h"
#include <GL/glew.h>
#include "plasma/ScalableFont.h"
#include "plasma/FontEngine.h"
#include "Workspace.h"
#include "plasma/Node.h"

FontWidget::FontWidget(plasma::Engine *engine, Workspace *workspace, plasma::Node *node)
    : plasma::Widget(engine, node, L"FontWidget"),
      m_workspace(workspace)
{}

FontWidget::~FontWidget() {}

bool FontWidget::contains(const plasma::Vector<2, float> &point)
{
    return true;
}

void FontWidget::pointerReleased()
{
    int index = (int)(getPointerOffset().m_elements[1] / (double)ROW_HEIGHT);
    if (index >= 0 && index < (int)m_workspace->m_textModule->m_filteredFonts.size())
        m_workspace->m_textModule->selectFont(m_workspace->m_textModule->m_filteredFonts[index], true);
    m_engine->setPopUpWidget(0, 0);
}

void FontWidget::refresh()
{
    if (getParentWidget()) {
        resizeTo(getParentWidget()->viewWidth(),
                 (float)(ROW_HEIGHT * m_workspace->m_textModule->m_filteredFonts.size()));
        getParentWidget()->resize(getViewSize());
        getParentWidget()->clampToParent();
        getParentWidget()->updateScrollSliders();
    }
}

plasma::Vector<2, float> FontWidget::getMaximum()
{
    return getViewSize();
}

bool FontWidget::hasBounds()
{
    return true;
}

// Layout positions, extents and spacing use display units; colour tuples style the overlays.
void FontWidget::draw()
{
    glDisable(GL_LINE_SMOOTH);
    glEnable(GL_TEXTURE_2D);
    int first = (int)(-getParentWidget()->viewY() / (double)ROW_HEIGHT);
    int last = (int)(getParentWidget()->getParentWidget()->getViewHeight() / (double)ROW_HEIGHT + (double)first +
                     (double)1.0f);
    if (first < 0)
        first = 0;
    if (last >= (int)m_workspace->getTextModule()->m_filteredFonts.size())
        last = m_workspace->getTextModule()->m_filteredFonts.size() - 1;
    int hovered = (int)(getPointerOffset().m_elements[1] / (double)ROW_HEIGHT);
    if (getNode() != m_engine->getPointerNode())
        hovered = -1;
    glPushAttrib(GL_SCISSOR_BIT);
    int viewportHeight = m_engine->m_viewportHeight;
    glScissor((int)getParentWidget()->getParentWidget()->getContentPosition()[0],
              viewportHeight - (int)(getParentWidget()->getParentWidget()->getViewHeight() +
                                     getParentWidget()->getParentWidget()->getContentPosition()[1]),
              (int)getParentWidget()->viewWidth(), (int)getParentWidget()->getViewHeight());
    bool plasmaFont = m_workspace->getTextModule()->getPanel()->getNode()->isChecked(L"text:type:plasma", true);
    plasma::Vector<4, float> extrusion(0.0f, 0.0f, 0.0f, 0.0f);
    for (int i = first; i <= last; ++i) {
        plasma::Vector<4, float> fill(0.0f, 0.0f, 0.0f, 1.0f);
        if (i == hovered)
            fill = plasma::Vector<4, float>(0.0f, 0.0f, 1.0f, 1.0f);
        plasma::Vector<4, float> outline(0.0f, 0.0f, 0.0f, 0.0f);
        const std::wstring &path = m_workspace->getTextModule()->m_filteredFonts[i];
        if (plasmaFont) {
            int slash = (int)path.find_last_of(L'/');
            int backslash = (int)path.find_last_of(L'\\');
            int start = slash > backslash ? slash : backslash;
            std::wstring caption = path.substr(start + 1, path.size() - start - 1);
            m_engine->getFontEngine()->renderText(L"tahoma.ttf", caption, 0.0f, 0.0f, 5.0f,
                                                  (float)(ROW_HEIGHT * i + 23), 16.0f, 0.0f, fill.m_elements,
                                                  outline.m_elements, extrusion.m_elements, 0, -1.0f, true);
        } else {
            plasma::ScalableFont *font = m_engine->getFontEngine()->getFont(path);
            if (font) {
                const std::wstring &filename = font->m_path;
                m_engine->getFontEngine()->renderText(filename, m_workspace->getTextModule()->m_fontCaptions[path],
                                                      0.0f, 0.0f, 5.0f, (float)(ROW_HEIGHT * i + 23), 16.0f, 0.0f,
                                                      fill.m_elements, outline.m_elements, extrusion.m_elements, 0,
                                                      -1.0f, true);
            }
        }
    }
    glPopAttrib();
}
