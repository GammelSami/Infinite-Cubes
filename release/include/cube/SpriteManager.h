// SpriteManager loads .cub models from sprites and supplies them to the renderer by index. It caches procedurally
// generated artifact models by type and seed.
#pragma once
#include <array>
#include <map>
#include <string>
#include <vector>

namespace gfx {
class Graphics;
}

namespace cube {

class Sprite;
struct Item;

class SpriteManager {
public:
    explicit SpriteManager(gfx::Graphics *graphics);
    virtual ~SpriteManager();

    // Loads the sprite models from directory.
    void loadSprites(const std::string &directory);
    Sprite *itemSprite(const Item *item);
    // The generated model of an artifact (item category 23) of the given type (the item's id, 0..6) and seed (its
    // modifier), created on first use by cube::ArtifactGenerator and kept in m_artifactSprites.
    Sprite *artifactSprite(int type, int seed);
    Sprite *createSprite(bool flag);
    Sprite *decorationSprite(int index);
    Sprite *objectSprite(int index);
    Sprite *partSprite(int index);
    // m_sprites[index], or null outside the vector.
    Sprite *sprite(int index)
    {
        if (index < 0 || index >= (int)m_sprites.size())
            return nullptr;
        return m_sprites[index];
    }

    // The graphics interface the sprites draw through.
    gfx::Graphics *graphics() { return m_graphics; }

protected:
    gfx::Graphics *m_graphics;       // Graphics interface supplied to each new Sprite.
    std::vector<Sprite *> m_sprites; // owned
    std::map<std::array<int, 2>, Sprite *> m_artifactSprites;
    std::vector<Sprite *> m_environmentSprites;
    std::vector<Sprite *> m_staticEntitySprites;
    std::vector<Sprite *> m_staticEntityDoorAndLidSprites;
};

}
