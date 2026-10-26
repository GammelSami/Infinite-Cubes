// Builds and updates the UI; see cube/Interface.h. Layout values are pixel positions, extents, text sizes and RGBA
// colours for the named controls.
#include "cube/Interface.h"

#include <sstream>
#include <iomanip>
#include "cube/Common.h"
#include "cube/WorldMap.h"
#include <windows.h>
#include <mmsystem.h>

#include "cube/widgets/AdaptionWidget.h"
#include "cube/widgets/BaseWidget.h"
#include "cube/widgets/CharacterPreviewWidget.h"
#include "cube/widgets/CharacterStyleWidget.h"
#include "cube/widgets/CharacterWidget.h"
#include "cube/widgets/ChatWidget.h"
#include "cube/widgets/ControlsWidget.h"
#include "cube/Creature.h"
#include "cube/widgets/EnchantWidget.h"
#include "cube/widgets/FormulaDetailsWidget.h"
#include "cube/Game.h"
#include "cube/widgets/HelpWidget.h"
#include "cube/widgets/InventoryWidget.h"
#include "cube/widgets/MapOverlayWidget.h"
#include "cube/widgets/MultiplayerWidget.h"
#include "cube/Noise.h"
#include "cube/widgets/OptionsWidget.h"
#include "cube/widgets/PreviewWidget.h"
#include "cube/widgets/QuestionWidget.h"
#include "cube/widgets/SpeechWidget.h"
#include "cube/widgets/SpriteWidget.h"
#include "cube/widgets/StartMenuWidget.h"
#include "cube/widgets/SystemWidget.h"
#include "cube/widgets/VoxelWidget.h"
#include "cube/World.h"
#include "cube/WorldInfo.h"
#include "cube/widgets/WorldPreviewWidget.h"
#include "plasma/D3D11Engine.h"
#include "plasma/Display.h"
#include "plasma/Engine.h"
#include "plasma/Font.h"
#include "plasma/Matrix.h"
#include "plasma/Node.h"
#include "plasma/PLXStreamReader.h"
#include "plasma/TextShape.h"
#include "plasma/Transformation.h"
#include "plasma/Widget.h"
#include "cube/CreatureIds.h"
#include "cube/InterfaceIds.h"

