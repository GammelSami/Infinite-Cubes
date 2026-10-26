#include <iostream>
#include "plasma/GLEngine.h"
#include "plasma/Node.h"
#include "plasma/Display.h"
#include "plasma/GLTexture.h"
#include "plasma/GLDrawing.h"
#include "plasma/GLRenderSurface.h"
#include "plasma/Widget.h"

namespace plasma {

// The depth layer of the next drawing: BeginWindowDrawing resets it to 1 here. GLDrawing::Draw advances it by two
// layers per drawing.
int g_drawingLayer;

GLEngine::GLEngine()
    : m_frameBuffer(0),
      m_colorMaterial(false),
      m_alphaShader(0),
      m_downsample2Shader(0),
      m_downsample3Shader(0),
      m_downsample4Shader(0),
      m_verticalBlurShader(0),
      m_horizontalBlurShader(0),
      m_shapeShader(0),
      m_unused234(0),
      m_filter(0),
      m_textureEnabled(false),
      m_textureOpacity(1.0f),
      m_textureBrightness(1.0f),
      m_textureContrast(1.0f),
      m_textureSaturation(1.0f),
      m_widgetDeformationEnabled(false)
{
    m_texture = 0;
    m_unused1FC = 0;
    m_unused200 = 0;
}

GLEngine::~GLEngine()
{
    delete m_frameBuffer;
    if (m_texture)
        glDeleteTextures(1, &m_texture);
    delete m_shapeShader;
    delete m_alphaShader;
    delete m_downsample2Shader;
    delete m_downsample3Shader;
    delete m_downsample4Shader;
    delete m_verticalBlurShader;
    delete m_horizontalBlurShader;
}

void GLEngine::setupShadersAndFrameBuffer()
{
    createShaders();
    m_frameBuffer = new GLFrameBufferObject;
    glGenTextures(1, &m_texture);
}

void GLEngine::createShaders()
{
    // The shape shader: texturing (opacity, brightness, contrast, saturation), the filter texture and the widget
    // deformation.
    m_shapeShader = new GLShader(
        std::string("uniform bool widgetDeformationEnabled;uniform mat4 widgetBindMatrix;"
                    "uniform mat4 inverseWidgetBindMatrix;uniform vec2 widgetBindPos;"
                    "uniform vec2 widgetBindSize;uniform vec2 deformedWidgetPos;"
                    "uniform vec2 deformedWidgetSize;uniform float aaOffset;vec4 deformedPosition() {"
                    "\tvec4 v = gl_Vertex;\tif (widgetDeformationEnabled) {\t\tvec4 p = widgetBindMatrix * v;"
                    "\t\tvec2 w = vec2(0, 0);\t\tif (p.x < widgetBindPos.x) {\t\t\tw.x = 0;\t\t}"
                    "\t\telse if (p.x < widgetBindPos.x + widgetBindSize.x) {"
                    "\t\t\tw.x = (p.x - widgetBindPos.x) / widgetBindSize.x;\t\t}\t\telse {\t\t\tw.x = 1;\t\t}"
                    "\t\tif (p.y < widgetBindPos.y) {\t\t\tw.y = 0;\t\t}"
                    "\t\telse if (p.y < widgetBindPos.y + widgetBindSize.y) {"
                    "\t\t\tw.y = (p.y - widgetBindPos.y) / widgetBindSize.y;\t\t}\t\telse {\t\t\tw.y = 1;\t\t}"
                    "\t\tvec2 q = p.xy + deformedWidgetPos - widgetBindPos + w * (deformedWidgetSize - widgetBindSize);"
                    "\t\tv = inverseWidgetBindMatrix * vec4(q.x, q.y, p.z, p.w);\t}"
                    "\tif (dot(gl_MultiTexCoord2.xy, gl_MultiTexCoord2.xy) > 0 && dot(gl_MultiTexCoord2.zw, "
                    "gl_MultiTexCoord2.zw) > 0) {"
                    "\t\tvec3 n1 = gl_NormalMatrix * vec3(gl_MultiTexCoord2.x, gl_MultiTexCoord2.y, 0);"
                    "\t\tvec3 n2 = gl_NormalMatrix * vec3(gl_MultiTexCoord2.z, gl_MultiTexCoord2.w, 0);"
                    "\t\tn1.z = 0; n2.z = 0;\t\tvec4 p = gl_ModelViewMatrix * v;"
                    "\t\tif (dot(n1, n1) > 0 && dot(n2, n2) > 0) {\t\t\tn1 = normalize(n1);"
                    "\t\t\tn2 = normalize(n2);\t\t\tvec3 n = normalize(n1 + n2);"
                    "\t\t\tfloat s = max(0.7, dot(n1, n));\t\t\tn *= aaOffset / s;\t\t\tp.xyz += n;"
                    "\t\t\tp.z += 0.5;\t\t\treturn p;\t\t}\t}\treturn gl_ModelViewMatrix * v;}void main() {"
                    "\tgl_FrontColor = gl_Color * gl_FrontMaterial.ambient;"
                    "\tgl_TexCoord[0] = gl_TextureMatrix[0] * gl_MultiTexCoord0;\tvec4 p = deformedPosition();"
                    "\tgl_TexCoord[1] = p;\tgl_Position = gl_ProjectionMatrix * p;}"),
        std::string(
            "uniform sampler2D shapeTexture;uniform sampler2DRect filterTexture;"
            "uniform bool textureEnabled;uniform int filter;uniform float textureOpacity;"
            "uniform float textureBrightness;uniform float textureContrast;"
            "uniform float textureSaturation;void main() {\tvec4 shapeColor;\tif (textureEnabled) {"
            "\t\tvec4 texColor = texture2D(shapeTexture, gl_TexCoord[0].st);"
            "\t\tif (textureSaturation != 1.0) {\t\t\tvec3 weights = vec3(0.3, 0.59, 0.11);"
            "\t\t\tvec3 gray = vec3(dot(weights, texColor.xyz));"
            "\t\t\ttexColor.xyz = mix(texColor.xyz, gray, 1 - textureSaturation);\t\t}"
            "\t\tif (textureBrightness != 1.0) texColor.xyz *= textureBrightness;"
            "\t\tif (textureContrast != 1.0) texColor.xyz = mix(texColor.xyz, vec3(0.5,0.5,0.5), 1 - textureContrast);"
            "\t\ttexColor.xyz = min(texColor.xyz, vec3(1,1,1));"
            "\t\tshapeColor = mix(gl_Color, gl_Color * texColor, textureOpacity);\t}\telse {"
            "\t\tshapeColor = gl_Color;\t}\tif (filter == 0)\t\tgl_FragColor = shapeColor;\telse {"
            "\t\tvec4 c = texture2DRect(filterTexture, gl_TexCoord[1].st);"
            "\t\tif (c.a > 0) c.xyz *= 1.0 / c.a;\t\tif (filter == 1) {"
            "\t\t\tgl_FragColor = shapeColor * (1 - c.a) + vec4(c.r, c.g, c.b, shapeColor.a) * c.a;"
            "\t\t}\t\telse if (filter == 2) {\t\t\tgl_FragColor = c * shapeColor;\t\t}\t\telse {"
            "\t\t\tgl_FragColor = vec4(shapeColor.r, shapeColor.g, shapeColor.b, c.a * shapeColor.a);"
            "\t\t}\t}}"));
    glUseProgram(m_shapeShader->m_program);

    // The uniform lookup is spelled shapeTexure, while the shader declares shapeTexture.
    glUniform1i(glGetUniformLocation(m_shapeShader->m_program, "shapeTexure"), 0);
    glUniform1i(glGetUniformLocation(m_shapeShader->m_program, "filterTexture"), 1);
    m_aaOffsetLocation = glGetUniformLocation(m_shapeShader->m_program, "aaOffset");
    m_widgetDeformationEnabledLocation = glGetUniformLocation(m_shapeShader->m_program, "widgetDeformationEnabled");
    m_widgetBindMatrixLocation = glGetUniformLocation(m_shapeShader->m_program, "widgetBindMatrix");
    m_inverseWidgetBindMatrixLocation = glGetUniformLocation(m_shapeShader->m_program, "inverseWidgetBindMatrix");
    m_widgetBindPosLocation = glGetUniformLocation(m_shapeShader->m_program, "widgetBindPos");
    m_widgetBindSizeLocation = glGetUniformLocation(m_shapeShader->m_program, "widgetBindSize");
    m_deformedWidgetPosLocation = glGetUniformLocation(m_shapeShader->m_program, "deformedWidgetPos");
    m_deformedWidgetSizeLocation = glGetUniformLocation(m_shapeShader->m_program, "deformedWidgetSize");
    m_textureEnabledLocation = glGetUniformLocation(m_shapeShader->m_program, "textureEnabled");
    m_filterLocation = glGetUniformLocation(m_shapeShader->m_program, "filter");
    m_textureOpacityLocation = glGetUniformLocation(m_shapeShader->m_program, "textureOpacity");
    m_textureBrightnessLocation = glGetUniformLocation(m_shapeShader->m_program, "textureBrightness");
    m_textureContrastLocation = glGetUniformLocation(m_shapeShader->m_program, "textureContrast");
    m_textureSaturationLocation = glGetUniformLocation(m_shapeShader->m_program, "textureSaturation");
    glUniform1f(m_aaOffsetLocation, 0.7f);
    glUniform1i(m_widgetDeformationEnabledLocation, 0);
    glUniform1i(m_textureEnabledLocation, 0);
    glUniform1i(m_filterLocation, 0);
    glUniform1f(m_textureOpacityLocation, 1.0f);
    glUniform1f(m_textureBrightnessLocation, 1.0f);
    glUniform1f(m_textureContrastLocation, 1.0f);
    glUniform1f(m_textureSaturationLocation, 1.0f);

    // Divides the rectangle texture by alpha, times the colour.
    m_alphaShader = new GLShader(
        std::string("void main() {\tgl_FrontColor = gl_Color;\tvec4 p = gl_Vertex;"
                    "\tgl_Position = gl_ModelViewProjectionMatrix * p;\tgl_TexCoord[0] = gl_MultiTexCoord0;}"),
        std::string("#extension GL_ARB_texture_rectangle : enable\nuniform sampler2DRect tex;void main() {"
                    "\tvec4 c = texture2DRect(tex, gl_TexCoord[0].st);\tif (c.a > 0) c.xyz *= 1.0 / c.a;"
                    "\tgl_FragColor = c * gl_Color;}"));
    m_alphaShader->use();
    glUniform1i(glGetUniformLocation(m_alphaShader->m_program, "tex"), 0);

    // Averages 2 x 2 pixels.
    m_downsample2Shader = new GLShader(
        std::string("void main() {\tvec4 p = gl_Vertex;\tgl_Position = gl_ModelViewProjectionMatrix * p;"
                    "\tgl_TexCoord[0] = gl_MultiTexCoord0;}"),
        std::string(
            "#extension GL_ARB_texture_rectangle : enable\nuniform sampler2DRect tex;void main() {"
            "\tvec2 t = 2.0 * floor(gl_TexCoord[0].st);"
            "\tvec4 c = 0.25 * (texture2DRect(tex, t) +\t\t\t\t\t texture2DRect(tex, vec2(t.x + 1, t.y)) +\t\t\t\t\t "
            "texture2DRect(tex, vec2(t.x, t.y + 1)) +\t\t\t\t\t texture2DRect(tex, vec2(t.x + 1, t.y + 1)));"
            "\tif (c.a > 0) c.xyz *= 1.0 / c.a;\tgl_FragColor = c;}"));
    m_downsample2Shader->use();
    glUniform1i(glGetUniformLocation(m_downsample2Shader->m_program, "tex"), 0);

    // Averages 3 x 3 pixels.
    m_downsample3Shader = new GLShader(
        std::string("void main() {\tvec4 p = gl_Vertex;\tgl_Position = gl_ModelViewProjectionMatrix * p;"
                    "\tgl_TexCoord[0] = gl_MultiTexCoord0;}"),
        std::string("#extension GL_ARB_texture_rectangle : enable\nuniform sampler2DRect tex;void main() {"
                    "\tvec2 t = 3.0 * floor(gl_TexCoord[0].st);"
                    "\tvec4 c = 1/9.0 * (texture2DRect(tex, vec2(t.x + 0, t.y + 0)) +\t\t\t\t\t  texture2DRect(tex, "
                    "vec2(t.x + 1, t.y + 0)) +\t\t\t\t\t  texture2DRect(tex, vec2(t.x + 2, t.y + 0)) +\t\t\t\t\t  "
                    "texture2DRect(tex, vec2(t.x + 0, t.y + 1)) +\t\t\t\t\t  texture2DRect(tex, vec2(t.x + 1, t.y + "
                    "1)) +\t\t\t\t\t  texture2DRect(tex, vec2(t.x + 2, t.y + 1)) +\t\t\t\t\t  texture2DRect(tex, "
                    "vec2(t.x + 0, t.y + 2)) +\t\t\t\t\t  texture2DRect(tex, vec2(t.x + 1, t.y + 2)) +\t\t\t\t\t  "
                    "texture2DRect(tex, vec2(t.x + 2, t.y + 2)));"
                    "\tif (c.a > 0) c.xyz *= 1.0 / c.a;\tgl_FragColor = c;}"));
    m_downsample3Shader->use();
    glUniform1i(glGetUniformLocation(m_downsample3Shader->m_program, "tex"), 0);

    // Averages 4 x 4 pixels.
    m_downsample4Shader = new GLShader(
        std::string("void main() {\tvec4 p = gl_Vertex;\tgl_Position = gl_ModelViewProjectionMatrix * p;"
                    "\tgl_TexCoord[0] = gl_MultiTexCoord0;}"),
        std::string("#extension GL_ARB_texture_rectangle : enable\nuniform sampler2DRect tex;void main() {"
                    "\tvec2 t = 4.0 * floor(gl_TexCoord[0].st);"
                    "\tvec4 c = 1/16.0 * (texture2DRect(tex, vec2(t.x + 0, t.y + 0)) +\t\t\t\t\t   texture2DRect(tex, "
                    "vec2(t.x + 1, t.y + 0)) +\t\t\t\t\t   texture2DRect(tex, vec2(t.x + 2, t.y + 0)) +\t\t\t\t\t   "
                    "texture2DRect(tex, vec2(t.x + 3, t.y + 0)) +\t\t\t\t\t   texture2DRect(tex, vec2(t.x + 0, t.y + "
                    "1)) +\t\t\t\t\t   texture2DRect(tex, vec2(t.x + 1, t.y + 1)) +\t\t\t\t\t   texture2DRect(tex, "
                    "vec2(t.x + 2, t.y + 1)) +\t\t\t\t\t   texture2DRect(tex, vec2(t.x + 3, t.y + 1)) +\t\t\t\t\t   "
                    "texture2DRect(tex, vec2(t.x + 0, t.y + 2)) +\t\t\t\t\t   texture2DRect(tex, vec2(t.x + 1, t.y + "
                    "2)) +\t\t\t\t\t   texture2DRect(tex, vec2(t.x + 2, t.y + 2)) +\t\t\t\t\t   texture2DRect(tex, "
                    "vec2(t.x + 3, t.y + 2)) +\t\t\t\t\t   texture2DRect(tex, vec2(t.x + 0, t.y + 3)) +\t\t\t\t\t   "
                    "texture2DRect(tex, vec2(t.x + 1, t.y + 3)) +\t\t\t\t\t   texture2DRect(tex, vec2(t.x + 2, t.y + "
                    "3)) +\t\t\t\t\t   texture2DRect(tex, vec2(t.x + 3, t.y + 3)));"
                    "\tif (c.a > 0) c.xyz *= 1.0 / c.a;\tgl_FragColor = c;}"));
    m_downsample4Shader->use();
    glUniform1i(glGetUniformLocation(m_downsample4Shader->m_program, "tex"), 0);

    // A 9-tap vertical blur (the radius in the third texture coordinate).
    m_verticalBlurShader = new GLShader(
        std::string("void main() {\tgl_Position = gl_ProjectionMatrix * gl_Vertex;"
                    "\tgl_TexCoord[0] = gl_MultiTexCoord0;}"),
        std::string("#extension GL_ARB_texture_rectangle : enable\nuniform sampler2DRect tex;void main() {"
                    "   float blurSize = gl_TexCoord[0].z;\tvec4 sum = vec4(0,0,0,0);"
                    "\tsum += texture2DRect(tex, vec2(gl_TexCoord[0].x, gl_TexCoord[0].y - 4.0*blurSize)) * 0.05;"
                    "\tsum += texture2DRect(tex, vec2(gl_TexCoord[0].x, gl_TexCoord[0].y - 3.0*blurSize)) * 0.09;"
                    "\tsum += texture2DRect(tex, vec2(gl_TexCoord[0].x, gl_TexCoord[0].y - 2.0*blurSize)) * 0.12;"
                    "\tsum += texture2DRect(tex, vec2(gl_TexCoord[0].x, gl_TexCoord[0].y - blurSize)) * 0.15;"
                    "\tsum += texture2DRect(tex, vec2(gl_TexCoord[0].x, gl_TexCoord[0].y)) * 0.18;"
                    "\tsum += texture2DRect(tex, vec2(gl_TexCoord[0].x, gl_TexCoord[0].y + blurSize)) * 0.15;"
                    "\tsum += texture2DRect(tex, vec2(gl_TexCoord[0].x, gl_TexCoord[0].y + 2.0*blurSize)) * 0.12;"
                    "\tsum += texture2DRect(tex, vec2(gl_TexCoord[0].x, gl_TexCoord[0].y + 3.0*blurSize)) * 0.09;"
                    "\tsum += texture2DRect(tex, vec2(gl_TexCoord[0].x, gl_TexCoord[0].y + 4.0*blurSize)) * 0.05;"
                    "\tgl_FragColor = sum;}"));
    m_verticalBlurShader->use();
    glUniform1i(glGetUniformLocation(m_verticalBlurShader->m_program, "tex"), 0);

    // A 9-tap horizontal blur.
    m_horizontalBlurShader = new GLShader(
        std::string("void main() {\tgl_Position = gl_ProjectionMatrix * gl_Vertex;"
                    "\tgl_TexCoord[0] = gl_MultiTexCoord0;}"),
        std::string("#extension GL_ARB_texture_rectangle : enable\nuniform sampler2DRect tex;void main() {"
                    "   float blurSize = gl_TexCoord[0].z;\tvec4 sum = vec4(0,0,0,0);"
                    "\tsum += texture2DRect(tex, vec2(gl_TexCoord[0].x - 4.0*blurSize, gl_TexCoord[0].y)) * 0.05;"
                    "\tsum += texture2DRect(tex, vec2(gl_TexCoord[0].x - 3.0*blurSize, gl_TexCoord[0].y)) * 0.09;"
                    "\tsum += texture2DRect(tex, vec2(gl_TexCoord[0].x - 2.0*blurSize, gl_TexCoord[0].y)) * 0.12;"
                    "\tsum += texture2DRect(tex, vec2(gl_TexCoord[0].x - blurSize, gl_TexCoord[0].y)) * 0.15;"
                    "\tsum += texture2DRect(tex, vec2(gl_TexCoord[0].x, gl_TexCoord[0].y)) * 0.18;"
                    "\tsum += texture2DRect(tex, vec2(gl_TexCoord[0].x + blurSize, gl_TexCoord[0].y)) * 0.15;"
                    "\tsum += texture2DRect(tex, vec2(gl_TexCoord[0].x + 2.0*blurSize, gl_TexCoord[0].y)) * 0.12;"
                    "\tsum += texture2DRect(tex, vec2(gl_TexCoord[0].x + 3.0*blurSize, gl_TexCoord[0].y)) * 0.09;"
                    "\tsum += texture2DRect(tex, vec2(gl_TexCoord[0].x + 4.0*blurSize, gl_TexCoord[0].y)) * 0.05;"
                    "\tgl_FragColor = sum;}"));
    m_horizontalBlurShader->use();
    glUniform1i(glGetUniformLocation(m_horizontalBlurShader->m_program, "tex"), 0);

    GLint current;
    glGetIntegerv(GL_CURRENT_PROGRAM, &current);
    if (current)
        glUseProgram(0);
}

Drawing *GLEngine::createDrawing()
{
    return new GLDrawing(this);
}

RenderSurface *GLEngine::createRenderSurface(int)
{
    return new GLRenderSurface(this);
}

Texture *GLEngine::createTexture(int width, int height, const unsigned char *pixels, const TextureFormat &format,
                                 const std::wstring &name, bool transient)
{
    return new GLTexture(this, width, height, pixels, format, name, transient);
}

void GLEngine::pushNodeMatrix(Node *node)
{
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glMultMatrixf(node->m_transformation->m_matrix.m_elements);
}

void GLEngine::popNodeMatrix(Node *)
{
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
}

void GLEngine::setShapeShader(Node *node, bool textured, float opacity, float brightness, float contrast,
                              float saturation)
{
    m_shapeShader->use();
    if (m_renderFlags & (1 << RENDER_ANTIALIAS))
        glUniform1f(m_aaOffsetLocation, 0.7f);
    else
        glUniform1f(m_aaOffsetLocation, 0.0f);
    Shape *shape;
    if (node && node->m_enclosingWidget && (shape = node->m_shape) && shape->isDeformable() &&
        node->m_enclosingWidget->getClampedVisibleExtent() != node->m_enclosingWidget->getInnerBindSize()) {
        Matrix<float> bindMatrix =
            node->m_enclosingWidget->m_bindMatrix * node->m_enclosingWidget->m_node->m_inverseMatrix * node->m_matrix;
        Matrix<float> inverseBindMatrix = node->m_inverseMatrix * node->m_enclosingWidget->m_node->m_matrix *
                                          node->m_enclosingWidget->m_inverseBindMatrix;
        glUniformMatrix4fv(m_widgetBindMatrixLocation, 1, GL_FALSE, bindMatrix.m_elements);
        glUniformMatrix4fv(m_inverseWidgetBindMatrixLocation, 1, GL_FALSE, inverseBindMatrix.m_elements);
        if (!m_widgetDeformationEnabled) {
            glUniform1i(m_widgetDeformationEnabledLocation, 1);
            m_widgetDeformationEnabled = true;
        }
        glUniform2fv(m_widgetBindPosLocation, 1, node->m_enclosingWidget->getInnerBindPos().m_elements);
        glUniform2fv(m_widgetBindSizeLocation, 1, node->m_enclosingWidget->getInnerBindSize().m_elements);
        glUniform2fv(m_deformedWidgetPosLocation, 1, node->m_enclosingWidget->getScrollPosition().m_elements);
        glUniform2fv(m_deformedWidgetSizeLocation, 1, node->m_enclosingWidget->getClampedVisibleExtent().m_elements);
    } else if (m_widgetDeformationEnabled) {
        glUniform1i(m_widgetDeformationEnabledLocation, 0);
        m_widgetDeformationEnabled = false;
    }
    if (textured) {
        if (!m_textureEnabled) {
            glUniform1i(m_textureEnabledLocation, 1);
            m_textureEnabled = true;
        }
        if (m_textureOpacity != opacity) {
            glUniform1f(m_textureOpacityLocation, opacity);
            m_textureOpacity = opacity;
        }
        if (m_textureBrightness != brightness) {
            glUniform1f(m_textureBrightnessLocation, brightness);
            m_textureBrightness = brightness;
        }
        if (m_textureContrast != contrast) {
            glUniform1f(m_textureContrastLocation, contrast);
            m_textureContrast = contrast;
        }
        if (m_textureSaturation != saturation) {
            glUniform1f(m_textureSaturationLocation, saturation);
            m_textureSaturation = saturation;
        }
    } else if (m_textureEnabled) {
        glUniform1i(m_textureEnabledLocation, 0);
        m_textureEnabled = false;
    }
    int filter = 0;
    if (node && node->m_display->m_clipping.value() != 0) {
        const unsigned flags = node->m_display->m_flags;
        filter = flags & (1 << DISPLAY_MODULATION) ? 2 : (flags & (1 << DISPLAY_SCREEN) | 2) >> 1;
    }
    if (m_filter != filter) {
        glUniform1i(m_filterLocation, filter);
        m_filter = filter;
    }
}

void GLEngine::beginTextDrawing(Node *node)
{
    const bool textured =
        node && m_filterSurface &&
        (!(node->m_display->m_flags & (1 << DISPLAY_BLUR)) || node->m_display->m_clipping.value() != 0);
    if (textured) {
        glActiveTexture(GL_TEXTURE1);
        m_filterSurface->bindTexture();
    }
    glActiveTexture(GL_TEXTURE0);
    glClientActiveTexture(GL_TEXTURE0);
    if (textured)
        setShapeShader(node, true, 1.0f, 1.0f, 1.0f, 1.0f);
    else
        useFixedFunction();
    glMatrixMode(GL_TEXTURE);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    GLfloat ambient[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    glMaterialfv(GL_FRONT, GL_AMBIENT, ambient);
    glDisable(GL_DEPTH_TEST);
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_INDEX_ARRAY);
}

void GLEngine::beginSurface(RenderSurface *surface, const Vector<2, float> &position, const Vector<2, float> &size,
                            float scale, bool alignPixels)
{
    pushSurface(surface);
    surface->begin();
    glPushAttrib(GL_VIEWPORT_BIT | GL_SCISSOR_BIT);
    glViewport(0, 0, static_cast<int>(size.m_elements[0]), static_cast<int>(size.m_elements[1]));
    glDisable(GL_SCISSOR_TEST);
    Matrix<float> modelView;
    getModelViewMatrix(modelView);
    if (alignPixels) {
        Vector<2, float> fraction = Vector<2, float>(modelView.m_elements[12], modelView.m_elements[13]) * scale;
        Vector<2, int> whole(fraction);
        Vector<2, float> wholePixels(whole);
        fraction -= wholePixels;
        if (scale == 1.0f)
            fraction = Vector<2, float>(0.0f, 0.0f);
        glMatrixMode(GL_PROJECTION);
        GLfloat projection[16];
        glGetFloatv(GL_PROJECTION_MATRIX, projection);
        m_projectionStack.push_back(Matrix<float>(projection));
        glLoadIdentity();
        Vector<2, float> minimum = position;
        Vector<2, float> maximum = size;
        float margin = static_cast<float>((1.0 - scale) * 0.5);
        for (unsigned dimension = 0; dimension < 2; ++dimension)
            maximum.m_elements[dimension] *= margin;
        minimum -= maximum;
        maximum = size;
        maximum += minimum;
        glOrtho(minimum.m_elements[0], maximum.m_elements[0], minimum.m_elements[1], maximum.m_elements[1], -1.0,
                10000000.0);
        glTranslatef(-fraction.m_elements[0], -fraction.m_elements[1], 0.0f);
    } else {
        modelView.m_elements[12] = static_cast<float>(static_cast<int>(modelView.m_elements[12] + 0.5));
        modelView.m_elements[13] = static_cast<float>(static_cast<int>(modelView.m_elements[13] + 0.5));
        glMatrixMode(GL_MODELVIEW);
        loadMatrix(modelView);
        glMatrixMode(GL_PROJECTION);
        GLfloat projection[16];
        glGetFloatv(GL_PROJECTION_MATRIX, projection);
        m_projectionStack.push_back(Matrix<float>(projection));
        glLoadIdentity();
        glOrtho(position.m_elements[0], size.m_elements[0] + position.m_elements[0], position.m_elements[1],
                size.m_elements[1] + position.m_elements[1], -1.0, 10000000.0);
    }
    glScalef(scale, scale, 1.0f);
    glMatrixMode(GL_MODELVIEW);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
}

void GLEngine::endSurface()
{
    glPopAttrib();
    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(m_projectionStack.back().m_elements);
    m_projectionStack.pop_back();
    glMatrixMode(GL_MODELVIEW);
    popSurface();

    // Render into the surface below, or to the window again.
    if (m_surfaceStack.empty())
        GLFrameBufferObject::disable();
    else if (!m_surfaceStack.back())
        GLFrameBufferObject::disable();
    else
        m_surfaceStack.back()->begin();
}

void GLEngine::beginWindowDrawing()
{
    glPushAttrib(GL_CURRENT_BIT | GL_POINT_BIT | GL_LINE_BIT | GL_POLYGON_BIT | GL_PIXEL_MODE_BIT | GL_LIGHTING_BIT |
                 GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT | GL_VIEWPORT_BIT | GL_TRANSFORM_BIT |
                 GL_COLOR_BUFFER_BIT | GL_TEXTURE_BIT);
    glViewport(m_viewportX, m_viewportY, m_viewportWidth, m_viewportHeight);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, m_viewportWidth, 0.0, m_viewportHeight, -1.0, 10000000.0);
    glTranslatef(0.0f, (float)m_viewportHeight, 0.0f);
    glScalef(1.0f, -1.0f, 1.0f);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    g_drawingLayer = 1;
    glClearDepth(0.0);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_LIGHTING);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_2D);
    if (m_colorMaterial) {
        glEnable(GL_COLOR_MATERIAL);
        glColorMaterial(GL_FRONT_AND_BACK, GL_DIFFUSE);
        glEnable(GL_LIGHTING);
        glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f).m_elements);
        glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f).m_elements);
        glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f).m_elements);
        glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f).m_elements);
    }
}

