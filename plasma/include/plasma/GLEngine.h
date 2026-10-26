// plasma::GLEngine: the OpenGL engine, the only class derived from plasma::Engine. It creates the GL textures,
// drawings and render surfaces, keeps the GLSL programs (the shape shader and the filter shaders) with the uniform
// values last given to the shape shader, and draws the interface's rectangles in immediate mode.
#pragma once
#include <list>
#include "plasma/Engine.h"
#include "plasma/Matrix.h"
#include "plasma/GLShader.h"
#include "plasma/GLFrameBufferObject.h"

class TexturePreviewWidget;

namespace plasma {

class GLEngine : public Engine {
public:
    GLEngine();
    virtual ~GLEngine();

    virtual void setupShadersAndFrameBuffer();
    virtual Texture *createTexture(int width, int height, const unsigned char *pixels, const TextureFormat &format,
                                   const std::wstring &name = L"", bool transient = false);
    virtual void drawCaret(const Vector<2, float> &, const Vector<2, float> &, const Vector<4, float> &);
    virtual void invertRectangle(const Vector<2, float> &, const Vector<2, float> &);
    virtual void drawTexturedRectangle(const Vector<2, float> &, const Vector<2, float> &, const Vector<2, float> &,
                                       const Vector<2, float> &, const Vector<4, float> &);
    virtual void fillRectangle(const Vector<2, float> &, const Vector<2, float> &, const Vector<4, float> &);
    virtual void frameRectangle(const Vector<2, float> &, const Vector<2, float> &, const Vector<4, float> &);
    virtual void pushIdentityMatrix();
    virtual void popMatrix();
    virtual void enableTexture2D();
    virtual void disableTexture2D();
    virtual Drawing *createDrawing();
    virtual RenderSurface *createRenderSurface(int);
    virtual void getModelViewMatrix(Matrix<float> &);
    virtual void loadMatrix(Matrix<float> &);
    virtual void beginTextDrawing(Node *node);
    virtual void beginWindowDrawing();
    virtual void endWindowDrawing();
    virtual void pushNodeMatrix(Node *);
    virtual void popNodeMatrix(Node *);
    virtual void unused22(Node *) {}
    virtual void unused23(Node *) {}
    virtual void beginSurface(RenderSurface *, const Vector<2, float> &, const Vector<2, float> &, float scale,
                              bool alignPixels);
    virtual void endSurface();
    virtual void drawPixels(const unsigned char *, int stride, int, int x, int y, int width, int height);

    // Uses no GLSL program (the fixed-function pipeline).
    void useFixedFunction();

    // Uses the filter shader that averages factor x factor pixels (2, 3 or 4), or the one that only divides by alpha.
    void useDownsampleShader(int factor);

    // Uses the horizontal or the vertical blur shader.
    void useBlurShader(bool horizontal);

private:
    friend class GLRenderSurface;        // Renders through m_frameBuffer.
    friend class GLDrawing;              // Selects the shape shader before drawing.
    friend class ::TexturePreviewWidget; // Selects it for the texture preview.

    // Compiles the shaders and sets the shape shader's uniforms to their initial values.
    void createShaders();
    void setShapeShader(Node *node, bool textured, float opacity, float brightness, float contrast, float saturation);

    GLFrameBufferObject *m_frameBuffer;
    // Zeroed by the constructor and otherwise unused.
    int m_unused1FC;
    int m_unused200;
    GLuint m_texture; // Generated with the framebuffer object.
    std::list<Matrix<float>> m_projectionStack;
    bool m_colorMaterial;    // BeginWindowDrawing enables GL_COLOR_MATERIAL.
    GLShader *m_alphaShader; // Divides by alpha, times the colour.
    GLShader *m_downsample2Shader;
    GLShader *m_downsample3Shader;
    GLShader *m_downsample4Shader;
    GLShader *m_verticalBlurShader;
    GLShader *m_horizontalBlurShader;
    GLShader *m_shapeShader;
    int m_unused234; // Zeroed by the constructor; no other access.
    int m_unused238; // Not initialised by the constructor; no access.

    // The shape shader's uniform values, as last set.
    int m_filter;
    bool m_textureEnabled;
    float m_textureOpacity;
    float m_textureBrightness;
    float m_textureContrast;
    float m_textureSaturation;
    bool m_widgetDeformationEnabled;

    // The shape shader's uniform locations.
    GLint m_filterLocation;
    GLint m_textureEnabledLocation;
    GLint m_textureOpacityLocation;
    GLint m_textureBrightnessLocation;
    GLint m_textureContrastLocation;
    GLint m_textureSaturationLocation;
    GLint m_aaOffsetLocation;
    GLint m_widgetDeformationEnabledLocation;
    GLint m_widgetBindMatrixLocation;
    GLint m_inverseWidgetBindMatrixLocation;
    GLint m_widgetBindPosLocation;
    GLint m_widgetBindSizeLocation;
    GLint m_deformedWidgetPosLocation;
    GLint m_deformedWidgetSizeLocation;
};

}