namespace cube {

static const int TravelMapZoom = 4;

Interface::Interface(Game *game)
    : m_pointerY(0.0f),
      m_scale(1.0f),
      m_mapOpen(false),
      m_choosingShrine(false),
      m_selectedCharacter(-1),
      m_game(game),
      m_land()
{}

Interface::WorldSelection::WorldSelection()
    : m_scroll(0.0f),
      m_online(false)
{}

// The point (x, y) of the widget, in the units of its node, as a button centre on the screen.
void Interface::addButtonCenter(plasma::Widget *widget, int x, int y)
{
    plasma::Node *node = widget->m_node;
    plasma::Matrix<float> matrix;
    node->getMatrix(matrix);
    plasma::Vector<2, float> point =
        matrix.transform(plasma::Vector<2, float>((float)x, (float)y) + node->transformation()->m_pivot.value());
    m_buttonCenters.push_back(plasma::Vector<2, int>((int)point[0], (int)point[1]));
}

// Builds the whole widget tree. Every screen is a child of the engine's root: the start menu, the character selection
// and creation, the world selection and creation, and the in-game interface (m_inGame.m_node). The icons of icons.plx
// stay in a node without a parent (m_icons), as do the templates taken out of gui.plx (the life bars, "blackwidget",
// "speech", the buttons and "edit"), which are cloned or instanced where they are used. The three .plx files are
// sealed with the key of "PlasmaXGraphics".
void Interface::load(const std::vector<ResolutionInfo> *displayModes)
{
    m_textFx = m_game->m_engine->createNode(nullptr, nullptr, nullptr, m_game->m_engine->root(), L"textFX");
    m_lifeBars = m_game->m_engine->createNode(nullptr, nullptr, nullptr, m_game->m_engine->root(), L"lifebars");
    m_inGame.m_node = m_game->m_engine->createNode(nullptr, nullptr, nullptr, m_game->m_engine->root());
    m_startMenuNode = m_game->m_engine->createNode(nullptr, nullptr, nullptr, m_game->m_engine->root());
    m_startMenuNode->setVisible(true);
    m_characterSelection.m_node = m_game->m_engine->createNode(nullptr, nullptr, nullptr, m_game->m_engine->root());
    m_characterSelection.m_node->setVisible(false);
    m_characterCreationNode = m_game->m_engine->createNode(nullptr, nullptr, nullptr, m_game->m_engine->root());
    m_characterCreationNode->setVisible(false);
    m_worldSelection.m_node = m_game->m_engine->createNode(nullptr, nullptr, nullptr, m_game->m_engine->root());
    m_worldSelection.m_node->setVisible(false);
    m_worldCreationNode = m_game->m_engine->createNode(nullptr, nullptr, nullptr, m_game->m_engine->root());
    m_worldCreationNode->setVisible(false);

    unsigned __int64 key = plasma::keyFromPhrase("PlasmaXGraphics");
    std::vector<unsigned __int64> keys(1, key);

    // The icons: auras by their id, the rarity frames, abilities by their id.
    m_icons = m_game->m_engine->createNode(nullptr, nullptr, nullptr, nullptr);
    m_game->m_engine->readPlasmaFileIntoNode(L"icons.plx", m_icons, (1 << plasma::LOAD_HINTED_TEXT), nullptr, keys);
    m_auraIcons[BuffBulwark] = m_icons->findNode(L"aura:bulwark");
    m_auraIcons[BuffWarfrenzy] = m_icons->findNode(L"aura:warfrenzy");
    m_auraIcons[BuffCamouflage] = m_icons->findNode(L"aura:camouflage");
    m_auraIcons[BuffPoisoned] = m_icons->findNode(L"aura:poisoned");
    m_auraIcons[BuffManashield] = m_icons->findNode(L"aura:manashield");
    m_auraIcons[BuffInstantfire] = m_icons->findNode(L"aura:instantfire");
    m_auraIcons[BuffInstantshoot] = m_icons->findNode(L"aura:instantshoot");
    m_auraIcons[BuffDodge] = m_icons->findNode(L"aura:dodge");
    m_auraIcons[BuffSwiftness] = m_icons->findNode(L"aura:swiftness");
    m_auraIcons[BuffNinjutsu] = m_icons->findNode(L"aura:ninjutsu");
    m_auraIcons[BuffShadowclone] = m_icons->findNode(L"aura:shadowclone");
    m_auraIcons[BuffFlamerush] = m_icons->findNode(L"aura:flamerush");
    m_auraIcons[BuffRegeneration] = m_icons->findNode(L"aura:regeneration");
    m_auraIcons[BuffSpiritworld] = m_icons->findNode(L"aura:spiritworld");
    m_auraIcons[BuffLifeflask] = m_icons->findNode(L"aura:lifeflask");
    m_auraIcons[BuffAttackflask] = m_icons->findNode(L"aura:attackflask");
    m_auraIcons[BuffArmorflask] = m_icons->findNode(L"aura:armorflask");
    m_auraIcons[BuffResistanceflask] = m_icons->findNode(L"aura:resistanceflask");
    m_auraIcons[BuffColdresistance] = m_icons->findNode(L"aura:coldresistance");
    m_auraIcons[BuffHeatresistance] = m_icons->findNode(L"aura:heatresistance");
    m_auraIcons[BuffPlagueresistance] = m_icons->findNode(L"aura:plagueresistance");
    m_auraIcons[BuffFrozen] = m_icons->findNode(L"aura:frozen");
    m_auraIcons[BuffAncientpower] = m_icons->findNode(L"aura:ancientpower");
    m_auraIcons[BuffManaabsorption] = m_icons->findNode(L"aura:manaabsorption");
    m_weaponRarity[0] = m_icons->findNode(L"weaponrarity0");
    m_weaponRarity[1] = m_icons->findNode(L"weaponrarity1");
    m_weaponRarity[2] = m_icons->findNode(L"weaponrarity2");
    m_weaponRarity[3] = m_icons->findNode(L"weaponrarity3");
    m_weaponRarity[4] = m_icons->findNode(L"weaponrarity4");
    m_weaponRarity[5] = m_icons->findNode(L"weaponrarity5");
    m_armorRarity[0] = m_icons->findNode(L"armorrarity0");
    m_armorRarity[1] = m_icons->findNode(L"armorrarity1");
    m_armorRarity[2] = m_icons->findNode(L"armorrarity2");
    m_armorRarity[3] = m_icons->findNode(L"armorrarity3");
    m_armorRarity[4] = m_icons->findNode(L"armorrarity4");
    m_armorRarity[5] = m_icons->findNode(L"armorrarity5");
    m_abilityIcons[AbilitySmash] = m_icons->findNode(L"smash");
    m_abilityIcons[AbilityCyclone] = m_icons->findNode(L"cyclone");
    m_abilityIcons[AbilityWarFrenzy] = m_icons->findNode(L"warfrenzy");
    m_abilityIcons[AbilityRockFist] = m_icons->findNode(L"rock-fist");

    // The interface's own graphics.
    plasma::Node *gui = m_game->m_engine->createNode(nullptr, nullptr, nullptr, nullptr);
    m_game->m_engine->readPlasmaFileIntoNode(L"gui.plx", gui, (1 << plasma::LOAD_HINTED_TEXT), nullptr, keys);
    m_game->m_engine->readPlasmaFileIntoNode(L"quest-tag.plx", gui, (1 << plasma::LOAD_HINTED_TEXT), nullptr, keys);

    // The start menu and its "Please wait..." text.
    m_startMenuButtons = m_game->m_engine->createNode(nullptr, nullptr, nullptr, m_startMenuNode);
    m_startMenuWidget = new StartMenuWidget(m_game, m_startMenuButtons);
    m_startMenuWidget->resizeTo(200.0f, 100.0f, true);
    plasma::TextShape *wait = m_game->m_engine->createTextShape(L"", L"");
    wait->m_color = plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f);
    wait->m_pixelSize = 12.0f;
    wait->m_strokeColor = plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f);
    wait->m_strokeRadius = 3.0f;
    wait->m_lineSpacing = 3.0f;
    wait->m_string = L"Please wait...";
    wait->m_fontName = L"resource1.dat";
    wait->m_flags = (1 << plasma::TEXT_CENTER);
    wait->attributesChanged(true);
    m_wait = m_game->m_engine->createNode(nullptr, wait, nullptr, m_game->m_engine->root(), L"wait");

    m_inGame.m_auraBar = m_game->m_engine->createNode(nullptr, nullptr, nullptr, m_inGame.m_node);
    m_inGame.m_comboPoints = gui->findNode(L"combopoints");
    m_inGame.m_comboPoints->setParent(m_inGame.m_node);

    // The map icons stay where gui.plx put them.
    m_enemyIcon = gui->findNode(L"enemy");
    m_monsterIcon = gui->findNode(L"monster");
    m_crystalIcon = gui->findNode(L"crystal");
    m_brazierIcon = gui->findNode(L"brazier");
    m_manaPumpIcon = gui->findNode(L"manapump");
    m_npcIcon = gui->findNode(L"npc");
    m_staticIcon = gui->findNode(L"static");
    m_objectIcon = gui->findNode(L"object");
    m_starIcon = gui->findNode(L"star");
    m_crossIcon = gui->findNode(L"cross");
    m_flightPointIcon = gui->findNode(L"flight-point");
    m_unknownFlightPointIcon = gui->findNode(L"unknown-flight-point");
    m_homeIcon = gui->findNode(L"home");
    m_mapPositionIcon = gui->findNode(L"mapposition");
    m_directionIcon = gui->findNode(L"direction");
    m_armorShopIcon = gui->findNode(L"armor-shop");
    m_weaponShopIcon = gui->findNode(L"weapon-shop");
    m_itemShopIcon = gui->findNode(L"item-shop");
    m_identifierIcon = gui->findNode(L"identifier");
    m_smithyIcon = gui->findNode(L"smithy");
    m_carpentersShopIcon = gui->findNode(L"carpenters-shop");
    m_tailorsShopIcon = gui->findNode(L"tailors-shop");
    m_innIcon = gui->findNode(L"inn");
    m_shrineIcon = gui->findNode(L"shrine");
    m_spiritIcon = gui->findNode(L"spirit");
    m_bellIcon = gui->findNode(L"bell");
    m_harpIcon = gui->findNode(L"harp");
    m_whistleIcon = gui->findNode(L"whistle");
    m_solvedIcon = gui->findNode(L"solved");
    m_gnomeIcon = gui->findNode(L"gnome");
    m_hangGliderIcon = gui->findNode(L"hangglider");
    m_boatIcon = gui->findNode(L"boat");
    m_climbingSpikesIcon = gui->findNode(L"climbingspikes");
    m_reinsIcon = gui->findNode(L"reins");
    m_destinationIcon = gui->findNode(L"destination");
    m_crafting1Icon = gui->findNode(L"crafting1");
    m_crafting2Icon = gui->findNode(L"crafting2");
    m_crafting3Icon = gui->findNode(L"crafting3");
    m_crafting4Icon = gui->findNode(L"crafting4");
    m_flightMasterIcon = gui->findNode(L"flight-master");
    m_guildHallIcon = gui->findNode(L"guild-hall");
    m_flowerIcon = gui->findNode(L"flower");
    m_keyIcon = gui->findNode(L"key");
    m_treasureIcon = gui->findNode(L"treasure");
    m_witchIcon = gui->findNode(L"witch");
    m_demonPortalIcon = gui->findNode(L"demonportal");

    // The templates, detached from gui.plx.
    m_enemyLifeBarSmall = gui->findNode(L"enemylifebar:small");
    m_enemyLifeBarSmall->setParent(nullptr);
    m_friendLifeBarSmall = gui->findNode(L"friendlifebar:small");
    m_friendLifeBarSmall->setParent(nullptr);
    m_staticLifeBarSmall = gui->findNode(L"staticlifebar:small");
    m_staticLifeBarSmall->setParent(nullptr);
    m_neutralLifeBarSmall = gui->findNode(L"neutrallifebar:small");
    m_neutralLifeBarSmall->setParent(nullptr);
    m_enemyLifeBar = gui->findNode(L"enemylifebar");
    m_enemyLifeBar->setParent(nullptr);
    m_friendLifeBar = gui->findNode(L"friendlifebar");
    m_friendLifeBar->setParent(nullptr);
    m_staticLifeBar = gui->findNode(L"staticlifebar");
    m_staticLifeBar->setParent(nullptr);
    m_neutralLifeBar = gui->findNode(L"neutrallifebar");
    m_neutralLifeBar->setParent(nullptr);
    m_blackWidget = gui->findNode(L"blackwidget");
    m_blackWidget->setParent(nullptr);
    m_speech = gui->findNode(L"speech");
    m_speech->setParent(nullptr);
    plasma::Node *leftButton = gui->findNode(L"leftbutton");
    leftButton->setParent(nullptr);
    plasma::Node *rightButton = gui->findNode(L"rightbutton");
    rightButton->setParent(nullptr);
    plasma::Node *upButton = gui->findNode(L"upbutton");
    upButton->setParent(nullptr);
    plasma::Node *downButton = gui->findNode(L"downbutton");
    downButton->setParent(nullptr);
    plasma::Node *scrollButton = gui->findNode(L"scrollbutton");
    scrollButton->setParent(nullptr);
    plasma::Node *edit = gui->findNode(L"edit");
    edit->setParent(nullptr);
    // The button templates' labels and frames must not take the pointer.
    plasma::Node *button = gui->findNode(L"button");
    for (auto &child : button->m_children)
        child->setHitTestable(false);
    button->setParent(nullptr);
    plasma::Node *button2 = gui->findNode(L"button2");
    for (auto &child : button2->m_children)
        child->setHitTestable(false);
    button2->setParent(nullptr);
    plasma::Node *smallButton = gui->findNode(L"smallbutton");
    for (auto &child : smallButton->m_children)
        child->setHitTestable(false);
    smallButton->setParent(nullptr);

    // The options and controls panels, on the root.
    m_optionsWidget =
        new OptionsWidget(m_game, m_blackWidget->instance(m_game->m_engine->root()), leftButton, rightButton, button2);
    m_optionsWidget->m_resolutions = *displayModes;
    m_optionsWidget->setOptions(m_game->m_options);
    m_optionsWidget->resizeTo(450.0f, 560.0f, true);
    m_optionsWidget->getNode()->setVisible(false);
    m_controlsWidget = new ControlsWidget(m_game, m_blackWidget->instance(m_game->m_engine->root()), button2, upButton,
                                          downButton, scrollButton);
    m_controlsWidget->resizeTo(500.0f, 600.0f, true);
    m_controlsWidget->getNode()->setVisible(false);

    m_backButton = button->clone(m_game->m_engine->root());
    m_backButton->widget()->resizeTo(200.0f, 40.0f, true);
    m_backButton->setText(L"Back");
    m_backButton->widget()->connect(plasma::WIDGET_EVENT_POINTER_PRESSED, m_game, &Game::onBack, true, true);

    // The character selection.
    m_characterSelection.m_selectCharacterButton = button2->clone(m_characterSelection.m_node);
    m_characterSelection.m_selectCharacterButton->widget()->resizeTo(200.0f, 30.0f, true);
    m_characterSelection.m_selectCharacterButton->setText(L"Select");
    m_characterSelection.m_selectCharacterButton->widget()->connect(plasma::WIDGET_EVENT_POINTER_RELEASED, m_game,
                                                                    &Game::onSelectCharacter, true, true);
    m_characterSelection.m_editCharacterButton = smallButton->clone(m_characterSelection.m_node);
    m_characterSelection.m_editCharacterButton->widget()->resizeTo(20.0f, 20.0f, true);
    m_characterSelection.m_editCharacterButton->widget()->connect(plasma::WIDGET_EVENT_POINTER_RELEASED, m_game,
                                                                  &Game::onEditCharacter, true, true);
    m_characterSelection.m_editCharacterButton->setVisible(false);
    m_characterSelection.m_deleteCharacterButton = button->clone(m_characterSelection.m_node);
    m_characterSelection.m_deleteCharacterButton->widget()->resizeTo(300.0f, 30.0f, true);
    m_characterSelection.m_deleteCharacterButton->setText(L"Delete Character");
    m_characterSelection.m_deleteCharacterButton->widget()->connect(plasma::WIDGET_EVENT_POINTER_RELEASED, m_game,
                                                                    &Game::onDeleteCharacter, true, true);
    m_characterSelection.m_deleteCharacterButton->setVisible(false);

    // The character creation: its button, the style panel and the name.
    m_createCharacterButton = button->clone(m_characterCreationNode);
    m_createCharacterButton->widget()->resizeTo(300.0f, 40.0f, true);
    m_createCharacterButton->setText(L"Create Character");
    m_createCharacterButton->widget()->connect(plasma::WIDGET_EVENT_POINTER_RELEASED, m_game, &Game::onDoneCharacter,
                                               true, true);
    m_characterStyleNode = m_blackWidget->instance(m_characterCreationNode);
    m_characterStyleNode->setTransformation(m_characterStyleNode->transformation()->clone());
    m_characterStyleNode->setDisplay(m_characterStyleNode->display()->clone());
    m_characterStyleNode->widget()->resizeTo(300.0f, 330.0f, true);
    m_characterStyleWidget = new CharacterStyleWidget(
        m_game, m_game->m_engine->createNode(nullptr, nullptr, nullptr, nullptr), leftButton, rightButton);
    m_characterStyleNode->moveIntoWidget(m_blackWidget->getName(), m_characterStyleWidget, true);

    m_inGame.m_goldText = createText(L"100", L"gold", m_inGame.m_node, 0, 24, 5);
    m_inGame.m_contextItemText = createText(L"100", L"contextitem", m_inGame.m_node, 0, 24, 5);

    // The world selection.
    m_worldSelection.m_selectWorldButton = button2->clone(m_worldSelection.m_node);
    m_worldSelection.m_selectWorldButton->widget()->resizeTo(200.0f, 30.0f, true);
    m_worldSelection.m_selectWorldButton->setText(L"Select");
    m_worldSelection.m_selectWorldButton->widget()->connect(plasma::WIDGET_EVENT_POINTER_RELEASED, m_game,
                                                            &Game::onSelectWorld, true, true);
    m_worldSelection.m_editWorldButton = smallButton->clone(m_worldSelection.m_node);
    m_worldSelection.m_editWorldButton->widget()->resizeTo(20.0f, 20.0f, true);
    m_worldSelection.m_editWorldButton->widget()->connect(plasma::WIDGET_EVENT_POINTER_RELEASED, m_game,
                                                          &Game::onEditWorld, true, true);
    m_worldSelection.m_editWorldButton->setVisible(false);
    m_worldSelection.m_deleteWorldButton = button->clone(m_worldSelection.m_node);
    m_worldSelection.m_deleteWorldButton->widget()->resizeTo(300.0f, 30.0f, true);
    m_worldSelection.m_deleteWorldButton->setText(L"Delete World");
    m_worldSelection.m_deleteWorldButton->widget()->connect(plasma::WIDGET_EVENT_POINTER_RELEASED, m_game,
                                                            &Game::onDeleteWorld, true, true);
    m_worldSelection.m_deleteWorldButton->setVisible(false);

    // The world creation: the name and seed fields, each a stroked label with a plain copy as its child and the "edit"
    // template cloned below its position, and the button.
    plasma::TextShape *stroke = m_game->m_engine->createTextShape(std::wstring(), L"");
    stroke->m_color = plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f);
    stroke->m_pixelSize = 12.0f;
    stroke->m_strokeColor = plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f);
    stroke->m_strokeRadius = 3.0f;
    stroke->m_lineSpacing = 3.0f;
    stroke->m_string = L"Name";
    stroke->m_flags = (1 << plasma::TEXT_CENTER);
    stroke->m_fontName = L"resource1.dat";
    stroke->attributesChanged(true);
    m_worldName = m_game->m_engine->createNode(nullptr, stroke, nullptr, m_worldCreationNode, L"worldName");
    plasma::TextShape *fill = m_game->m_engine->createTextShape(std::wstring(), L"");
    fill->m_color = plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f);
    fill->m_pixelSize = stroke->m_pixelSize;
    fill->m_lineSpacing = stroke->m_lineSpacing;
    fill->m_string = stroke->m_string;
    fill->m_flags = (1 << plasma::TEXT_CENTER);
    fill->m_fontName = L"resource1.dat";
    fill->attributesChanged(true);
    m_game->m_engine->createNode(nullptr, fill, nullptr, m_worldName, L"worldName");
    plasma::Node *worldNameEdit = edit->clone(m_worldName);
    worldNameEdit->widget()->moveTo(m_worldName->transformation()->m_translation.value()[0] -
                                        worldNameEdit->widget()->viewWidth() * 0.5f,
                                    m_worldName->transformation()->m_translation.value()[1] + 10.0f, true);
    m_worldName->setText(L"edit", L"", true);

    stroke = m_game->m_engine->createTextShape(std::wstring(), L"");
    stroke->m_color = plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f);
    stroke->m_pixelSize = 12.0f;
    stroke->m_strokeColor = plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f);
    stroke->m_strokeRadius = 3.0f;
    stroke->m_lineSpacing = 3.0f;
    stroke->m_string = L"Seed (must be a number)";
    stroke->m_flags = (1 << plasma::TEXT_CENTER);
    stroke->m_fontName = L"resource1.dat";
    stroke->attributesChanged(true);
    m_worldSeed = m_game->m_engine->createNode(nullptr, stroke, nullptr, m_worldCreationNode, L"worldSeed");
    fill = m_game->m_engine->createTextShape(std::wstring(), L"");
    fill->m_color = plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f);
    fill->m_pixelSize = stroke->m_pixelSize;
    fill->m_lineSpacing = stroke->m_lineSpacing;
    fill->m_string = stroke->m_string;
    fill->m_flags = (1 << plasma::TEXT_CENTER);
    fill->m_fontName = L"resource1.dat";
    fill->attributesChanged(true);
    m_game->m_engine->createNode(nullptr, fill, nullptr, m_worldSeed, L"worldSeed");
    plasma::Node *worldSeedEdit = edit->clone(m_worldSeed);
    worldSeedEdit->widget()->moveTo(m_worldSeed->transformation()->m_translation.value()[0] -
                                        worldSeedEdit->widget()->viewWidth() * 0.5f,
                                    m_worldSeed->transformation()->m_translation.value()[1] + 10.0f, true);
    m_worldSeed->setText(L"edit", L"", true);

    m_createWorldButton = button->clone(m_worldCreationNode);
    m_createWorldButton->widget()->resizeTo(300.0f, 40.0f, true);
    m_createWorldButton->setText(L"Create World");
    m_createWorldButton->widget()->connect(plasma::WIDGET_EVENT_POINTER_RELEASED, m_game, &Game::onDoneWorld, true,
                                           true);

    // The character creation's name field, in the same way.
    stroke = m_game->m_engine->createTextShape(std::wstring(), L"");
    stroke->m_color = plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f);
    stroke->m_pixelSize = 12.0f;
    stroke->m_strokeColor = plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f);
    stroke->m_strokeRadius = 3.0f;
    stroke->m_lineSpacing = 3.0f;
    stroke->m_string = L"Name";
    stroke->m_flags = (1 << plasma::TEXT_CENTER);
    stroke->m_fontName = L"resource1.dat";
    stroke->attributesChanged(true);
    m_characterName = m_game->m_engine->createNode(nullptr, stroke, nullptr, m_characterCreationNode, L"charactername");
    fill = m_game->m_engine->createTextShape(std::wstring(), L"");
    fill->m_color = plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f);
    fill->m_pixelSize = stroke->m_pixelSize;
    fill->m_lineSpacing = stroke->m_lineSpacing;
    fill->m_string = stroke->m_string;
    fill->m_flags = (1 << plasma::TEXT_CENTER);
    fill->m_fontName = L"resource1.dat";
    fill->attributesChanged(true);
    m_game->m_engine->createNode(nullptr, fill, nullptr, m_characterName, L"charactername");
    plasma::Node *characterNameEdit = edit->clone(m_characterName);
    characterNameEdit->widget()->moveTo(m_characterName->transformation()->m_translation.value()[0] -
                                            characterNameEdit->widget()->viewWidth() * 0.5f,
                                        m_characterName->transformation()->m_translation.value()[1] + 10.0f, true);
    m_characterName->setText(L"edit", L"", true);

    m_worldSelection.m_multiplayerWorldsButton = button->clone(m_game->m_engine->root());
    m_worldSelection.m_multiplayerWorldsButton->widget()->resizeTo(300.0f, 40.0f, true);
    m_worldSelection.m_multiplayerWorldsButton->widget()->moveTo(40.0f, 40.0f, true);
    m_worldSelection.m_multiplayerWorldsButton->setText(L"Multiplayer Worlds...");
    m_worldSelection.m_multiplayerWorldsButton->widget()->connect(plasma::WIDGET_EVENT_POINTER_RELEASED, m_game,
                                                                  &Game::onToggleWorlds, true, true);

    // The map overlay and the land buttons beside the "recentlands" text.
    m_inGame.m_mapOverlayWidget =
        new MapOverlayWidget(m_game, m_game->m_engine->createNode(nullptr, nullptr, nullptr, m_game->m_engine->root()));
    m_inGame.m_recentLandsText = createText(L"land", L"recentlands", m_inGame.m_node, 0, 16, 3);
    std::list<plasma::TextShape *> shapes;
    m_inGame.m_recentLandsText->collectShapes(shapes);
    for (plasma::TextShape *shape : shapes) {
        shape->m_flags = (1 << plasma::TEXT_CENTER) | (1 << plasma::TEXT_MIDDLE) | (1 << plasma::TEXT_WRAP);
        shape->m_wrapWidth = 180.0f;
    }
    m_inGame.m_leftLandButton = leftButton->clone(m_inGame.m_recentLandsText);
    plasma::Transformation *transformation = m_inGame.m_leftLandButton->transformation();
    transformation->m_translation.value() = plasma::Vector<2, float>(-100.0f, 2.0f) - transformation->m_pivot.value();
    m_inGame.m_leftLandButton->transformation()->attributesChanged(true);
    m_inGame.m_rightLandButton = rightButton->clone(m_inGame.m_recentLandsText);
    transformation = m_inGame.m_rightLandButton->transformation();
    transformation->m_translation.value() = plasma::Vector<2, float>(100.0f, 2.0f) - transformation->m_pivot.value();
    m_inGame.m_rightLandButton->transformation()->attributesChanged(true);
    m_inGame.m_leftLandButton->connectWidget(L"leftbutton", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                             &Interface::nextLand, true);
    m_inGame.m_rightLandButton->connectWidget(L"rightbutton", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                              &Interface::lastLand, true);

    // The bars.
    m_inGame.m_quickBar = m_game->m_engine->createNode(nullptr, nullptr, nullptr, m_inGame.m_node);
    m_inGame.m_levelText = createText(L"LV 1", L"playername", m_inGame.m_node, 0, 24, 5);
    m_inGame.m_levelText->setVisible(false);
    m_inGame.m_levelText->setParent(m_inGame.m_node);
    m_inGame.m_levelInfo = gui->findNode(L"levelinfo");
    m_inGame.m_levelInfo->setParent(m_inGame.m_node);
    m_inGame.m_levelInfo->setVisible(false);
    m_inGame.m_xpBar = m_inGame.m_levelInfo->findNode(L"xpbar");
    m_inGame.m_hpBar = gui->findNode(L"hpbar");
    m_inGame.m_hpBar->setParent(m_inGame.m_quickBar);
    m_inGame.m_hpBarFill = m_inGame.m_hpBar->findNode(L"bar");
    m_inGame.m_mpBar = gui->findNode(L"mpbar");
    m_inGame.m_mpBar->setParent(m_inGame.m_quickBar);
    m_inGame.m_mpBarFill = m_inGame.m_mpBar->findNode(L"bar");
    m_inGame.m_chargeBar = m_inGame.m_mpBar->findNode(L"chargebar");
    m_inGame.m_experienceBar = gui->findNode(L"experiencebar");
    m_inGame.m_experienceBar->setParent(m_inGame.m_node);
    m_inGame.m_experienceBar->setVisible(false);
    m_inGame.m_castBar = gui->findNode(L"castbar");
    m_inGame.m_castBar->setParent(m_inGame.m_quickBar);
    m_inGame.m_castBarFill = m_inGame.m_castBar->findNode(L"bar");
    m_inGame.m_staminaBar = gui->findNode(L"staminabar");
    m_inGame.m_staminaBar->setParent(m_inGame.m_quickBar);
    m_inGame.m_staminaBarFill = m_inGame.m_staminaBar->findNode(L"bar");
    m_inGame.m_infoTextSmall = createText(L"", L"info", m_inGame.m_node, (1 << plasma::TEXT_RIGHT), 12, 3);
    m_inGame.m_infoTextSmall->setParent(m_inGame.m_node);
    m_inGame.m_infoTextLarge = createText(L"", L"info", m_inGame.m_node, (1 << plasma::TEXT_RIGHT), 28, 6);
    m_inGame.m_infoTextMedium = createText(L"", L"info", m_inGame.m_node, (1 << plasma::TEXT_RIGHT), 21, 5);
    m_inGame.m_location = gui->findNode(L"location");
    m_inGame.m_location->setParent(m_inGame.m_node);
    m_inGame.m_location->setIgnoresPointer(true);

    // The four quick buttons, each a clone of "quickbutton" under the quick bar with its "cooldown" and "disabled"
    // children and its item sprite. One record is filled and pushed four times; its last field is set once, by the
    // constructor.
    QuickButton quickButton;
    quickButton.button = gui->findNode(L"quickbutton")->clone(m_inGame.m_quickBar);
    quickButton.cooldown = quickButton.button->findNode(L"cooldown");
    quickButton.disabled = quickButton.button->findNode(L"disabled");
    m_inGame.m_quickButtons.push_back(quickButton);
    m_inGame.m_quickItemSprites[QuickSlotItem] = new SpriteWidget(m_game, quickButton.button->findNode(L"sprite"));
    quickButton.button = gui->findNode(L"quickbutton")->clone(m_inGame.m_quickBar);
    quickButton.cooldown = quickButton.button->findNode(L"cooldown");
    quickButton.disabled = quickButton.button->findNode(L"disabled");
    m_inGame.m_quickButtons.push_back(quickButton);
    m_inGame.m_quickItemSprites[QuickSlotUse] = new SpriteWidget(m_game, quickButton.button->findNode(L"sprite"));
    quickButton.button = gui->findNode(L"quickbutton")->clone(m_inGame.m_quickBar);
    quickButton.cooldown = quickButton.button->findNode(L"cooldown");
    quickButton.disabled = quickButton.button->findNode(L"disabled");
    m_inGame.m_quickButtons.push_back(quickButton);
    m_inGame.m_quickItemSprites[QuickSlotSkill] = new SpriteWidget(m_game, quickButton.button->findNode(L"sprite"));
    quickButton.button = gui->findNode(L"quickbutton")->clone(m_inGame.m_quickBar);
    quickButton.cooldown = quickButton.button->findNode(L"cooldown");
    quickButton.disabled = quickButton.button->findNode(L"disabled");
    m_inGame.m_quickButtons.push_back(quickButton);
    m_inGame.m_quickItemSprites[QuickSlotUnused] = new SpriteWidget(m_game, quickButton.button->findNode(L"sprite"));

    m_inGame.m_missionTag = gui->findNode(L"missiontag-white");
    m_inGame.m_missionTag->setParent(m_inGame.m_node);
    m_inGame.m_missionTag->setVisible(false);

    m_inGame.m_speechWidget = new SpeechWidget(m_game, m_blackWidget->instance(m_inGame.m_node),
                                               gui->findNode(L"speech-next"), gui->findNode(L"speech-end"));
    m_inGame.m_speechWidget->resizeTo(320.0f, 180.0f, true);

    m_inGame.m_crosshair = gui->findNode(L"crosshair");
    m_inGame.m_crosshair->setParent(m_inGame.m_node);
    m_inGame.m_zoomCrosshair = gui->findNode(L"zoomcrosshair");
    m_inGame.m_zoomCrosshair->setParent(m_inGame.m_node);
    m_inGame.m_lockedTarget = gui->findNode(L"lockedtarget");
    m_inGame.m_lockedTarget->setParent(m_inGame.m_node);
    m_inGame.m_lockedTarget->setVisible(false);
    m_inGame.m_highlightedTarget = gui->findNode(L"highlightedtarget");
    m_inGame.m_highlightedTarget->setParent(m_inGame.m_node);
    m_inGame.m_highlightedTarget->setVisible(false);
    m_inGame.m_selector = gui->findNode(L"selector");
    m_inGame.m_selector->setParent(m_inGame.m_node);
    m_inGame.m_selector->setVisible(false);

    // The panels: the inventories with their tabs, the character, crafting, multiplayer and help panels.
    m_inGame.m_equipmentInventoryWidget =
        new InventoryWidget(m_game, 0, m_blackWidget->instance(m_inGame.m_node), gui->findNode(L"wideitembox"),
                            upButton, downButton, scrollButton, leftButton, rightButton, nullptr);
    m_inGame.m_equipmentInventoryWidget->resizeTo(400.0f, 290.0f, true);
    m_inGame.m_equipmentInventoryWidget->getNode()->setVisible(false);
    m_tabNode = gui->findNode(L"tab");
    std::list<plasma::Node *> tabs;
    tabs.push_back(m_tabNode->clone(nullptr));
    setIcon(tabs.back(), L"background", L"equipment");
    tabs.push_back(m_tabNode->clone(nullptr));
    setIcon(tabs.back(), L"background", L"questitems");
    tabs.push_back(m_tabNode->clone(nullptr));
    setIcon(tabs.back(), L"background", L"consumables");
    tabs.push_back(m_tabNode->clone(nullptr));
    setIcon(tabs.back(), L"background", L"ingredients");
    tabs.push_back(m_tabNode->clone(nullptr));
    setIcon(tabs.back(), L"background", L"pets");
    tabs.push_back(m_tabNode->clone(nullptr));
    setIcon(tabs.back(), L"background", L"artifacts");
    m_inGame.m_equipmentInventoryWidget->setTabNodes(tabs);

    m_inGame.m_craftingInventoryWidget =
        new InventoryWidget(m_game, 2, m_blackWidget->instance(m_inGame.m_node), gui->findNode(L"wideitembox"),
                            upButton, downButton, scrollButton, nullptr, nullptr, gui->findNode(L"itemselector"));
    m_inGame.m_craftingInventoryWidget->resizeTo(400.0f, 285.0f, true);
    m_inGame.m_craftingInventoryWidget->getNode()->setVisible(false);
    tabs.clear();
    tabs.push_back(m_tabNode->clone(nullptr));
    tabs.push_back(m_tabNode->clone(nullptr));
    tabs.push_back(m_tabNode->clone(nullptr));
    tabs.push_back(m_tabNode->clone(nullptr));
    m_inGame.m_craftingInventoryWidget->setTabNodes(tabs);

    m_inGame.m_characterWidget =
        new CharacterWidget(m_game, m_game->m_engine->createNode(nullptr, nullptr, nullptr, m_inGame.m_node));
    m_inGame.m_formulaDetailsWidget =
        new FormulaDetailsWidget(m_game, m_blackWidget->instance(m_inGame.m_node), gui->findNode(L"itemshadow"),
                                 gui->findNode(L"craftbutton"), gui->findNode(L"craftbar"));
    m_inGame.m_formulaDetailsWidget->resizeTo(400.0f, 274.0f, true);
    m_inGame.m_formulaDetailsWidget->getNode()->setVisible(false);
    m_inGame.m_vendorInventoryWidget =
        new InventoryWidget(m_game, 3, m_blackWidget->instance(m_inGame.m_node), gui->findNode(L"wideitembox"),
                            upButton, downButton, scrollButton, nullptr, nullptr, nullptr);
    m_inGame.m_vendorInventoryWidget->resizeTo(400.0f, 285.0f, true);
    m_inGame.m_vendorInventoryWidget->getNode()->setVisible(false);
    m_inGame.m_multiplayerWidget = new MultiplayerWidget(m_game, m_blackWidget->instance(m_game->m_engine->root()),
                                                         button2, upButton, downButton, scrollButton);
    m_inGame.m_multiplayerWidget->resizeTo(400.0f, 600.0f, true);
    m_inGame.m_multiplayerWidget->getNode()->setVisible(false);
    m_inGame.m_helpWidget =
        new HelpWidget(m_game, m_game->m_engine->createNode(nullptr, nullptr, nullptr, m_inGame.m_node));
    m_inGame.m_helpWidget->getNode()->setVisible(false);
    tabs.clear();
    tabs.push_back(m_tabNode->clone(nullptr));
    setIcon(tabs.back(), L"background", L"vendor-buy");
    tabs.push_back(m_tabNode->clone(nullptr));
    setIcon(tabs.back(), L"background", L"vendor-undo");
    m_inGame.m_vendorInventoryWidget->setTabNodes(tabs);

    // The ten equipment slots.
    plasma::Node *equipmentBoxLeft = gui->findNode(L"equipmentboxleft");
    plasma::Node *equipmentBoxRight = gui->findNode(L"equipmentboxright");
    for (int i = 0; i < EquipmentSlotCount; i++)
        m_inGame.m_equipmentSlots.push_back(new SpriteWidget(
            m_game, i % 2 ? equipmentBoxLeft->clone(m_inGame.m_node) : equipmentBoxRight->clone(m_inGame.m_node)));
    m_inGame.m_equipmentSlots[EquipmentSlotWeaponLeft]->m_node->setText(L"info", L"Left\nWeapon", true);
    m_inGame.m_equipmentSlots[EquipmentSlotWeaponRight]->m_node->setText(L"info", L"Right\nWeapon", true);
    m_inGame.m_equipmentSlots[EquipmentSlotRingLeft]->m_node->setText(L"info", L"Left Ring", true);
    m_inGame.m_equipmentSlots[EquipmentSlotRingRight]->m_node->setText(L"info", L"Right Ring", true);
    m_inGame.m_equipmentSlots[EquipmentSlotNeck]->m_node->setText(L"info", L"Neck", true);
    m_inGame.m_equipmentSlots[EquipmentSlotShoulder]->m_node->setText(L"info", L"Shoulder", true);
    m_inGame.m_equipmentSlots[EquipmentSlotChest]->m_node->setText(L"info", L"Chest", true);
    m_inGame.m_equipmentSlots[EquipmentSlotHands]->m_node->setText(L"info", L"Hands", true);
    m_inGame.m_equipmentSlots[EquipmentSlotFeet]->m_node->setText(L"info", L"Feet", true);
    m_inGame.m_equipmentSlots[EquipmentSlotPet]->m_node->setText(L"info", L"Pet", true);

    m_inGame.m_chatWidget =
        new ChatWidget(m_game->m_engine, m_game->m_engine->createNode(nullptr, nullptr, nullptr, m_inGame.m_node));
    m_inGame.m_chatWidget->resizeTo(400.0f, 250.0f, true);
    plasma::Node *preview = gui->findNode(L"preview");
    preview->setParent(m_game->m_engine->root());
    m_inGame.m_previewWidget = new PreviewWidget(m_game, preview);
    m_inGame.m_previewWidget->resizeTo(300.0f, 300.0f, true);

    // The six menu buttons.
    plasma::Node *menuButton = gui->findNode(L"menubutton");
    for (int i = 0; i < 6; i++) {
        m_inGame.m_menuButtons.push_back(menuButton->clone(m_inGame.m_node));
        switch (i) {
        default:
            setIcon(*(m_inGame.m_menuButtons.end() - 1), L"background", L"crafting-menu");
            (*(m_inGame.m_menuButtons.end() - 1))
                ->widget()
                ->connect(plasma::WIDGET_EVENT_POINTER_PRESSED, this, &Interface::toggleCraftingWidget, true, true);
            break;
        case InGameMenuInventory:
            setIcon(*(m_inGame.m_menuButtons.end() - 1), L"background", L"inventory-menu");
            (*(m_inGame.m_menuButtons.end() - 1))
                ->widget()
                ->connect(plasma::WIDGET_EVENT_POINTER_PRESSED, this, &Interface::toggleInventoryWidget, true, true);
            break;
        case InGameMenuWorldMap:
            setIcon(*(m_inGame.m_menuButtons.end() - 1), L"background", L"worldmap-menu");
            (*(m_inGame.m_menuButtons.end() - 1))
                ->widget()
                ->connect(plasma::WIDGET_EVENT_POINTER_PRESSED, this, &Interface::toggleWorldMap, true, true);
            break;
        case InGameMenuSystem:
            setIcon(*(m_inGame.m_menuButtons.end() - 1), L"background", L"system-menu");
            (*(m_inGame.m_menuButtons.end() - 1))
                ->widget()
                ->connect(plasma::WIDGET_EVENT_POINTER_PRESSED, this, &Interface::toggleSystemWidget, true, true);
            break;
        case InGameMenuMultiplayer:
            setIcon(*(m_inGame.m_menuButtons.end() - 1), L"background", L"multiplayer-menu");
            (*(m_inGame.m_menuButtons.end() - 1))
                ->widget()
                ->connect(plasma::WIDGET_EVENT_POINTER_PRESSED, this, &Interface::toggleMultiplayerWidget, true, true);
            break;
        case InGameMenuHelp:
            setIcon(*(m_inGame.m_menuButtons.end() - 1), L"background", L"help-menu");
            (*(m_inGame.m_menuButtons.end() - 1))
                ->widget()
                ->connect(plasma::WIDGET_EVENT_POINTER_PRESSED, this, &Interface::toggleHelpWidget, true, true);
            break;
        }
    }

    // The remaining panels, each on an instance of "blackwidget".
    m_inGame.m_adaptionWidget = new AdaptionWidget(m_game, m_blackWidget->instance(m_inGame.m_node),
                                                   gui->findNode(L"itemframe"), gui->findNode(L"rightarrow"));
    m_inGame.m_adaptionWidget->resizeTo(500.0f, 450.0f, true);
    m_inGame.m_adaptionWidget->getNode()->setVisible(false);
    m_inGame.m_enchantWidget =
        new EnchantWidget(m_game, m_blackWidget->instance(m_inGame.m_node), gui->findNode(L"itemframe"));
    m_inGame.m_enchantWidget->resizeTo(350.0f, 285.0f, true);
    m_inGame.m_enchantWidget->getNode()->setVisible(false);
    m_inGame.m_voxelWidget = new VoxelWidget(m_game, m_blackWidget->instance(m_inGame.m_node));
    m_inGame.m_voxelWidget->resizeTo(800.0f, 800.0f, true);
    m_inGame.m_voxelWidget->getNode()->setVisible(false);
    m_inGame.m_questionWidget = new QuestionWidget(m_game, m_blackWidget->instance(m_inGame.m_node));
    m_inGame.m_questionWidget->resizeTo(300.0f, 200.0f, true);
    m_inGame.m_questionWidget->getNode()->setVisible(false);
    m_inGame.m_equipmentInventoryWidget->getNode()->setParent(m_inGame.m_node);
    m_inGame.m_systemWidget = new SystemWidget(m_game, m_blackWidget->instance(m_inGame.m_node));
    m_inGame.m_systemWidget->resizeTo(150.0f, 150.0f, true);
    m_inGame.m_systemWidget->getNode()->setVisible(false);

    // The cursor, which the pointer must not hit.
    m_cursor = m_game->m_engine->createNode(nullptr, nullptr, nullptr, m_game->m_engine->root());
    m_game->m_engine->readPlasmaFileIntoNode(L"cursor.plx", m_cursor, 0, nullptr, keys);
    m_cursor->setHitTestable(false);
    m_cursor->setText(L"");

    m_game->m_engine->initializeWidgets();
    m_game->m_engine->root()->initializeWidgets();
    updateSlots();
    updateWorldList();
}

