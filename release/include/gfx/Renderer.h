// Renderer gathers visible zones, sprites, effects and lights and draws the world, world map and interface. It keeps
// camera matrices and sky and light colours; D3D11Renderer implements its drawing and shader-parameter interface.
#pragma once
#include <vector>
#include <array>
#include <list>
#include "cube/Creature.h"
#include "gfx/RendererIds.h"
#include "gfx/SpriteBatch.h"
#include "plasma/Matrix.h"
#include "plasma/Vector.h"

namespace cube {
struct Item;
struct Light;
struct Particle;
struct StaticEntity;
struct MenuEntry;
class Sprite;
class World;
class WorldMap;
class Zone;
class ZoneNeighborhood;
}

namespace gfx {

class ByteGrid;

// Builds a perspective projection with depth 0..1 using Direct3D's left-handed convention.
inline void perspectiveFov(plasma::Matrix<float> &matrix, float fieldOfView, float aspect, float zNear, float zFar)
{
    float cotangent = 1.0f / tanf(fieldOfView * 0.5f);
    matrix(0, 0) = cotangent / aspect;
    matrix(0, 1) = 0.0f;
    matrix(0, 2) = 0.0f;
    matrix(0, 3) = 0.0f;
    matrix(1, 0) = 0.0f;
    matrix(1, 1) = cotangent;
    matrix(1, 2) = 0.0f;
    matrix(1, 3) = 0.0f;
    matrix(2, 0) = 0.0f;
    matrix(2, 1) = 0.0f;
    matrix(2, 2) = zFar / (zFar - zNear);
    matrix(2, 3) = -zNear * zFar / (zFar - zNear);
    matrix(3, 0) = 0.0f;
    matrix(3, 1) = 0.0f;
    matrix(3, 2) = 1.0f;
    matrix(3, 3) = 0.0f;
}

struct GroundSpot {
    int kind;                                // 0 the soft spot texture, otherwise the ring texture
    plasma::Vector<3, cube::Fixed> position; // World position in fixed-point units (1/65536 block).
    plasma::Vector<4, float> color;          // 0..1 each
    float radius;                            // in blocks
};

struct GroundShadow {
    plasma::Vector<3, cube::Fixed> position; // World position in fixed-point units (1/65536 block).
    float squaredDistance;                   // to the camera, in blocks; collectSprites sorts by it
    float size;                              // the width of the near edge, in blocks
    float height;                            // the far edge lies size * height / 4 away and height lower
};

class Renderer {
public:
    Renderer(cube::World *world, cube::WorldMap *worldMap);
    virtual ~Renderer();

    virtual void activateMode(int mode) {}
    virtual void drawInterface() {}
    virtual void unimplementedHook() {}
    virtual void drawSky() {}
    virtual void drawNightSky() {}
    virtual void drawGroundSpots() {}
    virtual void drawShadows(const plasma::Vector<4, float> &lightColor) {}
    // Weapon trails.
    virtual void drawTrails() {}
    virtual void setHighlight(const plasma::Vector<4, float> &value) {}

