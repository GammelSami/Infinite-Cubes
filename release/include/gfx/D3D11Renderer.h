// D3D11Renderer implements the game's rendering interface using Direct3D 11. It keeps the world shaders, pipeline
// states, unit quad, dynamic drawing buffers and generated spot textures; the supplied engine, device and immediate
// context are borrowed.
#pragma once
#include <array>
#include <list>
#include <vector>
#include "cube/Creature.h"
#include "cube/Game.h"
#include "gfx/D3D11Shader.h"
#include "gfx/Renderer.h"
#include "gfx/RendererIds.h"
#include "plasma/Matrix.h"
#include "plasma/Vector.h"

// Direct3D 11 interfaces (d3d11.h); only pointers are held.
struct ID3D11Device;
struct ID3D11DeviceContext;
struct ID3D11Buffer;
struct ID3D11RasterizerState;
struct ID3D11DepthStencilState;
struct ID3D11BlendState;
struct ID3D11ShaderResourceView;

namespace cube {
struct Item;
class World;
struct Item;
class Sprite;
class WorldMap;
struct Item;
class Sprite;
}

namespace plasma {
class D3D11Engine;
}

namespace gfx {

struct TexturedVertex {
    plasma::Vector<4, float> position;
    plasma::Vector<4, unsigned char> color;
    plasma::Vector<2, float> textureCoordinate;
};

class D3D11Renderer : public Renderer {
public:
    D3D11Renderer(cube::World *world, cube::WorldMap *worldMap, plasma::D3D11Engine *engine, ID3D11Device *device,
                  ID3D11DeviceContext *context);
    virtual ~D3D11Renderer();
    virtual void activateMode(int mode);
    // Draws with scratch buffers holding at most 256 vertices and 256 indices; does not check those limits. Used by
    // ground spots, shadows and trails.
    void drawDynamic(const std::vector<TexturedVertex> &vertices, const std::vector<int> &indices);
    virtual void drawInterface();
    virtual void drawSky();
    virtual void drawGroundSpots();
    virtual void drawShadows(const plasma::Vector<4, float> &lightColor);
    virtual void drawTrails();
    virtual void setHighlight(const plasma::Vector<4, float> &value);

    virtual void setWorldMatrix(const plasma::Matrix<float> &matrix);
    virtual void setViewMatrix(const plasma::Matrix<float> &matrix);
    virtual void setProjectionMatrix(const plasma::Matrix<float> &matrix);
    virtual void setMatrices(const plasma::Matrix<float> &world, const plasma::Matrix<float> &view,
                             const plasma::Matrix<float> &projection);
    virtual void setAlpha(float value);
    virtual void setGlow(float value);
    virtual void setFogScale(float value);
    virtual void setFogGradientTranslation(float value);
    virtual void setDepthOffset(float value);
    virtual void setMaterialColor(const plasma::Vector<4, float> &value);
    virtual void setSky(const plasma::Vector<4, float> &skyColor1, const plasma::Vector<4, float> &skyColor2,
                        const plasma::Vector<4, float> &fogColor, const plasma::Vector<4, float> &sunColor);
    virtual void setSunScreenPosition(const plasma::Vector<3, float> &direction, const plasma::Matrix<float> &view,
                                      const plasma::Matrix<float> &projection, const float &cameraPitch, int width,
                                      int height);
    virtual void setShininess(float value);
    virtual void setWhite(float value);
    virtual void setLight(const plasma::Vector<4, float> &frontColor, const plasma::Vector<4, float> &backColor,
                          const plasma::Vector<3, float> &direction, const plasma::Vector<4, float> &ambientColor);
    virtual void setAmbientColor(const plasma::Vector<4, float> &color);
    virtual void setPointLights(const std::array<plasma::Vector<4, float>, 16> &positions,
                                const std::array<plasma::Vector<4, float>, 16> &colors);

protected:
    plasma::D3D11Engine *m_engine;  // constructor argument; drawInterface calls Engine::drawWindow on it
    ID3D11Device *m_device;         // constructor argument; not released
    ID3D11DeviceContext *m_context; // constructor argument; not released
    D3D11Shader m_shader;

    // Fill solid, DepthClipEnable; indexed by RasterizerSlot (RendererIds.h): cull front; cull front with
    // FrontCounterClockwise; no culling.
    ID3D11RasterizerState *m_rasterizerStates[RasterizerSlotCount];
    // Indexed by DepthStencilSlot: depth test LESS_EQUAL and write; no depth test; depth test without write.
    ID3D11DepthStencilState *m_depthStencilStates[DepthStencilSlotCount];
    // Indexed by BlendSlot: SRC_ALPHA / INV_SRC_ALPHA (alpha ONE / ZERO); no blending; no blending and no colour
    // writes.
    ID3D11BlendState *m_blendStates[BlendSlotCount];
    ID3D11Buffer *m_quadVertexBuffer;    // four gfx::Vertex (32 bytes), immutable
    ID3D11Buffer *m_quadIndexBuffer;     // 0, 1, 2, 0, 2, 3 (uint32)
    ID3D11Buffer *m_dynamicVertexBuffer; // bytes (256 vertices of 28), D3D11_USAGE_DYNAMIC; not released
    ID3D11Buffer *m_dynamicIndexBuffer;  // bytes (256 uint32), dynamic; not released
    ID3D11ShaderResourceView *m_spotTexture;
    ID3D11ShaderResourceView *m_ringTexture;
};

}