// Creates a text node named name under parent: a text shape with a black stroke and, as its child, the same text
// without the stroke. Both use "resource1.dat" at the given pixel size.
plasma::Node *Interface::createText(const std::wstring &text, const std::wstring &name, plasma::Node *parent, int flags,
                                    int size, int strokeRadius)
{
    plasma::TextShape *stroke = m_game->m_engine->createTextShape(std::wstring(), L"");
    stroke->m_color.value() = plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f);
    stroke->m_pixelSize = (float)size;
    stroke->m_strokeColor.value() = plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f);
    stroke->m_strokeRadius = (float)strokeRadius;
    stroke->m_lineSpacing = 3.0f;
    stroke->m_string.value() = text;
    stroke->m_flags = flags;
    stroke->m_fontName = L"resource1.dat";
    stroke->attributesChanged(true);
    plasma::Node *node = m_game->m_engine->createNode(nullptr, stroke, nullptr, parent, name);

    plasma::TextShape *fill = m_game->m_engine->createTextShape(std::wstring(), L"");
    fill->m_color.value() = plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f);
    fill->m_pixelSize = stroke->m_pixelSize;
    fill->m_lineSpacing = stroke->m_lineSpacing;
    fill->m_string.value() = text;
    fill->m_flags = flags;
    fill->m_fontName = L"resource1.dat";
    fill->attributesChanged(true);
    m_game->m_engine->createNode(nullptr, fill, nullptr, node, name);
    return node;
}

