#include "plasma/GLDrawing.h"
#include "plasma/GLEngine.h"
#include "plasma/Display.h"
#include "plasma/Node.h"
#include "plasma/RenderSurface.h"
#include "plasma/Texture.h"

namespace plasma {

GLDrawing::GLDrawing(GLEngine *engine)
    : m_engine(engine),
      m_vertexBuffer(0),
      m_indexCount(0),
      m_vertexCount(0)
{}

GLDrawing::~GLDrawing()
{
    if (m_vertexBuffer) {
        glDeleteBuffers(1, &m_vertexBuffer);
        glDeleteBuffers(1, &m_colorBuffer);
        glDeleteBuffers(1, &m_normalBuffer);
        glDeleteBuffers(1, &m_textureCoordinateBuffer);
        glDeleteBuffers(1, &m_indexBuffer);
    }
}

extern int g_drawingLayer;

void GLDrawing::update()
{
    Drawing::update();
    if (m_dirty & ((1 << DRAWING_DIRTY_VERTICES) | (1 << DRAWING_DIRTY_INDICES)))
        m_dirty |= DRAWING_DIRTY_ALL;
    if (!m_vertexBuffer) {
        glGenBuffers(1, &m_vertexBuffer);
        glGenBuffers(1, &m_colorBuffer);
        glGenBuffers(1, &m_normalBuffer);
        glGenBuffers(1, &m_textureCoordinateBuffer);
        glGenBuffers(1, &m_indexBuffer);
    }
    if (!m_pathVertices.empty()) {
        int previousVertexCount = m_vertexCount;
        int previousIndexCount = m_indexCount;
        buildBuffers();
        m_indexCount = m_indices.size();
        m_vertexCount = m_vertices.size();
        if (m_dirty & (1 << DRAWING_DIRTY_VERTICES)) {
            glBindBuffer(GL_ARRAY_BUFFER, m_vertexBuffer);
            if (previousVertexCount != (int)m_vertices.size())
                glBufferData(GL_ARRAY_BUFFER, m_vertices.size() * sizeof(Vector<2, float>), &m_vertices[0],
                             GL_DYNAMIC_DRAW);
            else
                glBufferSubData(GL_ARRAY_BUFFER, 0, m_vertices.size() * sizeof(Vector<2, float>), &m_vertices[0]);
            glBindBuffer(GL_ARRAY_BUFFER, m_normalBuffer);
            if (previousVertexCount != (int)m_normals.size())
                glBufferData(GL_ARRAY_BUFFER, m_normals.size() * sizeof(Vector<4, float>), &m_normals[0],
                             GL_DYNAMIC_DRAW);
            else
                glBufferSubData(GL_ARRAY_BUFFER, 0, m_normals.size() * sizeof(Vector<4, float>), &m_normals[0]);
        }
        if (m_dirty & (1 << DRAWING_DIRTY_COLORS)) {
            glBindBuffer(GL_ARRAY_BUFFER, m_colorBuffer);
            if (previousVertexCount != (int)m_colors.size())
                glBufferData(GL_ARRAY_BUFFER, m_colors.size() * sizeof(Vector<4, float>), &m_colors[0],
                             GL_DYNAMIC_DRAW);
            else
                glBufferSubData(GL_ARRAY_BUFFER, 0, m_colors.size() * sizeof(Vector<4, float>), &m_colors[0]);
        }
        if (m_dirty & (1 << DRAWING_DIRTY_TEXCOORDS)) {
            glBindBuffer(GL_ARRAY_BUFFER, m_textureCoordinateBuffer);
            if (previousVertexCount != (int)m_textureCoordinates.size())
                glBufferData(GL_ARRAY_BUFFER, m_textureCoordinates.size() * sizeof(Vector<2, float>),
                             &m_textureCoordinates[0], GL_DYNAMIC_DRAW);
            else
                glBufferSubData(GL_ARRAY_BUFFER, 0, m_textureCoordinates.size() * sizeof(Vector<2, float>),
                                &m_textureCoordinates[0]);
        }
        if (m_dirty & (1 << DRAWING_DIRTY_INDICES)) {
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indexBuffer);
            if (m_indices.empty())
                glBufferData(GL_ELEMENT_ARRAY_BUFFER, 0, 0, GL_STATIC_DRAW);
            else if (previousIndexCount != (int)m_indices.size())
                glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_indices.size() * sizeof(int), &m_indices[0], GL_STATIC_DRAW);
            else
                glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, m_indices.size() * sizeof(int), &m_indices[0]);
        }
        m_vertices.clear();
        m_textureCoordinates.clear();
        m_colors.clear();
        m_normals.clear();
        m_indices.clear();
        m_vertices.reserve(0);
        m_textureCoordinates.reserve(0);
        m_colors.reserve(0);
        m_normals.reserve(0);
        m_indices.reserve(0);
    }
}