    virtual void setWorldMatrix(const plasma::Matrix<float> &matrix) {}
    virtual void setViewMatrix(const plasma::Matrix<float> &matrix) {}
    virtual void setProjectionMatrix(const plasma::Matrix<float> &matrix) {}
    virtual void setMatrices(const plasma::Matrix<float> &world, const plasma::Matrix<float> &view,
                             const plasma::Matrix<float> &projection)
    {}
    // Shader parameter setters for Alpha, Glow, FogDistance, FogAngle, DepthOffset, Shininess and White.
    virtual void setAlpha(float value) {}
    virtual void setGlow(float value) {}
    virtual void setFogScale(float value) {}
    virtual void setFogGradientTranslation(float value) {}
    virtual void setDepthOffset(float value) {}
    virtual void setMaterialColor(const plasma::Vector<4, float> &value) {}
    virtual void setSky(const plasma::Vector<4, float> &skyColor1, const plasma::Vector<4, float> &skyColor2,
                        const plasma::Vector<4, float> &fogColor, const plasma::Vector<4, float> &sunColor)
    {}
    // Projects direction * 100 through view and projection and uploads the screen x, y (y lowered by (cameraPitch -
    // 80) / 30), the float quotient width / height and whether the point's depth is in 0..1.
    virtual void setSunScreenPosition(const plasma::Vector<3, float> &direction, const plasma::Matrix<float> &view,
                                      const plasma::Matrix<float> &projection, const float &cameraPitch, int width,
                                      int height)
    {}
    virtual void setShininess(float value) {}
    virtual void setWhite(float value) {}
    // Draw passes m_lightFrontColor, m_lightBackColor, the sun direction from the world's time of day, and
    // m_ambientColor.
    virtual void setLight(const plasma::Vector<4, float> &frontColor, const plasma::Vector<4, float> &backColor,
                          const plasma::Vector<3, float> &direction, const plasma::Vector<4, float> &ambientColor)
    {}
    virtual void setAmbientColor(const plasma::Vector<4, float> &color) {}
    virtual void setPointLights(const std::array<plasma::Vector<4, float>, 16> &positions,
                                const std::array<plasma::Vector<4, float>, 16> &colors)
    {}
    virtual void endChunkBufferDraw() {}
    virtual void beginScene() {}
    virtual void endScene() {}

