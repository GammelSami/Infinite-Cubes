// CharacterStyleWidget edits the player appearance and class.
#include "cube/DefaultBlocks.h"
#include "cube/widgets/CharacterStyleWidget.h"
#include "cube/SpriteIds.h"
#include "plasma/Node.h"
#include "cube/GameController.h"
#include "cube/RaceIds.h"
#include "cube/SoundIds.h"
#include "plasma/Engine.h"
#include "plasma/Transformation.h"
#include "plasma/Display.h"
#include "plasma/SmoothMeshShape.h"
#include "plasma/FontEngine.h"
#include "plasma/Font.h"
#include <sstream>
#include <vector>
namespace cube {

// The hair colour starts golden (255, 200, 50). The palette's input area is 280 x 150 pixels at
// 10, 200.
CharacterStyleWidget::CharacterStyleWidget(plasma::Engine *engine, plasma::Node *node, GameController *game,
                                           plasma::Node *leftButton, plasma::Node *rightButton)
    : Widget(engine, node),
      m_game(game),
      m_race(CharacterRaceChoiceHuman),
      m_class(CharacterClassChoiceWarrior),
      m_gender(0),
      m_face(0),
      m_haircut(0),
      m_hairColor(255, 200, 50),
      m_previewHairColor(255, 255, 255)
{
    m_raceLeft = leftButton->instance(node);
    m_raceLeft->setTransformation(leftButton->m_transformation->clone());
    m_raceLeft->setDisplay(leftButton->m_display->clone());
    m_raceLeft->connectWidget(L"leftbutton", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                              &CharacterStyleWidget::decreaseRace, true);
    m_raceRight = rightButton->instance(node);
    m_raceRight->setTransformation(rightButton->m_transformation->clone());
    m_raceRight->setDisplay(rightButton->m_display->clone());
    m_raceRight->connectWidget(L"rightbutton", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                               &CharacterStyleWidget::increaseRace, true);
    m_classLeft = leftButton->instance(node);
    m_classLeft->setTransformation(leftButton->m_transformation->clone());
    m_classLeft->setDisplay(leftButton->m_display->clone());
    m_classLeft->connectWidget(L"leftbutton", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                               &CharacterStyleWidget::decreaseClass, true);
    m_classRight = rightButton->instance(node);
    m_classRight->setTransformation(rightButton->m_transformation->clone());
    m_classRight->setDisplay(rightButton->m_display->clone());
    m_classRight->connectWidget(L"rightbutton", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                &CharacterStyleWidget::increaseClass, true);
    m_genderLeft = leftButton->instance(node);
    m_genderLeft->setTransformation(leftButton->m_transformation->clone());
    m_genderLeft->setDisplay(leftButton->m_display->clone());
    m_genderLeft->connectWidget(L"leftbutton", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                &CharacterStyleWidget::decreaseGender, true);
    m_genderRight = rightButton->instance(node);
    m_genderRight->setTransformation(rightButton->m_transformation->clone());
    m_genderRight->setDisplay(rightButton->m_display->clone());
    m_genderRight->connectWidget(L"rightbutton", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                 &CharacterStyleWidget::decreaseGender, true);
    m_faceLeft = leftButton->instance(node);
    m_faceLeft->setTransformation(leftButton->m_transformation->clone());
    m_faceLeft->setDisplay(leftButton->m_display->clone());
    m_faceLeft->connectWidget(L"leftbutton", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                              &CharacterStyleWidget::decreaseFace, true);
    m_faceRight = rightButton->instance(node);
    m_faceRight->setTransformation(rightButton->m_transformation->clone());
    m_faceRight->setDisplay(rightButton->m_display->clone());
    m_faceRight->connectWidget(L"rightbutton", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                               &CharacterStyleWidget::increaseFace, true);
    m_haircutLeft = leftButton->instance(node);
    m_haircutLeft->setTransformation(leftButton->m_transformation->clone());
    m_haircutLeft->setDisplay(leftButton->m_display->clone());
    m_haircutLeft->connectWidget(L"leftbutton", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                 &CharacterStyleWidget::decreaseHaircut, true);
    m_haircutRight = rightButton->instance(node);
    m_haircutRight->setTransformation(rightButton->m_transformation->clone());
    m_haircutRight->setDisplay(rightButton->m_display->clone());
    m_haircutRight->connectWidget(L"rightbutton", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                  &CharacterStyleWidget::increaseHaircut, true);

    plasma::SmoothMeshShape *palette = engine->createSmoothMeshShape();
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
    palette->attributesChanged(plasma::ATTRIBUTES_CHANGED_ALL);
    plasma::Node *paletteNode = engine->createNode(0, palette, 0, node);
    plasma::Widget *paletteWidget = engine->createWidget(paletteNode);
    paletteWidget->connect(plasma::WIDGET_EVENT_POINTER_PRESSED, this, &CharacterStyleWidget::selectHairColor, true,
                           true);
    getTranslation(paletteNode->m_transformation) = plasma::Vector<2, float>(10.0f, 200.0f);
    paletteNode->m_transformation->attributesChanged(plasma::ATTRIBUTES_CHANGED_ALL);
}

// The character creation panel: each row's label at the left and the choice centred between its
// two arrow buttons (cyan under the pointer), then the hair colour palette: 16 base colours in
// columns of 7 shades, dark (rows 0..2) through the colour (row 3) to white (rows 4..6), 17 units
// apart. The shade under the pointer becomes the preview colour; the creature is updated when it
// changed. Its layout, in pixels: the labels at x 15 and the values centred in the width less 110
// right of x 100, the rows' baselines 30 apart from 27, the "Hair color" title at 182, the
// swatches 17 apart from 13, 197 (15 x 15 in a black frame); text 12 high, stroke 3. A dark
// shade's brightness is 0.2 plus 0.25 per row, a light one moves a quarter per row towards white.
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
    m_engine->getFontEngine()->renderText(L"resource1.dat", L"Race", 0.0f, 0.0f, 15.0f, 27.0f, 12.0f, 3.0f,
                                          plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f).m_elements,
                                          plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f).m_elements,
                                          plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
                                          plasma::TEXT_FLAGS_NONE, -1.0f, true);
    m_engine->getFontEngine()->renderText(L"resource1.dat", L"Race", 0.0f, 0.0f, 15.0f, 27.0f, 12.0f, 0.0f,
                                          plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f).m_elements,
                                          plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
                                          plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
                                          plasma::TEXT_FLAGS_NONE, -1.0f, true);
    m_engine->getFontEngine()->renderText(
        L"resource1.dat", text.str(), 0.0f, 0.0f, (getContentWidth() - 110.0f) * 0.5f + 100.0f, 27.0f, 12.0f, 3.0f,
        plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f).m_elements,
        plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f).m_elements,
        plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, (1 << plasma::TEXT_CENTER), -1.0f, true);
    m_engine->getFontEngine()->renderText(
        L"resource1.dat", text.str(), 0.0f, 0.0f, (getContentWidth() - 110.0f) * 0.5f + 100.0f, 27.0f, 12.0f, 0.0f,
        plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f).m_elements,
        plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
        plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, (1 << plasma::TEXT_CENTER), -1.0f, true);
    text.str(L"");
    if (m_gender)
        text << "Female";
    else
        text << "Male";
    m_engine->getFontEngine()->renderText(L"resource1.dat", L"Gender", 0.0f, 0.0f, 15.0f, 57.0f, 12.0f, 3.0f,
                                          plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f).m_elements,
                                          plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f).m_elements,
                                          plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
                                          plasma::TEXT_FLAGS_NONE, -1.0f, true);
    m_engine->getFontEngine()->renderText(L"resource1.dat", L"Gender", 0.0f, 0.0f, 15.0f, 57.0f, 12.0f, 0.0f,
                                          plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f).m_elements,
                                          plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
                                          plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
                                          plasma::TEXT_FLAGS_NONE, -1.0f, true);
    m_engine->getFontEngine()->renderText(
        L"resource1.dat", text.str(), 0.0f, 0.0f, (getContentWidth() - 110.0f) * 0.5f + 100.0f, 57.0f, 12.0f, 3.0f,
        plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f).m_elements,
        plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f).m_elements,
        plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, (1 << plasma::TEXT_CENTER), -1.0f, true);
    m_engine->getFontEngine()->renderText(
        L"resource1.dat", text.str(), 0.0f, 0.0f, (getContentWidth() - 110.0f) * 0.5f + 100.0f, 57.0f, 12.0f, 0.0f,
        plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f).m_elements,
        plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
        plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, (1 << plasma::TEXT_CENTER), -1.0f, true);
    text.str(L"");
    switch (m_class) {
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
    m_engine->getFontEngine()->renderText(L"resource1.dat", L"Class", 0.0f, 0.0f, 15.0f, 87.0f, 12.0f, 3.0f,
                                          plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f).m_elements,
                                          plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f).m_elements,
                                          plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
                                          plasma::TEXT_FLAGS_NONE, -1.0f, true);
    m_engine->getFontEngine()->renderText(L"resource1.dat", L"Class", 0.0f, 0.0f, 15.0f, 87.0f, 12.0f, 0.0f,
                                          plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f).m_elements,
                                          plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
                                          plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
                                          plasma::TEXT_FLAGS_NONE, -1.0f, true);
    m_engine->getFontEngine()->renderText(
        L"resource1.dat", text.str(), 0.0f, 0.0f, (getContentWidth() - 110.0f) * 0.5f + 100.0f, 87.0f, 12.0f, 3.0f,
        plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f).m_elements,
        plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f).m_elements,
        plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, (1 << plasma::TEXT_CENTER), -1.0f, true);
    m_engine->getFontEngine()->renderText(
        L"resource1.dat", text.str(), 0.0f, 0.0f, (getContentWidth() - 110.0f) * 0.5f + 100.0f, 87.0f, 12.0f, 0.0f,
        plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f).m_elements,
        plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
        plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, (1 << plasma::TEXT_CENTER), -1.0f, true);
    text.str(L"");
    text << L"Face " << m_face + 1;
    m_engine->getFontEngine()->renderText(L"resource1.dat", L"Face", 0.0f, 0.0f, 15.0f, 117.0f, 12.0f, 3.0f,
                                          plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f).m_elements,
                                          plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f).m_elements,
                                          plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
                                          plasma::TEXT_FLAGS_NONE, -1.0f, true);
    m_engine->getFontEngine()->renderText(L"resource1.dat", L"Face", 0.0f, 0.0f, 15.0f, 117.0f, 12.0f, 0.0f,
                                          plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f).m_elements,
                                          plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
                                          plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
                                          plasma::TEXT_FLAGS_NONE, -1.0f, true);
    m_engine->getFontEngine()->renderText(
        L"resource1.dat", text.str(), 0.0f, 0.0f, (getContentWidth() - 110.0f) * 0.5f + 100.0f, 117.0f, 12.0f, 3.0f,
        plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f).m_elements,
        plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f).m_elements,
        plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, (1 << plasma::TEXT_CENTER), -1.0f, true);
    m_engine->getFontEngine()->renderText(
        L"resource1.dat", text.str(), 0.0f, 0.0f, (getContentWidth() - 110.0f) * 0.5f + 100.0f, 117.0f, 12.0f, 0.0f,
        plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f).m_elements,
        plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
        plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, (1 << plasma::TEXT_CENTER), -1.0f, true);
    text.str(L"");
    text << L"Haircut " << m_haircut + 1;
    m_engine->getFontEngine()->renderText(L"resource1.dat", L"Haircut", 0.0f, 0.0f, 15.0f, 147.0f, 12.0f, 3.0f,
                                          plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f).m_elements,
                                          plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f).m_elements,
                                          plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
                                          plasma::TEXT_FLAGS_NONE, -1.0f, true);
    m_engine->getFontEngine()->renderText(L"resource1.dat", L"Haircut", 0.0f, 0.0f, 15.0f, 147.0f, 12.0f, 0.0f,
                                          plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f).m_elements,
                                          plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
                                          plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
                                          plasma::TEXT_FLAGS_NONE, -1.0f, true);
    m_engine->getFontEngine()->renderText(
        L"resource1.dat", text.str(), 0.0f, 0.0f, (getContentWidth() - 110.0f) * 0.5f + 100.0f, 147.0f, 12.0f, 3.0f,
        plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f).m_elements,
        plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f).m_elements,
        plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, (1 << plasma::TEXT_CENTER), -1.0f, true);
    m_engine->getFontEngine()->renderText(
        L"resource1.dat", text.str(), 0.0f, 0.0f, (getContentWidth() - 110.0f) * 0.5f + 100.0f, 147.0f, 12.0f, 0.0f,
        plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f).m_elements,
        plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
        plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, (1 << plasma::TEXT_CENTER), -1.0f, true);
    m_raceLeft->display()->setFillColor(m_raceLeft->widget()->isPointerOver()
                                            ? plasma::Vector<4, float>(0.0f, 1.0f, 1.0f, 1.0f)
                                            : plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f));
    m_raceRight->display()->setFillColor(m_raceRight->widget()->isPointerOver()
                                             ? plasma::Vector<4, float>(0.0f, 1.0f, 1.0f, 1.0f)
                                             : plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f));
    m_genderLeft->display()->setFillColor(m_genderLeft->widget()->isPointerOver()
                                              ? plasma::Vector<4, float>(0.0f, 1.0f, 1.0f, 1.0f)
                                              : plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f));
    m_genderRight->display()->setFillColor(m_genderRight->widget()->isPointerOver()
                                               ? plasma::Vector<4, float>(0.0f, 1.0f, 1.0f, 1.0f)
                                               : plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f));
    m_classLeft->display()->setFillColor(m_classLeft->widget()->isPointerOver()
                                             ? plasma::Vector<4, float>(0.0f, 1.0f, 1.0f, 1.0f)
                                             : plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f));
    m_classRight->display()->setFillColor(m_classRight->widget()->isPointerOver()
                                              ? plasma::Vector<4, float>(0.0f, 1.0f, 1.0f, 1.0f)
                                              : plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f));
    m_faceLeft->display()->setFillColor(m_faceLeft->widget()->isPointerOver()
                                            ? plasma::Vector<4, float>(0.0f, 1.0f, 1.0f, 1.0f)
                                            : plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f));
    m_faceRight->display()->setFillColor(m_faceRight->widget()->isPointerOver()
                                             ? plasma::Vector<4, float>(0.0f, 1.0f, 1.0f, 1.0f)
                                             : plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f));
    m_haircutLeft->display()->setFillColor(m_haircutLeft->widget()->isPointerOver()
                                               ? plasma::Vector<4, float>(0.0f, 1.0f, 1.0f, 1.0f)
                                               : plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f));
    m_haircutRight->display()->setFillColor(m_haircutRight->widget()->isPointerOver()
                                                ? plasma::Vector<4, float>(0.0f, 1.0f, 1.0f, 1.0f)
                                                : plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f));

    text.str(L"");
    text << L"Hair color";
    m_engine->getFontEngine()->renderText(L"resource1.dat", text.str(), 0.0f, 0.0f, getContentWidth() * 0.5f, 182.0f,
                                          12.0f, 3.0f, plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f).m_elements,
                                          plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f).m_elements,
                                          plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
                                          (1 << plasma::TEXT_CENTER), -1.0f, true);
    m_engine->getFontEngine()->renderText(L"resource1.dat", text.str(), 0.0f, 0.0f, getContentWidth() * 0.5f, 182.0f,
                                          12.0f, 0.0f, plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f).m_elements,
                                          plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
                                          plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
                                          (1 << plasma::TEXT_CENTER), -1.0f, true);

    plasma::Vector<3, unsigned char> previousPreview;
    previousPreview = m_previewHairColor;
    m_previewHairColor = m_hairColor;
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
            if (shade > 3)
                color += (plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f) - color) * ((shade - 3.0f) * 0.25f);
            int x = column * 17 + 13;
            m_engine->fillRectangle(plasma::Vector<2, float>((float)x, (float)y),
                                    plasma::Vector<2, float>(17.0f, 17.0f),
                                    plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f));
            m_engine->fillRectangle(plasma::Vector<2, float>((float)(x + 1), (float)(y + 1)),
                                    plasma::Vector<2, float>(15.0f, 15.0f), color);
            if (pointerX >= x && pointerY >= y && pointerX < x + 17 && pointerY < y + 17)
                m_previewHairColor = plasma::Vector<3, unsigned char>((unsigned char)(color[0] * 255.0f),
                                                                      (unsigned char)(color[1] * 255.0f),
                                                                      (unsigned char)(color[2] * 255.0f));
            shade++;
        }
    }
    if (m_previewHairColor != previousPreview)
        updateCreature(true);
}
}

