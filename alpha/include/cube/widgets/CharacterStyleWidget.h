// CharacterStyleWidget displays character appearance and class controls.
#pragma once
#include "plasma/Widget.h"
#include "cube/CreatureIds.h"
namespace cube {
class GameController;

enum CharacterClassChoice {
    CharacterClassChoiceWarrior,
    CharacterClassChoiceRanger,
    CharacterClassChoiceMage,
    CharacterClassChoiceRogue
};
class CharacterStyleWidget : public plasma::Widget {
public:
    CharacterStyleWidget(plasma::Engine *, plasma::Node *, GameController *, plasma::Node *leftButton,
                         plasma::Node *rightButton);
    virtual void draw();
    virtual void resized();
    void decreaseRace();
    void increaseRace();
    void decreaseClass();
    void increaseClass();
    void decreaseGender();
    void decreaseFace();
    void increaseFace();
    void decreaseHaircut();
    void increaseHaircut();
    void selectHairColor();
    void reset();
    void updateCreature(bool);
    GameController *m_game;
    plasma::Node *m_raceLeft;
    plasma::Node *m_raceRight;
    plasma::Node *m_classLeft;
    plasma::Node *m_classRight;
    plasma::Node *m_genderLeft;
    plasma::Node *m_genderRight;
    plasma::Node *m_faceLeft;
    plasma::Node *m_faceRight;
    plasma::Node *m_haircutLeft;
    plasma::Node *m_haircutRight;
    int m_race;   // wraps 0..7
    int m_class;  // wraps 0..3
    int m_gender; // (gender + 1) % 2
    int m_face;
    int m_haircut;
    plasma::Vector<3, unsigned char> m_hairColor;
    plasma::Vector<3, unsigned char> m_previewHairColor;
};
}