    void clearPointLights();
    // Called by draw; unused is ignored.
    void renderMap(const plasma::Vector<3, cube::Fixed> &center, int zoom, float unused,
                   const plasma::Vector<2, float> &cursor, float viewScaleFactor, bool controlKeyDown, bool passiveView,
                   float frameTime);
    void renderTranslucentSprites(SpriteBatch &batch, int flags);
    // Draws water surfaces using m_prerequisiteEntityIndices as alpha, restoring alpha to 1 afterwards.
    void renderWater();
    void renderChunks(const plasma::Vector<4, float> &frontColor, const plasma::Vector<4, float> &backColor,
                      const plasma::Vector<3, float> &direction, const plasma::Vector<4, float> &ambientColor);
    // Clouds nearer than 180 blocks fade out towards 80 and are drawn last, first without and then with their alpha.
    // The first two arguments (render passes the sky's gradient colours) are not read; the last four go to setLight at
    // the end.
    void renderClouds(const plasma::Vector<4, float> &skyColor1, const plasma::Vector<4, float> &skyColor2,
                      const plasma::Vector<4, float> &frontColor, const plasma::Vector<4, float> &backColor,
                      const plasma::Vector<3, float> &direction, const plasma::Vector<4, float> &ambientColor);
    // Called by draw.
    void renderEffects();
    // The five draw functions of renderEffects, one per list, each drawing cubes (sprite 2630) that move with the
    // time: drawBox a rising ring, drawCube a widening spiral, drawParticle an arc over a sphere, drawImpact three
    // cubes per step circling a vector (clipped at the first block in the way when asked), drawLine a stretched cube.
    void drawBox(const plasma::Vector<3, cube::Fixed> &position, const plasma::Vector<4, float> &startColor,
                 const plasma::Vector<4, float> &endColor, unsigned int time, float radius, float scale, int count);
    void drawCube(const plasma::Vector<3, cube::Fixed> &position, const plasma::Vector<4, float> &startColor,
                  const plasma::Vector<4, float> &endColor, unsigned int time, float radius, float scale, float turns,
                  int count);
    void drawImpact(const plasma::Vector<3, cube::Fixed> &position, const plasma::Vector<3, float> &vector,
                    const plasma::Vector<3, float> &rotation, const plasma::Vector<4, float> &innerColor,
                    const plasma::Vector<4, float> &middleColor, const plasma::Vector<4, float> &outerColor, int time,
                    float width, float scale, float frequency, int count, bool clipped);
    void drawLine(const plasma::Vector<3, cube::Fixed> &position, const plasma::Vector<3, float> &vector,
                  const plasma::Vector<4, float> &color, float width);
    void drawParticle(const plasma::Vector<3, cube::Fixed> &position, const plasma::Vector<4, float> &startColor,
                      const plasma::Vector<4, float> &endColor, unsigned int time, float radius, float scale,
                      int count);
    // Called by draw.
    void extractFrustum();
    bool isBoxVisible(const plasma::Vector<3, cube::Fixed> &minimum, const plasma::Vector<3, cube::Fixed> &maximum,
                      float range);
    bool isSphereVisible(const plasma::Vector<3, cube::Fixed> &position, float radius, float range);
    bool canMesh(int x, int y);
    float attenuation(const plasma::Vector<3, cube::Fixed> &position, float range);
    // Uses the model, view and projection matrices used by drawItem.
    void drawCubes(const cube::Item *item, const plasma::Matrix<float> &model, const plasma::Matrix<float> &view,
                   const plasma::Matrix<float> &projection, const plasma::Vector<4, float> &color, float glow);
    // With SpritePassFlagZoneLights each instance gets the point lights of the zone its centre lies in; with
    // SpritePassFlagScreenPosition the projection is shifted to the instance's window position (drawItem's and
    // drawCreature's batches).
    void renderSprites(SpriteBatch &batch, int flags);
    // Updates view distance slowly upwards and immediately downwards. Holds it at 256 in a shaded dungeon or with buff
    // 19, and at least 64 on a long flight. Stores dungeon shade in m_dungeonShade and moves water opacity towards 1.
    void updateViewDistance(float frameTime);
    // It sets the camera and the daylight, moves the light and sky colours towards the place's, collects the frame's
    // sprites and lights and draws the passes: sky, chunks, sprites, effects, particles, water, ground spots, clouds,
    // stars, shadows, trails, then the map and the interface's sprites (logo, compass, items, ring menu) and the
    // interface.
    void render(const plasma::Vector<3, cube::Fixed> &cameraPosition,
                const plasma::Vector<3, cube::Fixed> &focusPosition, const plasma::Vector<3, float> &cameraAngle,
                int width, int height, float step, plasma::Matrix<float> &worldViewMatrix,
                plasma::Matrix<float> &viewMatrix, plasma::Matrix<float> &projectionMatrix, int zoneRadius,
                const plasma::Vector<3, cube::Fixed> &mapPosition, int mapZoom, float interfaceScale,
                std::list<cube::Particle> &particles, std::list<cube::Light> &lights,
                std::vector<cube::MenuEntry> &usableItems, float ringRadius, float ringRotation,
                const int *lookedAtBlock, const plasma::Vector<2, float> &mapOverlayPosition, float mapOverlayScale,
                int flags);
    void resetViewDistance();
    // Sorts lights by distance and assigns each to the zones it reaches, up to sixteen per zone. render calls it after
    // collectSprites fills m_zones.
    void collectLights(std::list<cube::Light> &lights);
    // It fills m_zones with the meshed zones in view around the camera (nearest first, each with its squared distance
    // in Chunk::squaredCameraDistance and its point lights cleared) and m_worldBatch / m_underwaterPlantBatch with the
    // sprites of their plants (which sway and swing with the time), of the creatures (AnimationState::addSprites, with
    // the animation of the ability in progress, the stars over a stunned creature and the sign over its head), of the
    // items on the ground and the static entities of the seven by seven zones around the player (placeStaticEntity)
    // and of the projectiles.
    void collectSprites(const plasma::Vector<3, int> &lookedAt, int flags);
    // The types 0, 4, 5 and 6 take the light of the block they are in; the others glow.
    void renderParticles(std::list<cube::Particle> &particles);
    void placeStaticEntity(const cube::StaticEntity &entity);

    // Draws a sprite with the model, view and projection matrices in the colour (VoxelWidget, WorldPreviewWidget).
    void drawSprite(cube::Sprite *sprite, const plasma::Matrix<float> &model, const plasma::Matrix<float> &view,
                    const plasma::Matrix<float> &projection, const plasma::Vector<4, float> &color);

