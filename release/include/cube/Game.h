// Game owns the interface, local host and client, options, controls, camera, crafting state, worker threads and
// character and world databases. WinMain creates the sole instance. Startup preloads sounds, opens Save/characters.db,
// loads characters and the interface, creates the local creature and starts the worker threads. Some auxiliary record
// types remain opaque.
#pragma once
#include <windows.h>
#include <list>
#include <map>
#include <set>
#include <string>
#include <thread>
#include <vector>
#include "plasma/Matrix.h"
#include "plasma/Vector.h"
#include "steam/steam_api.h"
#include "cube/Client.h"
#include "cube/Controls.h"
#include "cube/Creature.h"
#include "cube/Database.h"
#include "cube/Host.h"
#include "cube/Item.h"
#include "cube/Speech.h"
#include "cube/Interface.h"
#include "cube/KeyNames.h"
#include "cube/Options.h"
#include "cube/ResolutionInfo.h"
#include "cube/Server.h"
#include "cube/ServerUpdates.h"

namespace plasma {
class D3D11Engine;
}

namespace gfx {
class D3D11Renderer;
}

namespace cube {

// Appends dialog commands: integers by value; items as category, id, modifier, material and rarity.
std::vector<int> &operator<<(std::vector<int> &commands, const Item &item);
std::vector<int> &operator<<(std::vector<int> &commands, int value);

// Rounds to one decimal place, half away from zero, for interface values.
float roundTenth(float value);
// Free helper used by Game.cpp.
double blend(double seconds, float percent);

class InventoryWidget;

class World;
class WorldMap;
class Audio;
class Creature;
class BaseWidget;
struct WorldInfo;
struct ZoneItem;

// A particle of the game's own effects.
struct Particle {
    Particle();

    ParticleType type; // the packet's particleType
    int age;           // 0, or rand() % 500 for type 5
    plasma::Vector<3, Fixed> position;
    plasma::Vector<3, Fixed> startPosition;
    plasma::Vector<3, float> rotation; // random, in degrees
    plasma::Vector<4, float> color;
    plasma::Vector<3, float> acceleration; // the packet's acceleration spread by spread
    plasma::Vector<3, float> spin;         // random, times 100
    plasma::Matrix<float> matrix;          // the identity
    float size;                            // 1.0, or the packet's size
    bool collidesWithBlocks;               // true for type 5
    unsigned char motionDisabled;
    int lifetime; // Duration in milliseconds, initially 4000.
};

struct TextFX {
    TextFX();

    int age;                           // Initially zero; updated each frame.
    float rise;                        // Initially -100.0; updated each frame.
    plasma::Vector<3, Fixed> position; // Player position tracked by update.
    float offsetX;                     // Initial value 5.0; maintained by update.
    float offsetY;                     // Initial value -100.0; maintained by update.
    plasma::Vector<4, float> color;    // Colour (0.7, 0.2, 1, 1) for a feat notification.
    std::wstring text;
    int duration;    // Duration: initially 1000 milliseconds, or 3000 for a feat.
    float pixelSize; // Initial value 30.0; maintained by update.
    int textFlags;   // Initially zero.
};

struct Light {
    plasma::Vector<3, Fixed> position; // the hit's position
    plasma::Vector<3, float> color;    // Item::effectColor's red, green and blue
    float radius;                      // Initially 20.0.
    int age;                           // milliseconds
    int duration;                      // Initially 400.
};

struct MissionNotice {
    int unused00; // The display does not read this value; m_missionNotices is never filled.
    std::wstring info;
    std::wstring xp;
};

struct NearbyCreature {
    Creature *creature;
    float squaredDistance; // in blocks, from the creature's drawn position
    bool inClearView;      // false when collected

    bool operator<(const NearbyCreature &other) const { return squaredDistance < other.squaredDistance; }
};

struct MenuEntry {
    MenuEntry()
        : creature(nullptr),
          item(nullptr)
    {}