// value as text: "1.2M" from a million, "12.3K" from ten thousand, the number itself below.
std::wstring formatNumber(int value)
{
    std::wstringstream text;
    if (value >= 1000000)
        text << value / 1000000 << "." << value % 1000000 / 100000 << "M";
    else if (value >= 10000)
        text << value / 1000 << "." << value % 1000 / 100 << "K";
    else
        text << value;
    return text.str();
}

// The engine gets every press. A right click outside every node also closes the inventories and the formula details.
void Interface::handleMouseDown(int button)
{
    m_pointerY = m_game->m_engine->m_pointer[1];
    switch (button) {
    case MouseButton_Left:
        m_game->m_engine->pointerPressed();
        break;
    case MouseButton_Right:
        m_game->m_engine->rightButtonPressed();
        break;
    case MouseButton_Middle:
        m_game->m_engine->middleButtonPressed();
        break;
    }
    if (button == MouseButton_Right && m_game->m_engine->getPointerNode() == nullptr) {
        m_inGame.m_menuOpen = false;
        m_inGame.m_equipmentInventoryWidget->m_node->setVisible(false);
        m_inGame.m_craftingInventoryWidget->m_node->setVisible(false);
        m_inGame.m_formulaDetailsWidget->m_node->setVisible(false);
    }
}

void Interface::handleMouseUp(int button)
{
    switch (button) {
    case MouseButton_Left:
        m_game->m_engine->pointerReleased();
        break;
    case MouseButton_Right:
        m_game->m_engine->rightButtonReleased();
        break;
    case MouseButton_Middle:
        m_game->m_engine->middleButtonReleased();
        break;
    }
}

// Moves the engine's pointer and, while the left button is held on the character or the world selection, drags the
// cards by the smoothed vertical pointer movement.
void Interface::movePointer(int deltaX, int deltaY)
{
    m_game->m_engine->movePointer((float)deltaX, (float)deltaY);
    static unsigned int lastTime = timeGetTime();
    unsigned int time = timeGetTime();
    int elapsed = time - lastTime;
    lastTime = time;
    float previousY = m_pointerY;
    m_pointerY = blendFactor(elapsed, 0.01f) * (m_game->m_engine->getPointer()[1] - previousY) + previousY;
    float scrolled = m_pointerY - previousY;
    if (m_characterSelection.m_node->isVisible() && m_game->m_engine->isButtonDown(0)) {
        for (CharacterPreviewWidget *widget : m_characterSelection.m_previewWidgets) {
            int width = m_game->m_width;
            float y = widget->viewY() + scrolled;
            widget->moveTo(width / 2 - widget->viewWidth() * 0.5f, y, true);
        }
        m_characterSelection.m_characterScroll +=
            (m_game->m_engine->getPointer()[1] - m_game->m_engine->getPreviousPointer()[1]) * elapsed * 2.0f;
    } else if (m_worldSelection.m_node->isVisible() && m_game->m_engine->isButtonDown(0)) {
        for (plasma::Node *card : m_worldSelection.m_previewCards) {
            plasma::Widget *widget = card->widget();
            int width = m_game->m_width;
            float y = widget->viewY() + scrolled;
            widget->moveTo(width / 2 - widget->viewWidth() * 0.5f, y, true);
        }
        m_worldSelection.m_scroll +=
            (m_game->m_engine->getPointer()[1] - m_game->m_engine->getPreviousPointer()[1]) * elapsed * 2.0f;
    }
}

// Whether the pointer belongs to the interface: a menu screen, the map or any panel is open, or the chat is being
// typed into.
bool Interface::isInMenu()
{
    return m_startMenuNode->isVisible() || m_characterSelection.m_node->isVisible() ||
           m_characterCreationNode->isVisible() || m_worldSelection.m_node->isVisible() ||
           m_worldCreationNode->isVisible() || m_mapOpen || m_inGame.m_equipmentInventoryWidget->m_node->isVisible() ||
           m_inGame.m_craftingInventoryWidget->m_node->isVisible() ||
           m_inGame.m_multiplayerWidget->m_node->isVisible() || m_inGame.m_vendorInventoryWidget->m_node->isVisible() ||
           m_inGame.m_menuOpen || m_inGame.m_adaptionWidget->m_node->isVisible() ||
           m_inGame.m_voxelWidget->m_node->isVisible() || m_inGame.m_enchantWidget->m_node->isVisible() ||
           m_inGame.m_chatWidget->m_typeboxActive || m_inGame.m_questionWidget->m_node->isVisible() ||
           m_inGame.m_systemWidget->m_node->isVisible() || m_controlsWidget->m_node->isVisible() ||
           m_optionsWidget->m_node->isVisible();
}

void Interface::lastLand()
{
    if (m_land != m_game->m_world->m_localCreature->m_playerData->visitedRegions.begin())
        --m_land;
    updateTravel();
    plasma::Vector<2, int> center = m_game->m_world->landCenter((*m_land)[0], (*m_land)[1]);
    m_game->scrollMap(center[0], center[1]);
    if (m_game->m_mapZoom < TravelMapZoom)
        m_game->m_mapZoom = TravelMapZoom;
}

void Interface::nextLand()
{
    std::list<plasma::Vector<2, int>>::iterator next = m_land;
    if (++next != m_game->m_world->m_localCreature->m_playerData->visitedRegions.end())
        ++m_land;
    updateTravel();
    plasma::Vector<2, int> center = m_game->m_world->landCenter((*m_land)[0], (*m_land)[1]);
    m_game->scrollMap(center[0], center[1]);
    if (m_game->m_mapZoom < TravelMapZoom)
        m_game->m_mapZoom = TravelMapZoom;
}

