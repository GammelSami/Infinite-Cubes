// Image provides RGB/RGBA pixel buffers. WireFrame draws mesh outlines from OpenGL buffer objects.
#include "Image.h"
#include "WireFrame.h"
#include <string.h>

Image::Image(int width, int height, int bytesPerPixel)
    : m_width(width),
      m_height(height),
      m_bytesPerPixel(bytesPerPixel)
{
    m_rows = NULL;
    m_rows = new unsigned char *[height];
    memset(m_rows, 0, height * sizeof(unsigned char *));
    for (int row = 0; row < height; row++)
        m_rows[row] = new unsigned char[width * bytesPerPixel];
}

Image::~Image()
{
    if (m_rows) {
        for (int row = 0; row < m_height; row++) {
            if (m_rows[row])
                delete[] m_rows[row];
        }
        delete[] m_rows;
    }
}

WireFrame::WireFrame()
    : m_pointsBuffer(0)
{}

WireFrame::~WireFrame()
{
    clear();
}

void WireFrame::clear()
{
    if (m_pointsBuffer) {
        // Zeroed before the call, so glDeleteBuffers is handed name 0 and the points buffer itself is never deleted (as in
        // the code).
        m_pointsBuffer = 0;
        glDeleteBuffers(1, &m_pointsBuffer);
        glDeleteBuffers(1, &m_outlinePointsBuffer);
        glDeleteBuffers(1, &m_textureOutlinePointsBuffer);
        glDeleteBuffers(1, &m_pointColorsBuffer);
        glDeleteBuffers(1, &m_fillColorsBuffer);
        glDeleteBuffers(1, &m_strokeColorsBuffer);
        glDeleteBuffers(1, &m_texturePointsBuffer);
        glDeleteBuffers(1, &m_extrudedPointsBuffer);
        glDeleteBuffers(1, &m_frontColorsBuffer);
        glDeleteBuffers(1, &m_backColorsBuffer);
        glDeleteBuffers(1, &m_unusedIndices0Buffer);
        glDeleteBuffers(1, &m_unusedIndices1Buffer);
        glDeleteBuffers(1, &m_unusedIndices2Buffer);
        glDeleteBuffers(1, &m_lineIndicesBuffer);
        glDeleteBuffers(1, &m_unselectedLineIndicesBuffer);
        glDeleteBuffers(1, &m_selectedLineIndicesBuffer);
        glDeleteBuffers(1, &m_hoveredLineIndicesBuffer);
        glDeleteBuffers(1, &m_triangleIndicesBuffer);
        glDeleteBuffers(1, &m_unselectedTriangleIndicesBuffer);
        glDeleteBuffers(1, &m_selectedTriangleIndicesBuffer);
        glDeleteBuffers(1, &m_hoveredTriangleIndicesBuffer);
        glDeleteBuffers(1, &m_outlineIndicesBuffer);

        m_points.clear();
        m_texturePoints.clear();
        m_extrudedPoints.clear();
        m_outlinePoints.clear();
        m_textureOutlinePoints.clear();
        m_pointColors.clear();
        m_fillColors.clear();
        m_strokeColors.clear();
        m_frontColors.clear();
        m_backColors.clear();
        m_unusedIndices0.clear();
        m_unusedIndices1.clear();
        m_unusedIndices2.clear();
        m_lineIndices.clear();
        m_unselectedLineIndices.clear();
        m_selectedLineIndices.clear();
        m_hoveredLineIndices.clear();
        m_triangleIndices.clear();
        m_unselectedTriangleIndices.clear();
        m_selectedTriangleIndices.clear();
        m_hoveredTriangleIndices.clear();
        m_outlineIndices.clear();

        m_points.reserve(0);
        m_texturePoints.reserve(0);
        m_extrudedPoints.reserve(0);
        m_outlinePoints.reserve(0);
        m_pointColors.reserve(0);
        m_fillColors.reserve(0);
        m_strokeColors.reserve(0);
        m_frontColors.reserve(0);
        m_backColors.reserve(0);
        m_unusedIndices0.reserve(0);
        m_unusedIndices1.reserve(0);
        m_unusedIndices2.reserve(0);
        m_lineIndices.reserve(0);
        m_unselectedLineIndices.reserve(0);
        m_selectedLineIndices.reserve(0);
        m_hoveredLineIndices.reserve(0);
        m_triangleIndices.reserve(0);
        m_unselectedTriangleIndices.reserve(0);
        m_selectedTriangleIndices.reserve(0);
        m_hoveredTriangleIndices.reserve(0);
        m_outlineIndices.reserve(0);
    }
}