void GLEngine::endWindowDrawing()
{
    glPopAttrib();
}

void GLEngine::drawTexturedRectangle(const Vector<2, float> &positionArgument, const Vector<2, float> &sizeArgument,
                                     const Vector<2, float> &texturePositionArgument,
                                     const Vector<2, float> &textureSizeArgument, const Vector<4, float> &colorArgument)
{
    const float *position = positionArgument.m_elements;
    const float *size = sizeArgument.m_elements;
    const float *texturePosition = texturePositionArgument.m_elements;
    const float *textureSize = textureSizeArgument.m_elements;
    glBegin(GL_QUADS);
    glColor4fv(colorArgument.m_elements);
    glTexCoord2f(texturePosition[0], texturePosition[1]);
    glVertex2f(position[0], position[1]);
    glTexCoord2f(texturePosition[0] + textureSize[0], texturePosition[1]);
    glVertex2f(position[0] + size[0], position[1]);
    glTexCoord2f(texturePosition[0] + textureSize[0], textureSize[1] + texturePosition[1]);
    glVertex2f(position[0] + size[0], size[1] + position[1]);
    glTexCoord2f(texturePosition[0], textureSize[1] + texturePosition[1]);
    glVertex2f(position[0], size[1] + position[1]);
    glEnd();
}

