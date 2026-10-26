// WireFrame keeps a mesh's 2D outline in point, colour and index arrays, mirrored in OpenGL buffers. Draw renders
// points, lines and triangles; MeshInterface fills the arrays and chooses the drawing flags.
#pragma once
#include <GL/glew.h>
#include <vector>
#include "plasma/Vector.h"

// Draw flags (chosen by MeshInterface::Render).
enum {
    WIREFRAME_DRAW_POINTS = 0x01,      // The vertex tools: the vertex dots.
    WIREFRAME_HIGHLIGHT = 0x02,        // The edge tool: unselected, selected and hovered edges apart.
    WIREFRAME_DRAW_TRIANGLES = 0x04,   // The face tool: the face triangles.
    WIREFRAME_TEXTURE_SPACE = 0x08,    // The texture positions instead of the sheet positions.
    WIREFRAME_EXTRUSION_COLORS = 0x10, // Colour mode extrusion: the extruded points, front and back colours.
    WIREFRAME_DRAW_LINES = 0x40,       // The edges (a selected or snapable node).
    WIREFRAME_DRAW_OUTLINE = 0x80,     // The subdivided contours.
    WIREFRAME_STROKE_COLORS = 0x100,   // Colour mode stroke: the vertex dots in the stroke colours.
};

class WireFrame {
public:
    typedef plasma::Vector<2, float> Point;
    typedef plasma::Vector<4, float> Color;

    WireFrame();
    virtual ~WireFrame();

    // Deletes the buffer objects and empties every array.
    void clear();

    // Creates the buffer objects on first use and uploads every array.
    void upload();

    // Uploads the arrays that change while editing.
    void uploadEditArrays();
    void draw(unsigned int flags, const Color &color);

    std::vector<Point> m_points;         // The vertices.
    std::vector<Point> m_texturePoints;  // The vertices' texture positions (WIREFRAME_TEXTURE_SPACE).
    std::vector<Point> m_extrudedPoints; // The vertices through the shape's extrusion matrix.
    std::vector<Point> m_outlinePoints;
    std::vector<Point> m_textureOutlinePoints; // The contours' texture positions.
    std::vector<Color> m_pointColors;          // Per vertex: selected, hovered or its vertex type.
    std::vector<Color> m_fillColors;           // The shape's fill colours (MeshShape::m_vertexColors), opaque.
    std::vector<Color> m_strokeColors;         // Its stroke colours (m_strokeColors), opaque.
    std::vector<Color> m_frontColors;          // Its front extrusion colours (m_extrusionFrontColors), opaque.
    std::vector<Color> m_backColors;           // Its back extrusion colours (m_extrusionBackColors), opaque.

    // Cleared and uploaded only: nothing in the tree fills or draws them.
    std::vector<unsigned int> m_unusedIndices0;
    std::vector<unsigned int> m_unusedIndices1;
    std::vector<unsigned int> m_unusedIndices2;
    std::vector<unsigned int> m_lineIndices; // Every edge (boundary edges twice).
    std::vector<unsigned int> m_unselectedLineIndices;
    std::vector<unsigned int> m_selectedLineIndices;
    std::vector<unsigned int> m_hoveredLineIndices; // Within the tool radius (the edge tool).
    std::vector<unsigned int> m_triangleIndices;    // Every face as a triangle fan.
    std::vector<unsigned int> m_unselectedTriangleIndices;
    std::vector<unsigned int> m_selectedTriangleIndices;
    std::vector<unsigned int> m_hoveredTriangleIndices; // Under the pointer (the face tool).
    std::vector<unsigned int> m_outlineIndices;

    // One buffer object per array, in the same order; 0 until Upload.
    GLuint m_pointsBuffer;
    GLuint m_texturePointsBuffer;
    GLuint m_extrudedPointsBuffer;
    GLuint m_outlinePointsBuffer;
    GLuint m_textureOutlinePointsBuffer;
    GLuint m_pointColorsBuffer;
    GLuint m_fillColorsBuffer;
    GLuint m_strokeColorsBuffer;
    GLuint m_frontColorsBuffer;
    GLuint m_backColorsBuffer;
    GLuint m_unusedIndices0Buffer;
    GLuint m_unusedIndices1Buffer;
    GLuint m_unusedIndices2Buffer;
    GLuint m_lineIndicesBuffer;
    GLuint m_unselectedLineIndicesBuffer;
    GLuint m_selectedLineIndicesBuffer;
    GLuint m_hoveredLineIndicesBuffer;
    GLuint m_triangleIndicesBuffer;
    GLuint m_unselectedTriangleIndicesBuffer;
    GLuint m_selectedTriangleIndicesBuffer;
    GLuint m_hoveredTriangleIndicesBuffer;
    GLuint m_outlineIndicesBuffer;
};
