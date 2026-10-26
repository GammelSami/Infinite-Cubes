// plasma::GLShader: one GLSL program linked from an optional vertex and an optional fragment shader (GLEngine
// creates the engine's eight shaders with it).
#pragma once
#include <string>
#include <GL/glew.h>

namespace plasma {

struct GLShader {
    // Compiles each non-empty source (printing its info log) and links them.
    GLShader(const std::string &vertexSource, const std::string &fragmentSource);
    virtual ~GLShader();

    // Makes this program current unless it already is.
    void use() const
    {
        GLuint program = m_program;
        GLint current;
        glGetIntegerv(GL_CURRENT_PROGRAM, &current);
        if (program != current)
            glUseProgram(program);
    }

    GLuint m_program;
    GLuint m_vertexShader;   // 0 without a vertex source.
    GLuint m_fragmentShader; // 0 without a fragment source.
};

}