namespace cube {

void CharacterStyleWidget::decreaseClass()
{
    m_game->playSound(SoundMenuSelect, m_game->m_cameraPosition, 1.0f, 1.0f);
    if (--m_class < CharacterClassChoiceWarrior)
        m_class = CharacterClassChoiceRogue;
    updateCreature(false);
}

void CharacterStyleWidget::increaseClass()
{
    m_game->playSound(SoundMenuSelect, m_game->m_cameraPosition, 1.0f, 1.0f);
    if (++m_class > CharacterClassChoiceRogue)
        m_class = CharacterClassChoiceWarrior;
    updateCreature(false);
}

void CharacterStyleWidget::selectHairColor()
{
    m_game->playSound(SoundMenuSelect, m_game->m_cameraPosition, 1.0f, 1.0f);
    m_hairColor = m_previewHairColor;
    updateCreature(false);
}

void CharacterStyleWidget::decreaseFace()
{
    m_game->playSound(SoundMenuSelect, m_game->m_cameraPosition, 1.0f, 1.0f);
    --m_face;
    updateCreature(false);
}

void CharacterStyleWidget::increaseFace()
{
    m_game->playSound(SoundMenuSelect, m_game->m_cameraPosition, 1.0f, 1.0f);
    ++m_face;
    updateCreature(false);
}

void CharacterStyleWidget::decreaseGender()
{
    m_game->playSound(SoundMenuSelect, m_game->m_cameraPosition, 1.0f, 1.0f);
    m_gender = (m_gender + 1) % 2;
    updateCreature(false);
}

void CharacterStyleWidget::decreaseHaircut()
{
    m_game->playSound(SoundMenuSelect, m_game->m_cameraPosition, 1.0f, 1.0f);
    --m_haircut;
    updateCreature(false);
}

void CharacterStyleWidget::increaseHaircut()
{
    m_game->playSound(SoundMenuSelect, m_game->m_cameraPosition, 1.0f, 1.0f);
    ++m_haircut;
    updateCreature(false);
}

void CharacterStyleWidget::decreaseRace()
{
    m_game->playSound(SoundMenuSelect, m_game->m_cameraPosition, 1.0f, 1.0f);
    if (--m_race < CharacterRaceChoiceHuman)
        m_race = CharacterRaceChoiceFrogman;
    updateCreature(false);
}

void CharacterStyleWidget::increaseRace()
{
    m_game->playSound(SoundMenuSelect, m_game->m_cameraPosition, 1.0f, 1.0f);
    if (++m_race > CharacterRaceChoiceFrogman)
        m_race = CharacterRaceChoiceHuman;
    updateCreature(false);
}

}