void GLEngine::fillRectangle(const Vector<2, float> &positionArgument, const Vector<2, float> &sizeArgument,
                             const Vector<4, float> &colorArgument)
{
    const float *position = positionArgument.m_elements;
    const float *size = sizeArgument.m_elements;
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_TEXTURE_RECTANGLE_ARB);
    glColor4fv(colorArgument.m_elements);
    glBegin(GL_QUADS);
    glVertex2f(position[0], position[1]);
    glVertex2f(position[0] + size[0], position[1]);
    glVertex2f(position[0] + size[0], position[1] + size[1]);
    glVertex2f(position[0], position[1] + size[1]);
    glEnd();
}

void GLEngine::frameRectangle(const Vector<2, float> &positionArgument, const Vector<2, float> &sizeArgument,
                              const Vector<4, float> &colorArgument)
{
    const float *position = positionArgument.m_elements;
    const float *size = sizeArgument.m_elements;
    useFixedFunction();
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_TEXTURE_RECTANGLE_ARB);
    glEnable(GL_LINE_SMOOTH);
    glColor4fv(colorArgument.m_elements);
    glLineWidth(1.0f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(position[0], position[1]);
    glVertex2f(position[0] + size[0], position[1]);
    glVertex2f(position[0] + size[0], position[1] + size[1]);
    glVertex2f(position[0], position[1] + size[1]);
    glEnd();
}

