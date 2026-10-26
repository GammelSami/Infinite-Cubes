// See cube/CharacterStyleWidget.h.
#include "plasma/TextIds.h"
#include <sstream>
#include <vector>
#include "cube/widgets/CharacterStyleWidget.h"
#include "cube/Creature.h"
#include "cube/Game.h"
#include "cube/World.h"
#include "plasma/D3D11Engine.h"
#include "plasma/Display.h"
#include "plasma/Engine.h"
#include "plasma/Node.h"
#include "plasma/SmoothMeshShape.h"
#include "plasma/Transformation.h"
#include "plasma/Widget.h"
#include "cube/CreatureIds.h"
#include "cube/SpriteIds.h"
#include "plasma/EditFilter.h"

namespace cube {

CharacterStyleWidget::CharacterStyleWidget(Game *game, plasma::Node *node, plasma::Node *leftButton,
                                           plasma::Node *rightButton)
    : BaseWidget(game->m_engine, node, L""),
      m_game(game),
      m_race(0),
      m_classType(0),
      m_gender(0),
      m_face(0),
      m_haircut(0),
      m_currentHairColor(0xff, 0xc8, 0x32),
      m_previewHairColor(0xff, 0xff, 0xff)
{
    // Each row has its own copies of the two button nodes, with their own transformation and display; the "leftbutton"
    // / "rightbutton" widget inside each copy calls the row's handler when pressed.
    m_raceLeftButton = leftButton->instance(node);
    m_raceLeftButton->setTransformation(leftButton->transformation()->clone());
    m_raceLeftButton->setDisplay(leftButton->display()->clone());
    m_raceLeftButton->connectWidget(L"leftbutton", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                    &CharacterStyleWidget::decreaseRace, true);
    m_raceRightButton = rightButton->instance(node);
    m_raceRightButton->setTransformation(rightButton->transformation()->clone());
    m_raceRightButton->setDisplay(rightButton->display()->clone());
    m_raceRightButton->connectWidget(L"rightbutton", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                     &CharacterStyleWidget::increaseRace, true);
    m_classLeftButton = leftButton->instance(node);
    m_classLeftButton->setTransformation(leftButton->transformation()->clone());
    m_classLeftButton->setDisplay(leftButton->display()->clone());
    m_classLeftButton->connectWidget(L"leftbutton", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                     &CharacterStyleWidget::decreaseClass, true);
    m_classRightButton = rightButton->instance(node);
    m_classRightButton->setTransformation(rightButton->transformation()->clone());
    m_classRightButton->setDisplay(rightButton->display()->clone());
    m_classRightButton->connectWidget(L"rightbutton", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                      &CharacterStyleWidget::increaseClass, true);
    m_genderLeftButton = leftButton->instance(node);
    m_genderLeftButton->setTransformation(leftButton->transformation()->clone());
    m_genderLeftButton->setDisplay(leftButton->display()->clone());
    m_genderLeftButton->connectWidget(L"leftbutton", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                      &CharacterStyleWidget::decreaseGender, true);
    m_genderRightButton = rightButton->instance(node);
    m_genderRightButton->setTransformation(rightButton->transformation()->clone());
    m_genderRightButton->setDisplay(rightButton->display()->clone());
    m_genderRightButton->connectWidget(L"rightbutton", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                       &CharacterStyleWidget::increaseGender, true);
    m_faceLeftButton = leftButton->instance(node);
    m_faceLeftButton->setTransformation(leftButton->transformation()->clone());
    m_faceLeftButton->setDisplay(leftButton->display()->clone());
    m_faceLeftButton->connectWidget(L"leftbutton", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                    &CharacterStyleWidget::decreaseFace, true);
    m_faceRightButton = rightButton->instance(node);
    m_faceRightButton->setTransformation(rightButton->transformation()->clone());
    m_faceRightButton->setDisplay(rightButton->display()->clone());
    m_faceRightButton->connectWidget(L"rightbutton", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                     &CharacterStyleWidget::increaseFace, true);
    m_haircutLeftButton = leftButton->instance(node);
    m_haircutLeftButton->setTransformation(leftButton->transformation()->clone());
    m_haircutLeftButton->setDisplay(leftButton->display()->clone());
    m_haircutLeftButton->connectWidget(L"leftbutton", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                       &CharacterStyleWidget::decreaseHaircut, true);
    m_haircutRightButton = rightButton->instance(node);
    m_haircutRightButton->setTransformation(rightButton->transformation()->clone());
    m_haircutRightButton->setDisplay(rightButton->display()->clone());
    m_haircutRightButton->connectWidget(L"rightbutton", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                        &CharacterStyleWidget::increaseHaircut, true);

    // The hair colour palette's hit area: an invisible 280 x 150 quad (four transparent vertices, one face, no
    // subdivision) whose widget calls selectHairColor when pressed. Render draws the colours over it.
    plasma::SmoothMeshShape *palette = m_engine->createSmoothMeshShape(L"");
    palette->m_vertexPositions.resize(4);
    palette->m_vertexColors.resize(4);
    palette->m_vertexPositions.value()[0] = plasma::Vector<2, float>(0.0f, 0.0f);
    palette->m_vertexPositions.value()[1] = plasma::Vector<2, float>(280.0f, 0.0f);
    palette->m_vertexPositions.value()[2] = plasma::Vector<2, float>(280.0f, 150.0f);
    palette->m_vertexPositions.value()[3] = plasma::Vector<2, float>(0.0f, 150.0f);
    palette->m_vertexColors.value()[0] = plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f);
    palette->m_vertexColors.value()[1] = plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f);
    palette->m_vertexColors.value()[2] = plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f);
    palette->m_vertexColors.value()[3] = plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f);
    std::vector<std::vector<int>> faces;
    faces.resize(1);
    faces.back().push_back(0);
    faces.back().push_back(1);
    faces.back().push_back(2);
    faces.back().push_back(3);
    palette->setFaces(faces, true);
    palette->setSubdivisions(0, true);
    palette->attributesChanged(true);
    plasma::Node *paletteNode = m_engine->createNode(nullptr, palette, nullptr, node, L"");
    plasma::Widget *paletteWidget = m_engine->createWidget(paletteNode, L"");
    paletteWidget->connect(plasma::WIDGET_EVENT_POINTER_PRESSED, this, &CharacterStyleWidget::selectHairColor, true,
                           true);
    paletteNode->transformation()->m_translation.value() = plasma::Vector<2, float>(10.0f, 200.0f);
    paletteNode->transformation()->attributesChanged(true);
    setFont(L"resource1.dat");
}

