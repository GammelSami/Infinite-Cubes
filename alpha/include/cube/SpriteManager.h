#pragma once
#include <vector>
#include <string>

struct IDirect3DDevice9;
namespace cube {
class Sprite;
struct Item;

int consumableSpriteIndex(int id);
int weaponSpriteIndex(int id, int modifier, int rarity, int material);

// Alpha stores only the owned sprite table and the borrowed Direct3D device.
class SpriteManager {
public:
    explicit SpriteManager(IDirect3DDevice9 *device);
    virtual ~SpriteManager();
    void loadSprites(const std::string &directory);
    Sprite *itemSprite(const Item *item);
    Sprite *sprite(int index)
    {
        if (index < 0 || index >= (int)m_sprites.size())
            return 0;
        return m_sprites[index];
    }

private:
    std::vector<Sprite *> m_sprites;
    IDirect3DDevice9 *m_device;
};
}