// Binds buffer to target and replaces its contents with data. (Point), (Color), (unsigned int).
template<typename T> static void uploadBuffer(GLenum target, GLuint buffer, const std::vector<T> &data, GLenum usage)
{
    glBindBuffer(target, buffer);
    if (data.empty())
        glBufferData(target, 0, NULL, usage);
    else
        glBufferData(target, data.size() * sizeof(T), &data[0], usage);
}

void WireFrame::upload()
{
    if (!m_pointsBuffer) {
        glGenBuffers(1, &m_pointsBuffer);
        glGenBuffers(1, &m_outlinePointsBuffer);
        glGenBuffers(1, &m_textureOutlinePointsBuffer);
        glGenBuffers(1, &m_pointColorsBuffer);
        glGenBuffers(1, &m_strokeColorsBuffer);
        glGenBuffers(1, &m_fillColorsBuffer);
        glGenBuffers(1, &m_texturePointsBuffer);
        glGenBuffers(1, &m_extrudedPointsBuffer);
        glGenBuffers(1, &m_frontColorsBuffer);
        glGenBuffers(1, &m_backColorsBuffer);
        glGenBuffers(1, &m_unusedIndices0Buffer);
        glGenBuffers(1, &m_unusedIndices1Buffer);
        glGenBuffers(1, &m_unusedIndices2Buffer);
        glGenBuffers(1, &m_lineIndicesBuffer);
        glGenBuffers(1, &m_unselectedLineIndicesBuffer);
        glGenBuffers(1, &m_selectedLineIndicesBuffer);
        glGenBuffers(1, &m_hoveredLineIndicesBuffer);
        glGenBuffers(1, &m_triangleIndicesBuffer);
        glGenBuffers(1, &m_unselectedTriangleIndicesBuffer);
        glGenBuffers(1, &m_selectedTriangleIndicesBuffer);
        glGenBuffers(1, &m_hoveredTriangleIndicesBuffer);
        glGenBuffers(1, &m_outlineIndicesBuffer);
    }
    uploadBuffer(GL_ARRAY_BUFFER, m_pointsBuffer, m_points, GL_DYNAMIC_DRAW);
    uploadBuffer(GL_ARRAY_BUFFER, m_outlinePointsBuffer, m_outlinePoints, GL_DYNAMIC_DRAW);
    uploadBuffer(GL_ARRAY_BUFFER, m_textureOutlinePointsBuffer, m_textureOutlinePoints, GL_DYNAMIC_DRAW);
    uploadBuffer(GL_ARRAY_BUFFER, m_pointColorsBuffer, m_pointColors, GL_DYNAMIC_DRAW);
    uploadBuffer(GL_ARRAY_BUFFER, m_fillColorsBuffer, m_fillColors, GL_DYNAMIC_DRAW);
    uploadBuffer(GL_ARRAY_BUFFER, m_strokeColorsBuffer, m_strokeColors, GL_DYNAMIC_DRAW);
    uploadBuffer(GL_ARRAY_BUFFER, m_texturePointsBuffer, m_texturePoints, GL_DYNAMIC_DRAW);
    uploadBuffer(GL_ARRAY_BUFFER, m_extrudedPointsBuffer, m_extrudedPoints, GL_DYNAMIC_DRAW);
    uploadBuffer(GL_ARRAY_BUFFER, m_frontColorsBuffer, m_frontColors, GL_DYNAMIC_DRAW);
    uploadBuffer(GL_ARRAY_BUFFER, m_backColorsBuffer, m_backColors, GL_DYNAMIC_DRAW);
    uploadBuffer(GL_ELEMENT_ARRAY_BUFFER, m_unusedIndices0Buffer, m_unusedIndices0, GL_DYNAMIC_DRAW);
    uploadBuffer(GL_ELEMENT_ARRAY_BUFFER, m_unusedIndices1Buffer, m_unusedIndices1, GL_DYNAMIC_DRAW);
    uploadBuffer(GL_ELEMENT_ARRAY_BUFFER, m_unusedIndices2Buffer, m_unusedIndices2, GL_DYNAMIC_DRAW);
    uploadBuffer(GL_ELEMENT_ARRAY_BUFFER, m_lineIndicesBuffer, m_lineIndices, GL_STATIC_DRAW);
    uploadBuffer(GL_ELEMENT_ARRAY_BUFFER, m_unselectedLineIndicesBuffer, m_unselectedLineIndices, GL_DYNAMIC_DRAW);
    uploadBuffer(GL_ELEMENT_ARRAY_BUFFER, m_selectedLineIndicesBuffer, m_selectedLineIndices, GL_DYNAMIC_DRAW);
    uploadBuffer(GL_ELEMENT_ARRAY_BUFFER, m_hoveredLineIndicesBuffer, m_hoveredLineIndices, GL_DYNAMIC_DRAW);
    uploadBuffer(GL_ELEMENT_ARRAY_BUFFER, m_triangleIndicesBuffer, m_triangleIndices, GL_STATIC_DRAW);
    uploadBuffer(GL_ELEMENT_ARRAY_BUFFER, m_unselectedTriangleIndicesBuffer, m_unselectedTriangleIndices,
                 GL_DYNAMIC_DRAW);
    uploadBuffer(GL_ELEMENT_ARRAY_BUFFER, m_selectedTriangleIndicesBuffer, m_selectedTriangleIndices, GL_DYNAMIC_DRAW);
    uploadBuffer(GL_ELEMENT_ARRAY_BUFFER, m_hoveredTriangleIndicesBuffer, m_hoveredTriangleIndices, GL_DYNAMIC_DRAW);
    uploadBuffer(GL_ELEMENT_ARRAY_BUFFER, m_outlineIndicesBuffer, m_outlineIndices, GL_STATIC_DRAW);
}

