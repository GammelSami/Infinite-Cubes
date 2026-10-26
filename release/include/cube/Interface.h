// Interface builds and updates the game's menus, HUD and widgets from Plasma nodes. Nodes and widgets belong to the
// Plasma engine; Interface does not delete them. Nested records group character selection, world selection and in-game
// controls.
#pragma once
#include <list>
#include "plasma/Vector.h"
#include <map>
#include <string>
#include <vector>
#include "plasma/Vector.h"
#include "cube/ResolutionInfo.h"
#include "cube/InterfaceIds.h"

namespace plasma {
class Node;
class Widget;
}

namespace cube {

class Game;
class StartMenuWidget;
class CharacterPreviewWidget;
class CharacterStyleWidget;
class InventoryWidget;
class CharacterWidget;
class FormulaDetailsWidget;
class SystemWidget;
class EnchantWidget;
class VoxelWidget;
class AdaptionWidget;
class QuestionWidget;
class MapOverlayWidget;
class MultiplayerWidget;
class HelpWidget;
class ChatWidget;
class PreviewWidget;
class SpriteWidget;
class SpeechWidget;
class OptionsWidget;
class ControlsWidget;

struct Interface {
    struct CharacterSelection {
        CharacterSelection()
            : m_characterScroll(0.0f)
        {}
        plasma::Node *m_node; // created unnamed
        std::vector<CharacterPreviewWidget *> m_previewWidgets;
        // The character selection's buttons: clones of gui.plx's "button2", "smallbutton" and "button" under the
        // selection's node, connected to Game's handlers.
        plasma::Node *m_selectCharacterButton;
        plasma::Node *m_editCharacterButton;
        plasma::Node *m_deleteCharacterButton;
        float m_characterScroll; // Initially zero; movePointer adds the dragged distance.
    };

    struct WorldSelection {
        WorldSelection();

        plasma::Node *m_node; // created unnamed
        std::vector<plasma::Node *> m_previewCards;
        std::map<plasma::Node *, int> m_cardIndices;  // Maps each world's card node to its index in Game::m_worlds.
        std::map<int, plasma::Node *> m_cardsByIndex; // Reverse mapping from world index to card node.
        // The buttons, cloned by load from the "button", "button2" and "smallbutton" templates of gui.plx and
        // connected to Game's handlers.
        plasma::Node *m_multiplayerWorldsButton; // "Multiplayer Worlds..." under the root, Game::onToggleWorlds
        plasma::Node *m_selectWorldButton;       // "Select", Game::onSelectWorld
        plasma::Node *m_editWorldButton;         // a small button, Game::onEditWorld, hidden
        plasma::Node *m_deleteWorldButton;       // "Delete World", Game::onDeleteWorld, hidden
        float m_scroll;                          // 0; movePointer adds the dragged distance
        bool m_online; // 0; updateWorldList lists the worlds named "online_..." when set, the others when clear
    };

    struct QuickButton {
        QuickButton()
            : unused18(-1)
        {}

        plasma::Node *button;   // "quickbutton"
        plasma::Node *cooldown; // "cooldown"
        plasma::Node *disabled; // "disabled"
        int unused18;           // -1 at creation; never read
    };

    struct InGame {
        InGame()
            : m_menuOpen(false),
              m_tab(0)
        {}
        plasma::Node *m_node; // created unnamed; parent of the widgets' nodes
        std::vector<QuickButton> m_quickButtons;
        std::vector<plasma::Node *> m_menuButtons;
        plasma::Node *m_levelText;
        plasma::Node *m_auraBar;
        plasma::Node *m_hpBar;             // "hpbar"
        plasma::Node *m_hpBarFill;         // "bar" under hpbar
        plasma::Node *m_mpBar;             // "mpbar"
        plasma::Node *m_mpBarFill;         // "bar" under mpbar
        plasma::Node *m_experienceBar;     // "experiencebar"
        plasma::Node *m_xpBar;             // "xpbar"
        plasma::Node *m_chargeBar;         // "chargebar"
        plasma::Node *m_castBar;           // "castbar"
        plasma::Node *m_castBarFill;       // "bar" under castbar
        plasma::Node *m_staminaBar;        // "staminabar"
        plasma::Node *m_staminaBarFill;    // "bar" under staminabar
        plasma::Node *m_quickBar;          // created unnamed; parent of the quickbutton clones
        plasma::Node *m_infoTextSmall;     // 12 pixels, stroke 3
        plasma::Node *m_infoTextLarge;     // 28 pixels, stroke 6
        plasma::Node *m_infoTextMedium;    // 21 pixels, stroke 5
        plasma::Node *m_location;          // "location"
        plasma::Node *m_levelInfo;         // "levelinfo"
        plasma::Node *m_crosshair;         // "crosshair"
        plasma::Node *m_zoomCrosshair;     // "zoomcrosshair"
        plasma::Node *m_lockedTarget;      // "lockedtarget"
        plasma::Node *m_highlightedTarget; // "highlightedtarget"
        plasma::Node *m_selector;          // "selector"
        plasma::Node *m_goldText;
        plasma::Node *m_contextItemText; // the text "100" named "contextitem"
        plasma::Node *m_comboPoints;     // "combopoints"
        InventoryWidget *m_equipmentInventoryWidget;
        CharacterWidget *m_characterWidget;
        InventoryWidget *m_craftingInventoryWidget;
        FormulaDetailsWidget *m_formulaDetailsWidget; // Read by Game.
        InventoryWidget *m_vendorInventoryWidget;
        SystemWidget *m_systemWidget;
        EnchantWidget *m_enchantWidget;
        VoxelWidget *m_voxelWidget;
        AdaptionWidget *m_adaptionWidget;
        QuestionWidget *m_questionWidget;
        MapOverlayWidget *m_mapOverlayWidget;
        MultiplayerWidget *m_multiplayerWidget;
        HelpWidget *m_helpWidget;
        ChatWidget *m_chatWidget;
        plasma::Node *m_recentLandsText;
        plasma::Node *m_leftLandButton;
        plasma::Node *m_rightLandButton;
        plasma::Node *m_missionTag; // "missiontag-white"
        PreviewWidget *m_previewWidget;
        std::vector<SpriteWidget *> m_equipmentSlots;
        SpriteWidget *m_quickItemSprites[QuickSlotCount];
        std::vector<SpeechWidget *> m_speechWidgets;
        SpeechWidget *m_speechWidget; // constructor on "speech-next"
        bool m_menuOpen;
        int m_tab; // 0; the panel showTab last toggled: 0 equipment, 1 crafting, 2 multiplayer
    };

