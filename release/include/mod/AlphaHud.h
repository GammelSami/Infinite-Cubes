#pragma once
#include <map>
#include <string>
#include <vector>

namespace cube {
class Interface;
}
namespace plasma {
class Node;
class Texture;
}

namespace mod {

class SkillMenu;

// The alpha's in-game interface in place of the release's: the life bar with name, level and class, the experience and
// mana cube bars at the top left; the hit point and mana bars and the ability bar (M1, M2, 1 to 4) at the bottom. Its
// graphics are the alpha's (alpha/gui.plx and alpha/icons, see tools/import-alpha-assets.py); without them the
// release's interface stays.
class AlphaHud {
public:
    explicit AlphaHud(cube::Interface *ui);
    void update();
    bool available() const { return m_available; }
    // Whether the alpha interface is shown (the release's elements make room for it).
    bool active() const;

    // The texture of an icon in alpha/icons (by name without ".png"), loaded once; nullptr if missing.
    plasma::Texture *icon(const std::string &name);
    // The alpha's icon name for an ability of the game (weapon attacks and class skills).
    static std::string abilityIcon(int ability);
    // Shows the icon (an alpha icon name) on a button of the alpha's gui.plx: the release's vector icon of icons.plx
    // where it has one, sharp at every size, else the alpha's image; dimmed unless bright. Empty shows the empty icon.
    void setIcon(plasma::Node *button, const std::string &name, bool bright);
    // The skill screen (key X), or nullptr without the alpha's graphics.
    SkillMenu *skillMenu() const { return m_skillMenu; }

private:
    static void setFill(plasma::Node *fill, float fraction);

    cube::Interface *m_interface;
    bool m_available = false;
    plasma::Node *m_gui = nullptr;
    plasma::Node *m_lifeBar = nullptr;
    plasma::Node *m_lifeBarFill = nullptr;
    plasma::Node *m_experienceBar = nullptr;
    plasma::Node *m_experienceBarFill = nullptr;
    plasma::Node *m_manaCubeBar = nullptr;
    plasma::Node *m_hpBar = nullptr;
    plasma::Node *m_hpBarFill = nullptr;
    plasma::Node *m_mpBar = nullptr;
    plasma::Node *m_mpBarFill = nullptr;
    plasma::Node *m_chargeBar = nullptr;
    std::vector<plasma::Node *> m_abilityButtons;
    std::vector<plasma::Node *> m_cooldowns;
    std::map<std::string, plasma::Texture *> m_icons;
    struct ShownIcon {
        std::string name;
        bool bright = false;
    };
    std::map<plasma::Node *, ShownIcon> m_shownIcons; // setIcon's last icon of each button
    SkillMenu *m_skillMenu = nullptr;
};

}