static void setDrawingMaterial(const Vector<4, float> &color)
{
    GLfloat previous[4];
    glGetMaterialfv(GL_FRONT, GL_AMBIENT, previous);
    if (previous[0] != color.m_elements[0] || previous[1] != color.m_elements[1] ||
        previous[2] != color.m_elements[2] || previous[3] != color.m_elements[3])
        glMaterialfv(GL_FRONT, GL_AMBIENT, color.m_elements);
}

void GLDrawing::draw(Node *node, const Vector<4, float> &color)
{
    if (!m_pathVertices.empty()) {
        if (m_engine->m_filterSurface) {
            if (!node || !(node->m_display->m_flags & (1 << DISPLAY_BLUR)) ||
                node->m_display->m_clipping.value() != DISPLAY_CLIPPING_DISABLED) {
                glActiveTexture(GL_TEXTURE1);
                m_engine->m_filterSurface->bindTexture();
            }
        }
        glActiveTexture(GL_TEXTURE0);
        glClientActiveTexture(GL_TEXTURE0);
        glDisable(GL_TEXTURE_RECTANGLE_ARB);
        if (m_texture) {
            glEnable(GL_TEXTURE_2D);
            m_texture->bind();
            glMatrixMode(GL_TEXTURE);
            glPushMatrix();
            glLoadMatrixf(m_textureMatrix.m_elements);
            glMatrixMode(GL_MODELVIEW);
        } else {
            glDisable(GL_TEXTURE_2D);
        }
        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        const Vector<4, float> *material;
        Vector<4, float> combined;
        if (node) {
            GLfloat z = -(double)g_drawingLayer - (double)g_drawingLayer;
            glTranslatef(0.0f, 0.0f, z);
            glEnable(GL_DEPTH_TEST);
            glDepthFunc(GL_GREATER);
            ++g_drawingLayer;
            combined = node->m_display->m_fillColor.value();
            for (unsigned channel = 0; channel < 4; ++channel)
                combined.m_elements[channel] *= color.m_elements[channel];
            material = &combined;
        } else {
            glDisable(GL_DEPTH_TEST);
            material = &color;
        }
        setDrawingMaterial(*material);
        glEnableClientState(GL_VERTEX_ARRAY);
        glEnableClientState(GL_COLOR_ARRAY);
        glEnableClientState(GL_INDEX_ARRAY);
        if (m_texture)
            glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        else
            glDisableClientState(GL_TEXTURE_COORD_ARRAY);
        if (m_indexCount) {
            glBindBuffer(GL_ARRAY_BUFFER, m_vertexBuffer);
            glVertexPointer(2, GL_FLOAT, 0, 0);
            glBindBuffer(GL_ARRAY_BUFFER, m_colorBuffer);
            glColorPointer(4, GL_FLOAT, 0, 0);
            glBindBuffer(GL_ARRAY_BUFFER, m_textureCoordinateBuffer);
            glTexCoordPointer(2, GL_FLOAT, 0, 0);
            glClientActiveTexture(GL_TEXTURE2);
            glEnableClientState(GL_TEXTURE_COORD_ARRAY);
            glBindBuffer(GL_ARRAY_BUFFER, m_normalBuffer);
            glTexCoordPointer(4, GL_FLOAT, 0, 0);
            glClientActiveTexture(GL_TEXTURE0);
            m_engine->setShapeShader(node, m_texture != 0, m_textureOpacity, m_textureBrightness, m_textureContrast,
                                     m_textureSaturation);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indexBuffer);
            glIndexPointer(GL_INT, 0, 0);
            glDrawRangeElements(GL_TRIANGLES, 0, m_vertexCount - 1, m_indexCount, GL_UNSIGNED_INT, 0);
        }
        glPopMatrix();
        if (m_texture) {
            glMatrixMode(GL_TEXTURE);
            glPopMatrix();
            glMatrixMode(GL_MODELVIEW);
        }
    }
}

}