    // Under the world's zone lock it takes the zone and its eight neighbours; then it computes the light of the blocks
    // (computeLight into a ByteGrid with a margin of 8), gives the zone's plants their light, finds the lowest and
    // highest block that shows a face and cuts that range into slices of 250 blocks (a vertex holds its position in
    // bytes). For every block a quad goes towards each neighbour that is open (air, or water beside a block that is
    // not water), each corner coloured by shade; the blocks below a column's base, which are not stored, get the
    // colour World::surfaceBlock makes for them. Water, lava and everything else go to index lists of their own; a wet
    // block under the open gets a second quad of water, and a zone of nothing but open sea one quad for the whole
    // surface. Each slice becomes a vertex buffer and its index buffers (sixteen-bit where the vertices allow), and
    // under the mesh lock the Chunk gets its bounds and the new ChunkBuffers; buffers of an earlier pass that the
    // chunk thread has not taken are deleted. Sleep(0) per column lets the other threads run.
    void updateChunk(int x, int y);
    // The colour of a mesh vertex at the corner (x, y, z) of a face that looks along direction: color darkened by how
    // much of the space around the corner is solid (the open share of the blocks within three of it on the face's
    // side, nearer ones weighing more, to the power 1.2), with the saturation raised in the dark; alpha is the mean of
    // grid's bytes over the open blocks, / 255. plain returns color as it is. The zone mesher calls it for every
    // corner of every face.
    plasma::Vector<4, float> shade(cube::ZoneNeighborhood &zones, ByteGrid &grid, int x, int y, int z,
                                   const plasma::Vector<4, float> &color, const plasma::Vector<3, int> &direction,
                                   bool plain);
    void drawItem(const cube::Item *item, float x, float y, const plasma::Vector<3, float> &rotation, float scale,
                  float depth, bool outlined);
    // Draws a creature's model at window position (x, y), turned by rotation (degrees, passed by address) and scaled
    // by scale.
    void drawCreature(cube::Creature::AnimationState *animationState, float x, float y,
                      const plasma::Vector<3, float> &rotation, float scale);
    void drawSpriteModel(cube::Sprite *sprite, float x, float y, const plasma::Vector<3, float> &rotation, float scale,
                         float depthOffset);

protected:
    cube::World *m_world;
    cube::WorldMap *m_worldMap; // constructor argument (WinMain passes the world map)
    // Effect records passed to their respective drawing functions; some fields have unknown meanings.
    struct Impact { // Effect record storage.
        Impact(const plasma::Vector<3, cube::Fixed> &position, const plasma::Vector<3, float> &vector,
               const plasma::Vector<3, float> &rotation, const plasma::Vector<4, float> &innerColor,
               const plasma::Vector<4, float> &middleColor, const plasma::Vector<4, float> &outerColor, int time,
               float width, float scale, float frequency, int count, bool clipped)
            : position(position),
              direction(vector),
              rotation(rotation),
              innerColor(innerColor),
              middleColor(middleColor),
              outerColor(outerColor),
              time(time),
              width(width),
              scale(scale),
              frequency(frequency),
              count(count),
              clipped(clipped)
        {}

        plasma::Vector<3, cube::Fixed> position;
        plasma::Vector<3, float> direction;
        plasma::Vector<3, float> rotation;
        plasma::Vector<4, float> innerColor;
        plasma::Vector<4, float> middleColor;
        plasma::Vector<4, float> outerColor;
        int time;
        float width;
        float scale;
        float frequency;
        int count;
        bool clipped;
    };
    struct Box { // Effect record storage.
        Box(const plasma::Vector<3, cube::Fixed> &position, const plasma::Vector<4, float> &startColor,
            const plasma::Vector<4, float> &endColor, int time, float radius, float scale, int count)
            : position(position),
              startColor(startColor),
              endColor(endColor),
              time(time),
              radius(radius),
              scale(scale),
              count(count)
        {}

        plasma::Vector<3, cube::Fixed> position;
        plasma::Vector<4, float> startColor;
        plasma::Vector<4, float> endColor;
        int time;
        float radius;
        float scale;
        int count;
    };
    struct Particle { // Effect record storage.
        Particle(const plasma::Vector<3, cube::Fixed> &position, const plasma::Vector<4, float> &startColor,
                 const plasma::Vector<4, float> &endColor, int time, float radius, float scale, int count)
            : position(position),
              startColor(startColor),
              endColor(endColor),
              time(time),
              radius(radius),
              scale(scale),
              count(count)
        {}