void CharacterStyleWidget::draw()
{
    std::wstringstream text;
    switch (m_race) {
    case CharacterRaceChoiceHuman:
        text << L"Human";
        break;
    case CharacterRaceChoiceElf:
        text << L"Elf";
        break;
    case CharacterRaceChoiceDwarf:
        text << L"Dwarf";
        break;
    case CharacterRaceChoiceOrc:
        text << L"Orc";
        break;
    case CharacterRaceChoiceGoblin:
        text << L"Goblin";
        break;
    case CharacterRaceChoiceLizard:
        text << L"Lizard";
        break;
    case CharacterRaceChoiceUndead:
        text << L"Undead";
        break;
    case CharacterRaceChoiceFrogman:
        text << L"Frogman";
        break;
    }
    setTextSize(12.0f);
    setTextStrokeRadius(3.0f);
    setTextStrokeColor(plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f));
    setTextColor(plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f));

    // Each row: its label at the left, the choice centred between the buttons (1 << plasma::TEXT_CENTER).
    setTextFlags(0);
    renderText(L"Race", 15.0f, 27.0f);
    setTextFlags(1 << plasma::TEXT_CENTER);
    renderText(text.str(), (getContentWidth() - 110.0f) * 0.5f + 100.0f, 27.0f);
    text.str(L"");
    if (m_gender)
        text << "Female";
    else
        text << "Male";
    setTextFlags(0);
    renderText(L"Gender", 15.0f, 57.0f);
    setTextFlags(1 << plasma::TEXT_CENTER);
    renderText(text.str(), (getContentWidth() - 110.0f) * 0.5f + 100.0f, 57.0f);
    text.str(L"");
    switch (m_classType) {
    case CharacterClassChoiceWarrior:
        text << L"Warrior";
        break;
    case CharacterClassChoiceRanger:
        text << L"Ranger";
        break;
    case CharacterClassChoiceMage:
        text << L"Mage";
        break;
    case CharacterClassChoiceRogue:
        text << L"Rogue";
        break;
    }
    setTextFlags(0);
    renderText(L"Class", 15.0f, 87.0f);
    setTextFlags(1 << plasma::TEXT_CENTER);
    renderText(text.str(), (getContentWidth() - 110.0f) * 0.5f + 100.0f, 87.0f);
    text.str(L"");
    text << L"Face " << m_face + 1;
    setTextFlags(0);
    renderText(L"Face", 15.0f, 117.0f);
    setTextFlags(1 << plasma::TEXT_CENTER);
    renderText(text.str(), (getContentWidth() - 110.0f) * 0.5f + 100.0f, 117.0f);
    text.str(L"");
    text << L"Haircut " << m_haircut + 1;
    setTextFlags(0);
    renderText(L"Haircut", 15.0f, 147.0f);
    setTextFlags(1 << plasma::TEXT_CENTER);
    renderText(text.str(), (getContentWidth() - 110.0f) * 0.5f + 100.0f, 147.0f);

    // A button under the pointer is drawn cyan, the others white.
    m_raceLeftButton->display()->m_fillColor.value() = m_raceLeftButton->widget()->isPointerOver()
                                                           ? plasma::Vector<4, float>(0.0f, 1.0f, 1.0f, 1.0f)
                                                           : plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f);
    m_raceRightButton->display()->m_fillColor.value() = m_raceRightButton->widget()->isPointerOver()
                                                            ? plasma::Vector<4, float>(0.0f, 1.0f, 1.0f, 1.0f)
                                                            : plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f);
    m_genderLeftButton->display()->m_fillColor.value() = m_genderLeftButton->widget()->isPointerOver()
                                                             ? plasma::Vector<4, float>(0.0f, 1.0f, 1.0f, 1.0f)
                                                             : plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f);
    m_genderRightButton->display()->m_fillColor.value() = m_genderRightButton->widget()->isPointerOver()
                                                              ? plasma::Vector<4, float>(0.0f, 1.0f, 1.0f, 1.0f)
                                                              : plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f);
    m_classLeftButton->display()->m_fillColor.value() = m_classLeftButton->widget()->isPointerOver()
                                                            ? plasma::Vector<4, float>(0.0f, 1.0f, 1.0f, 1.0f)
                                                            : plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f);
    m_classRightButton->display()->m_fillColor.value() = m_classRightButton->widget()->isPointerOver()
                                                             ? plasma::Vector<4, float>(0.0f, 1.0f, 1.0f, 1.0f)
                                                             : plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f);
    m_faceLeftButton->display()->m_fillColor.value() = m_faceLeftButton->widget()->isPointerOver()
                                                           ? plasma::Vector<4, float>(0.0f, 1.0f, 1.0f, 1.0f)
                                                           : plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f);
    m_faceRightButton->display()->m_fillColor.value() = m_faceRightButton->widget()->isPointerOver()
                                                            ? plasma::Vector<4, float>(0.0f, 1.0f, 1.0f, 1.0f)
                                                            : plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f);
    m_haircutLeftButton->display()->m_fillColor.value() = m_haircutLeftButton->widget()->isPointerOver()
                                                              ? plasma::Vector<4, float>(0.0f, 1.0f, 1.0f, 1.0f)
                                                              : plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f);
    m_haircutRightButton->display()->m_fillColor.value() = m_haircutRightButton->widget()->isPointerOver()
                                                               ? plasma::Vector<4, float>(0.0f, 1.0f, 1.0f, 1.0f)
                                                               : plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f);

    text.str(L"");
    text << L"Hair color";
    setTextFlags(1 << plasma::TEXT_CENTER);
    renderText(text.str(), getContentWidth() * 0.5f, 182.0f);

    // The palette: 16 base colours in columns, each in 7 shades from dark (rows 0..2) through the colour itself (row
    // 3) to white (rows 4..6), 17 pixels apart. The shade under the pointer becomes the preview colour; the creature
    // is updated when it changed.
    plasma::Vector<3, unsigned char> previousPreview = m_previewHairColor;
    m_previewHairColor = m_currentHairColor;
    std::vector<plasma::Vector<4, float>> baseColors;
    baseColors.push_back(plasma::Vector<4, float>(1.0f, 0.0f, 0.0f, 1.0f));
    baseColors.push_back(plasma::Vector<4, float>(1.0f, 0.25f, 0.0f, 1.0f));
    baseColors.push_back(plasma::Vector<4, float>(1.0f, 0.5f, 0.0f, 1.0f));
    baseColors.push_back(plasma::Vector<4, float>(1.0f, 1.0f, 0.0f, 1.0f));
    baseColors.push_back(plasma::Vector<4, float>(0.5f, 1.0f, 0.0f, 1.0f));
    baseColors.push_back(plasma::Vector<4, float>(0.0f, 1.0f, 0.0f, 1.0f));
    baseColors.push_back(plasma::Vector<4, float>(0.0f, 1.0f, 0.5f, 1.0f));
    baseColors.push_back(plasma::Vector<4, float>(0.0f, 1.0f, 1.0f, 1.0f));
    baseColors.push_back(plasma::Vector<4, float>(0.0f, 0.5f, 1.0f, 1.0f));
    baseColors.push_back(plasma::Vector<4, float>(0.0f, 0.25f, 1.0f, 1.0f));
    baseColors.push_back(plasma::Vector<4, float>(0.0f, 0.0f, 1.0f, 1.0f));
    baseColors.push_back(plasma::Vector<4, float>(0.25f, 0.0f, 1.0f, 1.0f));
    baseColors.push_back(plasma::Vector<4, float>(0.5f, 0.0f, 1.0f, 1.0f));
    baseColors.push_back(plasma::Vector<4, float>(0.75f, 0.0f, 1.0f, 1.0f));
    baseColors.push_back(plasma::Vector<4, float>(1.0f, 0.0f, 1.0f, 1.0f));
    baseColors.push_back(plasma::Vector<4, float>(0.5f, 0.5f, 0.5f, 1.0f));
    int pointerX = (int)getPointerOffset()[0];
    int pointerY = (int)getPointerOffset()[1];
    // Read but then replaced by the identity: the squares are drawn unscaled.
    plasma::Matrix<float> matrix;
    m_engine->getModelViewMatrix(matrix);
    matrix.identity();
    int x = 13;
    for (int column = 0; column < (int)baseColors.size(); column++) {
        int shade = 0;
        for (int y = 197; y < 316; y += 17) {
            plasma::Vector<4, float> color = baseColors[column];
            if (shade < 3) {
                float brightness = shade * 0.25f + 0.2f;
                color[0] *= brightness;
                color[1] *= brightness;
                color[2] *= brightness;
            }
            if (shade > 3) {
                float whiteness = (shade - 3.0f) * 0.25f;
                color += (plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f) - color) * whiteness;
            }
            plasma::Vector<2, float> &pivot = m_node->transformation()->m_pivot.value();
            m_engine->fillRectangle(
                plasma::Vector<2, float>((pivot[0] + x) * matrix.m_elements[0], (pivot[1] + y) * matrix.m_elements[0]),
                plasma::Vector<2, float>(matrix.m_elements[0] * 17.0f, matrix.m_elements[0] * 17.0f),
                plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f));
            plasma::Vector<2, float> &inner = m_node->transformation()->m_pivot.value();
            m_engine->fillRectangle(
                plasma::Vector<2, float>((inner[0] + (x + 1)) * matrix.m_elements[0],
                                         (inner[1] + (y + 1)) * matrix.m_elements[0]),
                plasma::Vector<2, float>(matrix.m_elements[0] * 15.0f, matrix.m_elements[0] * 15.0f), color);
            if (pointerX >= x && pointerY >= y && pointerX < x + 17 && pointerY < y + 17)
                m_previewHairColor = plasma::Vector<3, unsigned char>((unsigned char)(color[0] * 255.0f),
                                                                      (unsigned char)(color[1] * 255.0f),
                                                                      (unsigned char)(color[2] * 255.0f));
            shade++;
        }
        x += 17;
    }
    if (m_previewHairColor != previousPreview)
        updateCreature(true);
}