void WireFrame::uploadEditArrays()
{
    uploadBuffer(GL_ARRAY_BUFFER, m_pointColorsBuffer, m_pointColors, GL_DYNAMIC_DRAW);
    uploadBuffer(GL_ELEMENT_ARRAY_BUFFER, m_hoveredLineIndicesBuffer, m_hoveredLineIndices, GL_DYNAMIC_DRAW);
    uploadBuffer(GL_ELEMENT_ARRAY_BUFFER, m_hoveredTriangleIndicesBuffer, m_hoveredTriangleIndices, GL_DYNAMIC_DRAW);
}

// Draws count indices of the bound element buffer over the first vertexCount vertices.
#define DRAW_INDICES(mode, vertexCount, indices)                                                                       \
    glDrawRangeElements(mode, 0, (vertexCount) - 1, (indices).size(), GL_UNSIGNED_INT, NULL)

// Layout positions, extents and spacing use display units; colour tuples style the overlays. Wireframe overlay
// palette: white unselected, green selected and cyan hovered, with quarter/half opacity backing and extrusion
// layers.
void WireFrame::draw(unsigned int flags, const Color &color)
{
    if (!m_pointsBuffer || m_points.empty())
        return;

    glUseProgram(0);
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_INDEX_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_NORMAL_ARRAY);
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_LINE_SMOOTH);

    if (flags & WIREFRAME_DRAW_LINES) {
        if (flags & WIREFRAME_TEXTURE_SPACE)
            glBindBuffer(GL_ARRAY_BUFFER, m_texturePointsBuffer);
        else
            glBindBuffer(GL_ARRAY_BUFFER, m_pointsBuffer);
        glVertexPointer(2, GL_FLOAT, 0, NULL);
        if ((flags & WIREFRAME_DRAW_TRIANGLES) && !m_triangleIndices.empty()) {
            glColor4f(1.0f, 1.0f, 1.0f, 0.5f);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_triangleIndicesBuffer);
            glIndexPointer(GL_INT, 0, NULL);
            DRAW_INDICES(GL_TRIANGLES, m_points.size(), m_triangleIndices);
            if (!m_unselectedTriangleIndices.empty()) {
                glColor4f(1.0f, 1.0f, 1.0f, 0.25f);
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_unselectedTriangleIndicesBuffer);
                glIndexPointer(GL_INT, 0, NULL);
                DRAW_INDICES(GL_TRIANGLES, m_points.size(), m_unselectedTriangleIndices);
            }
            if (!m_selectedTriangleIndices.empty()) {
                glColor4f(0.0f, 1.0f, 0.0f, 0.5f);
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_selectedTriangleIndicesBuffer);
                glIndexPointer(GL_INT, 0, NULL);
                DRAW_INDICES(GL_TRIANGLES, m_points.size(), m_selectedTriangleIndices);
            }
            if (!m_hoveredTriangleIndices.empty()) {
                glColor4f(0.0f, 1.0f, 1.0f, 0.5f);
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_hoveredTriangleIndicesBuffer);
                glIndexPointer(GL_INT, 0, NULL);
                DRAW_INDICES(GL_TRIANGLES, m_points.size(), m_hoveredTriangleIndices);
            }
        }
        if (!m_lineIndices.empty()) {
            if ((flags & WIREFRAME_EXTRUSION_COLORS) && !m_extrudedPoints.empty()) {
                glBindBuffer(GL_ARRAY_BUFFER, m_extrudedPointsBuffer);
                glVertexPointer(2, GL_FLOAT, 0, NULL);
                glColor4f(0.0f, 0.0f, 0.0f, 0.25f);
                glLineWidth(2.0f);
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_lineIndicesBuffer);
                glIndexPointer(GL_INT, 0, NULL);
                DRAW_INDICES(GL_LINES, m_points.size(), m_lineIndices);
                glLineWidth(1.0f);
                glColor4fv((color * Color(1.0f, 1.0f, 1.0f, 0.5f)).m_elements);
                DRAW_INDICES(GL_LINES, m_points.size(), m_lineIndices);
                glBindBuffer(GL_ARRAY_BUFFER, m_pointsBuffer);
                glVertexPointer(2, GL_FLOAT, 0, NULL);
            }
            if (flags & WIREFRAME_HIGHLIGHT)
                glColor4f(0.5f, 0.5f, 0.5f, 0.5f);
            else
                glColor4f(0.5f, 0.5f, 0.5f, 0.25f);
            glLineWidth(2.0f);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_lineIndicesBuffer);
            glIndexPointer(GL_INT, 0, NULL);
            DRAW_INDICES(GL_LINES, m_points.size(), m_lineIndices);
            glLineWidth(1.0f);
            if (flags & WIREFRAME_HIGHLIGHT) {
                if (!m_unselectedLineIndices.empty()) {
                    glColor4f(1.0f, 1.0f, 1.0f, 0.5f);
                    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_unselectedLineIndicesBuffer);
                    glIndexPointer(GL_INT, 0, NULL);
                    DRAW_INDICES(GL_LINES, m_points.size(), m_unselectedLineIndices);
                }
                if (!m_selectedLineIndices.empty()) {
                    glColor4f(0.0f, 1.0f, 0.0f, 1.0f);
                    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_selectedLineIndicesBuffer);
                    glIndexPointer(GL_INT, 0, NULL);
                    DRAW_INDICES(GL_LINES, m_points.size(), m_selectedLineIndices);
                }
                if (!m_hoveredLineIndices.empty()) {
                    glColor4f(0.0f, 1.0f, 1.0f, 0.5f);
                    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_hoveredLineIndicesBuffer);
                    glIndexPointer(GL_INT, 0, NULL);
                    DRAW_INDICES(GL_LINES, m_points.size(), m_hoveredLineIndices);
                }
            } else {
                glColor4f(0.0f, 0.0f, 0.0f, 0.25f);
                DRAW_INDICES(GL_LINES, m_points.size(), m_lineIndices);
            }
        }
    }

    if ((flags & WIREFRAME_DRAW_OUTLINE) && !(flags & WIREFRAME_HIGHLIGHT) && !m_outlineIndices.empty()) {
        glDisableClientState(GL_COLOR_ARRAY);
        glLineWidth(2.0f);
        if (flags & WIREFRAME_TEXTURE_SPACE)
            glBindBuffer(GL_ARRAY_BUFFER, m_textureOutlinePointsBuffer);
        else
            glBindBuffer(GL_ARRAY_BUFFER, m_outlinePointsBuffer);
        glVertexPointer(2, GL_FLOAT, 0, NULL);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_outlineIndicesBuffer);
        glIndexPointer(GL_INT, 0, NULL);
        glColor4fv((Color(0.5f, 0.5f, 0.5f, 1.0f) * color).m_elements);
        if (flags & WIREFRAME_TEXTURE_SPACE)
            DRAW_INDICES(GL_LINES, m_textureOutlinePoints.size(), m_outlineIndices);
        else
            DRAW_INDICES(GL_LINES, m_outlinePoints.size(), m_outlineIndices);
        glColor4fv(color.m_elements);
        glLineWidth(1.0f);
        if (flags & WIREFRAME_TEXTURE_SPACE)
            DRAW_INDICES(GL_LINES, m_textureOutlinePoints.size(), m_outlineIndices);
        else
            DRAW_INDICES(GL_LINES, m_outlinePoints.size(), m_outlineIndices);
    }
    glLineWidth(1.0f);

    if (flags & WIREFRAME_DRAW_POINTS) {
        if ((flags & WIREFRAME_EXTRUSION_COLORS) && !m_extrudedPoints.empty()) {
            glBindBuffer(GL_ARRAY_BUFFER, m_extrudedPointsBuffer);
            glVertexPointer(2, GL_FLOAT, 0, NULL);
            glDisableClientState(GL_COLOR_ARRAY);
            glColor3f(0.0f, 0.0f, 0.0f);
            glPointSize(9.0f);
            glDrawArrays(GL_POINTS, 0, m_extrudedPoints.size());
            if (!m_pointColors.empty()) {
                glPointSize(7.0f);
                glEnableClientState(GL_COLOR_ARRAY);
                glBindBuffer(GL_ARRAY_BUFFER, m_pointColorsBuffer);
                glColorPointer(4, GL_FLOAT, 0, NULL);
                glDrawArrays(GL_POINTS, 0, m_extrudedPoints.size());
            }
            glDisableClientState(GL_COLOR_ARRAY);
            glColor3f(0.0f, 0.0f, 0.0f);
            glPointSize(5.0f);
            glDrawArrays(GL_POINTS, 0, m_extrudedPoints.size());
            if (!m_backColors.empty()) {
                glPointSize(3.0f);
                glEnableClientState(GL_COLOR_ARRAY);
                glBindBuffer(GL_ARRAY_BUFFER, m_backColorsBuffer);
                glColorPointer(4, GL_FLOAT, 0, NULL);
                glDrawArrays(GL_POINTS, 0, m_extrudedPoints.size());
            }
        }
        if (flags & WIREFRAME_TEXTURE_SPACE)
            glBindBuffer(GL_ARRAY_BUFFER, m_texturePointsBuffer);
        else
            glBindBuffer(GL_ARRAY_BUFFER, m_pointsBuffer);
        glVertexPointer(2, GL_FLOAT, 0, NULL);
        glDisableClientState(GL_COLOR_ARRAY);
        glColor3f(0.0f, 0.0f, 0.0f);
        glPointSize(9.0f);
        glDrawArrays(GL_POINTS, 0, m_points.size());
        if (!m_pointColors.empty()) {
            glPointSize(7.0f);
            glEnableClientState(GL_COLOR_ARRAY);
            glBindBuffer(GL_ARRAY_BUFFER, m_pointColorsBuffer);
            glColorPointer(4, GL_FLOAT, 0, NULL);
            glDrawArrays(GL_POINTS, 0, m_points.size());
        }
        glDisableClientState(GL_COLOR_ARRAY);
        glColor3f(0.0f, 0.0f, 0.0f);
        glPointSize(5.0f);
        glDrawArrays(GL_POINTS, 0, m_points.size());
        if (flags & WIREFRAME_STROKE_COLORS) {
            if (!m_strokeColors.empty()) {
                glPointSize(3.0f);
                glEnableClientState(GL_COLOR_ARRAY);
                glBindBuffer(GL_ARRAY_BUFFER, m_strokeColorsBuffer);
                glColorPointer(4, GL_FLOAT, 0, NULL);
                glDrawArrays(GL_POINTS, 0, m_points.size());
            }
        } else if (flags & WIREFRAME_EXTRUSION_COLORS) {
            if (!m_frontColors.empty()) {
                glPointSize(3.0f);
                glEnableClientState(GL_COLOR_ARRAY);
                glBindBuffer(GL_ARRAY_BUFFER, m_frontColorsBuffer);
                glColorPointer(4, GL_FLOAT, 0, NULL);
                glDrawArrays(GL_POINTS, 0, m_points.size());
            }
        } else {
            if (!m_fillColors.empty()) {
                glPointSize(3.0f);
                glEnableClientState(GL_COLOR_ARRAY);
                glBindBuffer(GL_ARRAY_BUFFER, m_fillColorsBuffer);
                glColorPointer(4, GL_FLOAT, 0, NULL);
                glDrawArrays(GL_POINTS, 0, m_points.size());
            }
        }
    }
}