    Creature *creature; // drawn as its model when set (Renderer::render)
    Item *item;
};

class Game {
public:
    Game(World *world, WorldMap *worldMap, gfx::D3D11Renderer *renderer, plasma::D3D11Engine *engine, Audio *audio,
         const Options *options, const std::vector<ResolutionInfo> *displayModes);
    virtual ~Game();

    void applyControls(const Controls &controls);
    // Copies the options under the world's zone mesh lock, saves them and sets m_musicVolume under
    // m_musicCriticalSection.
    void applyOptions(const Options &options);
    // The answer chosen in the question widget for creature (null: only clears the widget): "change_specialization?"
    // toggles the local creature's specialization and makes the speech widget say so, "rest_inn?" pays 10 gold for an
    // interaction of type 5 with the creature when it is night (time >= 18:00 or <= 6:00).
    void ask(Creature *creature, const std::string &answer);
    // WinMain passes the key or button to bind to the controls widget.
    void assignControl(const Controls::Button &button);
    void cancelBinding();
    // Sets the seed and name of the world to play (also in the local creature's PlayerData) and removes the other
    // creatures.
    void changeWorld(int seed, const std::string &name);
    void controlCharacter(double step, std::vector<NearbyCreature> &creatures);
    void craft();
    void createParticles(Sprite *sprite, const plasma::Matrix<float> &matrix,
                         const plasma::Vector<3, float> &unusedVector, const plasma::Vector<3, float> &velocity,
                         float unusedFactor, float originX, float originY, const plasma::Vector<4, float> &color,
                         int particleType);
    // Leaves a joined game: closes the socket, joins the receive and send threads and prints "Disconnected." (nothing
    // when not connected).
    void disconnect();
    // Generates random creature information.
    void equipNewCharacter();
    // L"FEAT!".
    std::wstring getFeatText(const unsigned char &feat);
    static std::wstring getFlatDistanceText(int metres);
    static std::wstring getFoldedCountText(int count);
    std::vector<Creature *> getFollowingCreatures();
    // (tab, index) of every non-empty stack of the local creature's inventory whose item has category 1.
    std::vector<plasma::Vector<2, int>> getFoodSlots();
    Item *getHoveredEnchantItem();
    // The local creature's equipment item whose slot widget (Interface's ten sprite widgets) is under the pointer
    // while the equipment inventory is shown, else null.
    Item *getHoveredEquipment();
    // The formula under the pointer in the crafting inventory, else null.
    Item *getHoveredFormula();
    // The formula widget's hovered item, or the voxel widget's hovered upgrade, else null.
    Item *getHoveredIngredient();
    // (tab, index) of the non-empty stack under the pointer in the equipment inventory, else (-1, -1).
    plasma::Vector<2, int> getHoveredSlot();
    // The vendor's item under the pointer, else null.
    Item *getHoveredVendorItem();
    // The window procedure's input: mouse buttons 0 left, 1 right, 2 middle; keys arrive as their low byte; the wheel
    // as -1, 0 or 1; WinMain passes the mouse movement.
    void handleButtonDown(int button);
    void handleButtonUp(int button);
    void handleKeyDown(unsigned char key);
    void handleKeyUp(unsigned char key);
    void handleMouseMove(float deltaX, float deltaY);
    void handleMouseWheel(int direction);
    // Queues an interaction of type 3 with creature for the local creature.
    void interact(Creature *creature);
    // updates the creature/static-object interaction prompt; returns whether it handled one.
    bool interactionPrompt();
    // Whether the controls widget waits for a key to bind (WinMain).
    bool isAssigningControl();
    bool isInMenu();
    // Adds the formulas a character starts with to the local creature's known formulas: seven of category 1 in the
    // given rarity and, for rarity 0, the category 28 and 11 ones (one of them by class).
    void learnFormulas(int rarity);
    void loadCharacter(int slot, Creature *creature);
    // Selects the indexed map-marked creature.
    Creature *markedCreature();
    // Applies input acceleration and jump or glide controls.
    void moveCharacter(double step);
    void notifyItem(const Item &item, int count, Creature *creature);
    // The interface's "Back" button: one menu level up.
    void onBack();
    // Called by cube::InventoryWidget when its shown tab changes: refills the vendor tabs when widget is the vendor
    // inventory.
    void onChangeInventoryTab(InventoryWidget *widget);
    // Handles the character selection's Delete button.
    void onDeleteCharacter();
    // The world selection's "Delete" (deletes "Save\\world_<name>.db" and "Save\\map_<name>.db").
    void onDeleteWorld();
    // Handles the character creation page's Done button.
    void onDoneCharacter();
    // Handles the world creation page's Done button.
    void onDoneWorld();
    // Shows the character and world selection Delete buttons.
    void onEditCharacter();
    void onEditWorld();
    // m_shutdown = true (SystemWidget's "Exit Game").
    void onExit();
    // A typed character goes to the chat widget while its text box is active and no engine widget has the focus.
    void onKeyTyped(unsigned char character);
    // Handles the character selection's Play button.
    void onSelectCharacter();
    // The world selection's "Select": resets the camera and changes to the selected world, or opens the world creation
    // page for an empty slot.
    void onSelectWorld();
    void onStartMenu();
    // Switches the world list between single and multiplayer worlds.
    void onToggleWorlds();
    // Plays sound at the camera: playSound(sound, m_cameraPosition, 1, 1, true).
    void playSound(int sound);
    // Plays sound if position is within 64 blocks of the camera, fading with the squared distance, panned by the
    // position's screen x; sounds 109..119 keep m_musicDuckTime at 6000 or more. The last argument is not read.
    void playSound(int sound, const plasma::Vector<3, Fixed> &position, float volume, float speed, bool unusedFlag);
    void processHitLights();
    // Shows the next queued mission notice when the mission tag is free.
    void processNotices();
    // Stores the window size and passes it to the interface.
    void resize(int width, int height);
    void saveCharacter();
    void saveWorldInfo(int index, WorldInfo *world, bool lock);
    // Gives the speaker's dialogs to its speech widget (created on first use), and those that carry a command to the
    // question widget.
    void say(Creature *speaker, Creature *listener, const std::vector<Dialog> &dialogs);
    // Moves the map to block (x, y) at the terrain's height.
    void scrollMap(int x, int y);
    // WinMain's gamepad and key state: m_controlStates[control].
    void setControl(int control, bool pressed);
    // The gamepad sticks, zeroed inside a dead zone of radius 0.2.
    void setStickMove(float x, float y);
    void setStickTurn(float x, float y);
    // The audio engine's id of sound 0..124 ("Sounds\<name>.wav"), -1 for any other number. The constructor calls it
    // for all 125 to preload.
    int soundId(int sound);
    void update(double milliseconds);
    void updateCamera(double step);
    void updateClimateSounds(int milliseconds);
    // Advances crafting while the formula widget's craft button is held: copies the widget's formula when it starts,
    // crafts at progress 1 and speeds up (x1.2 up to 0.005 per ms) while the button stays down.
    void updateCrafting(int milliseconds);
    void updateCreatureLabels(std::vector<NearbyCreature> &creatures);
    // Refills the multiplayer widget's friend list from Steam. Fills m_craftingInventory with the local player's
    // craftable formulas.
    void updateFormulas();
    // Fills tab of m_craftingInventory with formulas and the number of times the player can craft each from its
    // ingredients.
    void updateFormulaTab(int tab, std::vector<Formula> &formulas);
    void updateFriends();
    void updateFx(int milliseconds);
    // Ages m_lights and removes the expired ones.
    void updateLights(int milliseconds);
    void updateSorting();
    void updateSpeech(double step);
    void updateTextFx(int milliseconds);
    void updateUsableItems();
    // Refills m_vendorInventory: the sold items (newest first) for tab 1, else the shop of the vendor by its class.
    void updateVendor();
    // Uses an inventory stack's item: formulas and categories 24, 27 and 28. handleButtonDown holds the zone mesh lock
    // while calling it.
    void useItem(ItemStack &stack);
    // Draws item's description panel with widget's text helpers from (x, y) downwards, scaled and wrapped at width,
    // and returns the position below the last line. showName draws the name line, showClasses the classes that can use
    // an item the player's class cannot; zoneItem, when given with an owner seed, colours an (empty) owner line;
    // compareItem is shown against item as the difference of each statistic (AdaptionWidget passes its item for an
    // upgraded copy, PreviewWidget the equipped item). unusedFlag is not read.
    plasma::Vector<2, float> writeItemInfo(BaseWidget *widget, Item *item, int count, int x, int y, float scale,
                                           int width, bool showName, bool showClasses, bool unusedFlag,
                                           ZoneItem *zoneItem, Item *compareItem);