void CharacterStyleWidget::decreaseClass()
{
    m_game->playSound(SoundMenuSelect);
    if (--m_classType < CharacterClassChoiceWarrior)
        m_classType = CharacterClassChoiceRogue;
    updateCreature(false);
}

void CharacterStyleWidget::increaseClass()
{
    m_game->playSound(SoundMenuSelect);
    if (++m_classType > CharacterClassChoiceRogue)
        m_classType = CharacterClassChoiceWarrior;
    updateCreature(false);
}

void CharacterStyleWidget::selectHairColor()
{
    m_game->playSound(SoundMenuSelect);
    m_currentHairColor = m_previewHairColor;
    updateCreature(false);
}

void CharacterStyleWidget::decreaseFace()
{
    m_game->playSound(SoundMenuSelect);
    m_face--;
    updateCreature(false);
}

void CharacterStyleWidget::increaseFace()
{
    m_game->playSound(SoundMenuSelect);
    m_face++;
    updateCreature(false);
}

void CharacterStyleWidget::decreaseGender()
{
    m_game->playSound(SoundMenuSelect);
    m_gender = (m_gender + 1) % 2;
    updateCreature(false);
}

// The same code as decreaseGender.
void CharacterStyleWidget::increaseGender()
{
    m_game->playSound(SoundMenuSelect);
    m_gender = (m_gender + 1) % 2;
    updateCreature(false);
}