// Lays every screen out for a window of width x height. Positions are setPosition(x, y, offsetX, offsetY): a point of
// the window plus an offset in interface units.
void Interface::updateResolution(int width, int height)
{
    // The whole interface is scaled by the option.
    m_game->m_engine->root()->transformation()->m_deformation.value().identity();
    m_game->m_engine->root()->transformation()->m_deformation.value().scale(m_scale, m_scale);
    m_game->m_engine->root()->transformation()->attributesChanged(true);

    m_startMenuButtons->setPosition(
        (plasma::Vector<2, float>((float)width, height * 1.6f) - m_startMenuButtons->widget()->getViewSize()) * 0.5f,
        plasma::Vector<2, float>(0.0f, 0.0f));
    m_backButton->setPosition(0.0f, 0.0f, 20.0f, 20.0f);
    m_worldSelection.m_multiplayerWorldsButton->setPosition(
        (float)width, 0.0f, -m_worldSelection.m_multiplayerWorldsButton->widget()->getContentWidth() - 20.0f, 20.0f);
    m_optionsWidget->getNode()->setPosition(
        (plasma::Vector<2, float>((float)width, (float)height) - m_optionsWidget->screenSize()) * 0.5f,
        plasma::Vector<2, float>(0.0f, 0.0f));
    m_controlsWidget->getNode()->setPosition(
        (plasma::Vector<2, float>((float)width, (float)height) - m_controlsWidget->screenSize()) * 0.5f,
        plasma::Vector<2, float>(0.0f, 0.0f));
    m_inGame.m_systemWidget->getNode()->setPosition(
        (plasma::Vector<2, float>((float)width, (float)height) - m_inGame.m_systemWidget->screenSize()) * 0.5f,
        plasma::Vector<2, float>(0.0f, 0.0f));
    m_inGame.m_speechWidget->getNode()->setPosition((width - m_inGame.m_speechWidget->screenSize()[0]) * 0.5f,
                                                    (float)height, 0.0f,
                                                    -150.0f - m_inGame.m_speechWidget->getViewHeight());
    m_inGame.m_questionWidget->getNode()->setPosition((width - m_inGame.m_questionWidget->screenSize()[0]) * 0.5f,
                                                      (float)height, 0.0f,
                                                      -150.0f - m_inGame.m_questionWidget->getViewHeight());

    // The character and world screens.
    m_characterName->setPosition((float)(width / 2), (float)height, 0.0f, -50.0f);
    m_createCharacterButton->setPosition(width - m_createCharacterButton->widget()->viewWidth(),
                                         height - m_createCharacterButton->widget()->getViewHeight(), -20.0f, -20.0f);
    m_characterStyleNode->setPosition(0.0f, height - m_characterStyleNode->widget()->getViewHeight(), 20.0f, -20.0f);
    m_worldName->setPosition((float)(width / 2), (float)height, 0.0f, -50.0f);
    m_worldSeed->setPosition((float)(width / 2), (float)height, 0.0f, -100.0f);
    m_createWorldButton->setPosition(width - m_createWorldButton->widget()->viewWidth(),
                                     height - m_createWorldButton->widget()->getViewHeight(), -20.0f, -20.0f);
    m_wait->setPosition((float)(width / 2), (float)(height / 2), 0.0f, 0.0f);

    // The in-game interface.
    m_inGame.m_recentLandsText->setPosition(0.0f, (float)height, 150.0f, -50.0f);
    m_inGame.m_quickButtons[QuickSlotSkill].button->setPosition(0.0f, (float)height, 90.0f, -110.0f);
    m_inGame.m_quickButtons[QuickSlotItem].button->setPosition(0.0f, (float)height, 30.0f, -80.0f);
    m_inGame.m_quickButtons[QuickSlotUse].button->setPosition((float)(width / 2), (float)height, -23.0f, -150.0f);
    m_inGame.m_hpBar->setPosition((float)(width / 2), (float)height, -181.0f, -50.0f);
    m_inGame.m_mpBar->setPosition((float)(width / 2), (float)height, 8.0f, -50.0f);
    m_inGame.m_goldText->setPosition(0.0f, 0.0f, 85.0f, 60.0f);
    m_inGame.m_contextItemText->setPosition(0.0f, 0.0f, 255.0f, 60.0f);
    m_inGame.m_levelInfo->setPosition(0.0f, 0.0f, 140.0f, 30.0f);
    m_inGame.m_experienceBar->setPosition((float)width, 0.0f, -315.0f, 35.0f);
    m_inGame.m_characterWidget->getNode()->setPosition(0.0f, 0.0f, 20.0f, 170.0f);
    m_inGame.m_multiplayerWidget->getNode()->setPosition(0.0f, 0.0f, 20.0f, 150.0f);
    m_inGame.m_helpWidget->getNode()->setPosition(0.0f, (float)height, 20.0f, -250.0f);
    m_inGame.m_infoTextSmall->setPosition((float)width, 0.0f, -120.0f, 20.0f);
    m_inGame.m_infoTextLarge->setPosition((float)width, 0.0f, -120.0f, 60.0f);
    m_inGame.m_infoTextMedium->setPosition((float)width, 0.0f, -120.0f, 100.0f);
    m_inGame.m_location->setPosition((float)(width / 2), (float)(height / 3), 0.0f, 0.0f);
    m_inGame.m_crosshair->setPosition((float)(width / 2), (float)(height / 2), 0.0f, 0.0f);
    m_inGame.m_zoomCrosshair->setPosition((float)(width / 2), (float)(height / 2), 0.0f, 0.0f);
    m_inGame.m_selector->setPosition((float)(width / 2), (float)(height / 2), 0.0f, -200.0f);
    m_inGame.m_castBar->setPosition((float)(width / 2), (float)height, 0.0f, -200.0f);
    m_inGame.m_staminaBar->setPosition((float)(width / 2), (float)height, 0.0f, -180.0f);
    m_inGame.m_comboPoints->setPosition((float)(width / 2), (float)(height / 2), 100.0f, 0.0f);
    m_inGame.m_missionTag->setPosition((float)(width / 2), (float)height, 0.0f, -300.0f);

    // The inventories, around the equipment inventory in the middle.
    m_inGame.m_equipmentInventoryWidget->getNode()->setPosition(
        width / 2 - m_inGame.m_equipmentInventoryWidget->viewWidth() * 0.5f,
        height / 2 - m_inGame.m_equipmentInventoryWidget->getViewHeight(), 0.0f, -50.0f);
    m_inGame.m_craftingInventoryWidget->getNode()->setPosition(
        width / 2 - m_inGame.m_equipmentInventoryWidget->viewWidth() * 0.5f -
            m_inGame.m_craftingInventoryWidget->viewWidth(),
        height / 2 - m_inGame.m_equipmentInventoryWidget->getViewHeight(), -10.0f, -50.0f);
    m_inGame.m_formulaDetailsWidget->getNode()->setPosition(
        width / 2 - m_inGame.m_equipmentInventoryWidget->viewWidth() * 0.5f -
            m_inGame.m_formulaDetailsWidget->viewWidth(),
        (float)(height / 2), -10.0f, -40.0f);
    m_inGame.m_vendorInventoryWidget->getNode()->setPosition(
        m_inGame.m_equipmentInventoryWidget->viewWidth() * 0.5f + width / 2,
        height / 2 - m_inGame.m_equipmentInventoryWidget->getViewHeight(), 10.0f, -50.0f);
    m_inGame.m_voxelWidget->getNode()->setPosition(width / 2 - m_inGame.m_voxelWidget->viewWidth() * 0.5f,
                                                   height / 2 - m_inGame.m_equipmentInventoryWidget->getViewHeight(),
                                                   0.0f, -50.0f);
    m_inGame.m_enchantWidget->getNode()->setPosition(
        m_inGame.m_equipmentInventoryWidget->viewWidth() * 0.5f + width / 2,
        height / 2 - m_inGame.m_equipmentInventoryWidget->getViewHeight(), 10.0f, -50.0f);
    m_inGame.m_adaptionWidget->getNode()->setPosition(
        m_inGame.m_equipmentInventoryWidget->viewWidth() * 0.5f + width / 2,
        height / 2 - m_inGame.m_equipmentInventoryWidget->getViewHeight(), 10.0f, -50.0f);
    for (int i = 0; i < (int)m_inGame.m_equipmentSlots.size(); i++)
        m_inGame.m_equipmentSlots[i]->getNode()->setPosition((float)(width / 2), (float)(height / 2),
                                                             (float)(i % 2 * 400 - 200), (float)(i / 2 * 56 - 40));
    m_inGame.m_chatWidget->getNode()->setPosition(width - m_inGame.m_chatWidget->viewWidth(),
                                                  height - m_inGame.m_chatWidget->getViewHeight(), -20.0f, -20.0f);
    for (int i = 0; i < (int)m_inGame.m_menuButtons.size(); i++)
        m_inGame.m_menuButtons[i]->setPosition((float)(width / 2), (float)height,
                                               (float)((i * 2 - (int)m_inGame.m_menuButtons.size() + 1) * 30), -200.0f);
}

void Interface::refresh() {}

// See cube/Interface.h. The icon is centred by its pivot, moved to the middle of the target's shape and scaled to the
// target's size.
void Interface::setIcon(plasma::Node *node, const std::wstring &targetName, const std::wstring &iconName)
{
    plasma::Node *target = node->findNode(targetName);
    if (!target)
        return;
    plasma::Node *icon = m_icons->findNode(iconName);
    if (!icon)
        return;
    plasma::Node *previous = target->findNode(L"icon");
    if (previous)
        m_game->m_engine->destroyNode(previous);
    plasma::Vector<2, float> targetMinimum;
    plasma::Vector<2, float> targetMaximum;
    target->getShapeBounds(targetMinimum, targetMaximum);
    plasma::Node *instance = icon->instance(target);
    instance->makeTransformationUnique();
    instance->setName(L"icon");
    instance->transformation()->m_pivot.value() =
        instance->transformation()->m_inverseMatrix * ((instance->getMinimum(0) + instance->getMaximum(0)) * 0.5f);
    instance->transformation()->attributesChanged(true);
    plasma::Vector<2, float> minimum;
    plasma::Vector<2, float> maximum;
    instance->getShapeBounds(minimum, maximum);
    plasma::Matrix<float> inverse;
    instance->getInverseMatrix(inverse);
    plasma::Vector<2, float> offset =
        inverse.transformDirection((targetMaximum + targetMinimum - minimum - maximum) * 0.5f);
    instance->transformation()->m_deformation.value().translate(offset[0], offset[1]);
    plasma::Vector<2, float> scale = (targetMaximum - targetMinimum) / (maximum - minimum);
    instance->transformation()->m_deformation.value().scale(scale[0], scale[1]);
    instance->transformation()->attributesChanged(true);
    instance->getShapeBounds(minimum, maximum);
}

void Interface::showTab(int tab)
{
    m_inGame.m_tab = tab;
    m_inGame.m_equipmentInventoryWidget->m_node->setVisible(false);
    m_inGame.m_craftingInventoryWidget->m_node->setVisible(false);
    m_inGame.m_multiplayerWidget->m_node->setVisible(false);
    switch (m_inGame.m_tab) {
    case InterfacePanelEquipment:
        toggleInventoryWidget();
        break;
    case InterfacePanelCrafting:
        toggleCraftingWidget();
        break;
    case InterfacePanelMultiplayer:
        m_game->updateFriends();
        m_inGame.m_multiplayerWidget->m_node->setVisible(!m_inGame.m_multiplayerWidget->m_node->isVisible());
        break;
    }
}

void Interface::toggleControlsWidget()
{
    m_controlsWidget->m_node->setVisible(!m_controlsWidget->m_node->isVisible());
}

void Interface::toggleCraftingWidget()
{
    m_inGame.m_craftingInventoryWidget->m_node->setVisible(!m_inGame.m_craftingInventoryWidget->m_node->isVisible());
    m_game->updateFormulas();
    if (m_inGame.m_craftingInventoryWidget->m_node->isVisible()) {
        m_inGame.m_craftingInventoryWidget->updateRows();
        m_inGame.m_craftingInventoryWidget->updateScrollButton();
    }
}

void Interface::toggleHelpWidget()
{
    m_inGame.m_helpWidget->m_node->setVisible(!m_inGame.m_helpWidget->m_node->isVisible());
}

void Interface::toggleInventoryWidget()
{
    m_inGame.m_equipmentInventoryWidget->m_node->setVisible(!m_inGame.m_equipmentInventoryWidget->m_node->isVisible());
    if (m_inGame.m_equipmentInventoryWidget->m_node->isVisible()) {
        m_inGame.m_equipmentInventoryWidget->m_region = m_game->m_world->m_localCreature->m_entityData.region;
        m_inGame.m_equipmentInventoryWidget->updateRows();
        m_inGame.m_equipmentInventoryWidget->updateScrollButton();
        m_inGame.m_systemWidget->m_node->setVisible(false);
    }
}

void Interface::toggleMultiplayerWidget()
{
    m_game->updateFriends();
    m_inGame.m_multiplayerWidget->m_node->setVisible(!m_inGame.m_multiplayerWidget->m_node->isVisible());
}

void Interface::toggleOptionsWidget()
{
    m_optionsWidget->m_node->setVisible(!m_optionsWidget->m_node->isVisible());
}

void Interface::toggleSystemWidget()
{
    m_inGame.m_systemWidget->m_node->setVisible(!m_inGame.m_systemWidget->m_node->isVisible());
    if (m_inGame.m_systemWidget->m_node->isVisible())
        m_inGame.m_equipmentInventoryWidget->m_node->setVisible(false);
}

void Interface::toggleWorldMap()
{
    m_mapOpen = !m_mapOpen;
}

