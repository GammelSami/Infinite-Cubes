// GenericShape supplies shared shape behavior and bounds.
#include "plasma/GenericShape.h"
#include "plasma/Engine.h"
#include "plasma/Drawing.h"

namespace plasma {
// A new generic shape's size (anonymous: this file's own).
namespace {
enum {
    DefaultShapeSize = 100
};
}

GenericShape::GenericShape(Engine *engine, const std::wstring &source, const std::wstring &name)
    : Shape(engine, name),
      m_position(0.0f, 0.0f),
      m_size(DefaultShapeSize, DefaultShapeSize),
      m_source(source)
{
    m_instanceNode = 0;
    m_type = SHAPE_GENERIC;
    m_drawing = engine->createDrawing();
    attributesChanged(ATTRIBUTES_CHANGED_ALL);
}

GenericShape::~GenericShape()
{
    m_engine->deleteDrawing(m_drawing);
}

Vector<2, float> GenericShape::getMaximum()
{
    return m_position + m_size;
}

Vector<2, float> GenericShape::getMinimum()
{
    return m_position;
}

Shape *GenericShape::clone()
{
    GenericShape *shape = m_engine->createGenericShape(m_source, m_name);
    shape->m_position = m_position;
    shape->m_size = m_size;
    shape->attributesChanged(ATTRIBUTES_CHANGED_ALL);
    return shape;
}

// An unresolved source is shown as a grey rectangle made of two triangles.
void GenericShape::updateDrawing()
{
    m_drawing->clear();
    if (!m_instanceNode) {
        Vector<2, float> minimum = m_position;
        Vector<2, float> maximum = m_position + m_size;
        m_drawing->m_pathVertices.resize(4);
        m_drawing->m_pathColors.resize(4);
        m_drawing->m_pathColors[0] = Vector<4, float>(0.7f, 0.7f, 0.7f, 1.0f);
        m_drawing->m_pathColors[1] = Vector<4, float>(0.6f, 0.6f, 0.6f, 1.0f);
        m_drawing->m_pathColors[2] = Vector<4, float>(0.5f, 0.5f, 0.5f, 1.0f);
        m_drawing->m_pathColors[3] = Vector<4, float>(0.6f, 0.6f, 0.6f, 1.0f);
        m_drawing->m_pathTextureCoordinates.resize(4, Vector<2, float>(0.0f, 0.0f));
        m_drawing->m_pathVertices[0] = Vector<2, float>(minimum[0], minimum[1]);
        m_drawing->m_pathVertices[1] = Vector<2, float>(maximum[0], minimum[1]);
        m_drawing->m_pathVertices[2] = Vector<2, float>(maximum[0], maximum[1]);
        m_drawing->m_pathVertices[3] = Vector<2, float>(minimum[0], maximum[1]);
        m_drawing->m_pathIndices.resize(6);
        m_drawing->m_pathIndices[0] = 0;
        m_drawing->m_pathIndices[1] = 1;
        m_drawing->m_pathIndices[2] = 2;
        m_drawing->m_pathIndices[3] = 2;
        m_drawing->m_pathIndices[4] = 3;
        m_drawing->m_pathIndices[5] = 0;
    }
    m_drawing->update();
}

void GenericShape::attributesChanged(int reason)
{
    m_changed = true;
}

}