    // Appends a chat line received from the host.
    void addChatMessage(const ChatMessage &message) { m_chatMessages.push_back(message); }

    World *m_world;
    WorldMap *m_worldMap;
    gfx::D3D11Renderer *m_renderer;
    plasma::D3D11Engine *m_engine;
    Audio *m_audio;
    KeyNames m_keyNames;
    Speech m_speech; // Constructed and destroyed with Game; supplied to Interface.
    Interface m_interface;
    Host m_host;        // Constructed with the world's sprite manager.
    Client m_client;    // Constructed with its host set to &m_host.
    int m_currentWorld; // index into m_worlds of the world in use; 0
    std::vector<Creature *> m_savedCharacters;
    std::vector<WorldInfo *> m_worlds; // the saved worlds; deleted by the destructor
    Inventory m_craftingInventory;
    Inventory m_vendorInventory;
    std::map<int, Item *> m_equippedCraftingItems;
    std::map<int, plasma::Vector<2, int>>
        m_craftingSources; // Maps a crafting row to the player's inventory (tab, slot).
    bool m_gamepadActive;  // 0; read by Tick, ControlsWidget and the interface drawing
    STEAM_CALLBACK(Game, onP2PSessionRequest, P2PSessionRequest_t);
    ServerUpdates m_serverUpdates; // Event queues for the current tick.
    bool m_controlStates[23];
    bool m_keyStates[256];
    Options m_options;                           // Copied from the constructor argument.
    Controls m_controls;                         // Constructed with Game.
    CRITICAL_SECTION m_characterCriticalSection; // spin count 16
    // Three critical sections that are initialised and deleted only (no EnterCriticalSection on any of them in the
    // program).
    CRITICAL_SECTION m_unusedLock1cc0; // spin count 16
    CRITICAL_SECTION m_unusedLock1ce8; // spin count 4000
    CRITICAL_SECTION m_unusedLock1d10; // spin count 16
    CRITICAL_SECTION
    m_musicCriticalSection; // Spin count 16; protects m_musicVolume while SetOptions updates it.
    int m_width;            // Window size set by updateResolution.
    int m_height;
    bool m_shutdown;
    plasma::Vector<2, float> m_gamepadWalkVector;
    plasma::Vector<2, float> m_gamepadTurnVector;
    plasma::Vector<3, double> m_cameraAngle;
    plasma::Vector<3, double> m_targetCameraAngle;
    bool m_pointerMoved; // Initially zero; changed by mouse-down and mouse-up handlers.
    float m_cameraDistance;
    float m_targetCameraDistance;
    plasma::Vector<3, Fixed> m_cameraPosition;
    plasma::Vector<3, Fixed> m_cameraFocusPosition;
    Fixed m_drawOriginX;
    Fixed m_drawOriginY;
    float m_screenShake;
    int m_mapZoom;
    plasma::Vector<3, Fixed> m_targetMapPosition;
    plasma::Vector<3, Fixed> m_mapPosition;
    bool m_mapZoomChanged;
    long long m_unused1e40; // 0; no other access
    plasma::Vector<2, float> m_mapOverlayPosition;
    float m_mapOverlayScale;
    bool
        m_mapRightClickToggle; // 0; handleButtonDown flips it on each right click while the map is open; nothing reads it
    long long m_crosshairCreatureId;   // ID of the creature under the pointer; initially zero.
    long long m_highlightedCreatureId; // Initially 0.
    long long m_promptCreatureId;      // Initially 0.
    plasma::Vector<3, int> m_crosshairItem;
    plasma::Vector<3, int> m_crosshairObject;
    bool m_creatureInView;
    // The ring menu's rotation (degrees, -360 / count per selected item) and its shown rotation, which update eases
    // towards it.
    float m_ringRotation;
    float m_ringRadius;
    long long m_vendorId; // ID of the creature whose shop is open; initially zero.
    int m_vendorClass;    // Vendor's classType; classes 129, 130 and 156 have separate stocks. Initially zero.
    plasma::Matrix<float> m_cameraMatrix; // Camera direction basis.
    plasma::Matrix<float> m_worldViewMatrix;
    plasma::Matrix<float> m_viewMatrix;
    plasma::Matrix<float> m_projectionMatrix;
    // The constructor starts all but receive and send.
    std::thread m_generationThread;
    std::thread m_chunkThread;
    std::thread m_mapThread;
    std::thread m_aiThread;
    std::thread m_musicThread;
    std::thread m_receiveThread;
    std::thread m_sendThread;
    std::thread m_hostThread;
    std::string m_currentMusicFile;
    float m_musicVolume;              // 1.0; set by SetOptions
    int m_musicDuckTime;              // Initially zero.
    int m_successMusicTime;           // Initially 0.
    std::list<Item> m_unusedItemList; // Constructed and destroyed only.
    volatile bool
        m_running; // Initially true; cleared before the destructor joins the worker threads, whose loops test it.
    int m_seed;
    std::string m_worldName;
    std::list<Particle> m_particles; // Records filled from a sprite's voxels.
    std::list<Light> m_lights;       // Records filled and aged by the game update.
    std::list<TextFX> m_textFx;
    int m_ringSelection;
    int m_unused20c4;          // Initially zero; no other access found.
    int m_markedCreatureIndex; // Signed selector index.
    float m_craftingProgress;  // Crafting progress: -1 when idle; completes an item at 1.0.
    float m_craftingSpeed;     // Progress per millisecond: reset to 0.001, multiplied by 1.2 up to 0.005.
    int m_unused20d4;          // not initialised by the constructor; no access
    Formula m_craftingFormula;
    plasma::Vector<2, int> m_craftingCell;
    std::list<ChatMessage> m_chatMessages; // Chat lines received from the host through Client::receive.
    std::list<ItemStack>
        m_soldItems; // Last ten items sold by the player, newest last; the vendor's second tab displays them newest first.
    ItemStack *m_mapItemStack;  // Stack of the map item (formula id 13) last used; initially zero.
    std::string m_unusedString; // empty; constructed and destroyed only
    volatile bool
        m_connected; // Set while connected to a joined game; the receive and send threads run while it is set. Initially zero.
    Server *m_server;
    UINT_PTR m_socket;                                  // Network socket, closed by the destructor; initially zero.
    std::list<ServerUpdates::HitPacket> m_hitPackets;   // Queued hit packets, using Client's hit packet type.
    std::list<ServerUpdates::BuffPacket> m_buffPackets; // Queued buff packets, using Client's buff packet type.
    std::list<ServerUpdates::BulletPacket>
        m_bulletPackets;                         // Queued projectile packets, using Client's projectile packet type.
    std::list<std::wstring> m_unusedMessageList; // Chat message queue; constructed and destroyed only.
    std::list<plasma::Vector<2, int>>
        m_requestedZones; // Requested zone coordinates; Client::update sends packet 15 once a second and Host::receive queues them for the connection.
    std::list<MissionNotice> m_missionNotices; // Queued notices displayed one at a time.
    int m_resetOnDisconnect;                   // 0; disconnect zeroes it again; nothing reads it
    int m_currentCharacterSlot;
    Database m_charactersDatabase; // Character database at Save/characters.db.
    Database m_worldsDatabase;     // Database holding the saved-world list.
};

}