void Interface::update(float step)
{
    if (m_game->isAssigningControl()) {
        m_cursor->setVisible(false);
    } else {
        m_cursor->setVisible(isInMenu());
        plasma::Vector<2, float> pivot(0.0f, 0.0f);
        plasma::Vector<2, float> position;
        if (m_game->m_gamepadActive) {
            plasma::D3D11Engine *engine = m_game->m_engine;
            float bounce = cosf(engine->getCaretClock() * 0.01f) * 8.0f;
            position = engine->getPointer() + plasma::Vector<2, float>(0.0f, bounce);
        } else {
            position = m_game->m_engine->getPointer();
        }
        m_cursor->setPosition(position, pivot);
    }
    m_lifeBars->setVisible(!isInMenu());
    if (!m_mapOpen)
        m_choosingShrine = false;
    m_wait->setVisible(false);
    if (m_inGame.m_chatWidget->m_typeboxActive)
        m_inGame.m_chatWidget->m_changed = true;
    m_inGame.m_characterWidget->getNode()->setVisible(m_inGame.m_equipmentInventoryWidget->getNode()->isVisible());
    m_worldSelection.m_multiplayerWorldsButton->setVisible(m_worldSelection.m_node->isVisible());
    m_backButton->setVisible(m_characterSelection.m_node->isVisible() || m_characterCreationNode->isVisible() ||
                             m_worldSelection.m_node->isVisible() || m_worldCreationNode->isVisible());

    if (m_characterSelection.m_node->isVisible()) {
        if (!m_game->m_engine->isButtonDown(0) && !m_characterSelection.m_previewWidgets.empty()) {
            float firstY = m_characterSelection.m_previewWidgets.front()->viewY();
            float top = 10.0f;
            float bottom =
                (float)(m_game->m_height - 10) - m_characterSelection.m_previewWidgets.back()->getViewHeight();
            int height = (int)(m_characterSelection.m_previewWidgets.back()->viewY() +
                               m_characterSelection.m_previewWidgets.back()->getViewHeight() -
                               m_characterSelection.m_previewWidgets.front()->viewY());
            if (height < m_game->m_height - 20) {
                top = (float)(m_game->m_height / 2 - height / 2);
                bottom = (float)(m_game->m_height / 2 + height / 2) -
                         m_characterSelection.m_previewWidgets.back()->getViewHeight();
            }
            if (firstY > top) {
                float offset = firstY + (top - firstY) * blendFactor(step, 0.01f) -
                               m_characterSelection.m_previewWidgets.front()->viewY();
                for (CharacterPreviewWidget *preview : m_characterSelection.m_previewWidgets) {
                    int width = m_game->m_width;
                    float newY = preview->viewY() + offset;
                    preview->moveTo((float)(width / 2) - preview->viewWidth() * 0.5f, newY, true);
                }
            }
            float lastY = m_characterSelection.m_previewWidgets.back()->viewY();
            if (top > m_characterSelection.m_previewWidgets.front()->viewY() && bottom > lastY) {
                float offset = lastY + (bottom - lastY) * blendFactor(step, 0.01f) -
                               m_characterSelection.m_previewWidgets.back()->viewY();
                for (CharacterPreviewWidget *preview : m_characterSelection.m_previewWidgets) {
                    int width = m_game->m_width;
                    float newY = preview->viewY() + offset;
                    preview->moveTo((float)(width / 2) - preview->viewWidth() * 0.5f, newY, true);
                }
            }
            int index = 0;
            for (CharacterPreviewWidget *preview : m_characterSelection.m_previewWidgets) {
                int width = m_game->m_width;
                float y = preview->viewY() + m_characterSelection.m_characterScroll * 0.001f * step;
                preview->moveTo((float)(width / 2) - preview->viewWidth() * 0.5f, y, true);
                if (preview->getNode()->isUnderPointer()) {
                    if (index != m_selectedCharacter)
                        m_characterSelection.m_deleteCharacterButton->setVisible(false);
                    m_selectedCharacter = index;
                }
                ++index;
            }
        }
        if (!m_game->m_engine->isButtonDown(0) && m_selectedCharacter >= 0 &&
            m_selectedCharacter < (int)m_characterSelection.m_previewWidgets.size()) {
            plasma::Widget *button = m_characterSelection.m_selectCharacterButton->widget();
            float bottom = m_characterSelection.m_previewWidgets[m_selectedCharacter]->getViewHeight() +
                           m_characterSelection.m_previewWidgets[m_selectedCharacter]->viewY();
            float right = m_characterSelection.m_previewWidgets[m_selectedCharacter]->viewWidth() +
                          m_characterSelection.m_previewWidgets[m_selectedCharacter]->viewX();
            float y = bottom - button->getViewHeight() - 20.0f;
            float x = right - button->viewWidth() - 20.0f;
            button->moveTo(x, y, true);
            m_characterSelection.m_selectCharacterButton->setVisible(true);
            button = m_characterSelection.m_editCharacterButton->widget();
            right = m_characterSelection.m_previewWidgets[m_selectedCharacter]->viewWidth() +
                    m_characterSelection.m_previewWidgets[m_selectedCharacter]->viewX();
            y = m_characterSelection.m_previewWidgets[m_selectedCharacter]->viewY() + 20.0f;
            x = right - button->viewWidth() - 20.0f;
            button->moveTo(x, y, true);
            m_characterSelection.m_editCharacterButton->setVisible(true);
            button = m_characterSelection.m_deleteCharacterButton->widget();
            y = m_characterSelection.m_previewWidgets[m_selectedCharacter]->viewY();
            x = m_characterSelection.m_previewWidgets[m_selectedCharacter]->viewWidth() +
                m_characterSelection.m_previewWidgets[m_selectedCharacter]->viewX() + 20.0f;
            button->moveTo(x, y, true);
        } else {
            m_characterSelection.m_selectCharacterButton->setVisible(false);
            m_characterSelection.m_editCharacterButton->setVisible(false);
            m_characterSelection.m_deleteCharacterButton->setVisible(false);
        }
        if (m_selectedCharacter >= (int)m_game->m_savedCharacters.size()) {
            m_characterSelection.m_deleteCharacterButton->setVisible(false);
            m_characterSelection.m_editCharacterButton->setVisible(false);
            m_characterSelection.m_selectCharacterButton->setText(L"Create");
        } else {
            m_characterSelection.m_selectCharacterButton->setText(L"Select");
        }
        blend(m_characterSelection.m_characterScroll, 0.0f, step, 0.005f);
    } else {
        m_characterSelection.m_characterScroll = 0.0f;
    }
    if (m_worldSelection.m_node->isVisible()) {
        if (!m_game->m_engine->isButtonDown(0) && !m_worldSelection.m_previewCards.empty()) {
            float firstY = m_worldSelection.m_previewCards.front()->widget()->viewY();
            float top = 10.0f;
            float bottom =
                (float)(m_game->m_height - 10) - m_worldSelection.m_previewCards.back()->widget()->getViewHeight();
            int height = (int)(m_worldSelection.m_previewCards.back()->widget()->viewY() +
                               m_worldSelection.m_previewCards.back()->widget()->getViewHeight() -
                               m_worldSelection.m_previewCards.front()->widget()->viewY());
            if (height < m_game->m_height - 20) {
                top = (float)(m_game->m_height / 2 - height / 2);
                bottom = (float)(m_game->m_height / 2 + height / 2) -
                         m_worldSelection.m_previewCards.back()->widget()->getViewHeight();
            }
            if (firstY > top) {
                float offset = firstY + (top - firstY) * blendFactor(step, 0.01f) -
                               m_worldSelection.m_previewCards.front()->widget()->viewY();
                for (plasma::Node *card : m_worldSelection.m_previewCards) {
                    plasma::Widget *preview = card->widget();
                    int width = m_game->m_width;
                    float newY = preview->viewY() + offset;
                    preview->moveTo((float)(width / 2) - preview->viewWidth() * 0.5f, newY, true);
                }
            }
            float lastY = m_worldSelection.m_previewCards.back()->widget()->viewY();
            if (top > m_worldSelection.m_previewCards.front()->widget()->viewY() && bottom > lastY) {
                float offset = lastY + (bottom - lastY) * blendFactor(step, 0.01f) -
                               m_worldSelection.m_previewCards.back()->widget()->viewY();
                for (plasma::Node *card : m_worldSelection.m_previewCards) {
                    plasma::Widget *preview = card->widget();
                    int width = m_game->m_width;
                    float newY = preview->viewY() + offset;
                    preview->moveTo((float)(width / 2) - preview->viewWidth() * 0.5f, newY, true);
                }
            }
            for (plasma::Node *card : m_worldSelection.m_previewCards) {
                plasma::Widget *preview = card->widget();
                int width = m_game->m_width;
                float y = preview->viewY() + m_worldSelection.m_scroll * 0.001f * step;
                preview->moveTo((float)(width / 2) - preview->viewWidth() * 0.5f, y, true);
                if (card->isUnderPointer()) {
                    int index = m_worldSelection.m_cardIndices[card];
                    if (index != m_game->m_currentWorld)
                        m_worldSelection.m_deleteWorldButton->setVisible(false);
                    m_game->m_currentWorld = index;
                }
            }
        }
        if (!m_game->m_engine->isButtonDown(0) && m_worldSelection.m_cardsByIndex[m_game->m_currentWorld]) {
            m_worldSelection.m_selectWorldButton->widget()->moveTo(
                m_worldSelection.m_cardsByIndex[m_game->m_currentWorld]->widget()->viewX() +
                    m_worldSelection.m_cardsByIndex[m_game->m_currentWorld]->widget()->viewWidth() -
                    m_worldSelection.m_selectWorldButton->widget()->viewWidth() - 20.0f,
                m_worldSelection.m_cardsByIndex[m_game->m_currentWorld]->widget()->viewY() +
                    m_worldSelection.m_cardsByIndex[m_game->m_currentWorld]->widget()->getViewHeight() -
                    m_worldSelection.m_selectWorldButton->widget()->getViewHeight() - 20.0f,
                true);
            m_worldSelection.m_selectWorldButton->setVisible(true);
            m_worldSelection.m_editWorldButton->widget()->moveTo(
                m_worldSelection.m_cardsByIndex[m_game->m_currentWorld]->widget()->viewWidth() +
                    m_worldSelection.m_cardsByIndex[m_game->m_currentWorld]->widget()->viewX() -
                    m_worldSelection.m_editWorldButton->widget()->viewWidth() - 20.0f,
                m_worldSelection.m_cardsByIndex[m_game->m_currentWorld]->widget()->viewY() + 20.0f, true);
            m_worldSelection.m_editWorldButton->setVisible(true);
            m_worldSelection.m_deleteWorldButton->widget()->moveTo(
                m_worldSelection.m_cardsByIndex[m_game->m_currentWorld]->widget()->viewX() +
                    m_worldSelection.m_cardsByIndex[m_game->m_currentWorld]->widget()->viewWidth() + 20.0f,
                m_worldSelection.m_cardsByIndex[m_game->m_currentWorld]->widget()->viewY(), true);
        } else {
            m_worldSelection.m_selectWorldButton->setVisible(false);
            m_worldSelection.m_editWorldButton->setVisible(false);
            m_worldSelection.m_deleteWorldButton->setVisible(false);
        }
        if (m_game->m_currentWorld >= (int)m_game->m_worlds.size()) {
            m_worldSelection.m_deleteWorldButton->setVisible(false);
            m_worldSelection.m_editWorldButton->setVisible(false);
            m_worldSelection.m_selectWorldButton->setText(L"Create");
        } else {
            m_worldSelection.m_selectWorldButton->setText(L"Select");
        }
        m_worldSelection.m_scroll += (0.0f - m_worldSelection.m_scroll) * blendFactor(step, 0.005f);
    } else {
        m_worldSelection.m_scroll = 0.0f;
    }
    if (m_characterCreationNode->isVisible()) {
        std::wstring name = m_characterName->getText(L"edit", true);
        m_createCharacterButton->setVisible(name.size() >= 2 && name.size() <= 15);
    }
    if (m_worldCreationNode->isVisible()) {
        std::wstring name = m_worldName->getText(L"edit", true);
        std::wstring seed = m_worldSeed->getText(L"edit", true);
        bool invalid = false;
        for (wchar_t digit : seed) {
            if (digit < L'0' || digit > L'9')
                invalid = true;
        }
        m_createWorldButton->setVisible(!invalid && name.size() >= 2 && !seed.empty());
    }
    updateAuras();
    std::wstringstream text;
    plasma::Vector<2, int> region =
        m_game->m_world->findRegion((int)m_game->m_world->m_localCreature->m_entityData.position[0],
                                    (int)m_game->m_world->m_localCreature->m_entityData.position[1]);
    m_inGame.m_equipmentInventoryWidget->m_tabs = &m_game->m_world->m_localCreature->m_inventory.m_inventoryTabs;
    m_inGame.m_craftingInventoryWidget->m_tabs = &m_game->m_craftingInventory.m_inventoryTabs;
    m_inGame.m_vendorInventoryWidget->m_tabs = &m_game->m_vendorInventory.m_inventoryTabs;
    text.str(L"");
    text << formatNumber((int)m_game->m_world->m_localCreature->m_entityData.hp) << "/"
         << formatNumber((int)m_game->m_world->m_localCreature->maxHP());
    m_inGame.m_hpBar->setText(L"text", text.str(), true);
    text.str(L"");
    text << (int)(m_game->m_world->m_localCreature->m_entityData.mp * 100.0f) << "/100";
    m_inGame.m_mpBar->setText(L"text", text.str(), true);
    int level = m_game->m_world->m_localCreature->m_entityData.level;
    int experience = m_game->m_world->m_localCreature->m_entityData.xp;
    text.str(L"");
    text << level;
    m_inGame.m_levelText->setText(text.str());
    text.str(L"");
    text << experience << L"/" << maxExperience(level);
    m_inGame.m_levelInfo->setText(L"xp", text.str(), true);
    text.str(L"");
    text << m_game->m_world->m_localCreature->m_inventory.m_gold;
    m_inGame.m_goldText->setText(text.str());
    text.str(L"");
    if (m_game->m_world->m_localCreature->m_playerData->contextItemCount > 1)
        text << m_game->m_world->m_localCreature->m_playerData->contextItemCount;
    m_inGame.m_contextItemText->setText(text.str());
    m_inGame.m_chargeBar->setVisible(m_game->m_world->m_localCreature->m_entityData.charge > 0.0f);
    m_inGame.m_hpBarFill->m_transformation->m_deformation.value().identity();
    m_inGame.m_hpBarFill->m_transformation->m_deformation.value().scale(
        (std::min)(1.0f, m_game->m_world->m_localCreature->m_entityData.hp / m_game->m_world->m_localCreature->maxHP()),
        1.0f);
    m_inGame.m_hpBarFill->m_transformation->attributesChanged(true);
    m_inGame.m_chargeBar->m_transformation->m_deformation.value().identity();
    m_inGame.m_chargeBar->m_transformation->m_deformation.value().scale(
        m_game->m_world->m_localCreature->m_entityData.charge, 1.0f);
    m_inGame.m_chargeBar->m_transformation->attributesChanged(true);
    m_inGame.m_mpBarFill->m_transformation->m_deformation.value().identity();
    m_inGame.m_mpBarFill->m_transformation->m_deformation.value().scale(
        m_game->m_world->m_localCreature->m_entityData.mp, 1.0f);
    m_inGame.m_mpBarFill->m_transformation->attributesChanged(true);
    m_inGame.m_mpBarFill->setVisible(true);
    m_inGame.m_xpBar->m_transformation->m_deformation.value().identity();
    m_inGame.m_xpBar->m_transformation->m_deformation.value().scale((float)experience / maxExperience(level), 1.0f);
    m_inGame.m_xpBar->m_transformation->attributesChanged(true);
    World::BiomeSample sample;
    m_game->m_world->biomeSample((int)m_game->m_world->m_localCreature->m_entityData.position[0],
                                 (int)m_game->m_world->m_localCreature->m_entityData.position[1], sample);
    World::AreaInfo area;
    m_game->m_world->fillAreaInfo((int)m_game->m_world->m_localCreature->m_entityData.position[0],
                                  (int)m_game->m_world->m_localCreature->m_entityData.position[1], area);
    text.str(L"");
    text << L"COORDS ";
    if (m_mapOpen && m_game->m_worldMap->m_cursorPosition[0] != Fixed(0x7fffffff)) {
        text << formatCoordinate((int)(m_game->m_worldMap->m_cursorPosition[0] - Fixed(512)),
                                 (int)(m_game->m_worldMap->m_cursorPosition[1] - Fixed(512)));
    } else {
        text << formatCoordinate((int)(m_game->m_world->m_localCreature->m_entityData.position[0] - Fixed(512)),
                                 (int)(m_game->m_world->m_localCreature->m_entityData.position[1] - Fixed(512)));
    }
    text << L"  ";
    text << L"TIME " << (int)m_game->m_world->m_state.m_time / MillisecondsPerHour << L":" << std::setw(2)
         << std::setfill(L'0') << (int)m_game->m_world->m_state.m_time / 60000 % 60;
    if (m_game->m_world->m_localCreature->m_entityData.currentAbility == AbilityEagleFlight) {
        text << L"  TEMP -";
        text << L"  HUM -";
    } else {
        text << L"  TEMP " << -20 - (int)(sample.temperature(area.type, area.influence) * -60.0f)
             << " \xB0"
                "C";
        text << L"  HUM " << (int)(sample.humidity() * 100.0f) << L"%";
    }
    text << L"  ZOOM 1:" << (int)(powf(2.0f, (float)m_game->m_mapZoom) * 4.0f);
    m_inGame.m_infoTextSmall->setText(text.str());

    m_inGame.m_castBar->setVisible(false);
    if (((m_game->m_world->m_localCreature->m_entityData.currentAbility == AbilityMarkCreature ||
          m_game->m_world->m_localCreature->m_entityData.currentAbility == AbilityUnmarkCreature ||
          m_game->m_world->m_localCreature->m_entityData.currentAbility == AbilityPortCity ||
          m_game->m_world->m_localCreature->m_entityData.currentAbility == AbilitySelfKill ||
          m_game->m_world->m_localCreature->m_entityData.currentAbility == AbilityWaterBubbles ||
          m_game->m_world->m_localCreature->m_entityData.currentAbility == Ability_48 ||
          m_game->m_world->m_localCreature->m_entityData.currentAbility == AbilityWaterSalvo ||
          m_game->m_world->m_localCreature->m_entityData.currentAbility == AbilityMonsterFirebolt ||
          m_game->m_world->m_localCreature->m_entityData.currentAbility == AbilityFirebolt ||
          m_game->m_world->m_localCreature->m_entityData.currentAbility == AbilityBraceletFirebolt ||
          m_game->m_world->m_localCreature->m_entityData.currentAbility == AbilityFireball ||
          m_game->m_world->m_localCreature->m_entityData.currentAbility == AbilityFireMissiles ||
          m_game->m_world->m_localCreature->m_entityData.currentAbility == AbilityFireBreath ||
          m_game->m_world->m_localCreature->m_entityData.currentAbility == AbilityLavaPuddle ||
          m_game->m_world->m_localCreature->m_entityData.currentAbility == AbilityFireRay ||
          m_game->m_world->m_localCreature->m_entityData.currentAbility == AbilityFireBeam ||
          m_game->m_world->m_localCreature->m_entityData.currentAbility == AbilityWaterRay ||
          m_game->m_world->m_localCreature->m_entityData.currentAbility == AbilityWaterBeam ||
          m_game->m_world->m_localCreature->m_entityData.currentAbility == AbilityDjinnExplosion ||
          m_game->m_world->m_localCreature->m_entityData.currentAbility == AbilitySummon ||
          m_game->m_world->m_localCreature->m_entityData.currentAbility == AbilityMonsterWaterbolt ||
          m_game->m_world->m_localCreature->m_entityData.currentAbility == AbilityWaterbolt ||
          m_game->m_world->m_localCreature->m_entityData.currentAbility == AbilityBraceletWaterbolt ||
          m_game->m_world->m_localCreature->m_entityData.currentAbility == AbilityWaterball ||
          m_game->m_world->m_localCreature->m_entityData.currentAbility == AbilityFireSwirl ||
          m_game->m_world->m_localCreature->m_entityData.currentAbility == AbilityFireVortex ||
          m_game->m_world->m_localCreature->m_entityData.currentAbility == AbilityWaterSwirl ||
          m_game->m_world->m_localCreature->m_entityData.currentAbility == AbilityWaterVortex ||
          m_game->m_world->m_localCreature->m_entityData.currentAbility == AbilityHeal ||
          m_game->m_world->m_localCreature->m_entityData.currentAbility == AbilityHealingStream ||
          m_game->m_world->m_localCreature->m_entityData.currentAbility == AbilityCenteredHealingStream ||
          m_game->m_world->m_localCreature->m_entityData.currentAbility == AbilityFireExplosion ||
          m_game->m_world->m_localCreature->m_entityData.currentAbility == AbilityTeleport) &&
         m_game->m_world->m_localCreature->m_entityData.timeSinceAbility <=
             m_game->m_world->m_localCreature->anticipationTime(AbilityUseCurrent)) ||
        m_game->m_world->m_localCreature->m_entityData.stealth > 0.0f ||
        m_game->m_world->m_localCreature->m_entityData.blockPower < 1.0f ||
        ((m_game->m_world->m_localCreature->m_entityData.currentAbility == AbilityCyclone ||
          m_game->m_world->m_localCreature->m_entityData.currentAbility == AbilityAssassinate) &&
         m_game->m_world->m_localCreature->m_entityData.timeSinceAbility >=
             m_game->m_world->m_localCreature->anticipationTime(AbilityUseCurrent) &&
         m_game->m_world->m_localCreature->m_entityData.timeSinceAbility <
             m_game->m_world->m_localCreature->anticipationTime(AbilityUseCurrent) +
                 m_game->m_world->m_localCreature->strikeTime(AbilityUseCurrent))) {
        m_inGame.m_castBar->setVisible(true);
        float fraction;
        if (!m_game->m_world->m_localCreature->isCaster() &&
            m_game->m_world->m_localCreature->m_entityData.stealth > 0.0f) {
            fraction = m_game->m_world->m_localCreature->m_entityData.stealth;
            m_inGame.m_castBar->setText(L"name", L"Stealth", true);
        } else if (m_game->m_world->m_localCreature->m_entityData.currentAbility == AbilityAssassinate) {
            fraction = (m_game->m_world->m_localCreature->m_entityData.timeSinceAbility -
                        m_game->m_world->m_localCreature->anticipationTime(AbilityUseCurrent)) /
                       m_game->m_world->m_localCreature->strikeTime(AbilityUseCurrent);
            fraction = (std::max)(0.0f, fraction);
            m_inGame.m_castBar->setText(L"name", L"Assassinate", true);
        } else if (!m_game->m_world->m_localCreature->isBlocking() &&
                   !(m_game->m_world->m_localCreature->m_entityData.blockPower < 1.0f)) {
            fraction = m_game->m_world->m_localCreature->m_entityData.timeSinceAbility /
                       m_game->m_world->m_localCreature->anticipationTime(AbilityUseCurrent);
            m_inGame.m_castBar->setText(
                L"name",
                m_game->m_speech.getText(
                    m_game->m_speech.skillKey(m_game->m_world->m_localCreature->m_entityData.currentAbility),
                    L"default", L""),
                true);
        } else {
            fraction = (std::max)(0.0f, m_game->m_world->m_localCreature->m_entityData.blockPower);
            m_inGame.m_castBar->setText(L"name", L"Block Power", true);
        }
        m_inGame.m_castBarFill->m_transformation->m_deformation.value().identity();
        m_inGame.m_castBarFill->m_transformation->m_deformation.value().scale(fraction, 1.0f);
        m_inGame.m_castBarFill->m_transformation->attributesChanged(true);
    } else {
        m_inGame.m_castBar->setVisible(false);
    }
    if (m_game->m_world->m_localCreature->m_stamina < 1.0f) {
        m_inGame.m_staminaBarFill->m_transformation->m_deformation.value().identity();
        m_inGame.m_staminaBarFill->m_transformation->m_deformation.value().scale(
            m_game->m_world->m_localCreature->m_stamina, 1.0f);
        m_inGame.m_staminaBarFill->m_transformation->attributesChanged(true);
        m_inGame.m_staminaBar->setVisible(true);
    } else {
        m_inGame.m_staminaBar->setVisible(false);
    }
    m_inGame.m_equipmentSlots[EquipmentSlotWeaponLeft]->m_item =
        m_game->m_world->m_localCreature->m_entityData.equipment.weaponLeft;
    m_inGame.m_equipmentSlots[EquipmentSlotWeaponRight]->m_item =
        m_game->m_world->m_localCreature->m_entityData.equipment.weaponRight;
    m_inGame.m_equipmentSlots[EquipmentSlotRingLeft]->m_item =
        m_game->m_world->m_localCreature->m_entityData.equipment.ringLeft;
    m_inGame.m_equipmentSlots[EquipmentSlotRingRight]->m_item =
        m_game->m_world->m_localCreature->m_entityData.equipment.ringRight;
    m_inGame.m_equipmentSlots[EquipmentSlotNeck]->m_item =
        m_game->m_world->m_localCreature->m_entityData.equipment.neck;
    m_inGame.m_equipmentSlots[EquipmentSlotShoulder]->m_item =
        m_game->m_world->m_localCreature->m_entityData.equipment.shoulder;
    m_inGame.m_equipmentSlots[EquipmentSlotChest]->m_item =
        m_game->m_world->m_localCreature->m_entityData.equipment.chest;
    m_inGame.m_equipmentSlots[EquipmentSlotHands]->m_item =
        m_game->m_world->m_localCreature->m_entityData.equipment.hands;
    m_inGame.m_equipmentSlots[EquipmentSlotFeet]->m_item =
        m_game->m_world->m_localCreature->m_entityData.equipment.feet;
    m_inGame.m_equipmentSlots[EquipmentSlotPet]->m_item = m_game->m_world->m_localCreature->m_entityData.equipment.pet;
    if (m_inGame.m_equipmentInventoryWidget->m_node->isVisible()) {
        for (SpriteWidget *slot : m_inGame.m_equipmentSlots) {
            m_buttonCenters.push_back(plasma::Vector<2, int>(slot->getContentPosition() + 0.5f * slot->screenSize()));
            slot->m_node->setText(L"count", L"", true);
            plasma::Node *frame = slot->m_node->findNode(L"frame");
            frame->m_display->m_fillColor =
                slot->m_item.textColor(m_game->m_world->m_localCreature->m_entityData.region);
            frame->m_display->attributesChanged(true);
        }
    }
    if (m_mapOpen)
        m_inGame.m_selector->setVisible(false);
    if (m_inGame.m_selector->isVisible()) {
        m_inGame.m_selector->findNode(L"left")->m_display->m_fillColor =
            m_game->m_controlStates[Controls::ButtonLeft] || m_game->m_gamepadWalkVector[0] < -0.2f
                ? plasma::Vector<4, float>(1.0f, 0.0f, 0.2f, 1.0f)
                : plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f);
        m_inGame.m_selector->findNode(L"right")->m_display->m_fillColor =
            m_game->m_controlStates[Controls::ButtonRight] || m_game->m_gamepadWalkVector[0] > 0.2f
                ? plasma::Vector<4, float>(1.0f, 0.0f, 0.2f, 1.0f)
                : plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f);
    }
    m_inGame.m_quickButtons[QuickSlotItem].button->m_display->m_fillColor =
        !isInMenu() && m_game->m_controlStates[Controls::ButtonQuickItem]
            ? plasma::Vector<4, float>(2.0f, 2.0f, 2.0f, 1.0f)
            : plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f);
    m_inGame.m_quickButtons[QuickSlotUse].button->m_display->m_fillColor =
        !isInMenu() && m_game->m_controlStates[Controls::ButtonUse] ? plasma::Vector<4, float>(2.0f, 2.0f, 2.0f, 1.0f)
                                                                    : plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f);
    m_inGame.m_quickButtons[QuickSlotSkill].button->m_display->m_fillColor =
        !isInMenu() && m_game->m_controlStates[Controls::ButtonUltimateSkill]
            ? plasma::Vector<4, float>(2.0f, 2.0f, 2.0f, 1.0f)
            : plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f);
    m_inGame.m_quickButtons[QuickSlotItem].button->findNode(L"frame")->m_display->m_fillColor =
        !isInMenu() && m_game->m_controlStates[Controls::ButtonQuickItem]
            ? plasma::Vector<4, float>(2.0f, 2.0f, 2.0f, 1.0f)
            : plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f);
    m_inGame.m_quickButtons[QuickSlotUse].button->findNode(L"frame")->m_display->m_fillColor =
        !isInMenu() && m_game->m_controlStates[Controls::ButtonUse] ? plasma::Vector<4, float>(2.0f, 2.0f, 2.0f, 1.0f)
                                                                    : plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f);
    m_inGame.m_quickButtons[QuickSlotSkill].button->findNode(L"frame")->m_display->m_fillColor =
        !isInMenu() && m_game->m_controlStates[Controls::ButtonUltimateSkill]
            ? plasma::Vector<4, float>(2.0f, 2.0f, 2.0f, 1.0f)
            : plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f);
    for (plasma::Node *button : m_inGame.m_menuButtons)
        button->setVisible(m_inGame.m_menuOpen);
    m_inGame.m_crosshair->setVisible(!m_inGame.m_speechWidget->m_node->isVisible() && !isInMenu() &&
                                     !m_game->m_world->m_localCreature->m_entityData.binaryToggles.leanForwardPose &&
                                     m_game->m_world->m_localCreature->m_entityData.currentAbility !=
                                         AbilityEagleFlight);
    m_inGame.m_zoomCrosshair->setVisible(!m_inGame.m_speechWidget->m_node->isVisible() && !isInMenu() &&
                                         m_game->m_world->m_localCreature->m_entityData.binaryToggles.leanForwardPose);
    m_inGame.m_node->setVisible(!m_startMenuNode->isVisible() && !m_characterSelection.m_node->isVisible() &&
                                !m_characterCreationNode->isVisible() && !m_worldSelection.m_node->isVisible() &&
                                !m_worldCreationNode->isVisible());
    m_inGame.m_mapOverlayWidget->m_node->setVisible(m_inGame.m_node->isVisible());
    if (m_mapOpen && !m_inGame.m_recentLandsText->isVisible()) {
        m_land = m_game->m_world->m_localCreature->m_playerData->visitedRegions.begin();
        updateTravel();
    }
    m_inGame.m_recentLandsText->setVisible(m_mapOpen);
    if (m_inGame.m_recentLandsText->isVisible()) {
        m_inGame.m_leftLandButton->m_display->m_fillColor = m_inGame.m_leftLandButton->widget()->isPointerOver()
                                                                ? plasma::Vector<4, float>(0.0f, 1.0f, 1.0f, 1.0f)
                                                                : plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f);
        m_inGame.m_rightLandButton->m_display->m_fillColor = m_inGame.m_rightLandButton->widget()->isPointerOver()
                                                                 ? plasma::Vector<4, float>(0.0f, 1.0f, 1.0f, 1.0f)
                                                                 : plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f);
    }
    if (m_mapOpen && m_game->m_engine->getPointer()[0] < m_scale * 300.0f &&
        m_game->m_engine->getPointer()[1] > (float)m_game->m_height - m_scale * 90.0f &&
        m_land != m_game->m_world->m_localCreature->m_playerData->visitedRegions.end()) {
        m_game->m_worldMap->m_selectedLand = *m_land;
        m_inGame.m_recentLandsText->setTextColor(plasma::Vector<4, float>(0.0f, 1.0f, 1.0f, 1.0f));
        m_cursor->setText(L"");
    } else {
        m_game->m_worldMap->m_selectedLand = plasma::Vector<2, int>(0x7fffffff, 0x7fffffff);
        m_inGame.m_recentLandsText->setTextColor(plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f));
    }
    static unsigned char previousHitCombo = 0;
    if (m_game->m_world->m_localCreature->m_entityData.hitCombo != 0 &&
        m_game->m_world->m_localCreature->m_entityData.hitCombo != previousHitCombo) {
        std::wstringstream combo;
        combo << L"     " << m_game->m_world->m_localCreature->m_entityData.hitCombo;
        if (m_game->m_world->m_localCreature->m_entityData.hitCombo == 1)
            combo << L" HIT";
        else
            combo << L" HITS";
        if (m_game->m_world->m_localCreature->m_entityData.hitCombo >= m_game->m_world->m_localCreature->maxCombo()) {
            combo << "!";
            m_inGame.m_comboPoints->setTextColor(plasma::Vector<4, float>(0.0f, 1.0f, 1.0f, 1.0f));
        } else {
            m_inGame.m_comboPoints->setTextColor(plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f));
        }
        m_inGame.m_comboPoints->setText(combo.str());
        m_inGame.m_comboPoints->play(L"hit", false);
    }
    previousHitCombo = (unsigned char)m_game->m_world->m_localCreature->m_entityData.hitCombo;
    m_inGame.m_comboPoints->setVisible(m_inGame.m_comboPoints->isPlaying());
    m_inGame.m_missionTag->setVisible(m_inGame.m_missionTag->isPlaying());
    m_game->m_engine->advance((int)step);
}