    explicit Interface(Game *game);

    void load(const std::vector<ResolutionInfo> *displayModes);
    void updateResolution(int width, int height);
    // Appends (x, y) in widget's units, moved by its node's pivot and taken through its node's accumulated matrix
    // (with the perspective divide) and truncated, to m_buttonCenters. Called by the Render of EnchantWidget,
    // QuestionWidget, StartMenuWidget and SystemWidget for each button label.
    void addButtonCenter(plasma::Widget *widget, int x, int y);
    // Toggles the controls and options widgets by changing their nodes' display visibility.
    void updateSlots();
    // Sets crafting-page backgrounds for the local player's class ("craft-warrior-weapons", etc.).
    void updateStyle();
    void refresh();
    void updateWorldList();
    void movePointer(int deltaX, int deltaY);
    void handleMouseDown(int button);
    void handleMouseUp(int button);
    bool isInMenu();
    void toggleControlsWidget();
    void toggleOptionsWidget();
    // The other panels' toggles.
    void toggleCraftingWidget();
    void toggleHelpWidget();
    void toggleInventoryWidget();
    void toggleMultiplayerWidget();
    void toggleSystemWidget();
    void toggleWorldMap();
    void update(float step);
    void updateAuras();
    // Hides the equipment, crafting and multiplayer panels, then toggles the requested panel using m_tab.
    void showTab(int tab);
    // Steps m_land through the player's land list and scrolls the map to it, zoomed to at least 4.
    void lastLand();
    void nextLand();
    // Shows land navigation buttons when a land exists in that direction and displays the current land's name.
    void updateTravel();
    // Creates a named text node under parent using resource1.dat at the requested pixel size, stroke radius and
    // TextFlags.
    plasma::Node *createText(const std::wstring &text, const std::wstring &name, plasma::Node *parent, int flags,
                             int size, int strokeRadius);
    // Replaces the child "icon" of node's descendant targetName by an instance of the icon iconName (found under
    // m_icons), centred on the target's shape and scaled to its size.
    void setIcon(plasma::Node *node, const std::wstring &targetName, const std::wstring &iconName);

