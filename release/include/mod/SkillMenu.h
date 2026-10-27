#pragma once
#include "cube/widgets/BaseWidget.h"
#include <map>
#include <string>
#include <vector>

namespace cube {
class Game;
}
namespace plasma {
class Node;
}

namespace mod {

class AlphaHud;

// The alpha's skill screen (key X): the two specializations, the class's skill tree and the general skills from
// classes.json. The left button raises a skill while points are left (a skill needing another once that one has its
// points), the right button lowers it; Learn takes the changes, Cancel drops them. Giving points back and changing the
// specialization cost gold and need a trainer of the class nearby, as in the alpha.
class SkillMenu : public cube::BaseWidget {
public:
    // panelTemplate is the alpha's "blackwidget"; the buttons are clones of its "skillbutton", "childskillbutton" and
    // "specializationbutton".
    SkillMenu(cube::Game *game, AlphaHud *hud, plasma::Node *parent, plasma::Node *gui);
    bool isOpen() const;
    void toggle();
    void close();
    // Each frame: the key, the panel's place, the buttons' icons, levels and frames.
    void update();
    // A mouse button pressed (Game::handleButtonDown); true when the menu took it.
    bool handleButtonDown(int button);
    void draw() override;

private:
    struct Entry {
        std::string skill;
        std::string requires;
        int requiredPoints = 0;
        plasma::Node *button = nullptr;
    };

    void rebuild();
    int pending(const std::string &skill) const;
    bool changed() const;
    int cost() const;
    bool trainerNearby() const;
    bool canLearn() const;
    bool pointerOverLearn();
    bool pointerOverCancel();
    // The text in lines no wider than width, in the current text size.
    std::vector<std::wstring> wrap(const std::wstring &text, float width);

    cube::Game *m_game;
    AlphaHud *m_hud;
    plasma::Node *m_parent; // the in-game interface, parent of the panel and the buttons
    plasma::Node *m_panel = nullptr;
    float m_panelWidth = 0.0f; // the panel's size in interface units at the interface scale m_panelScale (update)
    float m_panelHeight = 0.0f;
    float m_panelScale = 1.0f;
    plasma::Node *m_skillTemplate = nullptr;
    plasma::Node *m_childSkillTemplate = nullptr;
    plasma::Node *m_specializationButtons[2] = {nullptr, nullptr};
    std::vector<Entry> m_entries;
    std::map<std::string, int> m_points; // the levels shown, which Learn takes
    int m_specialization = 0;            // the specialization shown
    int m_builtClass = -1;
    int m_builtSpecialization = -1;
    std::string m_hovered;
    bool m_keyWasDown = false;
};

}