void Interface::updateAuras()
{
    m_inGame.m_auraBar->destroyChildren();
    for (const Creature::Buff &buff : m_game->m_world->m_localCreature->m_buffs) {
        int type = buff.type;
        m_auraIcons[type];
    }
    int x = -5;
    for (const Creature::Buff &buff : m_game->m_world->m_localCreature->m_buffs) {
        int type = buff.type;
        plasma::Node *icon = m_auraIcons[type];
        if (icon) {
            x += 50;
            plasma::Node *instance = icon->instance(m_inGame.m_auraBar);
            instance->makeTransformationUnique();
            instance->m_transformation->m_deformation.value().scale(3.0f, 3.0f);
            std::list<plasma::Node *> texts;
            instance->findNodes(texts, L"info");
            for (plasma::Node *text : texts)
                text->makeShapeUnique();
            if (buff.type == BuffAncientpower || buff.type == BuffManaabsorption) {
                instance->setText(L"");
                instance->setText(L"x " + std::to_wstring((int)buff.strength));
            } else if (buff.timeRemaining > MillisecondsPerHour) {
                instance->setText(
                    std::to_wstring((int)(buff.timeRemaining + MillisecondsPerHour / 2) / MillisecondsPerHour) + L" h");
            } else if (buff.timeRemaining > 60000.0f) {
                instance->setText(std::to_wstring((int)(buff.timeRemaining + 30000.0f) / 60000) + L" m");
            } else {
                instance->setText(std::to_wstring((int)(buff.timeRemaining + 500.0f) / 1000) + L" s");
            }
            instance->setPosition(0.0f, 0.0f, (float)x, 120.0f);
        }
    }
}