void CharacterStyleWidget::decreaseHaircut()
{
    m_game->playSound(SoundMenuSelect);
    m_haircut--;
    updateCreature(false);
}

void CharacterStyleWidget::increaseHaircut()
{
    m_game->playSound(SoundMenuSelect);
    m_haircut++;
    updateCreature(false);
}

void CharacterStyleWidget::decreaseRace()
{
    m_game->playSound(SoundMenuSelect);
    if (--m_race < CharacterRaceChoiceHuman)
        m_race = CharacterRaceChoiceFrogman;
    updateCreature(false);
}

void CharacterStyleWidget::increaseRace()
{
    m_game->playSound(SoundMenuSelect);
    if (++m_race > CharacterRaceChoiceFrogman)
        m_race = CharacterRaceChoiceHuman;
    updateCreature(false);
}

void CharacterStyleWidget::resized()
{
    // Five rows 30 apart: the left button at x 100, the right one 15 from the right edge.
    m_raceLeftButton->widget()->moveTo(100.0f, 12.0f, true);
    plasma::Widget *button = m_raceRightButton->widget();
    button->moveTo(viewWidth() - button->viewWidth() - 15.0f, 12.0f, true);
    m_genderLeftButton->widget()->moveTo(100.0f, 42.0f, true);
    button = m_genderRightButton->widget();
    button->moveTo(viewWidth() - button->viewWidth() - 15.0f, 42.0f, true);
    m_classLeftButton->widget()->moveTo(100.0f, 72.0f, true);
    button = m_classRightButton->widget();
    button->moveTo(viewWidth() - button->viewWidth() - 15.0f, 72.0f, true);
    m_faceLeftButton->widget()->moveTo(100.0f, 102.0f, true);
    button = m_faceRightButton->widget();
    button->moveTo(viewWidth() - button->viewWidth() - 15.0f, 102.0f, true);
    m_haircutLeftButton->widget()->moveTo(100.0f, 132.0f, true);
    button = m_haircutRightButton->widget();
    button->moveTo(viewWidth() - button->viewWidth() - 15.0f, 132.0f, true);
}

