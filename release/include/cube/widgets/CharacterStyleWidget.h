// CharacterStyleWidget edits race, class, gender, face, haircut and hair colour during character creation. Each change
// plays a sound and updates the preview creature.
#pragma once
#include "cube/widgets/BaseWidget.h"
#include "plasma/Vector.h"

namespace plasma {
class Node;
}

namespace cube {

class Game;

class CharacterStyleWidget : public BaseWidget {
public:
    CharacterStyleWidget(Game *game, plasma::Node *node, plasma::Node *leftButton, plasma::Node *rightButton);

    virtual void draw();
    // Lays out the ten buttons in their rows.
    virtual void resized();

    // Button handlers update race (wrapping 0..7), class (wrapping 0..3) or gender, play a sound and call
    // updateCreature.
    void decreaseClass();
    void increaseClass();
    void selectHairColor(); // (current = preview)
    void decreaseFace();
    void increaseFace();
    // Two genders: both buttons toggle.
    void decreaseGender();
    void increaseGender();
    void decreaseHaircut();
    void increaseHaircut();
    void decreaseRace();
    void increaseRace();
    void reset();
    // Updates the preview creature from the selected appearance choices.
    void updateCreature(bool preview);

    // Public: cube::Game reads the choices when the character is created.
    Game *m_game;                       // Game used by the appearance handlers.
    plasma::Node *m_raceLeftButton;     // Used by decreaseRace.
    plasma::Node *m_raceRightButton;    // Used by increaseRace.
    plasma::Node *m_classLeftButton;    // Used by decreaseClass.
    plasma::Node *m_classRightButton;   // Used by increaseClass.
    plasma::Node *m_genderLeftButton;   // Used by decreaseGender.
    plasma::Node *m_genderRightButton;  // Used by increaseGender.
    plasma::Node *m_faceLeftButton;     // Used by decreaseFace.
    plasma::Node *m_faceRightButton;    // Used by increaseFace.
    plasma::Node *m_haircutLeftButton;  // Used by decreaseHaircut.
    plasma::Node *m_haircutRightButton; // Used by increaseHaircut.
    int m_race;                         // Race index, initially zero; changed by decreaseRace and increaseRace.
    int m_classType;                    // Class index, initially zero; changed by decreaseClass and increaseClass.
    int m_gender;                       // Gender index, initially zero; changed by both gender buttons.
    int m_face;                         // Face index, initially zero; changed by the face buttons.
    int m_haircut;                      // Haircut index, initially zero; changed by the haircut buttons.
    plasma::Vector<3, unsigned char> m_currentHairColor; // Hair colour selected by selectHairColor.
    plasma::Vector<3, unsigned char> m_previewHairColor; // Hair colour component selected by selectHairColor.
};

}