// One card per saved character and one empty card for a new character, 230 apart; the three buttons are taken out
// while the cards are replaced.
void Interface::updateSlots()
{
    m_characterSelection.m_selectCharacterButton->setParent(nullptr);
    m_characterSelection.m_editCharacterButton->setParent(nullptr);
    m_characterSelection.m_deleteCharacterButton->setParent(nullptr);
    m_characterSelection.m_node->destroyChildren();
    m_characterSelection.m_previewWidgets.clear();
    for (int i = 0; i < (int)m_game->m_savedCharacters.size() + 1; i++) {
        Creature *creature = nullptr;
        if (i < (int)m_game->m_savedCharacters.size())
            creature = m_game->m_savedCharacters[i];
        CharacterPreviewWidget *widget =
            new CharacterPreviewWidget(m_game, m_blackWidget->instance(m_characterSelection.m_node));
        widget->resizeTo(600.0f, 220.0f, true);
        widget->m_creature = creature;
        widget->moveTo(300.0f, (float)(50 + i * 230), true);
        m_characterSelection.m_previewWidgets.push_back(widget);
    }
    m_characterSelection.m_node->initializeWidgets();
    m_characterSelection.m_selectCharacterButton->setParent(m_characterSelection.m_node);
    m_characterSelection.m_editCharacterButton->setParent(m_characterSelection.m_node);
    m_characterSelection.m_deleteCharacterButton->setParent(m_characterSelection.m_node);
}

// The crafting inventory's tabs: the class's weapons and armor, then the four common ones.
void Interface::updateStyle()
{
    std::list<plasma::Node *> tabNodes;
    plasma::Node *tab;
    switch (m_game->m_world->m_localCreature->m_entityData.classType) {
    case ClassWarrior:
        tab = m_tabNode->clone(nullptr);
        setIcon(tab, L"background", L"craft-warrior-weapons");
        tabNodes.push_back(tab);
        tab = m_tabNode->clone(nullptr);
        setIcon(tab, L"background", L"craft-warrior-armor");
        tabNodes.push_back(tab);
        break;
    case ClassRanger:
        tab = m_tabNode->clone(nullptr);
        setIcon(tab, L"background", L"craft-ranger-weapons");
        tabNodes.push_back(tab);
        tab = m_tabNode->clone(nullptr);
        setIcon(tab, L"background", L"craft-ranger-armor");
        tabNodes.push_back(tab);
        break;
    case ClassRogue:
        tab = m_tabNode->clone(nullptr);
        setIcon(tab, L"background", L"craft-rogue-weapons");
        tabNodes.push_back(tab);
        tab = m_tabNode->clone(nullptr);
        setIcon(tab, L"background", L"craft-rogue-armor");
        tabNodes.push_back(tab);
        break;
    case ClassMage:
        tab = m_tabNode->clone(nullptr);
        setIcon(tab, L"background", L"craft-mage-weapons");
        tabNodes.push_back(tab);
        tab = m_tabNode->clone(nullptr);
        setIcon(tab, L"background", L"craft-mage-armor");
        tabNodes.push_back(tab);
        break;
    }
    tab = m_tabNode->clone(nullptr);
    setIcon(tab, L"background", L"craft-jewels");
    tabNodes.push_back(tab);
    tab = m_tabNode->clone(nullptr);
    setIcon(tab, L"background", L"craft-cooking");
    tabNodes.push_back(tab);
    tab = m_tabNode->clone(nullptr);
    setIcon(tab, L"background", L"craft-alchemy");
    tabNodes.push_back(tab);
    tab = m_tabNode->clone(nullptr);
    setIcon(tab, L"background", L"craft-ingredients");
    tabNodes.push_back(tab);
    m_inGame.m_craftingInventoryWidget->setTabNodes(tabNodes);
}

void Interface::updateTravel()
{
    m_inGame.m_rightLandButton->setVisible(m_land !=
                                           m_game->m_world->m_localCreature->m_playerData->visitedRegions.begin());
    std::list<plasma::Vector<2, int>>::iterator next = m_land;
    ++next;
    m_inGame.m_leftLandButton->setVisible(next != m_game->m_world->m_localCreature->m_playerData->visitedRegions.end());
    if (m_land != m_game->m_world->m_localCreature->m_playerData->visitedRegions.end()) {
        plasma::Vector<2, int> land = *m_land;
        // The hash of the region seeds its generated name.
        m_inGame.m_recentLandsText->setText(
            m_game->m_speech.getText(m_game->m_speech.getLandName(m_game->m_world->landBiome(land)), L"named",
                                     randomName(random(land[0], land[1]), RaceNone)));
    }
}

// One card per saved world of the selected kind (local, or "online_...") and one empty card for a new world; both maps
// link a card's node and its index into Game::m_worlds.
void Interface::updateWorldList()
{
    m_worldSelection.m_selectWorldButton->setParent(nullptr);
    m_worldSelection.m_editWorldButton->setParent(nullptr);
    m_worldSelection.m_deleteWorldButton->setParent(nullptr);
    m_worldSelection.m_node->destroyChildren();
    m_worldSelection.m_previewCards.clear();
    m_worldSelection.m_cardIndices.clear();
    m_worldSelection.m_cardsByIndex.clear();
    int row = 0;
    for (int i = 0; i < (int)m_game->m_worlds.size() + 1; i++) {
        WorldInfo *world = nullptr;
        if (i < (int)m_game->m_worlds.size()) {
            world = m_game->m_worlds[i];
            bool online = world->m_name.size() >= 7 && world->m_name.substr(0, 7) == "online_";
            if (m_worldSelection.m_online != online)
                continue;
        }
        WorldPreviewWidget *widget = new WorldPreviewWidget(m_game, m_blackWidget->instance(m_worldSelection.m_node));
        widget->m_worldInfo = world;
        widget->resizeTo(600.0f, 220.0f, true);
        widget->moveTo(300.0f, (float)(50 + row * 230), true);
        m_worldSelection.m_previewCards.push_back(widget->getNode());
        m_worldSelection.m_cardIndices[widget->getNode()] = i;
        m_worldSelection.m_cardsByIndex[i] = widget->getNode();
        row++;
    }
    m_worldSelection.m_node->initializeWidgets();
    m_worldSelection.m_selectWorldButton->setParent(m_worldSelection.m_node);
    m_worldSelection.m_editWorldButton->setParent(m_worldSelection.m_node);
    m_worldSelection.m_deleteWorldButton->setParent(m_worldSelection.m_node);
}

}