void CharacterStyleWidget::reset()
{
    m_classType = CharacterClassChoiceWarrior;
    m_gender = 0;
    m_race = CharacterRaceChoiceHuman;
    m_face = 0;
    m_haircut = 0;
    m_currentHairColor = plasma::Vector<3, unsigned char>(0xff, 0xc8, 0x32);
    m_previewHairColor = plasma::Vector<3, unsigned char>(0xff, 0xff, 0xff);
}

void CharacterStyleWidget::updateCreature(bool preview)
{
    m_game->m_world->enterZoneMeshes();
    // The creature race of the chosen race and gender (male, female).
    switch (m_race) {
    case CharacterRaceChoiceHuman:
        m_game->m_world->m_localCreature->m_entityData.race = m_gender + RaceHumanMale;
        break;
    case CharacterRaceChoiceElf:
        m_game->m_world->m_localCreature->m_entityData.race = m_gender;
        break;
    case CharacterRaceChoiceDwarf:
        m_game->m_world->m_localCreature->m_entityData.race = m_gender + RaceDwarfMale;
        break;
    case CharacterRaceChoiceOrc:
        m_game->m_world->m_localCreature->m_entityData.race = m_gender + RaceOrcMale;
        break;
    case CharacterRaceChoiceGoblin:
        m_game->m_world->m_localCreature->m_entityData.race = m_gender + RaceGoblinMale;
        break;
    case CharacterRaceChoiceLizard:
        m_game->m_world->m_localCreature->m_entityData.race = m_gender + RaceLizardmanMale;
        break;
    case CharacterRaceChoiceUndead:
        m_game->m_world->m_localCreature->m_entityData.race = m_gender + RaceUndeadMale;
        break;
    case CharacterRaceChoiceFrogman:
        m_game->m_world->m_localCreature->m_entityData.race = m_gender + RaceFrogmanMale;
        break;
    }
    m_game->m_world->m_localCreature->m_entityData.classType = m_classType + 1;

    // The model ranges of the creature race: faces firstFace..lastFace, haircuts firstHaircut..lastHaircut; the
    // choices wrap around them.
    int firstFace = SpriteHumanHeadM01;
    int lastFace = SpriteHumanHeadM06;
    int firstHaircut = SpriteHumanHairM01;
    int lastHaircut = SpriteHumanHairM15;
    switch (m_game->m_world->m_localCreature->m_entityData.race) {
    case RaceElfMale:
        firstFace = SpriteElfHeadM01;
        lastFace = SpriteElfHeadM04;
        firstHaircut = SpriteElfHairM01;
        lastHaircut = SpriteElfHairM10;
        break;
    case RaceElfFemale:
        firstFace = SpriteElfHeadF01;
        lastFace = SpriteElfHeadF06;
        firstHaircut = SpriteElfHairF01;
        lastHaircut = SpriteElfHairF11;
        break;
    case RaceHumanFemale:
        firstFace = SpriteHumanHeadF01;
        lastFace = SpriteHumanHeadF06;
        firstHaircut = SpriteHumanHairF01;
        lastHaircut = SpriteHumanHairF09;
        break;
    case RaceDwarfMale:
        firstFace = SpriteDwarfHeadM01;
        lastFace = SpriteDwarfHeadM05;
        firstHaircut = SpriteDwarfHairM01;
        lastHaircut = SpriteDwarfHairM03;
        break;
    case RaceDwarfFemale:
        firstFace = SpriteDwarfHeadF01;
        lastFace = SpriteDwarfHeadF05;
        firstHaircut = SpriteDwarfHairF01;
        lastHaircut = SpriteDwarfHairF06;
        break;
    case RaceGoblinMale:
        firstFace = SpriteGoblinHeadM01;
        lastFace = SpriteGoblinHeadM05;
        firstHaircut = SpriteGoblinHairM01;
        lastHaircut = SpriteGoblinHairM06;
        break;
    case RaceGoblinFemale:
        firstFace = SpriteGoblinHeadF01;
        lastFace = SpriteGoblinHeadF05;
        firstHaircut = SpriteGoblinHairF01;
        lastHaircut = SpriteGoblinHairF06;
        break;
    case RaceOrcMale:
        firstFace = SpriteOrcHeadM01;
        lastFace = SpriteOrcHeadM05;
        firstHaircut = SpriteOrcHairM01;
        lastHaircut = SpriteOrcHairM08;
        break;
    case RaceOrcFemale:
        firstFace = SpriteOrcHeadF01;
        lastFace = SpriteOrcHeadF05;
        firstHaircut = SpriteOrcHairF01;
        lastHaircut = SpriteOrcHairF04;
        break;
    case RaceLizardmanMale:
        firstFace = SpriteLizardHeadM01;
        lastFace = SpriteLizardHeadM02;
        firstHaircut = SpriteLizardHairM01;
        lastHaircut = SpriteLizardHairM06;
        break;
    case RaceLizardmanFemale:
        firstFace = SpriteLizardHeadF01;
        lastFace = SpriteLizardHeadF05;
        firstHaircut = SpriteLizardHairM01;
        lastHaircut = SpriteLizardHairM06;
        break;
    case RaceUndeadMale:
        firstFace = SpriteUndeadHeadM01;
        lastFace = SpriteUndeadHeadM06;
        firstHaircut = SpriteUndeadHairM01;
        lastHaircut = SpriteUndeadHairM06;
        break;
    case RaceUndeadFemale:
        firstFace = SpriteUndeadHeadF01;
        lastFace = SpriteUndeadHeadF06;
        firstHaircut = SpriteUndeadHairF01;
        lastHaircut = SpriteUndeadHairF06;
        break;
    case RaceFrogmanMale:
        firstFace = SpriteFrogmanHeadM01;
        lastFace = SpriteFrogmanHeadM05;
        firstHaircut = SpriteFrogmanHairM01;
        lastHaircut = SpriteFrogmanHairM05;
        break;
    case RaceFrogmanFemale:
        firstFace = SpriteFrogmanHeadF01;
        lastFace = SpriteFrogmanHeadF04;
        firstHaircut = SpriteFrogmanHairF01;
        lastHaircut = SpriteFrogmanHairF04;
        break;
    }
    int faces = lastFace - firstFace;
    if (m_face < 0)
        m_face = faces;
    if (m_face >= faces + 1)
        m_face = 0;
    if (firstHaircut >= 0) {
        int haircuts = lastHaircut - firstHaircut;
        if (m_haircut < 0)
            m_haircut = haircuts;
        if (m_haircut >= haircuts + 1)
            m_haircut = 0;
    }

    m_game->m_world->m_localCreature->m_playerData->face = m_face;
    m_game->m_world->m_localCreature->m_playerData->haircut = m_haircut;
    // Always the preview colour: the parameter is not read.
    m_game->m_world->m_localCreature->m_playerData->hairColor = m_previewHairColor;
    m_game->m_world->m_localCreature->m_playerData->race = m_race;
    m_game->m_world->m_localCreature->m_playerData->gender = m_gender;
    m_game->m_world->m_localCreature->m_entityData.appearance = Creature::EntityData::Appearance();
    Creature *creature = m_game->m_world->m_localCreature;
    fillAppearance(creature->m_entityData.race, creature->m_entityData.appearance, creature->m_playerData, false);
    m_game->equipNewCharacter();
    m_game->m_world->leaveZoneMeshes();
}

}