        plasma::Vector<3, cube::Fixed> position;
        plasma::Vector<4, float> startColor;
        plasma::Vector<4, float> endColor;
        int time;
        float radius;
        float scale;
        int count;
    };
    struct Cube { // Effect record storage.
        // drawCube's arguments.
        Cube(const plasma::Vector<3, cube::Fixed> &position, const plasma::Vector<4, float> &startColor,
             const plasma::Vector<4, float> &endColor, int time, float radius, float scale, float turns, int count)
            : position(position),
              startColor(startColor),
              endColor(endColor),
              time(time),
              radius(radius),
              scale(scale),
              turns(turns),
              count(count)
        {}

        plasma::Vector<3, cube::Fixed> position;
        plasma::Vector<4, float> startColor;
        plasma::Vector<4, float> endColor;
        int time;
        float radius;
        float scale;
        float turns;
        int count;
    };
    struct Line {
        Line(const plasma::Vector<3, cube::Fixed> &position, const plasma::Vector<3, float> &vector,
             const plasma::Vector<4, float> &color, float width)
            : position(position),
              vector(vector),
              color(color),
              width(width)
        {}

        plasma::Vector<3, cube::Fixed> position;
        plasma::Vector<3, float> vector;
        plasma::Vector<4, float> color;
        float width;
    };
    // A point light of the frame: collectLights assigns its position, then the other members, and sorts the vector by
    // distance.
    struct Light {
        plasma::Vector<3, cube::Fixed> position; // world position
        float radius;                            // in blocks
        plasma::Vector<3, float> color;          // red, green, blue; may exceed 1
        float squaredDistance;                   // to the camera, in blocks
    };

    SpriteBatch m_screenBatch;
    std::list<Impact> m_impacts;
    std::list<Box> m_boxes;
    std::list<Particle> m_particles;
    std::list<Cube> m_cubes;
    std::list<Line> m_lines;
    std::list<cube::Creature *> m_trailCreatures;
    std::list<GroundSpot> m_groundSpots;
    std::vector<GroundShadow> m_shadows; // drawn by D3D11Renderer::drawShadows
    SpriteBatch m_worldBatch;
    SpriteBatch m_underwaterPlantBatch;
    SpriteBatch m_mapBatch; // the world map's sprites (WorldMap::render)
    std::vector<cube::Zone *> m_zones;
    std::vector<Light> m_lights;
    float m_frameTime;
    plasma::Vector<3, cube::Fixed> m_cameraPosition;
    plasma::Vector<3, float> m_cameraAngle;
    plasma::Matrix<float> m_worldViewMatrix;  // render's seventh argument; not read in this file
    plasma::Matrix<float> m_viewMatrix;       // Draw passes it to setViewMatrix and setSunScreenPosition
    plasma::Matrix<float> m_projectionMatrix; // Draw passes it to setProjectionMatrix and setSunScreenPosition
    int m_width;
    int m_height;
    // Render origin.
    cube::Fixed m_drawOriginX;
    cube::Fixed m_drawOriginY;
    int m_zoneRadius; // zeroed by the constructor; Draw uses it << 6 as a height
    // Set by extractFrustum, read by isBoxVisible and isSphereVisible.
    plasma::Vector<4, float> m_frustum[FrustumPlaneCount];
    // The first distance limits shadows and particles; the second has no reader in this file.
    float m_effectViewDistance;
    float m_creatureViewDistance;
    float m_viewDistance;
    float m_waterAlpha; // 1 at construction; updateViewDistance moves it towards 1; renderWater's alpha
    plasma::Vector<4, float> m_skyColors[4];
    plasma::Vector<4, float> m_dungeonShade;
    plasma::Vector<4, float> m_ambientColor;    // (0.3, 0.3, 0.7, 1); setLight / setAmbientColor
    plasma::Vector<4, float> m_lightFrontColor; // (0.7, 0.7, 0.3, 1); setLight, drawShadows
    plasma::Vector<4, float> m_lightBackColor;  // (0.5, 0.5, 0.5, 1); setLight
    // Daylight value.
    float m_daylight;
    plasma::Vector<4, float> m_tint; // not set by the constructor; setLightOrigin's argument
};

}
