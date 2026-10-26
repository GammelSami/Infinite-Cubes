#include "DesignSheet.h"
#include "Workspace.h"
#include "TextureSpaceWidget.h"
#include "plasma/Transformation.h"

// Layout positions, extents and spacing use display units; colour tuples style the overlays.
TextureSheet::TextureSheet(Workspace *workspace)
    : Sheet(workspace)
{
    plasma::Engine *engine = &workspace->m_engine;
    plasma::Node *content = engine->createNode(0, 0, 0, m_panel->m_node, L"");
    plasma::Transformation *transformation = content->m_transformation;
    plasma::Vector<2, float> origin = m_panel->getBindPos();
    plasma::Vector<2, float> &translation = plasma::Widget::getTranslation(transformation);
    translation.m_elements[0] += origin.m_elements[0];
    translation.m_elements[1] += origin.m_elements[1];
    transformation->attributesChanged(plasma::ATTRIBUTES_CHANGED_ALL);

    plasma::Node *view = engine->createNode(0, 0, 0, content, L"");
    new TextureSpaceWidget(engine, this, view);
    int viewportHeight = engine->viewportHeight();
    m_frame->resizeTo((double)viewportHeight * 0.5, 0.5 * (double)viewportHeight);
    m_frame->moveTo(100.0f, 100.0f, true);
    m_zoomScale = 1.0f;
    m_root->m_transformation->m_deformation.value().scale(1.0f, 1.0f, 1.0f);
    m_root->m_transformation->attributesChanged(plasma::ATTRIBUTES_CHANGED_ALL);

    std::wstring title(L"Texture");
    setPath(title);
}

TextureSheet::~TextureSheet() {}

plasma::Matrix<float> TextureSheet::getMatrix()
{
    plasma::Matrix<float> matrix = m_root->m_matrix;
    matrix.scale(m_page.size.m_elements[0], m_page.size.m_elements[1]);
    return matrix;
}