void GLEngine::invertRectangle(const Vector<2, float> &positionArgument, const Vector<2, float> &sizeArgument)
{
    const float *position = positionArgument.m_elements;
    const float *size = sizeArgument.m_elements;
    useFixedFunction();
    glEnable(GL_COLOR_LOGIC_OP);
    glLogicOp(GL_XOR);
    glDisable(GL_TEXTURE_2D);
    glBegin(GL_QUADS);
    glColor3f(0.0f, 0.0f, 0.0f);
    glVertex2f(position[0], position[1]);
    glVertex2f(position[0] + size[0], position[1]);
    glVertex2f(position[0] + size[0], position[1] + size[1]);
    glVertex2f(position[0], position[1] + size[1]);
    glEnd();
    glDisable(GL_COLOR_LOGIC_OP);
}

void GLEngine::pushIdentityMatrix()
{
    glPushMatrix();
    glLoadIdentity();
}

void GLEngine::popMatrix()
{
    glPopMatrix();
}

void GLEngine::drawCaret(const Vector<2, float> &positionArgument, const Vector<2, float> &sizeArgument,
                         const Vector<4, float> &)
{
    const float *position = positionArgument.m_elements;
    const float *size = sizeArgument.m_elements;
    if (m_caretClock / 500 % 2) {
        glEnable(GL_COLOR_LOGIC_OP);
        glLogicOp(GL_XOR);
        glDisable(GL_TEXTURE_2D);
        glColor3f(0.0f, 0.0f, 0.0f);
        glLineWidth(1.0f);
        glBegin(GL_LINES);
        glVertex2f(position[0], position[1]);
        glVertex2f(position[0], size[1] + position[1]);
        glEnd();
        glDisable(GL_COLOR_LOGIC_OP);
    }
}

