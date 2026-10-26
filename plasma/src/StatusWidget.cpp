#include "StatusWidget.h"
#include "plasma/FontEngine.h"
#include "plasma/Node.h"

StatusWidget::StatusWidget(plasma::Engine *engine, Workspace *workspace, plasma::Node *node)
    : plasma::Widget(engine, node, L"statuswidget"),
      m_workspace(workspace)
{}

StatusWidget::~StatusWidget() {}

void StatusWidget::initialize()
{
    m_node->setHitTestable(false);
}

// Layout positions, extents and spacing use display units; colour tuples style the overlays.
void StatusWidget::draw()
{
    int textY = 20;
    if (!m_annotation.empty()) {
        m_engine->getFontEngine()->renderText(L"verdana.ttf", m_annotation, 0.0f, 0.0f, 155.0f, 20.0f, 12.0f, 0.0f,
                                              plasma::Vector<4, float>(0.0f, 1.0f, 1.0f, 1.0f).m_elements,
                                              plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f).m_elements,
                                              plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, 16, 240.0f,
                                              true);
        textY = 40;
    }
    m_engine->getFontEngine()->renderText(L"verdana.ttf", m_text, 0.0f, 1.0f, 155.0f, (float)textY, 12.0f, 0.0f,
                                          plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f).m_elements,
                                          plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f).m_elements,
                                          plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, 16, 230.0f,
                                          true);
}