namespace cube {
// The arrows, in pixels: the left ones at x 100, the right ones 15 from the right edge, the rows
// 30 apart from y 12 (as draw's labels).
void CharacterStyleWidget::resized()
{
    m_raceLeft->m_widget->moveTo(100.0f, 12.0f, true);
    {
        plasma::Widget *widget = m_raceRight->m_widget;
        widget->moveTo(viewWidth() - widget->viewWidth() - 15.0f, 12.0f, true);
    }
    m_genderLeft->m_widget->moveTo(100.0f, 42.0f, true);
    {
        plasma::Widget *widget = m_genderRight->m_widget;
        widget->moveTo(viewWidth() - widget->viewWidth() - 15.0f, 42.0f, true);
    }
    m_classLeft->m_widget->moveTo(100.0f, 72.0f, true);
    {
        plasma::Widget *widget = m_classRight->m_widget;
        widget->moveTo(viewWidth() - widget->viewWidth() - 15.0f, 72.0f, true);
    }
    m_faceLeft->m_widget->moveTo(100.0f, 102.0f, true);
    {
        plasma::Widget *widget = m_faceRight->m_widget;
        widget->moveTo(viewWidth() - widget->viewWidth() - 15.0f, 102.0f, true);
    }
    m_haircutLeft->m_widget->moveTo(100.0f, 132.0f, true);
    {
        plasma::Widget *widget = m_haircutRight->m_widget;
        widget->moveTo(viewWidth() - widget->viewWidth() - 15.0f, 132.0f, true);
    }
}

// The choices of a new character: the first race, class and gender, the first face and haircut,
// and the default hair colours.
void CharacterStyleWidget::reset()
{
    m_gender = 0;
    m_race = CharacterRaceChoiceHuman;
    m_class = CharacterClassChoiceWarrior;
    m_haircut = 0;
    m_face = 0;
    m_hairColor = plasma::Vector<3, unsigned char>(255, 200, 50);
    m_previewHairColor = plasma::Vector<3, unsigned char>(255, 255, 255);
}

// Applies the chosen race, gender, class, face, haircut and hair colour to the player creature and
// rebuilds its appearance and starting equipment. Faces and haircuts wrap within the model ranges
// of the creature race. The parameter is not read.
void CharacterStyleWidget::updateCreature(bool)
{
    m_game->m_world.enterRegions();
    // The creature race of the chosen race and gender (male, female).
    switch (m_race) {
    case CharacterRaceChoiceHuman:
        m_game->m_player->m_entityData.race = m_gender + RaceHumanMale;
        break;
    case CharacterRaceChoiceElf:
        m_game->m_player->m_entityData.race = m_gender + RaceElfMale;
        break;
    case CharacterRaceChoiceDwarf:
        m_game->m_player->m_entityData.race = m_gender + RaceDwarfMale;
        break;
    case CharacterRaceChoiceOrc:
        m_game->m_player->m_entityData.race = m_gender + RaceOrcMale;
        break;
    case CharacterRaceChoiceGoblin:
        m_game->m_player->m_entityData.race = m_gender + RaceGoblinMale;
        break;
    case CharacterRaceChoiceLizard:
        m_game->m_player->m_entityData.race = m_gender + RaceLizardmanMale;
        break;
    case CharacterRaceChoiceUndead:
        m_game->m_player->m_entityData.race = m_gender + RaceUndeadMale;
        break;
    case CharacterRaceChoiceFrogman:
        m_game->m_player->m_entityData.race = m_gender + RaceFrogmanMale;
        break;
    }
    m_game->m_player->m_entityData.classType = m_class + 1;

    // The model ranges of the creature race: faces firstFace..lastFace, haircuts
    // firstHaircut..lastHaircut.
    int firstFace = SpriteHumanHeadM01;
    int lastFace = SpriteHumanHeadM06;
    int firstHaircut = SpriteHumanHairM01;
    int lastHaircut = SpriteHumanHairM15;
    switch (m_game->m_player->m_entityData.race) {
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
        lastHaircut = SpriteElfHairF10;
        break;
    case RaceHumanFemale:
        firstFace = SpriteHumanHeadF01;
        lastFace = SpriteHumanHeadF06;
        firstHaircut = SpriteHumanHairF01;
        lastHaircut = SpriteHumanHairF07;
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
        lastHaircut = SpriteDwarfHairF05;
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
        lastHaircut = SpriteOrcHairM10;
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
    int faces = lastFace - firstFace + 1;
    if (m_face < 0)
        m_face = faces - 1;
    if (m_face >= faces)
        m_face = 0;
    if (firstHaircut >= 0) {
        int haircuts = lastHaircut - firstHaircut + 1;
        if (m_haircut < 0)
            m_haircut = haircuts - 1;
        if (m_haircut >= haircuts)
            m_haircut = 0;
    }

    m_game->m_player->m_playerData->face = m_face;
    m_game->m_player->m_playerData->haircut = m_haircut;
    m_game->m_player->m_playerData->hairColor = m_previewHairColor;
    m_game->m_player->m_playerData->race = m_race;
    m_game->m_player->m_playerData->gender = m_gender;
    Creature::Appearance &appearance = m_game->m_player->m_entityData.appearance;
    appearance = Creature::Appearance();
    fillAppearance(m_game->m_player->m_entityData.race, m_game->m_player->m_entityData.appearance,
                   m_game->m_player->m_playerData);
    m_game->equipNewCharacter();
    m_game->m_world.leaveRegions();
}
}