void GLEngine::enableTexture2D()
{
    glEnable(GL_TEXTURE_2D);
}

void GLEngine::disableTexture2D()
{
    glDisable(GL_TEXTURE_2D);
}

void GLEngine::useFixedFunction()
{
    GLint current;
    glGetIntegerv(GL_CURRENT_PROGRAM, &current);
    if (current)
        glUseProgram(0);
}

void GLEngine::useDownsampleShader(int factor)
{
    switch (factor) {
    case 2:
        if (m_downsample2Shader)
            m_downsample2Shader->use();
        break;
    case 3:
        if (m_downsample3Shader)
            m_downsample3Shader->use();
        break;
    case 4:
        if (m_downsample4Shader)
            m_downsample4Shader->use();
        break;
    default:
        if (m_alphaShader)
            m_alphaShader->use();
        break;
    }
}

void GLEngine::useBlurShader(bool horizontal)
{
    if (horizontal) {
        if (m_horizontalBlurShader)
            m_horizontalBlurShader->use();
    } else if (m_verticalBlurShader) {
        m_verticalBlurShader->use();
    }
}

void GLEngine::getModelViewMatrix(Matrix<float> &matrix)
{
    GLfloat modelView[16];
    glGetFloatv(GL_MODELVIEW_MATRIX, modelView);
    matrix = Matrix<float>(modelView);
}

void GLEngine::loadMatrix(Matrix<float> &matrix)
{
    glLoadMatrixf(matrix.m_elements);
}

void GLEngine::drawPixels(const unsigned char *pixels, int stride, int unknown, int x, int y, int width, int height)
{
    std::cout << "drawPixels: " << stride << ", " << unknown << ", " << x << ", " << y << ", " << width << ", "
              << height << std::endl;
    glPushMatrix();
    glLoadIdentity();
    for (int row = 0; row < height; row++) {
        glRasterPos2i(0, row);
        glDrawPixels(width, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixels + (x + (y + row) * stride) * 4);
    }
    glRasterPos2i(0, 0);
    glPopMatrix();
}

}
