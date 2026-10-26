// D3D11Shader owns the world shader programs, their constant buffers and the cached values last uploaded to the device
// context.
#pragma once
#include <array>
#include "plasma/Matrix.h"
#include "plasma/Vector.h"

// Direct3D 11 interfaces (d3d11.h); only pointers are held.
struct ID3D11Device;
struct ID3D11DeviceContext;
struct ID3D11Buffer;
struct ID3D11VertexShader;
struct ID3D11PixelShader;
struct ID3D11InputLayout;

namespace gfx {

class D3D11Shader {
public:
    D3D11Shader(ID3D11Device *device, ID3D11DeviceContext *context);
    virtual ~D3D11Shader();

    // The shader programs.
    void apply();      // m_vertexShader, m_pixelShader (render modes 0..7)
    void applyCloud(); // m_vertexShader, m_cloudPixelShader (mode 10: the clouds)
    void applyQuads();
    void applySky();   // m_vertexShader, m_skyPixelShader (mode 8)
    void applyWater(); // m_vertexShader, m_waterPixelShader (mode 9)
    // D3D11Renderer::drawGroundSpots passes true, drawShadows and drawTrails false.
    void enableTexture(bool enabled);
    // Returns true.
    bool initialize();
    void invalidate();

    // The cached setters: each uploads to its constant buffer only when the value differs from the one last uploaded.
    void setAlpha(float value);                                  // PS b4
    void setAmbientColor(const plasma::Vector<4, float> &color); // VS b6
    void setConstantBuffers();                                   // binds VS b0..b9 and PS b0..b5
    void setDepthOffset(float value);                            // VS b8
    void setFogGradientTranslation(float value);                 // PS b2, (value - 80) / -30
    void setFogScale(float value);                               // PS b1, 1 / value when value > 0
    void setGlow(float value);                                   // VS b9
    void setHighlight(const plasma::Vector<4, float> &value);
    void setLight(const plasma::Vector<4, float> &frontColor, const plasma::Vector<4, float> &backColor,
                  const plasma::Vector<3, float> &direction, const plasma::Vector<4, float> &ambientColor);
    void setMaterialColor(const plasma::Vector<4, float> &value); // VS b3
    void setMatrices(const plasma::Matrix<float> &world, const plasma::Matrix<float> &view,
                     const plasma::Matrix<float> &projection);
    void setPointLights(const std::array<plasma::Vector<4, float>, 16> &positions,
                        const std::array<plasma::Vector<4, float>, 16> &colors);
    void setProjectionMatrix(const plasma::Matrix<float> &matrix); // VS b0
    void setShininess(float value);                                // PS b5
    void setSky(const plasma::Vector<4, float> &skyColor1, const plasma::Vector<4, float> &skyColor2,
                const plasma::Vector<4, float> &fogColor, const plasma::Vector<4, float> &sunColor); // PS b0
    // Projects 100 * direction (rotated by the view matrix) and uploads its screen x, its y less (cameraPitch - 80) /
    // 30, the aspect ratio width / height and 1 when the depth is within 0..1.
    void setSunScreenPosition(const plasma::Vector<3, float> &direction, const plasma::Matrix<float> &view,
                              const plasma::Matrix<float> &projection, const float &cameraPitch, int width, int height);
    void setViewMatrix(const plasma::Matrix<float> &matrix);  // VS b1
    void setWhite(float value);                               // VS b7
    void setWorldMatrix(const plasma::Matrix<float> &matrix); // VS b2

protected:
    ID3D11VertexShader *m_vertexShader;
    ID3D11PixelShader *m_pixelShader;
    ID3D11PixelShader *m_skyPixelShader;    // Sky pixel shader used in mode 8.
    ID3D11PixelShader *m_cloudPixelShader;  // Cloud pixel shader used in mode 10.
    ID3D11PixelShader *m_waterPixelShader;  // Water pixel shader used in mode 9.
    ID3D11PixelShader *m_unusedPixelShader; // Pixel shader created and released but never bound.
    ID3D11VertexShader *m_texturedVertexShader;
    ID3D11PixelShader *m_texturedPixelShader; // Textured shader bytecode.
    ID3D11Device *m_device;                   // constructor argument; not released
    ID3D11DeviceContext *m_context;           // constructor argument; not released
    ID3D11InputLayout *m_inputLayout;
    ID3D11InputLayout *m_texturedInputLayout;

    // The values as last uploaded. The matrix setters compare the argument's address with the member's and upload the
    // transpose.
    plasma::Matrix<float> m_worldMatrix;
    plasma::Matrix<float> m_viewMatrix;
    plasma::Matrix<float> m_projectionMatrix;
    plasma::Vector<4, float> m_materialColor;
    plasma::Vector<3, float> m_lightDirection;
    plasma::Vector<4, float> m_lightFrontColor;
    plasma::Vector<4, float> m_lightBackColor;
    plasma::Vector<4, float> m_ambientColor;
    float m_white;
    plasma::Vector<4, float> m_skyColors[4];
    plasma::Vector<4, float> m_sunScreenPosition;
    std::array<plasma::Vector<4, float>, 16> m_pointLightPositions;
    std::array<plasma::Vector<4, float>, 16> m_pointLightColors; // VS b4, interleaved with the positions
    float m_shininess;
    float m_alpha;
    float m_glow;
    float m_fogScale;
    float m_fogGradientTranslation;
    float m_depthOffset;
    // Never read.
    plasma::Vector<4, float> m_unusedCache3e0;

    ID3D11Buffer *m_projectionBuffer;        // VS b0, 64
    ID3D11Buffer *m_viewBuffer;              // VS b1, 64
    ID3D11Buffer *m_worldBuffer;             // VS b2, 64
    ID3D11Buffer *m_materialBuffer;          // VS b3, 16
    ID3D11Buffer *m_pointLightsBuffer;       // VS b4, 320 (ten position/colour pairs)
    ID3D11Buffer *m_lightBuffer;             // VS b5, 48
    ID3D11Buffer *m_ambientBuffer;           // VS b6, 16
    ID3D11Buffer *m_whiteBuffer;             // VS b7, 16
    ID3D11Buffer *m_depthOffsetBuffer;       // VS b8, 16
    ID3D11Buffer *m_glowBuffer;              // VS b9, 16
    ID3D11Buffer *m_skyBuffer;               // PS b0, 64
    ID3D11Buffer *m_fogDistanceBuffer;       // PS b1, 16
    ID3D11Buffer *m_fogAngleBuffer;          // PS b2, 16
    ID3D11Buffer *m_sunScreenPositionBuffer; // PS b3, 16
    ID3D11Buffer *m_alphaBuffer;             // PS b4, 16
    ID3D11Buffer *m_shininessBuffer;         // PS b5, 16 (not zeroed by the constructor)
    ID3D11Buffer *m_effectTextureBuffer;
};

}
