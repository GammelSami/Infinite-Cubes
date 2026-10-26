// plasma::GLDrawing: a drawing whose arrays live in GL buffer objects, drawn with the engine's shape shader.
#pragma once
#include "plasma/Drawing.h"
#include <GL/glew.h>

namespace plasma {

class GLEngine;

class GLDrawing : public Drawing {
public:
    explicit GLDrawing(GLEngine *engine);
    virtual ~GLDrawing();

    virtual void update();
    virtual void draw(Node *node, const Vector<4, float> &color);

private:
    GLEngine *m_engine;

    // Created on the first Update: vertices, colours, normals, texture coordinates, indices.
    GLuint m_vertexBuffer;
    GLuint m_colorBuffer;
    GLuint m_normalBuffer;
    GLuint m_textureCoordinateBuffer;
    GLuint m_indexBuffer;
    GLsizei m_indexCount;
    GLuint m_vertexCount;
};

}