    plasma::Node *m_startMenuNode;
    plasma::Node *m_startMenuButtons;
    StartMenuWidget *m_startMenuWidget;
    CharacterSelection m_characterSelection;
    plasma::Node *m_characterCreationNode;
    CharacterStyleWidget *m_characterStyleWidget;
    plasma::Node *m_characterStyleNode;
    plasma::Node *m_createCharacterButton;
    plasma::Node *m_characterName; // "charactername"
    WorldSelection m_worldSelection;
    plasma::Node *m_worldCreationNode;
    plasma::Node *m_createWorldButton;
    plasma::Node *m_worldSeed; // "worldSeed"; its "edit" text is parsed by Game
    plasma::Node *m_worldName; // The worldName edit field; its text names the world.
    InGame m_inGame;
    plasma::Node *m_blackWidget; // The blackwidget template, cloned under InGame::m_node for late-created widgets.
    plasma::Node *m_speech;      // "speech"
    plasma::Node *m_tabNode;     // "tab"; updateStyle clones it for each crafting tab
    plasma::Node *
        m_icons; // an unnamed node without a parent holding icons.plx; the aura, ability and rarity icons are its children, setIcon instances them
    plasma::Node *m_cursor;
    plasma::Node *m_wait; // "wait"
    plasma::Node *m_backButton;
    plasma::Node *m_enemyIcon;              // "enemy"
    plasma::Node *m_monsterIcon;            // "monster"
    plasma::Node *m_crystalIcon;            // "crystal"
    plasma::Node *m_brazierIcon;            // "brazier"
    plasma::Node *m_manaPumpIcon;           // "manapump"
    plasma::Node *m_npcIcon;                // "npc"
    plasma::Node *m_staticIcon;             // "static"
    plasma::Node *m_objectIcon;             // "object"
    plasma::Node *m_starIcon;               // "star"
    plasma::Node *m_crossIcon;              // "cross"
    plasma::Node *m_flightPointIcon;        // "flight-point"
    plasma::Node *m_unknownFlightPointIcon; // "unknown-flight-point"
    plasma::Node *m_homeIcon;               // "home"
    plasma::Node *m_mapPositionIcon;        // "mapposition"
    plasma::Node *m_directionIcon;          // "direction"
    plasma::Node *m_armorShopIcon;          // "armor-shop"
    plasma::Node *m_weaponShopIcon;         // "weapon-shop"
    plasma::Node *m_itemShopIcon;           // "item-shop"
    plasma::Node *m_identifierIcon;         // "identifier"
    plasma::Node *m_smithyIcon;             // "smithy"
    plasma::Node *m_carpentersShopIcon;     // "carpenters-shop"
    plasma::Node *m_tailorsShopIcon;        // "tailors-shop"
    plasma::Node *m_innIcon;                // "inn"
    plasma::Node *m_shrineIcon;             // "shrine"
    plasma::Node *m_spiritIcon;             // "spirit"
    plasma::Node *m_bellIcon;               // "bell"
    plasma::Node *m_harpIcon;               // "harp"
    plasma::Node *m_whistleIcon;            // "whistle"
    plasma::Node *m_solvedIcon;             // "solved"
    plasma::Node *m_gnomeIcon;              // "gnome"
    plasma::Node *m_hangGliderIcon;         // "hangglider"
    plasma::Node *m_boatIcon;               // "boat"
    plasma::Node *m_climbingSpikesIcon;     // "climbingspikes"
    plasma::Node *m_reinsIcon;              // "reins"
    plasma::Node *m_destinationIcon;        // "destination"
    plasma::Node *m_crafting1Icon;          // "crafting1"
    plasma::Node *m_crafting2Icon;          // "crafting2"
    plasma::Node *m_crafting3Icon;          // "crafting3"
    plasma::Node *m_crafting4Icon;          // "crafting4"
    plasma::Node *m_flightMasterIcon;       // "flight-master"
    plasma::Node *m_guildHallIcon;          // "guild-hall"
    plasma::Node *m_flowerIcon;             // "flower"
    plasma::Node *m_keyIcon;                // "key"
    plasma::Node *m_treasureIcon;           // "treasure"
    plasma::Node *m_witchIcon;              // "witch"
    plasma::Node *m_demonPortalIcon;        // "demonportal"
    // Icon map keyed by integer ability IDs.
    std::map<int, plasma::Node *> m_auraIcons;
    std::map<int, plasma::Node *> m_abilityIcons;
    plasma::Node *m_weaponRarity[6]; // "weaponrarity0".."weaponrarity5"
    plasma::Node *m_armorRarity[6];  // "armorrarity0".."armorrarity5"
    OptionsWidget *m_optionsWidget;
    ControlsWidget *m_controlsWidget;
    float m_pointerY;
    float m_scale;
    bool m_mapOpen;
    bool m_choosingShrine;               // Initially zero.
    plasma::Node *m_textFx;              // "textFX"
    plasma::Node *m_lifeBars;            // "lifebars"
    plasma::Node *m_enemyLifeBarSmall;   // "enemylifebar:small"
    plasma::Node *m_friendLifeBarSmall;  // "friendlifebar:small"
    plasma::Node *m_staticLifeBarSmall;  // "staticlifebar:small"
    plasma::Node *m_neutralLifeBarSmall; // "neutrallifebar:small"
    plasma::Node *m_enemyLifeBar;        // "enemylifebar"
    plasma::Node *m_friendLifeBar;       // "friendlifebar"
    plasma::Node *m_staticLifeBar;       // "staticlifebar"
    plasma::Node *m_neutralLifeBar;      // "neutrallifebar"
    int m_selectedCharacter;             // Initially -1.
    std::list<plasma::Vector<2, int>> m_buttonCenters;
    Game *m_game; // the constructor's argument
    std::list<plasma::Vector<2, int>>::iterator m_land;
};

// Formats a number for the interface using abbreviated units, such as "1.2M" or "12.3K".
std::wstring formatNumber(int value);

}
