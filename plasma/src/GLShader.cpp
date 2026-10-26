#include <stdio.h>
#include <stdlib.h>
#include "plasma/GLShader.h"

// Prints a shader or program object's info log (compiler messages) to stdout.
static void printInfoLog(GLhandleARB object)
{
    GLint length = 0;
    GLsizei written = 0;
    glGetObjectParameterivARB(object, GL_OBJECT_INFO_LOG_LENGTH_ARB, &length);
    if (length > 0) {
        GLcharARB *log = (GLcharARB *)malloc(length);
        glGetInfoLogARB(object, length, &written, log);
        printf("%s\n", log);
        free(log);
    }
}

namespace plasma {

GLShader::GLShader(const std::string &vertexSource, const std::string &fragmentSource)
    : m_program(0),
      m_vertexShader(0),
      m_fragmentShader(0)
{
    if (vertexSource.size()) {
        // GLSL 1.20 defines the numeric conversions these legacy shaders use.
        const std::string versioned = "#version 120\n" + vertexSource;
        const GLchar *source = versioned.c_str();
        m_vertexShader = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(m_vertexShader, 1, &source, NULL);
        glCompileShader(m_vertexShader);
        printInfoLog(m_vertexShader);
    }
    if (fragmentSource.size()) {
        const std::string versioned = "#version 120\n" + fragmentSource;
        const GLchar *source = versioned.c_str();
        m_fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(m_fragmentShader, 1, &source, NULL);
        glCompileShader(m_fragmentShader);
        printInfoLog(m_fragmentShader);
    }
    m_program = glCreateProgram();
    if (m_vertexShader)
        glAttachShader(m_program, m_vertexShader);
    if (m_fragmentShader)
        glAttachShader(m_program, m_fragmentShader);
    glLinkProgram(m_program);
}

GLShader::~GLShader()
{
    if (m_program)
        glDeleteProgram(m_program);
    if (m_vertexShader)
        glDeleteShader(m_vertexShader);
    if (m_fragmentShader)
        glDeleteShader(m_fragmentShader);
}

}
