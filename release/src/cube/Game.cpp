#include <winsock2.h>
#include <windows.h>
#include <mmsystem.h>
#include <algorithm>
// Game startup, input handling, simulation updates, menus, crafting, audio and persistence; see cube/Game.h.
#include <cstdlib>
#include "plasma/TextIds.h"
#include "cube/RendererIds.h"
#include "cube/MathConstants.h"
#include "cube/SpeechIds.h"
#include "cube/Game.h"

#include "Parallel.h"
#include <algorithm>
#include <array>
#include <iostream>
#include <sstream>

#include <zlib.h>

#include "cube/Audio.h"
#include "cube/BytesIO.h"
#include "cube/widgets/CharacterStyleWidget.h"
#include "cube/widgets/ChatWidget.h"
#include "cube/widgets/ControlsWidget.h"
#include "cube/widgets/FormulaDetailsWidget.h"
#include "cube/Sprite.h"
#include "cube/SpriteManager.h"
#include "cube/widgets/SpriteWidget.h"
#include "cube/Sprite.h"
#include "cube/SpriteManager.h"
#include "cube/widgets/HelpWidget.h"
#include "cube/widgets/AdaptionWidget.h"
#include "cube/Creature.h"
#include "cube/Database.h"
#include "cube/Fixed.h"
#include "cube/Noise.h"
#include "cube/widgets/OptionsWidget.h"
#include "cube/widgets/StartMenuWidget.h"
#include "cube/widgets/SystemWidget.h"
#include "cube/widgets/EnchantWidget.h"
#include "cube/widgets/InventoryWidget.h"
#include "cube/widgets/MapOverlayWidget.h"
#include "cube/Missions.h"
#include "cube/widgets/MultiplayerWidget.h"
#include "cube/widgets/PreviewWidget.h"
#include "cube/widgets/QuestionWidget.h"
#include "cube/Realm.h"
#include "cube/Serializer.h"
#include "cube/widgets/SpeechWidget.h"
#include "cube/widgets/VoxelWidget.h"
#include "cube/World.h"
#include "cube/WorldMap.h"
#include "cube/WorldInfo.h"
#include "gfx/D3D11Renderer.h"
#include "gfx/IndexBuffer.h"
#include "gfx/VertexBuffer.h"
#include "plasma/D3D11Engine.h"
#include "plasma/Display.h"
#include "plasma/Matrix.h"
#include "plasma/Node.h"
#include "plasma/TextShape.h"
#include "plasma/ScalableFont.h"
#include "cube/CreatureIds.h"
#include "cube/BlockIds.h"
#include "cube/InventoryWidgetIds.h"
#include "cube/ItemIds.h"
#include "cube/MissionIds.h"
#include "cube/ServerUpdateIds.h"
#include "cube/WorldIds.h"
#include "cube/ZoneIds.h"
#include "cube/SpriteIds.h"

#include "cube/InterfaceIds.h"
#include "mod/ClassData.h"
#include "mod/Hooks.h"
#include "mod/AlphaHud.h"
#include "mod/SkillMenu.h"
#include "plasma/EditFilter.h"

namespace cube {

// Simulation steps are at most 16 ms; Game::update compares the remaining time in double precision and advances it in
// float.
const double MaxWorldStepMs = 16.0;
const float MaxWorldStepMsFloat = 16.0f;

static const int CubeChangeRemeshMargin = 13;
static const float DodgeRollDurationMs = 600.0f;
static const float StaminaMoveCost = 0.25f;
static const float LeapSpeed = 50.0f;
static const float LeapSpeedSquared = 2500.0f;
static const float NearbyCreatureRangeSquared = 25600.0f;
static const float NearbyCreatureRange = 160.0f;
static const int MaximumMapZoom = 6;

static float s_cameraFieldOfView = 0.87266469f;

// The squared distance in blocks of the block (x, y) to the 64-block square of the zone (zoneX, zoneY); 0 inside it.
static inline int squaredDistanceToZone(int x, int y, int zoneX, int zoneY)
{
    int left = zoneX * BlocksPerZone;
    int right = zoneX * BlocksPerZone + BlocksPerZone;
    int top = zoneY * BlocksPerZone;
    int bottom = zoneY * BlocksPerZone + BlocksPerZone;
    if (x >= left && x <= right) {
        if (y >= top && y <= bottom) {
            return 0;
        }
        if (y >= top) {
            return (y - bottom) * (y - bottom);
        }
        return (top - y) * (top - y);
    }
    if (y >= top && y <= bottom) {
        if (x >= left) {
            return (x - right) * (x - right);
        }
        return (left - x) * (left - x);
    }
    if (x <= left && y <= top) {
        return (x - left) * (x - left) + (y - top) * (y - top);
    }
    if (x <= left && y >= bottom) {
        return (x - left) * (x - left) + (y - bottom) * (y - bottom);
    }
    if (x >= right && y <= top) {
        return (x - right) * (x - right) + (y - top) * (y - top);
    }
    return (x - right) * (x - right) + (y - bottom) * (y - bottom);
}

struct NearbyZone {
    Zone *zone;
    int squaredDistance;             // in zones, from the player's zone
    plasma::Vector<2, int> position; // the zone's coordinates
};

// A draw flag (RenderFlagDebugCreatureBlocks) nothing in the examined code sets; its partner s_debugDrawOnlyPlayer is
// with the debug switches before handleButtonDown.
static bool s_debugDrawCreatureBlocks;

// Set while the threads should idle (the map, chunk and generation threads sleep 100 ms per round while it is set). No
// writer was found.
static bool s_pauseThreads;

// Builds the game around WinMain's world, map, renderer, engine and audio: loads the bindings, the sounds, the saved
// characters and the interface, creates the local player ("Wollay", a level 1 warrior, replaced by the character of
// the saved slot when there is one) and starts the chunk, generation, host, map, AI and music threads.
Game::Game(World *world, WorldMap *worldMap, gfx::D3D11Renderer *renderer, plasma::D3D11Engine *engine, Audio *audio,
           const Options *options, const std::vector<ResolutionInfo> *displayModes)
    : m_world(world),
      m_worldMap(worldMap),
      m_renderer(renderer),
      m_engine(engine),
      m_audio(audio),
      m_interface(this),
      m_host(m_world->m_spriteManager),
      m_client(this),
      m_currentWorld(0),
      m_gamepadActive(false),
      m_options(*options),
      m_shutdown(false),
      m_gamepadWalkVector(0, 0),
      m_gamepadTurnVector(0, 0),
      m_cameraAngle(0, 0, 0),
      m_targetCameraAngle(0, 0, 0),
      m_pointerMoved(false),
      m_cameraDistance(8.0f),
      m_targetCameraDistance(8.0f),
      m_cameraPosition(0, 0, 0),
      m_cameraFocusPosition(0, 0, 0),
      m_drawOriginX(0),
      m_drawOriginY(0),
      m_screenShake(0),
      m_mapZoom(3),
      m_targetMapPosition(0, 0, 0),
      m_mapPosition(0, 0, 0),
      m_mapZoomChanged(false),
      m_unused1e40(0),
      m_mapOverlayPosition(0.8f, 0.8f),
      m_mapOverlayScale(1.0f),
      m_mapRightClickToggle(false),
      m_crosshairCreatureId(0),
      m_highlightedCreatureId(0),
      m_promptCreatureId(0),
      m_crosshairItem(0x7fffffff, 0x7fffffff, 0),
      m_crosshairObject(0x7fffffff, 0x7fffffff, 0),
      m_ringRotation(0),
      m_ringRadius(0),
      m_vendorId(0),
      m_vendorClass(0),
      m_musicVolume(1.0f),
      m_musicDuckTime(0),
      m_successMusicTime(0),
      m_running(true),
      m_seed(123),
      m_worldName("world"),
      m_ringSelection(0),
      m_unused20c4(0),
      m_markedCreatureIndex(0),
      m_craftingProgress(-1.0f),
      m_mapItemStack(nullptr),
      m_connected(false),
      m_server(nullptr),
      m_socket(0),
      m_resetOnDisconnect(0)
{
    InitializeCriticalSectionAndSpinCount(&m_unusedLock1ce8, 4000);
    InitializeCriticalSectionAndSpinCount(&m_unusedLock1cc0, 16);
    InitializeCriticalSectionAndSpinCount(&m_musicCriticalSection, 16);
    InitializeCriticalSectionAndSpinCount(&m_unusedLock1d10, 16);
    InitializeCriticalSectionAndSpinCount(&m_characterCriticalSection, 16);
    m_controls.load();
    m_world->init(m_seed, m_worldName);
    std::cout << "preload sounds" << std::endl;
    for (int i = 0; i < 125; i++) {
        soundId(i);
    }
    memset(m_keyStates, 0, sizeof(m_keyStates));
    std::cout << "load characters" << std::endl;
    m_charactersDatabase.open("Save/characters.db");
    BytesIO data;
    if (m_charactersDatabase.read("num", data)) {
        int count = 0;
        data.read(count);
        for (int i = 0; i < count; i++) {
            Creature *creature = new Creature(0);
            loadCharacter(i, creature);
            m_savedCharacters.push_back(creature);
        }
    }
    std::cout << "create iface" << std::endl;
    m_interface.load(displayModes);
    m_interface.m_scale = m_options.guiScale;
    applyControls(m_controls);

    Creature *player = new Creature(1);
    strcpy_s(player->m_entityData.name, 16, "Wollay");
    player->m_entityData.position = plasma::Vector<3, Fixed>(0, 0, 0);
    player->m_drawnPosition = player->m_entityData.position;
    player->m_entityData.origin = m_world->startPosition();
    player->m_entityData.rotation = plasma::Vector<3, float>(0, 0, 0);
    player->m_entityData.velocity = plasma::Vector<3, float>(0, 0, 0);
    player->m_entityData.flags = CreatureFlagNone;
    player->m_entityData.level = 1;
    player->m_entityData.hp = player->maxHP();
    player->m_entityData.equipment.weaponRight.category = ItemCategoryWeapon;
    player->m_entityData.equipment.weaponRight.id = WeaponSword;
    player->m_entityData.equipment.weaponRight.material = ItemMaterialIron;
    player->m_entityData.equipment.weaponLeft.category = ItemCategoryWeapon;
    player->m_entityData.equipment.weaponLeft.id = WeaponSword;
    player->m_entityData.equipment.weaponLeft.material = ItemMaterialIron;
    player->m_entityData.equipment.chest.material = ItemMaterialIron;
    player->m_entityData.hostilityType = HostilityPlayer;
    player->m_entityData.classType = ClassWarrior;
    player->m_entityData.race = RaceCreatorMale;
    player->m_playerData = new PlayerData;
    fillAppearance(player->m_entityData.race, player->m_entityData.appearance, nullptr, false);
    m_world->addCreature(player);
    m_world->m_localCreature = player;
    m_world->spawnLocalCreature();
    {
        BytesIO slot;
        if (m_charactersDatabase.read("slot", slot)) {
            slot.read(m_currentCharacterSlot);
            loadCharacter(m_currentCharacterSlot, m_world->getLocalCreature());
            m_world->spawnLocalCreature();
            learnFormulas(0);
            m_interface.updateStyle();
            m_interface.refresh();
            updateFormulas();
        }
    }
    m_targetCameraAngle = plasma::Vector<3, double>(90.0, 0.0, 180.0);
    m_cameraAngle = m_targetCameraAngle;

    // The chunk thread: unloads far zones and rebuilds the meshes of the loaded zones around the player, nearest
    // first.
    m_chunkThread = std::thread([this] {
        int performance = 1;
        while (m_running) {
            if (s_pauseThreads) {
                Sleep(100);
                continue;
            }
            m_world->enterZoneMeshes();
            Creature *player = m_world->m_localCreature;
            int zoneY = floorDivide(player->m_entityData.position[1].toBlock(), BlocksPerZone);
            int zoneX = floorDivide(player->m_entityData.position[0].toBlock(), BlocksPerZone);
            if (player->m_entityData.currentAbility == AbilityEagleFlight) {
                plasma::Vector<2, int> zone(floorDivide(player->m_entityData.destination[0], BlocksPerZone),
                                            floorDivide(player->m_entityData.destination[1], BlocksPerZone));
                zoneY = zone[1];
                zoneX = zone[0];
            }
            if (performance != m_options.cpuPerformance) {
                performance = m_options.cpuPerformance;
                int priority = performance != 0 ? THREAD_PRIORITY_NORMAL : THREAD_PRIORITY_BELOW_NORMAL;
                SetThreadPriority(GetCurrentThread(), priority);
                Concurrency::SchedulerPolicy policy;
                policy.SetConcurrencyLimits(1, performance != 0 ? 8 : 4);
                policy.SetPolicyValue(Concurrency::ContextPriority, priority);
                Concurrency::CurrentScheduler::Create(policy);
            }
            m_world->leaveZoneMeshes();
            int radius = (std::max)(3, 30 * m_options.renderDistance / 100);
            for (int i = 0; i < 8; i++) {
                m_world->removeFarZone(radius + 1);
            }
            // The loaded zones within the radius, with their squared distance in zones.
            std::vector<NearbyZone> entries;
            for (int x = zoneX - radius; x <= zoneX + radius; x++) {
                for (int y = zoneY - radius; y <= zoneY + radius; y++) {
                    Sleep(0);
                    m_world->enterZones();
                    Zone *zone = m_world->zone(x, y);
                    m_world->leaveZones();
                    if (zone) {
                        NearbyZone entry;
                        entry.zone = zone;
                        entry.squaredDistance = (y - zoneY) * (y - zoneY) + (x - zoneX) * (x - zoneX);
                        entry.position = plasma::Vector<2, int>(x, y);
                        entries.push_back(entry);
                    }
                }
            }
            std::sort(entries.begin(), entries.end(),
                      [](const NearbyZone &a, const NearbyZone &b) { return a.squaredDistance < b.squaredDistance; });
            // The zones to rebuild this round, nearest first: those without a mesh or with a changed one that the
            // renderer can mesh, at most eight. Once the meshes counted so far exceed the budget of triangles for the
            // radius, only zones that already have a mesh are still considered.
            long long triangles = 0;
            // The original kept std::max's reference to these temporaries and read it after the lock was taken; MSVC
            // left the value in place, GCC does not (the budget became 0 and no new zone was ever meshed). A copy.
            const int budgetRadius = (std::max)(3, 30 * m_options.renderDistance / 100);
            m_world->enterZoneMeshes();
            long long budget = (long long)((float)((long long)budgetRadius * budgetRadius * 98304) * 0.8f * 0.8f);
            m_world->leaveZoneMeshes();
            std::vector<plasma::Vector<2, int>> remeshed;
            for (std::vector<NearbyZone>::iterator it = entries.begin(); it != entries.end(); ++it) {
                if (triangles > budget && it->zone->m_chunk.triangleCount == 0) {
                    continue;
                }
                if ((it->zone->m_chunk.triangleCount == 0 || it->zone->m_chunk.needsRemesh) &&
                    m_renderer->canMesh(it->position[0], it->position[1]) && (int)remeshed.size() < 8) {
                    remeshed.push_back(it->position);
                }
                triangles += it->zone->m_chunk.triangleCount;
            }
            Concurrency::parallel_for_each(
                remeshed.begin(), remeshed.end(),
                [this](const plasma::Vector<2, int> &position) { m_renderer->updateChunk(position[0], position[1]); });
            // The new buffers are prepared outside the lock.
            for (std::vector<plasma::Vector<2, int>>::iterator it = remeshed.begin(); it != remeshed.end(); ++it) {
                Sleep(0);
                Zone *zone = m_world->zone((*it)[0], (*it)[1]);
                if (zone && zone->m_chunk.newChunkBuffers.size() != 0) {
                    for (gfx::ChunkBuffer *buffer : zone->m_chunk.newChunkBuffers) {
                        if (buffer->indexBuffers[gfx::ChunkPassSolid]) {
                            buffer->indexBuffers[gfx::ChunkPassSolid]->prepare();
                        }
                        if (buffer->vertexBuffer) {
                            buffer->vertexBuffer->prepare();
                        }
                        if (buffer->indexBuffers[gfx::ChunkPassWater]) {
                            buffer->indexBuffers[gfx::ChunkPassWater]->prepare();
                        }
                        if (buffer->indexBuffers[gfx::ChunkPassLava]) {
                            buffer->indexBuffers[gfx::ChunkPassLava]->prepare();
                        }
                    }
                }
            }
            // The new buffers replace the drawn ones while the zone meshes are held; the old ones are deleted
            // afterwards.
            for (std::vector<plasma::Vector<2, int>>::iterator it = remeshed.begin(); it != remeshed.end(); ++it) {
                Zone *zone = m_world->zone((*it)[0], (*it)[1]);
                if (zone && zone->m_chunk.newChunkBuffers.size() != 0) {
                    std::list<gfx::ChunkBuffer *> oldBuffers(zone->m_chunk.chunkBuffers);
                    m_world->enterZoneMeshes();
                    zone->m_chunk.chunkBuffers = zone->m_chunk.newChunkBuffers;
                    m_world->leaveZoneMeshes();
                    Sleep(0);
                    for (gfx::ChunkBuffer *buffer : oldBuffers) {
                        delete buffer;
                    }
                    zone->m_chunk.newChunkBuffers.clear();
                }
            }
        }
        m_world->removeZones();
    });
    // The generation thread: generates the missing zones around the player, nearest first and at most four at a time,
    // and saves the character once a minute and at the end.
    m_generationThread = std::thread([this] {
        unsigned int lastSecond = timeGetTime();
        unsigned int lastSave = timeGetTime();
        timeGetTime();
        SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);
        Concurrency::SchedulerPolicy policy;
        policy.SetConcurrencyLimits(1, 4);
        policy.SetPolicyValue(Concurrency::ContextPriority, THREAD_PRIORITY_BELOW_NORMAL);
        Concurrency::CurrentScheduler::Create(policy);
        int performance = 1;
        while (m_running) {
            if (s_pauseThreads) {
                Sleep(100);
                continue;
            }
            m_world->enterZoneMeshes();
            if (performance != m_options.cpuPerformance) {
                performance = m_options.cpuPerformance;
                int priority = performance != 0 ? THREAD_PRIORITY_NORMAL : THREAD_PRIORITY_BELOW_NORMAL;
                SetThreadPriority(GetCurrentThread(), priority);
                Concurrency::SchedulerPolicy newPolicy;
                newPolicy.SetConcurrencyLimits(1, performance != 0 ? 8 : 4);
                newPolicy.SetPolicyValue(Concurrency::ContextPriority, priority);
                Concurrency::CurrentScheduler::Create(newPolicy);
            }
            Creature *player = m_world->m_localCreature;
            plasma::Vector<2, int> block((int)player->m_entityData.position[0], (int)player->m_entityData.position[1]);
            if (player->m_entityData.currentAbility == AbilityEagleFlight) {
                block = player->m_entityData.destination;
            }
            int x = block[0];
            int y = block[1];
            int radius = (std::max)(BlocksPerZone, 1920 * m_options.renderDistance / 100);
            m_world->leaveZoneMeshes();
            unsigned int now = timeGetTime();
            if (now - lastSecond > 1000) {
                lastSecond = now;
            }
            struct SortedZone {
                std::array<int, 2> zone;
                int distance;
            };
            std::vector<SortedZone> missing;
            int minX = floorDivide(x - radius, BlocksPerZone);
            int minY = floorDivide(y - radius, BlocksPerZone);
            int maxX = floorDivide(x + radius, BlocksPerZone);
            int maxY = floorDivide(y + radius, BlocksPerZone);
            int centerX = floorDivide(x, BlocksPerZone);
            int centerY = floorDivide(y, BlocksPerZone);
            int zoneRadius = radius / BlocksPerZone + 1;
            for (int zoneX = minX; zoneX <= maxX; zoneX++) {
                for (int zoneY = minY; zoneY <= maxY; zoneY++) {
                    if (m_world->zone(zoneX, zoneY)) {
                        continue;
                    }
                    double offsetX = zoneX - centerX;
                    double offsetY = zoneY - centerY;
                    if (offsetX * offsetX + offsetY * offsetY <= zoneRadius * zoneRadius) {
                        int distance = squaredDistanceToZone(x, y, zoneX, zoneY);
                        SortedZone sorted = {{{zoneX, zoneY}}, distance};
                        missing.push_back(sorted);
                    }
                }
            }
            if (missing.empty()) {
                Sleep(5);
            } else {
                std::sort(missing.begin(), missing.end(),
                          [](const SortedZone &a, const SortedZone &b) { return a.distance < b.distance; });
                if (missing.size() > 4) {
                    missing.resize(4);
                }
                Concurrency::parallel_for_each(missing.begin(), missing.end(), [this](const SortedZone &sorted) {
                    m_world->generateZone(sorted.zone[0], sorted.zone[1]);
                });
            }
            Sleep(0);
            if (now - lastSave > 60000 && m_world->m_worldName.size() != 0 &&
                !m_interface.m_characterSelection.m_node->isVisible() && !m_interface.m_startMenuNode->isVisible() &&
                !m_interface.m_characterCreationNode->isVisible() && m_running) {
                saveCharacter();
                lastSave = now;
            }
        }
        if (!m_interface.m_characterSelection.m_node->isVisible() && !m_interface.m_startMenuNode->isVisible() &&
            !m_interface.m_characterCreationNode->isVisible()) {
            saveCharacter();
        }
    });
    m_client.m_host = &m_host;
    m_host.setLocalClientConnection(&m_client, &Client::addPacket);
    // The host thread: one host tick per 32 ms while this game is not connected to another host.
    m_hostThread = std::thread([this] {
        unsigned int last = timeGetTime();
        while (m_running) {
            if (m_client.isConnected()) {
                Sleep(33);
            } else {
                unsigned int now = timeGetTime();
                m_host.update(now - last);
                unsigned int after = timeGetTime();
                if (after - now < 32) {
                    Sleep(now - after + 32);
                }
                last = now;
            }
        }
        m_host.stop();
    });
    // The map thread: updates the map around the map position at the map zoom every 10 ms and cleans it up every five
    // seconds.
    m_mapThread = std::thread([this] {
        unsigned int last = timeGetTime();
        while (m_running) {
            if (s_pauseThreads) {
                Sleep(100);
            } else {
                m_world->enterZoneMeshes();
                plasma::Vector<3, Fixed> position = m_mapPosition;
                int zoom = m_mapZoom;
                m_world->leaveZoneMeshes();
                unsigned int now = timeGetTime();
                if (now - last > 5000) {
                    m_worldMap->cleanUp();
                    last = now;
                }
                m_worldMap->update(position, zoom);
                Sleep(10);
            }
        }
        m_worldMap->clear();
    });
    // The AI thread: one step of the host world's creatures per 40 ms.
    m_aiThread = std::thread([this] {
        unsigned int last = timeGetTime();
        while (m_running) {
            unsigned int now = timeGetTime();
            m_host.m_world.m_aiManager.update(now - last);
            last = now;
            unsigned int elapsed = timeGetTime() - now;
            Sleep(elapsed < 40 ? 40 - elapsed : 1);
        }
    });
    // The music thread: every 16 ms applies the music volume (or stops the music below 0.01), starts the current music
    // file when it changed and refills the streams.
    m_musicThread = std::thread([this] {
        timeGetTime();
        std::string playing;
        while (m_running) {
            Sleep(16);
            EnterCriticalSection(&m_musicCriticalSection);
            std::string file = m_currentMusicFile;
            float volume = m_musicVolume;
            LeaveCriticalSection(&m_musicCriticalSection);
            if (volume < 0.01f) {
                m_audio->stopMusic();
            } else {
                m_audio->setMusicVolume(volume, 0);
            }
            if (file != playing) {
                m_audio->playMusic(file, "", true);
                m_audio->setMusicVolume(volume, 0);
                playing = file;
            }
            m_audio->update();
        }
    });
}

Particle::Particle()
    : type(ParticleLitCube),
      age(0),
      rotation(0.0f, 0.0f, 0.0f),
      acceleration(0.0f, 0.0f, 0.0f),
      spin(0.0f, 0.0f, 0.0f),
      size(1.0f),
      collidesWithBlocks(false),
      motionDisabled(0),
      lifetime(4000)
{
    matrix.identity();
}

TextFX::TextFX()
    : rise(-100.0f),
      duration(1000),
      textFlags(0)
{}

// Leaves a joined game, stops the threads one by one (each "waiting for ... thread..." line precedes its join; the
// music is stopped once its thread is gone), closes the socket and deletes the saved characters and worlds. The
// receive and send threads are never started in this file.
Game::~Game()
{
    delete m_server;
    disconnect();
    m_running = false;
    std::cout << "waiting for music thread..." << std::endl;
    m_musicThread.join();
    m_audio->stopMusic();
    std::cout << "waiting for ai thread..." << std::endl;
    m_aiThread.join();
    std::cout << "waiting for generation thread..." << std::endl;
    m_generationThread.join();
    std::cout << "waiting for chunk thread..." << std::endl;
    m_chunkThread.join();
    std::cout << "waiting for host thread..." << std::endl;
    if (m_hostThread.joinable()) {
        m_hostThread.join();
    }
    std::cout << "waiting for map thread..." << std::endl;
    if (m_mapThread.joinable()) {
        m_mapThread.join();
    }
    std::cout << "waiting for receive thread..." << std::endl;
    if (m_receiveThread.joinable()) {
        m_receiveThread.join();
    }
    std::cout << "waiting for send thread..." << std::endl;
    if (m_sendThread.joinable()) {
        m_sendThread.join();
    }
    if (m_socket) {
        closesocket(m_socket);
    }
    DeleteCriticalSection(&m_characterCriticalSection);
    DeleteCriticalSection(&m_unusedLock1ce8);
    DeleteCriticalSection(&m_unusedLock1cc0);
    DeleteCriticalSection(&m_musicCriticalSection);
    DeleteCriticalSection(&m_unusedLock1d10);
    for (Creature *character : m_savedCharacters) {
        delete character;
    }
    for (WorldInfo *world : m_worlds) {
        delete world;
    }
}

std::vector<int> &operator<<(std::vector<int> &commands, const Item &item)
{
    commands << item.category << (int)item.id << (int)item.modifier << item.material << item.rarity;
    return commands;
}

std::vector<int> &operator<<(std::vector<int> &commands, int value)
{
    commands.push_back(value);
    return commands;
}

void Game::applyControls(const Controls &controls)
{
    m_controls = controls;
    m_controls.save();
}

void Game::applyOptions(const Options &options)
{
    m_world->enterZoneMeshes();
    m_options = options;
    m_options.save();
    EnterCriticalSection(&m_musicCriticalSection);
    m_musicVolume = (float)options.musicVolume * 0.01f;
    LeaveCriticalSection(&m_musicCriticalSection);
    m_world->leaveZoneMeshes();
}

void Game::ask(Creature *creature, const std::string &answer)
{
    m_interface.m_inGame.m_questionWidget->m_dialogs.clear();
    if (creature) {
        std::vector<std::string> script;
        if (answer == "change_specialization?") {
            m_world->m_localCreature->m_entityData.specialization =
                1 - m_world->m_localCreature->m_entityData.specialization;
            script << "define_specialization" << m_world->getLocalCreature()->m_entityData.classType
                   << m_world->getLocalCreature()->m_entityData.specialization << "your_specialization_is_now";
            m_speech.generateFromScript(script, (int)creature->m_id, m_interface.m_inGame.m_speechWidget->m_dialogs,
                                        L"");
        }
        if (answer == "rest_inn?") {
            if (m_world->getLocalCreature()->m_inventory.m_gold >= 10 &&
                (m_world->m_state.m_time >= 18 * MillisecondsPerHour ||
                 m_world->m_state.m_time <= 6 * MillisecondsPerHour)) {
                m_world->getLocalCreature()->m_inventory.m_gold -= 10;
                Interaction interaction;
                interaction.type = InteractionTalk;
                interaction.creatureId = creature->m_id;
                m_world->m_localCreature->m_interactions.push_back(interaction);
            }
        }
    }
}

void Game::assignControl(const Controls::Button &button)
{
    m_interface.m_controlsWidget->rebindSelected(button);
}

bool Inventory::canAdd(const Item &item, int count)
{
    int tab = itemTab(item);
    if (item.category == ItemCategoryNone)
        return false;
    if ((int)m_inventoryTabs.size() <= tab)
        return false;
    if (item.category == ItemCategoryCoin && item.material == ItemMaterialGold) {
        m_gold += count;
        return true;
    }
    if (item.category == ItemCategoryPlatinumCoin) {
        ++m_manaCubes;
        return true;
    }
    int i = 0;
    int emptySlot = -1;
    for (; i < (int)m_inventoryTabs[tab].size(); ++i) {
        if (m_inventoryTabs[tab][i].quantity == 0 && emptySlot < 0)
            emptySlot = i;
        if (itemIsStackable(item) && m_inventoryTabs[tab][i].item == item)
            break;
    }
    return true;
}

void Game::cancelBinding()
{
    m_interface.m_controlsWidget->resetSelectedIndex();
}

void Game::createParticles(Sprite *sprite, const plasma::Matrix<float> &matrix,
                           const plasma::Vector<3, float> &unusedVector, const plasma::Vector<3, float> &velocity,
                           float unusedFactor, float originX, float originY, const plasma::Vector<4, float> &color,
                           int particleType)
{
    if (!sprite)
        return;
    std::list<Particle> particles;
    plasma::Vector<3, float> center =
        matrix * plasma::Vector<3, float>((float)sprite->sizeX() * 0.5f, (float)sprite->sizeY() * 0.5f,
                                          (float)sprite->sizeZ() * 0.5f);
    for (int x = 0; x < sprite->sizeX(); ++x) {
        for (int y = 0; y < sprite->sizeY(); ++y) {
            for (int z = 0; z < sprite->sizeZ(); ++z) {
                plasma::Vector<3, unsigned char> &voxel = sprite->voxel(x, y, z);
                if (sprite->isEmpty(&voxel, false))
                    continue;
                if (!sprite->isEmpty(&sprite->voxel(x - 1, y, z), false) &&
                    !sprite->isEmpty(&sprite->voxel(x + 1, y, z), false) &&
                    !sprite->isEmpty(&sprite->voxel(x, y - 1, z), false) &&
                    !sprite->isEmpty(&sprite->voxel(x, y + 1, z), false) &&
                    !sprite->isEmpty(&sprite->voxel(x, y, z - 1), false) &&
                    !sprite->isEmpty(&sprite->voxel(x, y, z + 1), false))
                    continue;
                if (rand() % 2 != 0)
                    continue;
                Particle particle;
                particle.motionDisabled = false;
                particle.type = static_cast<ParticleType>(particleType);
                particle.age = -(rand() % 1000);
                plasma::Vector<3, float> position = matrix * plasma::Vector<3, float>((float)x, (float)y, (float)z);
                plasma::Vector<3, Fixed> dots(position);
                plasma::Vector<3, Fixed> centerDots(center);
                particle.acceleration = plasma::Vector<3, float>((float)((dots[0] - centerDots[0]) * Fixed(2.0f)),
                                                                 (float)((dots[1] - centerDots[1]) * Fixed(2.0f)),
                                                                 (float)((dots[2] - centerDots[2]) * Fixed(2.0f)));
                particle.rotation = plasma::Vector<3, float>(0, 0, 0);
                float spinZ = ((float)rand() / (float)RAND_MAX - 0.5f) * 100.0f;
                float spinY = ((float)rand() / (float)RAND_MAX - 0.5f) * 100.0f;
                float spinX = ((float)rand() / (float)RAND_MAX - 0.5f) * 100.0f;
                particle.spin = plasma::Vector<3, float>(spinX, spinY, spinZ);
                particle.color = plasma::Vector<4, float>((float)voxel[0] / 255.0f, (float)voxel[1] / 255.0f,
                                                          (float)voxel[2] / 255.0f, 1.0f) *
                                 color;
                particle.size = 1.0f;
                particle.matrix = matrix;
                particle.matrix[12] = 0;
                particle.matrix[13] = 0;
                particle.matrix[14] = 0;
                particle.collidesWithBlocks = true;
                particle.lifetime = 5000;
                plasma::Vector<3, Fixed> start(dots[0] - m_drawOriginX, dots[1] - m_drawOriginY, dots[2]);
                particle.position = start;
                particle.startPosition = start;
                if (rand() % 2)
                    particles.push_front(particle);
                else
                    particles.push_back(particle);
            }
        }
    }
    for (const Particle &particle : particles)
        m_particles.push_front(particle);
}

std::wstring Game::getFlatDistanceText(int metres)
{
    if (metres < 10)
        return std::to_wstring(metres) + L" m";
    if (metres < 500)
        return std::to_wstring(metres / 10 * 10) + L" m";
    if (metres < 1000)
        return std::to_wstring(metres / 100 * 100) + L" m";
    int kilometres = metres / 1000;
    if (metres < 10000) {
        int tenths = metres / 100 - kilometres * 10;
        if (tenths > 0)
            return std::to_wstring(kilometres) + L"." + std::to_wstring(tenths) + L" km";
    }
    return std::to_wstring(kilometres) + L" km";
}

std::wstring Game::getFoldedCountText(int count)
{
    std::wstringstream text;
    if (count >= 1000000)
        text << count / 1000000 << "." << (count % 1000000) / 100000 << "M";
    else if (count >= 10000)
        text << count / 1000 << "." << (count % 1000) / 100 << "K";
    else
        text << count;
    return text.str();
}

std::vector<Creature *> Game::getFollowingCreatures()
{
    std::vector<Creature *> creatures;
    for (std::pair<const long long, Creature *> &entry : m_worldMap->m_creatures) {
        creatures.push_back(entry.second);
    }
    return creatures;
}

std::vector<plasma::Vector<2, int>> Game::getFoodSlots()
{
    std::vector<plasma::Vector<2, int>> slots;
    for (int tab = 0; tab < (int)m_world->getLocalCreature()->m_inventory.m_inventoryTabs.size(); tab++) {
        for (int index = 0; index < (int)m_world->getLocalCreature()->m_inventory.m_inventoryTabs[tab].size();
             index++) {
            if (m_world->getLocalCreature()->m_inventory.m_inventoryTabs[tab][index].quantity != 0 &&
                m_world->getLocalCreature()->m_inventory.m_inventoryTabs[tab][index].item.category ==
                    ItemCategoryConsumable) {
                slots.push_back(plasma::Vector<2, int>(tab, index));
            }
        }
    }
    return slots;
}

Item *Game::getHoveredEnchantItem()
{
    if (m_interface.m_inGame.m_enchantWidget->getNode()->isVisible() &&
        m_interface.m_inGame.m_enchantWidget->m_inventoryTabIndex >= 0 &&
        m_interface.m_inGame.m_enchantWidget->m_inventoryTabIndex <
            (int)m_world->getLocalCreature()->m_inventory.m_inventoryTabs.size() &&
        m_interface.m_inGame.m_enchantWidget->m_inventoryItemIndex >= 0 &&
        m_interface.m_inGame.m_enchantWidget->m_inventoryItemIndex <
            (int)m_world->getLocalCreature()
                ->m_inventory.m_inventoryTabs[m_interface.m_inGame.m_enchantWidget->m_inventoryTabIndex]
                .size() &&
        m_world->getLocalCreature()
                ->m_inventory
                .m_inventoryTabs[m_interface.m_inGame.m_enchantWidget->m_inventoryTabIndex]
                                [m_interface.m_inGame.m_enchantWidget->m_inventoryItemIndex]
                .quantity != 0) {
        return nullptr;
    }
    return nullptr;
}
Item *Game::getHoveredEquipment()
{
    if (m_interface.m_inGame.m_equipmentInventoryWidget->getNode()->isVisible()) {
        if (m_interface.m_inGame.m_equipmentSlots[EquipmentSlotWeaponLeft]->getNode()->isUnderPointer()) {
            return &m_world->getLocalCreature()->m_entityData.equipment.weaponLeft;
        }
        if (m_interface.m_inGame.m_equipmentSlots[EquipmentSlotWeaponRight]->getNode()->isUnderPointer()) {
            return &m_world->getLocalCreature()->m_entityData.equipment.weaponRight;
        }
        if (m_interface.m_inGame.m_equipmentSlots[EquipmentSlotRingLeft]->getNode()->isUnderPointer()) {
            return &m_world->getLocalCreature()->m_entityData.equipment.ringLeft;
        }
        if (m_interface.m_inGame.m_equipmentSlots[EquipmentSlotRingRight]->getNode()->isUnderPointer()) {
            return &m_world->getLocalCreature()->m_entityData.equipment.ringRight;
        }
        if (m_interface.m_inGame.m_equipmentSlots[EquipmentSlotNeck]->getNode()->isUnderPointer()) {
            return &m_world->getLocalCreature()->m_entityData.equipment.neck;
        }
        if (m_interface.m_inGame.m_equipmentSlots[EquipmentSlotShoulder]->getNode()->isUnderPointer()) {
            return &m_world->getLocalCreature()->m_entityData.equipment.shoulder;
        }
        if (m_interface.m_inGame.m_equipmentSlots[EquipmentSlotChest]->getNode()->isUnderPointer()) {
            return &m_world->getLocalCreature()->m_entityData.equipment.chest;
        }
        if (m_interface.m_inGame.m_equipmentSlots[EquipmentSlotHands]->getNode()->isUnderPointer()) {
            return &m_world->getLocalCreature()->m_entityData.equipment.hands;
        }
        if (m_interface.m_inGame.m_equipmentSlots[EquipmentSlotFeet]->getNode()->isUnderPointer()) {
            return &m_world->getLocalCreature()->m_entityData.equipment.feet;
        }
        if (m_interface.m_inGame.m_equipmentSlots[EquipmentSlotPet]->getNode()->isUnderPointer()) {
            return &m_world->getLocalCreature()->m_entityData.equipment.pet;
        }
    }
    return nullptr;
}

Item *Game::getHoveredFormula()
{
    if (!m_interface.m_inGame.m_craftingInventoryWidget->getNode()->isVisible()) {
        return nullptr;
    }
    if (!m_interface.m_inGame.m_craftingInventoryWidget->isPointerOver()) {
        return nullptr;
    }
    int tab = m_interface.m_inGame.m_craftingInventoryWidget->m_hoveredCell[0];
    int index = m_interface.m_inGame.m_craftingInventoryWidget->m_hoveredCell[1];
    if (tab >= 0 && tab < (int)m_craftingInventory.m_inventoryTabs.size() && index >= 0 &&
        index < (int)m_craftingInventory.m_inventoryTabs[tab].size() &&
        m_craftingInventory.m_inventoryTabs[tab][index].quantity >= 0 &&
        m_craftingInventory.m_inventoryTabs[tab][index].item.category != ItemCategoryNone) {
        return &m_craftingInventory.m_inventoryTabs[tab][index].item;
    }
    return nullptr;
}

Item *Game::getHoveredIngredient()
{
    if (m_interface.m_inGame.m_formulaDetailsWidget->getNode()->isVisible()) {
        Item *item = &m_interface.m_inGame.m_formulaDetailsWidget->m_hoveredItem;
        if (item->category == ItemCategoryNone) {
            return nullptr;
        }
        return item;
    }
    if (m_interface.m_inGame.m_voxelWidget->getNode()->isVisible()) {
        return m_interface.m_inGame.m_voxelWidget->getHoveredUpgradeItem();
    }
    return nullptr;
}

plasma::Vector<2, int> Game::getHoveredSlot()
{
    if (m_interface.m_inGame.m_equipmentInventoryWidget->getNode()->isVisible() &&
        m_interface.m_inGame.m_equipmentInventoryWidget->isPointerOver() &&
        m_interface.m_inGame.m_equipmentInventoryWidget->getHoveredCell()[0] >= 0 &&
        m_interface.m_inGame.m_equipmentInventoryWidget->getHoveredCell()[0] <
            (int)m_world->getLocalCreature()->m_inventory.m_inventoryTabs.size() &&
        m_interface.m_inGame.m_equipmentInventoryWidget->getHoveredCell()[1] >= 0 &&
        m_interface.m_inGame.m_equipmentInventoryWidget->getHoveredCell()[1] <
            (int)m_world->getLocalCreature()
                ->m_inventory.m_inventoryTabs[m_interface.m_inGame.m_equipmentInventoryWidget->getHoveredCell()[0]]
                .size() &&
        m_world->getLocalCreature()
                ->m_inventory
                .m_inventoryTabs[m_interface.m_inGame.m_equipmentInventoryWidget->getHoveredCell()[0]]
                                [m_interface.m_inGame.m_equipmentInventoryWidget->getHoveredCell()[1]]
                .quantity != 0) {
        return m_interface.m_inGame.m_equipmentInventoryWidget->getHoveredCell();
    }
    return plasma::Vector<2, int>(-1, -1);
}

Item *Game::getHoveredVendorItem()
{
    if (!m_interface.m_inGame.m_vendorInventoryWidget->getNode()->isVisible() ||
        !m_interface.m_inGame.m_vendorInventoryWidget->isPointerOver()) {
        return nullptr;
    }
    int tab = m_interface.m_inGame.m_vendorInventoryWidget->m_hoveredCell[0];
    int index = m_interface.m_inGame.m_vendorInventoryWidget->m_hoveredCell[1];
    if (tab < 0 || tab >= (int)m_vendorInventory.m_inventoryTabs.size() || index < 0 ||
        index >= (int)m_vendorInventory.m_inventoryTabs[tab].size()) {
        return nullptr;
    }
    return &m_vendorInventory.m_inventoryTabs[tab][index].item;
}

// When the last button press was (timeGetTime), for the double click on the map.
static unsigned int s_lastButtonDownTime;
// Two debug switches nothing in the examined code sets: update carries the character 200 blocks above the land, and
// starts a flight to a random start cell.
static bool s_debugCruising;
static bool s_debugFlying;
// The other draw flag (RenderFlagOnlyPlayer; RenderFlagDebugCreatureBlocks is s_debugDrawCreatureBlocks), which
// nothing in the examined code sets.
static bool s_debugDrawOnlyPlayer;
// Whether the enchant widget was shown when the mouse button went down (set by HandleMouseDown; HandleMouseUp acts on
// the widget only then).
static bool s_enchantWidgetPressed;
// The music: when the next change is due, and whether the music is pausing (initialised data; how long the current
// file has played is update's).
static int s_nextMusicTime = 300000;
static bool s_musicPaused = true;

// A mouse button went down (0 left, 1 right, 2 middle). On the map: the middle button toggles a marker at the cursor,
// the left button flies to the hovered flight point (free when it is known, else for its price), a double click moves
// the map there. In the inventory: the left and right buttons equip, use, sell or select the hovered stack, take off
// the hovered equipment, or buy the hovered offer; the middle button drops the hovered stack. The left button also
// selects the crafting formula and the spirit cubes of the voxel widget.
void Game::handleButtonDown(int button)
{
    if (m_interface.m_alphaHud && m_interface.m_alphaHud->skillMenu() &&
        m_interface.m_alphaHud->skillMenu()->handleButtonDown(button)) {
        return;
    }
    m_interface.handleMouseDown(button);
    s_enchantWidgetPressed = m_interface.m_inGame.m_enchantWidget->getNode()->isVisible();
    if (m_interface.m_mapOpen) {
        if (m_interface.m_inGame.m_leftLandButton->isUnderPointer()) {
            return;
        }
        if (m_interface.m_inGame.m_rightLandButton->isUnderPointer()) {
            return;
        }
    }
    if (button == MouseButton_Middle) {
        if (m_interface.m_mapOpen) {
            if (m_worldMap->m_cursorPosition[2] >= Fixed(0)) {
                MapPlace place;
                place.type = PlotNone;
                float height =
                    m_world->height((int)m_worldMap->m_cursorPosition[0], (int)m_worldMap->m_cursorPosition[1], false);
                plasma::Vector<2, int> block((int)m_worldMap->m_cursorPosition[0],
                                             (int)m_worldMap->m_cursorPosition[1]);
                place.position = plasma::Vector<3, int>(block[0], block[1], (int)(std::max)(0.0f, height));
                std::array<int, 2> cell = {{block[0] / BlocksPerRegion, block[1] / BlocksPerRegion}};
                std::list<MapPlace> &places = m_world->m_localCreature->m_playerData->treasures[cell];
                std::list<MapPlace>::iterator it = places.begin();
                for (; it != places.end(); ++it) {
                    if (plasma::Vector<2, int>(it->position[0], it->position[1]) == block) {
                        break;
                    }
                }
                if (it == places.end()) {
                    places.push_back(place);
                } else {
                    places.erase(it);
                }
            }
            if (m_interface.m_mapOpen) {
                m_mapRightClickToggle = !m_mapRightClickToggle;
            }
        }
    } else if (button == MouseButton_Left && !m_pointerMoved && m_interface.m_mapOpen) {
        if (m_worldMap->m_selectedFlightPoint[0] != 0x7fffffff) {
            if (m_world->m_localCreature->knowsFlightPoint(m_worldMap->m_selectedFlightPoint) ||
                m_worldMap->m_selectedFlightTargetIsCreature) {
                m_world->m_localCreature->m_entityData.destination = m_worldMap->m_selectedFlightPoint;
                m_world->m_localCreature->m_entityData.origin = m_world->m_localCreature->m_entityData.position;
                m_world->m_localCreature->m_entityData.currentAbility = AbilityEagleFlight;
                m_world->m_localCreature->m_entityData.timeSinceAbility = 0;
                m_interface.m_mapOpen = false;
                m_interface.m_inGame.m_menuOpen = false;
            } else {
                int price = m_world->flightPrice(m_worldMap->m_selectedFlightPoint);
                if (m_world->m_localCreature->m_inventory.m_gold < price) {
                    m_interface.m_inGame.m_chatWidget->printMessage(L"Not enough money.\n",
                                                                    plasma::Vector<4, float>(1.0f, 0.2f, 0.2f, 1.0f));
                    playSound(SoundHumanVoice01, m_cameraPosition, 1.0f, 1.0f, true);
                } else {
                    m_world->m_localCreature->m_inventory.m_gold -= price;
                    std::array<int, 2> flightPoint = {
                        {m_worldMap->m_selectedFlightPoint[0], m_worldMap->m_selectedFlightPoint[1]}};
                    m_world->m_localCreature->m_playerData->knownFlightPoints.insert(flightPoint);
                    playSound(SoundDropCoin, m_cameraPosition, 1.0f, 1.0f, true);
                }
            }
        }
    }
    unsigned int now = timeGetTime();
    unsigned int elapsed = now - s_lastButtonDownTime;
    s_lastButtonDownTime = now;
    if (elapsed < 200 && m_interface.m_mapOpen) {
        if (m_worldMap->m_cursorPosition[2] > Fixed(0)) {
            m_targetMapPosition = m_worldMap->m_cursorPosition;
            m_targetMapPosition[2] =
                Fixed(m_world->height((int)m_targetMapPosition[0], (int)m_targetMapPosition[1], false));
            m_mapZoom--;
            if (m_mapZoom < 0) {
                m_mapZoom = 0;
            }
            m_mapZoomChanged = true;
        }
    }
    m_pointerMoved = false;
    float maxHP = m_world->m_localCreature->maxHP();
    if (button == MouseButton_Middle) {
        // Drop the hovered stack's item (not quest items, categories 22 to 24, nor the pet food being fed).
        plasma::Vector<2, int> slot = getHoveredSlot();
        if (slot[0] >= 0 && slot[1] >= 0) {
            Creature *player = m_world->m_localCreature;
            ItemStack &stack = player->m_inventory.m_inventoryTabs[slot[0]][slot[1]];
            if (stack.quantity != 0 &&
                !(stack.item.category >= ItemCategoryQuest && stack.item.category <= ItemCategorySpecial) &&
                !(player->m_entityData.currentAbility == AbilityOfferPetFood &&
                  player->m_entityData.actionItem == stack.item)) {
                Interaction interaction;
                interaction.type = InteractionDrop;
                interaction.item = stack.item;
                m_world->m_localCreature->m_interactions.push_back(interaction);
                stack.quantity--;
                if (stack.quantity <= 0) {
                    stack.quantity = 0;
                    stack.item.category = ItemCategoryNone;
                    stack.item.id = NoneCleared;
                }
            }
        }
    } else {
        if (button <= MouseButton_Right) {
            plasma::Vector<2, int> slot = getHoveredSlot();
            if (slot[0] >= 0 && slot[1] >= 0) {
                if (m_interface.m_inGame.m_enchantWidget->getNode()->isVisible()) {
                    m_interface.m_inGame.m_enchantWidget->m_inventoryTabIndex = slot[0];
                    m_interface.m_inGame.m_enchantWidget->m_inventoryItemIndex = slot[1];
                } else if (m_interface.m_inGame.m_vendorInventoryWidget->getNode()->isVisible()) {
                    if (slot[0] != InventoryTabSpecial && slot[0] != InventoryTabArtifacts) {
                        Item item = m_world->m_localCreature->m_inventory.m_inventoryTabs[slot[0]][slot[1]].item;
                        ItemStack &stack = m_world->m_localCreature->m_inventory.m_inventoryTabs[slot[0]][slot[1]];
                        stack.quantity--;
                        if (stack.quantity <= 0) {
                            stack.quantity = 0;
                            stack.item.category = ItemCategoryNone;
                            stack.item.id = NoneCleared;
                        }
                        m_world->m_localCreature->m_inventory.m_gold += item.sellPrice();
                        playSound(SoundDropCoin, m_world->m_localCreature->m_entityData.position, 0.75f, 1.0f, true);
                        ItemStack sold;
                        sold.quantity = 1;
                        sold.item = item;
                        m_soldItems.push_back(sold);
                        if (m_soldItems.size() > 10) {
                            m_soldItems.pop_front();
                        }
                        updateVendor();
                        updateFormulas();
                    } else {
                        playSound(SoundHumanVoice01, m_cameraPosition, 1.0f, 1.0f, true);
                    }
                } else if (m_interface.m_inGame.m_adaptionWidget->getNode()->isVisible()) {
                    Item &item = m_world->m_localCreature->m_inventory.m_inventoryTabs[slot[0]][slot[1]].item;
                    if ((item.category >= ItemCategoryWeapon && item.category <= ItemCategoryRing) ||
                        item.category == ItemCategoryEyePatch) {
                        m_interface.m_inGame.m_adaptionWidget->m_item =
                            &m_world->m_localCreature->m_inventory.m_inventoryTabs[slot[0]][slot[1]].item;
                    } else {
                        playSound(SoundHumanVoice01, m_cameraPosition, 1.0f, 1.0f, true);
                    }
                } else {
                    ItemStack &stack = m_world->m_localCreature->m_inventory.m_inventoryTabs[slot[0]][slot[1]];
                    if (!m_world->m_localCreature->canEquipItem(stack.item)) {
                        playSound(SoundHumanVoice02, m_cameraPosition, 1.0f, 1.0f, true);
                    } else if (stack.quantity != 0) {
                        if (stack.item.category == ItemCategoryWeapon) {
                            Item item = stack.item;
                            stack.quantity--;
                            if (stack.quantity <= 0) {
                                stack.quantity = 0;
                                stack.item.category = ItemCategoryNone;
                                stack.item.id = NoneCleared;
                            }
                            // Ids 13 and 14 go to the left hand (shields), 6 and 7 to the hand of the button with the
                            // other hand emptied, two-handed weapons to the right hand with the left emptied; other
                            // weapons to the right hand on a plain left click, to the left hand on a right click or
                            // with shift.
                            if (item.id == WeaponShield || item.id == WeaponArrows) {
                                if (m_world->m_localCreature->m_entityData.equipment.weaponRight.isTwoHanded()) {
                                    m_world->m_localCreature->m_inventory.add(
                                        m_world->m_localCreature->m_entityData.equipment.weaponRight, 1, -1);
                                    m_world->m_localCreature->m_entityData.equipment.weaponRight = Item();
                                }
                                m_world->m_localCreature->m_inventory.add(
                                    m_world->m_localCreature->m_entityData.equipment.weaponLeft, 1, -1);
                                m_world->m_localCreature->m_entityData.equipment.weaponLeft = item;
                                m_interface.m_inGame.m_equipmentInventoryWidget->updateRows();
                                m_interface.m_inGame.m_equipmentInventoryWidget->updateScrollButton();
                                playSound(SoundMenuDropItem, m_cameraPosition, 1.0f, 1.0f, true);
                                updateFormulas();
                            } else if (button == MouseButton_Left &&
                                       (item.id == WeaponBow || item.id == WeaponCrossbow)) {
                                m_world->m_localCreature->m_inventory.add(
                                    m_world->m_localCreature->m_entityData.equipment.weaponLeft, 1, -1);
                                m_world->m_localCreature->m_entityData.equipment.weaponLeft = item;
                                m_world->m_localCreature->m_inventory.add(
                                    m_world->m_localCreature->m_entityData.equipment.weaponRight, 1, -1);
                                m_world->m_localCreature->m_entityData.equipment.weaponRight = Item();
                                m_interface.m_inGame.m_equipmentInventoryWidget->updateRows();
                                m_interface.m_inGame.m_equipmentInventoryWidget->updateScrollButton();
                                playSound(SoundMenuDropItem, m_cameraPosition, 1.0f, 1.0f, true);
                                updateFormulas();
                            } else if (button == MouseButton_Right &&
                                       (item.id == WeaponBow || item.id == WeaponCrossbow)) {
                                m_world->m_localCreature->m_inventory.add(
                                    m_world->m_localCreature->m_entityData.equipment.weaponRight, 1, -1);
                                m_world->m_localCreature->m_entityData.equipment.weaponRight = item;
                                m_world->m_localCreature->m_inventory.add(
                                    m_world->m_localCreature->m_entityData.equipment.weaponLeft, 1, -1);
                                m_world->m_localCreature->m_entityData.equipment.weaponLeft = Item();
                                m_interface.m_inGame.m_equipmentInventoryWidget->updateRows();
                                m_interface.m_inGame.m_equipmentInventoryWidget->updateScrollButton();
                                playSound(SoundMenuDropItem, m_cameraPosition, 1.0f, 1.0f, true);
                                updateFormulas();
                            } else if ((button == MouseButton_Left && !m_keyStates[VK_SHIFT]) || item.isTwoHanded()) {
                                m_world->m_localCreature->m_inventory.add(
                                    m_world->m_localCreature->m_entityData.equipment.weaponRight, 1, -1);
                                m_world->m_localCreature->m_entityData.equipment.weaponRight = item;
                                if (item.isTwoHanded()) {
                                    m_world->m_localCreature->m_inventory.add(
                                        m_world->m_localCreature->m_entityData.equipment.weaponLeft, 1, -1);
                                    m_world->m_localCreature->m_entityData.equipment.weaponLeft = Item();
                                }
                                m_interface.m_inGame.m_equipmentInventoryWidget->updateRows();
                                m_interface.m_inGame.m_equipmentInventoryWidget->updateScrollButton();
                                playSound(SoundMenuDropItem, m_cameraPosition, 1.0f, 1.0f, true);
                                updateFormulas();
                            } else if (button == MouseButton_Right || m_keyStates[VK_SHIFT]) {
                                m_world->m_localCreature->m_inventory.add(
                                    m_world->m_localCreature->m_entityData.equipment.weaponLeft, 1, -1);
                                if (m_world->m_localCreature->m_entityData.equipment.weaponRight.isTwoHanded()) {
                                    m_world->m_localCreature->m_inventory.add(
                                        m_world->m_localCreature->m_entityData.equipment.weaponRight, 1, -1);
                                    m_world->m_localCreature->m_entityData.equipment.weaponRight = Item();
                                }
                                m_world->m_localCreature->m_entityData.equipment.weaponLeft = item;
                                m_interface.m_inGame.m_equipmentInventoryWidget->updateRows();
                                m_interface.m_inGame.m_equipmentInventoryWidget->updateScrollButton();
                                playSound(SoundMenuDropItem, m_cameraPosition, 1.0f, 1.0f, true);
                                updateFormulas();
                            } else {
                                updateFormulas();
                            }
                        } else if (stack.item.category == ItemCategoryPet) {
                            Item item = stack.item;
                            m_world->m_localCreature->m_inventory.add(
                                m_world->m_localCreature->m_entityData.equipment.pet, 1, -1);
                            m_world->m_localCreature->m_entityData.equipment.pet = item;
                            stack.quantity--;
                            if (stack.quantity <= 0) {
                                stack.quantity = 0;
                                stack.item.category = ItemCategoryNone;
                                stack.item.id = NoneCleared;
                            }
                            m_interface.m_inGame.m_equipmentInventoryWidget->updateRows();
                            m_interface.m_inGame.m_equipmentInventoryWidget->updateScrollButton();
                            playSound(SoundMenuDropItem, m_cameraPosition, 1.0f, 1.0f, true);
                        } else if (stack.item.category == ItemCategoryPetFood) {
                            // Pet food: feeding is ability 86 with the food as its item.
                            if (m_world->m_localCreature->m_entityData.currentAbility == AbilityNone ||
                                m_world->m_localCreature->m_entityData.timeSinceAbility >
                                    m_world->m_localCreature->attackDuration()) {
                                Item item = stack.item;
                                m_world->m_localCreature->m_entityData.actionItem = item;
                                m_world->m_localCreature->m_entityData.currentAbility = AbilityOfferPetFood;
                                m_world->m_localCreature->m_entityData.timeSinceAbility = 0;
                                m_interface.m_inGame.m_equipmentInventoryWidget->getNode()->setVisible(false);
                            }
                        } else if (stack.item.category == ItemCategoryEyePatch) {
                            Item item = stack.item;
                            stack.quantity--;
                            if (stack.quantity <= 0) {
                                stack.quantity = 0;
                                stack.item.category = ItemCategoryNone;
                                stack.item.id = NoneCleared;
                            }
                            m_world->m_localCreature->m_inventory.add(
                                m_world->m_localCreature->m_entityData.equipment.chest, 1, -1);
                            m_world->m_localCreature->m_entityData.equipment.head = item;
                            m_interface.m_inGame.m_equipmentInventoryWidget->updateRows();
                            m_interface.m_inGame.m_equipmentInventoryWidget->updateScrollButton();
                            playSound(SoundMenuDropItem, m_cameraPosition, 1.0f, 1.0f, true);
                        } else if (stack.item.category == ItemCategoryChest) {
                            Item item = stack.item;
                            stack.quantity--;
                            if (stack.quantity <= 0) {
                                stack.quantity = 0;
                                stack.item.category = ItemCategoryNone;
                                stack.item.id = NoneCleared;
                            }
                            m_world->m_localCreature->m_inventory.add(
                                m_world->m_localCreature->m_entityData.equipment.chest, 1, -1);
                            m_world->m_localCreature->m_entityData.equipment.chest = item;
                            m_interface.m_inGame.m_equipmentInventoryWidget->updateRows();
                            m_interface.m_inGame.m_equipmentInventoryWidget->updateScrollButton();
                            playSound(SoundMenuDropItem);
                        } else if (stack.item.category == ItemCategoryShoulder) {
                            Item item = stack.item;
                            stack.quantity--;
                            if (stack.quantity <= 0) {
                                stack.quantity = 0;
                                stack.item.category = ItemCategoryNone;
                                stack.item.id = NoneCleared;
                            }
                            m_world->m_localCreature->m_inventory.add(
                                m_world->m_localCreature->m_entityData.equipment.shoulder, 1, -1);
                            m_world->m_localCreature->m_entityData.equipment.shoulder = item;
                            m_interface.m_inGame.m_equipmentInventoryWidget->updateRows();
                            m_interface.m_inGame.m_equipmentInventoryWidget->updateScrollButton();
                            playSound(SoundMenuDropItem);
                        } else if (stack.item.category == ItemCategoryNeck) {
                            Item item = stack.item;
                            stack.quantity--;
                            if (stack.quantity <= 0) {
                                stack.quantity = 0;
                                stack.item.category = ItemCategoryNone;
                                stack.item.id = NoneCleared;
                            }
                            m_world->m_localCreature->m_inventory.add(
                                m_world->m_localCreature->m_entityData.equipment.neck, 1, -1);
                            m_world->m_localCreature->m_entityData.equipment.neck = item;
                            m_interface.m_inGame.m_equipmentInventoryWidget->updateRows();
                            m_interface.m_inGame.m_equipmentInventoryWidget->updateScrollButton();
                            playSound(SoundMenuDropItem);
                        } else if (stack.item.category == ItemCategoryRing) {
                            Item item = stack.item;
                            stack.quantity--;
                            if (stack.quantity <= 0) {
                                stack.quantity = 0;
                                stack.item.category = ItemCategoryNone;
                                stack.item.id = NoneCleared;
                            }
                            if (button == MouseButton_Left) {
                                m_world->m_localCreature->m_inventory.add(
                                    m_world->m_localCreature->m_entityData.equipment.ringRight, 1, -1);
                                m_world->m_localCreature->m_entityData.equipment.ringRight = item;
                            } else {
                                m_world->m_localCreature->m_inventory.add(
                                    m_world->m_localCreature->m_entityData.equipment.ringLeft, 1, -1);
                                m_world->m_localCreature->m_entityData.equipment.ringLeft = item;
                            }
                            m_interface.m_inGame.m_equipmentInventoryWidget->updateRows();
                            m_interface.m_inGame.m_equipmentInventoryWidget->updateScrollButton();
                            playSound(SoundMenuDropItem);
                        } else if (stack.item.category == ItemCategoryHands) {
                            Item item = stack.item;
                            stack.quantity--;
                            if (stack.quantity <= 0) {
                                stack.quantity = 0;
                                stack.item.category = ItemCategoryNone;
                                stack.item.id = NoneCleared;
                            }
                            m_world->m_localCreature->m_inventory.add(
                                m_world->m_localCreature->m_entityData.equipment.hands, 1, -1);
                            m_world->m_localCreature->m_entityData.equipment.hands = item;
                            m_interface.m_inGame.m_equipmentInventoryWidget->updateRows();
                            m_interface.m_inGame.m_equipmentInventoryWidget->updateScrollButton();
                            playSound(SoundMenuDropItem);
                        } else if (stack.item.category == ItemCategoryFeet) {
                            Item item = stack.item;
                            stack.quantity--;
                            if (stack.quantity <= 0) {
                                stack.quantity = 0;
                                stack.item.category = ItemCategoryNone;
                                stack.item.id = NoneCleared;
                            }
                            m_world->m_localCreature->m_inventory.add(
                                m_world->m_localCreature->m_entityData.equipment.feet, 1, -1);
                            m_world->m_localCreature->m_entityData.equipment.feet = item;
                            m_interface.m_inGame.m_equipmentInventoryWidget->updateRows();
                            m_interface.m_inGame.m_equipmentInventoryWidget->updateScrollButton();
                            playSound(SoundMenuDropItem);
                        } else if (stack.item.category == ItemCategoryConsumable ||
                                   stack.item.category == ItemCategorySpecial ||
                                   stack.item.category == ItemCategoryHouseIcon ||
                                   stack.item.category == ItemCategoryElixir) {
                            m_world->enterZoneMeshes();
                            useItem(stack);
                            m_world->leaveZoneMeshes();
                        }
                    }
                }
            } else {
                Item *equipped = getHoveredEquipment();
                if (equipped && equipped->category != ItemCategoryNone) {
                    if (m_interface.m_inGame.m_vendorInventoryWidget->getNode()->isVisible()) {
                        m_world->m_localCreature->m_inventory.m_gold += equipped->sellPrice();
                        playSound(SoundDropCoin, m_world->m_localCreature->m_entityData.position, 0.75f, 1.0f, true);
                        ItemStack sold;
                        sold.quantity = 1;
                        sold.item = *equipped;
                        m_soldItems.push_back(sold);
                        if (m_soldItems.size() > 10) {
                            m_soldItems.pop_front();
                        }
                        updateVendor();
                        equipped->category = ItemCategoryNone;
                        equipped->id = NoneCleared;
                    } else if (m_interface.m_inGame.m_adaptionWidget->getNode()->isVisible()) {
                        if ((equipped->category >= ItemCategoryWeapon && equipped->category <= ItemCategoryRing) ||
                            equipped->category == ItemCategoryEyePatch) {
                            m_interface.m_inGame.m_adaptionWidget->m_item = equipped;
                        } else {
                            playSound(SoundHumanVoice01, m_cameraPosition, 1.0f, 1.0f, true);
                        }
                    } else {
                        m_world->m_localCreature->m_inventory.add(*equipped, 1, -1);
                        equipped->category = ItemCategoryNone;
                        equipped->id = NoneCleared;
                        m_interface.m_inGame.m_equipmentInventoryWidget->updateRows();
                        m_interface.m_inGame.m_equipmentInventoryWidget->updateScrollButton();
                        playSound(SoundMenuDropItem);
                        updateFormulas();
                    }
                } else {
                    Item *offer = getHoveredVendorItem();
                    if (offer) {
                        if (m_interface.m_inGame.m_vendorInventoryWidget->m_tabIndex == VendorTabBuyback) {
                            // The buy-back tab shows m_soldItems newest first.
                            int index = (int)m_soldItems.size() - 1;
                            for (std::list<ItemStack>::iterator it = m_soldItems.begin(); it != m_soldItems.end();
                                 ++it, --index) {
                                if (index != m_interface.m_inGame.m_vendorInventoryWidget->m_hoveredCell[1]) {
                                    continue;
                                }
                                int bought = 0;
                                for (int i = 0; i < it->quantity; i++) {
                                    if (!m_world->m_localCreature->canTakeItem(it->item)) {
                                        m_interface.m_inGame.m_chatWidget->printMessage(
                                            L"You can't carry more of these items.\n",
                                            plasma::Vector<4, float>(1.0f, 0.2f, 0.2f, 1.0f));
                                        playSound(SoundHumanVoice01, m_world->m_localCreature->m_entityData.position,
                                                  1.0f, 1.0f, true);
                                        break;
                                    }
                                    if (offer->sellPrice() <= m_world->m_localCreature->m_inventory.m_gold) {
                                        m_world->m_localCreature->m_inventory.add(it->item, 1, -1);
                                        m_world->m_localCreature->m_inventory.m_gold -= it->item.sellPrice();
                                        notifyItem(it->item, 1, m_world->m_localCreature);
                                        bought++;
                                    }
                                }
                                it->quantity -= bought;
                                if (it->quantity == 0) {
                                    m_soldItems.erase(it);
                                }
                                playSound(SoundDrop, m_world->m_localCreature->m_entityData.position, 1.0f, 1.0f, true);
                                updateVendor();
                                m_interface.m_inGame.m_equipmentInventoryWidget->updateRows();
                                m_interface.m_inGame.m_equipmentInventoryWidget->updateScrollButton();
                                break;
                            }
                        } else if (offer->price() <= m_world->m_localCreature->m_inventory.m_gold) {
                            if (!m_world->m_localCreature->canTakeItem(*offer)) {
                                m_interface.m_inGame.m_chatWidget->printMessage(
                                    L"You can't carry more of these items.\n",
                                    plasma::Vector<4, float>(1.0f, 0.2f, 0.2f, 1.0f));
                                playSound(SoundHumanVoice01, m_world->m_localCreature->m_entityData.position, 1.0f,
                                          1.0f, true);
                            } else {
                                m_world->m_localCreature->m_inventory.add(*offer, 1, -1);
                                m_world->m_localCreature->m_inventory.m_gold -= offer->price();
                                m_world->m_localCreature->m_playerData->purchasesByShop[m_vendorId].push_back(*offer);
                                playSound(SoundDrop, m_world->m_localCreature->m_entityData.position, 1.0f, 1.0f, true);
                                m_interface.m_inGame.m_equipmentInventoryWidget->updateRows();
                                m_interface.m_inGame.m_equipmentInventoryWidget->updateScrollButton();
                                updateVendor();
                                notifyItem(*offer, 1, m_world->m_localCreature);
                                updateFormulas();
                            }
                        } else {
                            m_interface.m_inGame.m_chatWidget->printMessage(
                                L"Not enough money.\n", plasma::Vector<4, float>(1.0f, 0.2f, 0.2f, 1.0f));
                            playSound(SoundHumanVoice01);
                        }
                    }
                }
            }
        }
        if (button == MouseButton_Left) {
            if (m_interface.m_inGame.m_craftingInventoryWidget->getNode()->isVisible()) {
                m_interface.m_inGame.m_craftingInventoryWidget->updateSelection(true);
                InventoryWidget *crafting = m_interface.m_inGame.m_craftingInventoryWidget;
                if (crafting->m_selectedCell[0] < 0 ||
                    crafting->m_selectedCell[0] >= (int)m_craftingInventory.m_inventoryTabs.size() ||
                    crafting->m_selectedCell[1] < 0 ||
                    crafting->m_selectedCell[1] >=
                        (int)m_craftingInventory.m_inventoryTabs[crafting->m_selectedCell[0]].size()) {
                    m_interface.m_inGame.m_formulaDetailsWidget->m_formula = Formula();
                    m_interface.m_inGame.m_formulaDetailsWidget->m_formulaCell = plasma::Vector<2, int>(-1, -1);
                } else {
                    Formula formula;
                    if (m_world->groundedFormula(
                            m_craftingInventory
                                .m_inventoryTabs[m_interface.m_inGame.m_craftingInventoryWidget->m_selectedCell[0]]
                                                [m_interface.m_inGame.m_craftingInventoryWidget->m_selectedCell[1]]
                                .item,
                            formula,
                            m_interface.m_inGame.m_craftingInventoryWidget->m_selectedCell[0] == CraftingTabRepair)) {
                        m_interface.m_inGame.m_formulaDetailsWidget->m_formula = formula;
                        m_interface.m_inGame.m_formulaDetailsWidget->m_formulaCell =
                            m_interface.m_inGame.m_craftingInventoryWidget->m_selectedCell;
                    } else {
                        m_interface.m_inGame.m_formulaDetailsWidget->m_formula = Formula();
                        m_interface.m_inGame.m_formulaDetailsWidget->m_formulaCell = plasma::Vector<2, int>(-1, -1);
                    }
                }
            }
            if (m_interface.m_inGame.m_voxelWidget->getNode()->isVisible() &&
                m_interface.m_inGame.m_voxelWidget->selectedItem()) {
                VoxelWidget *voxelWidget = m_interface.m_inGame.m_voxelWidget;
                if (voxelWidget->getHoveredUpgradeItem()) {
                    voxelWidget->selectUpgrade();
                } else if (voxelWidget->m_hoveredItemIndex >= 0) {
                    voxelWidget->selectItem();
                } else {
                    m_interface.m_inGame.m_voxelWidget->placeUpgrade(voxelWidget->selectedItem()->material);
                }
            }
        }
    }
    // The equipment may have changed the maximum: keep the health's share.
    m_world->m_localCreature->m_entityData.hp =
        m_world->m_localCreature->maxHP() / maxHP * m_world->m_localCreature->m_entityData.hp;
    m_world->m_localCreature->m_entityData.hp =
        (std::min)(m_world->m_localCreature->m_entityData.hp, m_world->m_localCreature->maxHP());
    m_interface.m_inGame.m_chatWidget->m_typeboxActive = false;
    m_interface.m_inGame.m_selector->setVisible(false);
}

// s_enchantWidgetPressed is set by handleButtonDown.
void Game::handleButtonUp(int button)
{
    if (m_interface.m_startMenuNode->isVisible() && m_interface.m_startMenuButtons->isVisible() &&
        button == MouseButton_Left && m_interface.m_startMenuWidget->m_hoveredButton >= 0) {
        switch (m_interface.m_startMenuWidget->m_hoveredButton) {
        case StartMenuPlay:
            m_interface.m_startMenuNode->setVisible(false);
            m_interface.m_characterSelection.m_node->setVisible(true);
            break;
        case StartMenuOptions:
            m_interface.m_optionsWidget->getNode()->setVisible(true);
            break;
        case StartMenuExit:
            m_shutdown = true;
            break;
        case StartMenuContinue:
            loadCharacter(m_currentCharacterSlot, m_world->getLocalCreature());
            m_interface.m_startMenuNode->setVisible(false);
            m_targetCameraDistance = 8.0f;
            m_cameraDistance = 8.0f;
            m_world->spawnLocalCreature();
            break;
        }
    }
    m_interface.handleMouseUp(button);
    if (m_interface.m_inGame.m_questionWidget->getNode()->isVisible() && button == MouseButton_Left) {
        ask(m_world->creatureByID(m_interface.m_inGame.m_questionWidget->m_speakerId),
            m_interface.m_inGame.m_questionWidget->getSelectedAnswer());
    }
    if (m_interface.m_inGame.m_enchantWidget->getNode()->isVisible() && s_enchantWidgetPressed &&
        button == MouseButton_Left) {
        if (m_interface.m_inGame.m_enchantWidget->isPointerOverIdentify()) {
            Creature *player = m_world->m_localCreature;
            if (m_interface.m_inGame.m_enchantWidget->getCost() <= player->m_inventory.m_gold)

                if (m_interface.m_inGame.m_enchantWidget->m_canIdentify == 1 &&
                    m_interface.m_inGame.m_enchantWidget->m_inventoryTabIndex >= 0 &&
                    m_interface.m_inGame.m_enchantWidget->m_inventoryTabIndex <
                        (int)m_world->m_localCreature->m_inventory.m_inventoryTabs.size() &&
                    m_interface.m_inGame.m_enchantWidget->m_inventoryItemIndex >= 0 &&
                    m_interface.m_inGame.m_enchantWidget->m_inventoryItemIndex <
                        (int)m_world->m_localCreature->m_inventory
                            .m_inventoryTabs[m_interface.m_inGame.m_enchantWidget->m_inventoryTabIndex]
                            .size() &&
                    m_world->m_localCreature->m_inventory
                            .m_inventoryTabs[m_interface.m_inGame.m_enchantWidget->m_inventoryTabIndex]
                                            [m_interface.m_inGame.m_enchantWidget->m_inventoryItemIndex]
                            .quantity != 0 &&
                    m_interface.m_inGame.m_enchantWidget->isUnidentifiedItem()) {
                    m_world->m_localCreature->m_inventory.m_gold -= m_interface.m_inGame.m_enchantWidget->getCost();
                    Item item = m_world->m_localCreature->m_inventory
                                    .m_inventoryTabs[m_interface.m_inGame.m_enchantWidget->m_inventoryTabIndex]
                                                    [m_interface.m_inGame.m_enchantWidget->m_inventoryItemIndex]
                                    .item;
                    ItemStack &stack = m_world->m_localCreature->m_inventory
                                           .m_inventoryTabs[m_interface.m_inGame.m_enchantWidget->m_inventoryTabIndex]
                                                           [m_interface.m_inGame.m_enchantWidget->m_inventoryItemIndex];
                    stack.quantity--;
                    if (stack.quantity <= 0) {
                        stack.quantity = 0;
                        stack.item.category = ItemCategoryNone;
                        stack.item.id = NoneCleared;
                    }
                    Item reward = rand() % 2 == 0 ? m_world->randomWeapon(item.rarity, -1, false)
                                                  : m_world->randomArmor(item.rarity, -1, false);
                    if (reward.hasRegion()) {
                        reward.region = item.region;
                    }
                    m_world->m_localCreature->m_inventory.add(reward, 1);
                    m_interface.m_inGame.m_equipmentInventoryWidget->updateRows();
                    m_interface.m_inGame.m_equipmentInventoryWidget->updateScrollButton();
                    playSound(SoundDisenchant2, m_cameraPosition, 1.0f, 1.0f, true);
                    notifyItem(reward, 1, m_world->m_localCreature);
                }
        }
        if (m_interface.m_inGame.m_enchantWidget->isPointerOverGoodbye()) {
            m_interface.m_inGame.m_enchantWidget->getNode()->setVisible(false);
            m_interface.m_inGame.m_equipmentInventoryWidget->getNode()->setVisible(false);
        }
    }
    if (button == MouseButton_Left && m_interface.m_mapOpen && !m_pointerMoved &&
        m_worldMap->m_selectedMarker[0] != 0x7fffffff) {
        bool discovered;
        {
            std::array<int, 2> cell = {floorDivide(m_worldMap->m_selectedMarker[0], BlocksPerCell),
                                       floorDivide(m_worldMap->m_selectedMarker[1], BlocksPerCell)};
            discovered = m_world->m_localCreature->m_playerData->discoveredMarkers.find(cell) !=
                         m_world->m_localCreature->m_playerData->discoveredMarkers.end();
        }
        if (discovered) {
            m_world->m_localCreature->m_entityData.currentAbility = AbilityPortCity;
            m_world->m_localCreature->m_entityData.timeSinceAbility = 0.0f;
            m_world->m_localCreature->m_entityData.portCity =
                plasma::Vector<2, int>(floorDivide(m_worldMap->m_selectedMarker[0], BlocksPerCell),
                                       floorDivide(m_worldMap->m_selectedMarker[1], BlocksPerCell));
            m_interface.m_mapOpen = false;
        }
    }
    if (m_interface.m_inGame.m_adaptionWidget->getNode()->isVisible() && button == MouseButton_Left) {
        if (m_interface.m_inGame.m_adaptionWidget->isPointerOverUpgrade()) {
            m_interface.m_inGame.m_adaptionWidget->getCost();
        }
        if (m_interface.m_inGame.m_adaptionWidget->isPointerOverGoodbye()) {
            m_interface.m_inGame.m_adaptionWidget->getNode()->setVisible(false);
        }
    }
    m_interface.m_controlsWidget->resetSelectedIndex();
    if (m_interface.m_controlsWidget->getNode()->isVisible() && button == MouseButton_Left &&
        m_interface.m_controlsWidget->m_hoveredIndex >= 0) {
        m_interface.m_controlsWidget->selectHoveredIndex();
    }
}

// F1 help, F2/F3 interface scale down/up (x1.25, between 1.25^-4 and 1.25^5), F4 the whole interface, F5/F6 map scale
// (x1.1, between 1.1^-3 and 1.1^3), Escape the system menu; Return opens the chat line or sends it. The chat commands
// /check, /namepet <name>, /sit, /wave, /pet and /dance are handled here; anything else is queued for the host.
void Game::handleKeyDown(unsigned char key)
{
    m_keyStates[key] = true;
    switch (key) {
    case VK_F1:
        m_interface.m_inGame.m_helpWidget->getNode()->setVisible(
            !m_interface.m_inGame.m_helpWidget->getNode()->isVisible());
        break;
    case VK_F2:
        m_interface.m_scale /= 1.25f;
        m_interface.m_scale = (std::max)(m_interface.m_scale, 1.0f / 1.25f / 1.25f / 1.25f / 1.25f);
        m_interface.updateResolution(m_width, m_height);
        m_options.guiScale = m_interface.m_scale;
        m_options.save();
        break;
    case VK_F3:
        m_interface.m_scale *= 1.25f;
        m_interface.m_scale = (std::min)(m_interface.m_scale, 1.25f * 1.25f * 1.25f * 1.25f * 1.25f);
        m_interface.updateResolution(m_width, m_height);
        m_options.guiScale = m_interface.m_scale;
        m_options.save();
        break;
    case VK_F4:
        m_engine->root()->setVisible(!m_engine->root()->isVisible());
        break;
    case VK_F5:
        m_options.mapScale /= 1.1f;
        m_options.mapScale = (std::max)(m_options.mapScale, 1.0f / 1.1f / 1.1f / 1.1f);
        m_options.save();
        break;
    case VK_F6:
        m_options.mapScale *= 1.1f;
        m_options.mapScale = (std::min)(m_options.mapScale, 1.1f * 1.1f * 1.1f);
        m_options.save();
        break;
    case VK_ESCAPE:
        m_interface.m_inGame.m_menuOpen = !m_interface.m_inGame.m_menuOpen;
        break;
    }
    if (m_interface.m_inGame.m_node->isVisible() && m_engine->getFocusWidget() == nullptr &&
        m_interface.m_controlsWidget->m_selectedIndex < 0) {
        if (m_interface.m_inGame.m_chatWidget->m_typeboxActive) {
            if (key == VK_RETURN) {
                m_interface.m_inGame.m_chatWidget->m_changed = true;
                m_interface.m_inGame.m_chatWidget->m_typeboxActive = false;
                if (m_interface.m_inGame.m_chatWidget->m_typeboxText.size() != 0) {
                    std::wstringstream stream(m_interface.m_inGame.m_chatWidget->m_typeboxText);
                    std::wstring command;
                    stream >> command;
                    if (command == L"/check") {
                        m_interface.m_inGame.m_multiplayerWidget->m_checkGame =
                            !m_interface.m_inGame.m_multiplayerWidget->m_checkGame;
                        m_interface.m_inGame.m_chatWidget->printMessage(
                            L"multiplayerWidget->checkGame = " +
                                std::to_wstring(m_interface.m_inGame.m_multiplayerWidget->m_checkGame) + L"\n",
                            plasma::Vector<4, float>(1.0f, 0.5f, 0.5f, 1.0f));
                    } else if (command == L"/namepet") {
                        std::wstring name;
                        stream >> name;
                        if (name.size() != 0) {
                            if (name.size() >= 16) {
                                m_interface.m_inGame.m_chatWidget->printMessage(
                                    L"Name is too long.\n", plasma::Vector<4, float>(1.0f, 0.5f, 0.5f, 1.0f));
                            } else if (m_world->getLocalCreature()->m_entityData.equipment.pet.category ==
                                       ItemCategoryPet) {
                                m_world->getLocalCreature()->m_entityData.equipment.pet.setPetName(name);
                            }
                        }
                    } else if (command == L"/sit") {
                        if (m_world->getLocalCreature()->m_entityData.timeSinceAbility >
                            (float)m_world->getLocalCreature()->attackDuration()) {
                            m_world->getLocalCreature()->m_entityData.currentAbility = AbilitySit;
                            m_world->getLocalCreature()->m_entityData.timeSinceAbility = 0.0f;
                        }
                    } else if (command == L"/wave") {
                        if (m_world->getLocalCreature()->m_entityData.timeSinceAbility >
                            (float)m_world->getLocalCreature()->attackDuration()) {
                            m_world->getLocalCreature()->m_entityData.currentAbility = AbilityWave;
                            m_world->getLocalCreature()->m_entityData.timeSinceAbility = 0.0f;
                        }
                    } else if (command == L"/pet") {
                        if (m_world->getLocalCreature()->m_entityData.timeSinceAbility >
                            (float)m_world->getLocalCreature()->attackDuration()) {
                            m_world->getLocalCreature()->m_entityData.currentAbility = AbilityPetAnimal;
                            m_world->getLocalCreature()->m_entityData.timeSinceAbility = 0.0f;
                        }
                    } else if (command == L"/dance") {
                        if (m_world->getLocalCreature()->m_entityData.timeSinceAbility >
                            (float)m_world->getLocalCreature()->attackDuration()) {
                            m_world->getLocalCreature()->m_entityData.currentAbility = AbilityDance;
                            m_world->getLocalCreature()->m_entityData.timeSinceAbility = 0.0f;
                        }
                    } else {
                        ChatMessage message;
                        message.creatureId = m_world->getLocalCreature()->m_id;
                        message.text = m_interface.m_inGame.m_chatWidget->m_typeboxText;
                        m_chatMessages.push_back(message);
                    }
                }
                m_interface.m_inGame.m_chatWidget->m_typeboxText = L"";
            }
            m_interface.m_inGame.m_chatWidget->handleKeyDown(key);
        } else if (key == VK_RETURN) {
            m_interface.m_inGame.m_chatWidget->m_typeboxActive = true;
        }
    }
}

void Game::handleKeyUp(unsigned char key)
{
    m_keyStates[key] = false;
}

double blend(double seconds, float percent)
{
    double value = 0.0;
    int steps = (int)(seconds * 100.0);
    for (int i = 0; i < steps; i++) {
        value += ((i + 1.0) / steps - value) * (percent / 100.0f);
    }
    return value;
}

void Game::changeWorld(int seed, const std::string &name)
{
    m_world->enterZoneMeshes();
    m_world->getLocalCreature()->m_playerData->lastWorld.seed = seed;
    m_world->getLocalCreature()->m_playerData->lastWorld.name = name;
    m_seed = seed;
    m_worldName = name;
    m_world->retainLocalCreature();
    m_world->leaveZoneMeshes();
}

void Game::disconnect()
{
    m_resetOnDisconnect = 0;
    if (m_connected) {
        m_connected = false;
        if (m_socket != 0) {
            closesocket(m_socket);
            m_socket = 0;
            m_world->m_isClient = false;
        }
        if (m_receiveThread.joinable()) {
            m_receiveThread.join();
        }
        if (m_sendThread.joinable()) {
            m_sendThread.join();
        }
        m_world->enterZoneMeshes();
        m_world->m_isClient = false;
        m_interface.m_inGame.m_chatWidget->printMessage(L"Disconnected.\n",
                                                        plasma::Vector<4, float>(1.0f, 0.5f, 0.2f, 1.0f));
        m_world->leaveZoneMeshes();
    }
}

void Game::equipNewCharacter()
{
    srand((unsigned int)m_world->m_localCreature->m_entityData.seed);
    m_world->getLocalCreature()->m_entityData.equipment = Equipment();
    m_world->m_localCreature->m_inventory = Inventory();
    m_world->m_localCreature->m_inventory.m_inventoryTabs.resize(6);
    m_world->m_localCreature->m_inventory.m_inventoryTabs[0].resize(10);
    m_world->m_localCreature->m_inventory.m_inventoryTabs[1].resize(10);
    m_world->m_localCreature->m_inventory.m_inventoryTabs[2].resize(10);
    m_world->m_localCreature->m_inventory.m_inventoryTabs[3].resize(30);
    m_world->m_localCreature->m_inventory.m_inventoryTabs[4].resize(30);
    switch (m_world->m_localCreature->m_entityData.classType) {
    case ClassRogue: {
        m_world->m_localCreature->m_entityData.equipment.weaponRight.category = ItemCategoryWeapon;
        m_world->m_localCreature->m_entityData.equipment.weaponRight.id = WeaponDagger;
        m_world->m_localCreature->m_entityData.equipment.weaponRight.material = ItemMaterialIron;
        m_world->m_localCreature->m_entityData.equipment.weaponRight.rarity = ItemRarityWhite;
        m_world->m_localCreature->m_entityData.equipment.weaponLeft.category = ItemCategoryWeapon;
        m_world->m_localCreature->m_entityData.equipment.weaponLeft.id = WeaponDagger;
        m_world->m_localCreature->m_entityData.equipment.weaponLeft.material = ItemMaterialIron;
        m_world->m_localCreature->m_entityData.equipment.weaponLeft.rarity = ItemRarityWhite;
        m_world->m_localCreature->m_entityData.equipment.chest.category = ItemCategoryChest;
        m_world->m_localCreature->m_entityData.equipment.chest.material = ItemMaterialCotton;
        m_world->m_localCreature->m_entityData.equipment.chest.rarity = ItemRarityWhite;
        Item item;
        item.category = ItemCategoryWeapon;
        item.id = WeaponFist;
        item.material = ItemMaterialIron;
        item.rarity = ItemRarityWhite;
        for (int i = 0; i < 2; i++) {
            m_world->m_localCreature->m_inventory.add(item, 1, -1);
        }
        item.category = ItemCategoryWeapon;
        item.id = WeaponLongsword;
        m_world->m_localCreature->m_inventory.add(item, 1, -1);
        break;
    }
    case ClassMage: {
        m_world->m_localCreature->m_entityData.equipment.weaponRight.category = ItemCategoryWeapon;
        m_world->m_localCreature->m_entityData.equipment.weaponRight.id = WeaponStaff;
        m_world->m_localCreature->m_entityData.equipment.weaponRight.material = ItemMaterialWood;
        m_world->m_localCreature->m_entityData.equipment.weaponRight.rarity = ItemRarityWhite;
        m_world->m_localCreature->m_entityData.equipment.chest.category = ItemCategoryChest;
        m_world->m_localCreature->m_entityData.equipment.chest.material = ItemMaterialSilk;
        m_world->m_localCreature->m_entityData.equipment.chest.rarity = ItemRarityWhite;
        Item item;
        item.category = ItemCategoryWeapon;
        item.id = WeaponBracelet;
        item.material = ItemMaterialGold;
        item.rarity = ItemRarityWhite;
        for (int i = 0; i < 2; i++) {
            m_world->m_localCreature->m_inventory.add(item, 1, -1);
        }
        item.material = ItemMaterialWood;
        item.category = ItemCategoryWeapon;
        item.id = WeaponWand;
        m_world->m_localCreature->m_inventory.add(item, 1, -1);
        break;
    }
    case ClassRanger: {
        m_world->m_localCreature->m_entityData.equipment.weaponRight.category = ItemCategoryWeapon;
        m_world->m_localCreature->m_entityData.equipment.weaponRight.id = WeaponBow;
        m_world->m_localCreature->m_entityData.equipment.weaponRight.material = ItemMaterialWood;
        m_world->m_localCreature->m_entityData.equipment.weaponRight.rarity = ItemRarityWhite;
        m_world->m_localCreature->m_entityData.equipment.chest.category = ItemCategoryChest;
        m_world->m_localCreature->m_entityData.equipment.chest.material = ItemMaterialLinen;
        m_world->m_localCreature->m_entityData.equipment.chest.rarity = ItemRarityWhite;
        Item item;
        item.category = ItemCategoryWeapon;
        item.id = WeaponCrossbow;
        item.material = ItemMaterialWood;
        item.rarity = ItemRarityWhite;
        m_world->m_localCreature->m_inventory.add(item, 1, -1);
        item.material = ItemMaterialWood;
        item.category = ItemCategoryWeapon;
        item.id = WeaponBoomerang;
        m_world->m_localCreature->m_inventory.add(item, 1, -1);
        break;
    }
    case ClassWarrior: {
        m_world->m_localCreature->m_entityData.equipment.weaponRight.category = ItemCategoryWeapon;
        m_world->m_localCreature->m_entityData.equipment.weaponRight.id = WeaponGreatmace;
        m_world->m_localCreature->m_entityData.equipment.weaponRight.material = ItemMaterialWood;
        m_world->m_localCreature->m_entityData.equipment.weaponRight.rarity = ItemRarityWhite;
        m_world->m_localCreature->m_entityData.equipment.chest.category = ItemCategoryChest;
        m_world->m_localCreature->m_entityData.equipment.chest.material = ItemMaterialIron;
        m_world->m_localCreature->m_entityData.equipment.chest.rarity = ItemRarityWhite;
        Item item;
        item.category = ItemCategoryWeapon;
        item.id = WeaponSword;
        item.material = ItemMaterialIron;
        item.rarity = ItemRarityWhite;
        for (int i = 0; i < 2; i++) {
            m_world->m_localCreature->m_inventory.add(item, 1, -1);
        }
        item.category = ItemCategoryWeapon;
        item.id = WeaponShield;
        m_world->m_localCreature->m_inventory.add(item, 1, -1);
        break;
    }
    }
    {
        Item formula;
        formula.category = ItemCategoryConsumable;
        formula.id = ConsumableLifePotion;
        for (int i = 0; i < 5; i++) {
            m_world->m_localCreature->m_inventory.add(formula, 1, -1);
        }
    }
    if (m_world->m_localCreature->m_entityData.equipment.weaponLeft.category != ItemCategoryNone) {
        m_world->m_localCreature->m_entityData.equipment.weaponLeft.region =
            m_world->m_localCreature->m_entityData.region;
        m_world->getLocalCreature()->m_entityData.equipment.weaponLeft.modifier = rand();
    }
    if (m_world->m_localCreature->m_entityData.equipment.weaponRight.category != ItemCategoryNone) {
        m_world->m_localCreature->m_entityData.equipment.weaponRight.region =
            m_world->m_localCreature->m_entityData.region;
        m_world->getLocalCreature()->m_entityData.equipment.weaponRight.modifier = rand();
    }
    if (m_world->m_localCreature->m_entityData.equipment.chest.category != ItemCategoryNone) {
        m_world->m_localCreature->m_entityData.equipment.chest.region = m_world->m_localCreature->m_entityData.region;
        m_world->getLocalCreature()->m_entityData.equipment.chest.modifier = rand();
    }
    for (std::vector<ItemStack> &tab : m_world->m_localCreature->m_inventory.m_inventoryTabs) {
        for (ItemStack &stack : tab) {
            if (stack.quantity != 0 &&
                ((stack.item.category >= ItemCategoryWeapon && stack.item.category <= ItemCategoryRing) ||
                 stack.item.category == ItemCategoryEyePatch)) {
                stack.item.region = m_world->m_localCreature->m_entityData.region;
                stack.item.modifier = rand();
            }
        }
    }
    Creature *player = m_world->m_localCreature;
    player->m_entityData.hp = player->maxHP();
    m_world->m_localCreature->m_playerData->knownFormulas.clear();
    learnFormulas(0);
}

std::wstring Game::getFeatText(const unsigned char &feat)
{
    return L"FEAT!";
}

// In a menu the movement goes to the interface's pointer; with the right button it drags the map (rotated by the
// camera's z angle, scaled by 2^zoom) and turns the voxel widget's model. Outside menus, or with the left button on no
// node, it turns the camera.
void Game::handleMouseMove(float deltaX, float deltaY)
{
    if (m_interface.isInMenu()) {
        m_interface.movePointer((int)deltaX, (int)deltaY);
        if (m_gamepadActive) {
            return;
        }
        if (m_engine->m_pointer[0] - m_engine->m_previousPointer[0] != 0.0f ||
            m_engine->m_pointer[1] - m_engine->m_previousPointer[1] != 0.0f) {
            m_pointerMoved = true;
        }
        if (m_interface.m_mapOpen && m_engine->isButtonDown(1)) {
            plasma::Matrix<float> rotation;
            rotation.identity();
            rotation.rotateZ((float)m_cameraAngle[2]);
            plasma::Vector<3, float> drag(m_engine->m_pointer[0] - m_engine->m_previousPointer[0],
                                          -(m_engine->m_pointer[1] - m_engine->m_previousPointer[1]), 0.0f);
            drag *= powf(2.0f, (float)m_mapZoom);
            m_targetMapPosition += plasma::Vector<3, Fixed>(rotation.transformDirection(drag));
            m_mapZoomChanged = true;
        }
        if (m_interface.m_inGame.m_voxelWidget->getNode()->isVisible() && m_engine->isButtonDown(1)) {
            m_interface.m_inGame.m_voxelWidget->m_rotation[0] -=
                (m_engine->m_pointer[1] - m_engine->m_previousPointer[1]) * ((float)m_options.cameraSpeed * 0.005f);
            m_interface.m_inGame.m_voxelWidget->m_rotation[2] +=
                (m_engine->m_pointer[0] - m_engine->m_previousPointer[0]) * ((float)m_options.cameraSpeed * 0.005f);
        }
    }
    if (!m_interface.isInMenu() || (m_engine->isButtonDown(0) && m_engine->getPointerNode() == nullptr)) {
        if (m_interface.isInMenu()) {
            deltaX = m_engine->m_pointer[0] - m_engine->m_previousPointer[0];
            deltaY = m_engine->m_pointer[1] - m_engine->m_previousPointer[1];
        }
        if (m_options.invertY) {
            m_targetCameraAngle[0] -= (float)m_options.cameraSpeed * 0.005f * deltaY;
        } else {
            m_targetCameraAngle[0] += (float)m_options.cameraSpeed * 0.005f * deltaY;
        }
        if (m_targetCameraAngle[0] > 180.0) {
            m_targetCameraAngle[0] = 180.0;
        }
        if (m_targetCameraAngle[0] < 0.0) {
            m_targetCameraAngle[0] = 0.0;
        }
        m_targetCameraAngle[2] -= (float)m_options.cameraSpeed * 0.005f * deltaX;
    }
}

// The squared distance of two world positions, in blocks.
inline float distanceSquared(const plasma::Vector<3, Fixed> &a, const plasma::Vector<3, Fixed> &b)
{
    plasma::Vector<3, float> difference(a - b);
    return difference[0] * difference[0] + difference[1] * difference[1] + difference[2] * difference[2];
}

// The wheel scrolls the inventory, controls or multiplayer widget under the pointer; else, with control 13 held, it
// moves the camera in or out; else it zooms the map about the cursor.
void Game::handleMouseWheel(int direction)
{
    if (m_interface.m_inGame.m_equipmentInventoryWidget->getNode()->isVisible() &&
        m_interface.m_inGame.m_equipmentInventoryWidget->getNode()->isUnderPointer()) {
        if (direction < 0) {
            m_interface.m_inGame.m_equipmentInventoryWidget->scrollDown();
        } else if (direction > 0) {
            m_interface.m_inGame.m_equipmentInventoryWidget->scrollUp();
        }
    } else if (m_interface.m_inGame.m_vendorInventoryWidget->getNode()->isVisible() &&
               m_interface.m_inGame.m_vendorInventoryWidget->getNode()->isUnderPointer()) {
        if (direction < 0) {
            m_interface.m_inGame.m_vendorInventoryWidget->scrollDown();
        } else if (direction > 0) {
            m_interface.m_inGame.m_vendorInventoryWidget->scrollUp();
        }
    } else if (m_interface.m_inGame.m_craftingInventoryWidget->getNode()->isVisible() &&
               m_interface.m_inGame.m_craftingInventoryWidget->getNode()->isUnderPointer()) {
        if (direction < 0) {
            m_interface.m_inGame.m_craftingInventoryWidget->scrollDown();
        } else if (direction > 0) {
            m_interface.m_inGame.m_craftingInventoryWidget->scrollUp();
        }
    } else if (m_interface.m_controlsWidget->getNode()->isVisible() &&
               m_interface.m_controlsWidget->getNode()->isUnderPointer()) {
        if (direction < 0) {
            m_interface.m_controlsWidget->scrollDown();
        } else if (direction > 0) {
            m_interface.m_controlsWidget->scrollUp();
        }
    } else if (m_interface.m_inGame.m_multiplayerWidget->getNode()->isVisible() &&
               m_interface.m_inGame.m_multiplayerWidget->getNode()->isUnderPointer()) {
        if (direction < 0) {
            m_interface.m_inGame.m_multiplayerWidget->scrollDown();
        } else if (direction > 0) {
            m_interface.m_inGame.m_multiplayerWidget->scrollUp();
        }
    } else if (m_controlStates[Controls::ButtonWalk]) {
        m_targetCameraDistance -= (float)(direction * 2);
        if (m_targetCameraDistance < 0.0f) {
            m_targetCameraDistance = 0.0f;
        }
        if (m_targetCameraDistance > 50.0f) {
            m_targetCameraDistance = 50.0f;
        }
    } else {
        int oldZoom = m_mapZoom;
        int zoom = (std::max)(m_mapZoom - direction, 0);
        if (zoom > MaximumMapZoom)
            zoom = MaximumMapZoom;
        m_mapZoom = zoom;
        if (zoom != oldZoom && m_worldMap->m_cursorPosition[0] != Fixed(0x7fffffff)) {
            plasma::Vector<2, Fixed> offset(m_worldMap->m_cursorPosition[0] - m_targetMapPosition[0],
                                            m_worldMap->m_cursorPosition[1] - m_targetMapPosition[1]);
            if (zoom < oldZoom) {
                Fixed scale(0.5);
                offset[0] *= scale;
                offset[1] *= scale;
            } else if (zoom > oldZoom) {
                Fixed scale(-1.0);
                offset[0] *= scale;
                offset[1] *= scale;
            }
            m_targetMapPosition[0] += offset[0];
            m_targetMapPosition[1] += offset[1];
            m_targetMapPosition[2] =
                Fixed(m_world->height((int)m_targetMapPosition[0], (int)m_targetMapPosition[1], false));
            m_mapPosition = m_targetMapPosition;
            m_mapZoomChanged = true;
        }
    }
}

void Game::interact(Creature *creature)
{
    Interaction interaction;
    interaction.creatureId = creature->m_id;
    interaction.type = InteractionActivate;
    m_world->getLocalCreature()->m_interactions.push_back(interaction);
}

bool Game::isAssigningControl()
{
    return m_interface.m_controlsWidget->m_selectedIndex >= 0;
}

bool Game::isInMenu()
{
    return m_interface.isInMenu();
}

Creature *Game::markedCreature()
{
    std::vector<Creature *> creatures = getFollowingCreatures();
    if (creatures.empty()) {
        return nullptr;
    }
    int index = m_markedCreatureIndex;
    if (index < 0) {
        index = ((-index / creatures.size() + 1) * creatures.size() + index) % creatures.size();
    }
    return creatures[index % creatures.size()];
}

void Game::onChangeInventoryTab(InventoryWidget *widget)
{
    if (widget == m_interface.m_inGame.m_vendorInventoryWidget) {
        updateVendor();
    }
}

// The character selection's "Delete": removes the selected saved character and its record, and renames the records
// after it to close the gap.
void Game::onDeleteCharacter()
{
    delete m_savedCharacters[m_interface.m_selectedCharacter];
    m_savedCharacters.erase(m_savedCharacters.begin() + m_interface.m_selectedCharacter);
    BytesIO data;
    int count = (int)m_savedCharacters.size();
    data.write(count);
    m_charactersDatabase.write("num", data);
    std::stringstream key;
    key << m_interface.m_selectedCharacter;
    m_charactersDatabase.erase(key.str());
    for (int i = m_interface.m_selectedCharacter; i < count + 1; i++) {
        std::stringstream from;
        from << i + 1;
        std::stringstream to;
        to << i;
        m_charactersDatabase.rename(from.str(), to.str());
    }
    m_interface.updateSlots();
}

// Tests and appends the character's starting formulas.
void Game::learnFormulas(int rarity)
{
    Item item;
    item.category = ItemCategoryConsumable;
    item.id = ConsumableLifePotion;
    item.rarity = rarity;
    if (!m_world->getLocalCreature()->knowsFormula(item)) {
        m_world->getLocalCreature()->m_playerData->knownFormulas.push_back(item);
    }
    item.id = ConsumableCactusPotion;
    if (!m_world->getLocalCreature()->knowsFormula(item)) {
        m_world->getLocalCreature()->m_playerData->knownFormulas.push_back(item);
    }
    item.id = ConsumableGinsengSoup;
    if (!m_world->getLocalCreature()->knowsFormula(item)) {
        m_world->getLocalCreature()->m_playerData->knownFormulas.push_back(item);
    }
    item.id = ConsumablePineappleSlice;
    if (!m_world->getLocalCreature()->knowsFormula(item)) {
        m_world->getLocalCreature()->m_playerData->knownFormulas.push_back(item);
    }
    item.id = ConsumableMushroomSpit;
    if (!m_world->getLocalCreature()->knowsFormula(item)) {
        m_world->getLocalCreature()->m_playerData->knownFormulas.push_back(item);
    }
    item.id = ConsumablePumpkinMuffin;
    if (!m_world->getLocalCreature()->knowsFormula(item)) {
        m_world->getLocalCreature()->m_playerData->knownFormulas.push_back(item);
    }
    item.id = ConsumableSnowBerryMash;
    if (!m_world->getLocalCreature()->knowsFormula(item)) {
        m_world->getLocalCreature()->m_playerData->knownFormulas.push_back(item);
    }
    if (rarity == ItemRarityWhite) {
        item.rarity = ItemRarityWhite;
        item.category = ItemCategoryElixir;
        item.id = ElixirElixirOfLife;
        if (!m_world->getLocalCreature()->knowsFormula(item)) {
            m_world->getLocalCreature()->m_playerData->knownFormulas.push_back(item);
        }
        item.id = ElixirElixirOfPower;
        if (!m_world->getLocalCreature()->knowsFormula(item)) {
            m_world->getLocalCreature()->m_playerData->knownFormulas.push_back(item);
        }
        item.id = ElixirElixirOfToughness;
        if (!m_world->getLocalCreature()->knowsFormula(item)) {
            m_world->getLocalCreature()->m_playerData->knownFormulas.push_back(item);
        }
        item.id = ElixirElixirOfSanity;
        if (!m_world->getLocalCreature()->knowsFormula(item)) {
            m_world->getLocalCreature()->m_playerData->knownFormulas.push_back(item);
        }
        item.id = ElixirLemonade;
        if (!m_world->getLocalCreature()->knowsFormula(item)) {
            m_world->getLocalCreature()->m_playerData->knownFormulas.push_back(item);
        }
        item.id = ElixirHotChocolate;
        if (!m_world->getLocalCreature()->knowsFormula(item)) {
            m_world->getLocalCreature()->m_playerData->knownFormulas.push_back(item);
        }
        item.id = ElixirGreenSmoothie;
        if (!m_world->getLocalCreature()->knowsFormula(item)) {
            m_world->getLocalCreature()->m_playerData->knownFormulas.push_back(item);
        }
        item.category = ItemCategoryMaterial;
        item.id = MaterialItemWaterFlask;
        if (!m_world->getLocalCreature()->knowsFormula(item)) {
            m_world->getLocalCreature()->m_playerData->knownFormulas.push_back(item);
        }
        item.category = ItemCategoryMaterial;
        item.id = MaterialItemHeartflower;
        if (!m_world->getLocalCreature()->knowsFormula(item)) {
            m_world->getLocalCreature()->m_playerData->knownFormulas.push_back(item);
        }
        if (m_world->getLocalCreature()->m_entityData.classType == ClassRogue) {
            item.category = ItemCategoryMaterial;
            item.id = MaterialItemYarn;
            item.material = ItemMaterialCotton;
            if (!m_world->getLocalCreature()->knowsFormula(item)) {
                m_world->getLocalCreature()->m_playerData->knownFormulas.push_back(item);
            }
        }
        if (m_world->getLocalCreature()->m_entityData.classType == ClassRanger) {
            item.category = ItemCategoryMaterial;
            item.id = MaterialItemYarn;
            item.material = ItemMaterialLinen;
            if (!m_world->getLocalCreature()->knowsFormula(item)) {
                m_world->getLocalCreature()->m_playerData->knownFormulas.push_back(item);
            }
        }
        if (m_world->getLocalCreature()->m_entityData.classType == ClassMage) {
            item.category = ItemCategoryMaterial;
            item.id = MaterialItemYarn;
            item.material = ItemMaterialSilk;
            if (!m_world->getLocalCreature()->knowsFormula(item)) {
                m_world->getLocalCreature()->m_playerData->knownFormulas.push_back(item);
            }
        }
        if (m_world->getLocalCreature()->m_entityData.classType == ClassMage) {
            item.category = ItemCategoryMaterial;
            item.id = MaterialItemYarn;
            item.material = ItemMaterialSilk;
            if (!m_world->getLocalCreature()->knowsFormula(item)) {
                m_world->getLocalCreature()->m_playerData->knownFormulas.push_back(item);
            }
        }
        item.category = ItemCategoryMaterial;
        item.id = MaterialItemCube;
        item.material = ItemMaterialIron;
        if (!m_world->getLocalCreature()->knowsFormula(item)) {
            m_world->getLocalCreature()->m_playerData->knownFormulas.push_back(item);
        }
        item.category = ItemCategoryMaterial;
        item.id = MaterialItemCube;
        item.material = ItemMaterialWood;
        if (!m_world->getLocalCreature()->knowsFormula(item)) {
            m_world->getLocalCreature()->m_playerData->knownFormulas.push_back(item);
        }
        item.category = ItemCategoryMaterial;
        item.id = MaterialItemCube;
        item.material = ItemMaterialGold;
        if (!m_world->getLocalCreature()->knowsFormula(item)) {
            m_world->getLocalCreature()->m_playerData->knownFormulas.push_back(item);
        }
        item.category = ItemCategoryMaterial;
        item.id = MaterialItemCube;
        item.material = ItemMaterialSilver;
        if (!m_world->getLocalCreature()->knowsFormula(item)) {
            m_world->getLocalCreature()->m_playerData->knownFormulas.push_back(item);
        }
    }
}

// Reads the character of a slot from the characters database (the key is the slot number in decimal, the value a
// compressed character record) into creature. A character that does not know formula 7 yet learns it.
void Game::loadCharacter(int slot, Creature *creature)
{
    m_world->enterZoneMeshes();
    std::stringstream key;
    key << slot;
    BytesIO data;
    EnterCriticalSection(&m_characterCriticalSection);
    if (m_charactersDatabase.read(key.str(), data)) {
        decompress(data);
        Serializer(&data).loadCreature(creature);
        Item item;
        item.category = ItemCategoryConsumable;
        item.id = ConsumableSnowBerryMash;
        item.rarity = ItemRarityWhite;
        if (!creature->knowsFormula(item)) {
            creature->m_playerData->knownFormulas.push_back(item);
        }
    }
    LeaveCriticalSection(&m_characterCriticalSection);
    m_world->leaveZoneMeshes();
}

// Prints "You receive <item>." (or "<name> receives <item>.") in the chat: the count for stackable kinds, the item's
// name in its rarity colour, "+" for a renowned one and, with the show-rarity option, a "±" per rarity level.
void Game::notifyItem(const Item &item, int count, Creature *creature)
{
    if (item.category == ItemCategoryCoin) {
        return;
    }
    if (creature == m_world->m_localCreature) {
        if (item.category == ItemCategoryQuest) {
            m_interface.m_inGame.m_chatWidget->printMessage(L"You receive ",
                                                            plasma::Vector<4, float>(1.0f, 0.8f, 0.3f, 1.0f));
        } else {
            m_interface.m_inGame.m_chatWidget->printMessage(L"You receive ",
                                                            plasma::Vector<4, float>(0.5f, 1.0f, 1.0f, 1.0f));
        }
    } else {
        m_interface.m_inGame.m_chatWidget->printMessage(WidenString(creature->m_entityData.name) + L" receives ",
                                                        plasma::Vector<4, float>(0.5f, 1.0f, 1.0f, 1.0f));
    }
    std::wstringstream text;
    if (item.category == ItemCategoryCoin) {
        text << count << " " << m_speech.getText(m_speech.getMaterialKey(item.material), L"default", L"");
        m_interface.m_inGame.m_chatWidget->printMessage(text.str(), plasma::Vector<4, float>(0.5f, 1.0f, 1.0f, 1.0f));
    } else {
        if (item.category == ItemCategoryConsumable || item.category == ItemCategoryCube ||
            item.category == ItemCategoryMaterial || item.category == ItemCategoryPlatinumCoin ||
            item.category == ItemCategoryQuest || item.category == ItemCategoryElixir) {
            if (count > 0) {
                text << count << L" X ";
            }
        }
        text << m_speech.createItemName(item);
        if ((item.category == ItemCategoryWeapon ||
             (((unsigned char)(item.category - ItemCategoryChest) <= 5 || item.category == ItemCategoryEyePatch) &&
              ((item.category >= ItemCategoryChest && item.category <= ItemCategoryRing) ||
               item.category == ItemCategoryEyePatch))) &&
            item.modifier % ((item.rarity + 1) * 10) == 0) {
            text << L"+";
        }
        if (m_options.showRarity && ((item.category >= ItemCategoryWeapon && item.category <= ItemCategoryRing) ||
                                     item.category == ItemCategoryEyePatch)) {
            text << L" ";
            for (int i = 0; i <= item.getRarity(m_world->m_localCreature->m_entityData.region); i++) {
                text << L"\xb1";
            }
        }
        m_interface.m_inGame.m_chatWidget->printMessage(text.str(),
                                                        item.textColor(m_world->m_localCreature->m_entityData.region));
    }
    m_interface.m_inGame.m_chatWidget->printMessage(L".\n", plasma::Vector<4, float>(0.5f, 1.0f, 1.0f, 1.0f));
}

void Game::onBack()
{
    if (m_interface.m_worldCreationNode->isVisible()) {
        m_interface.m_worldCreationNode->setVisible(false);
        m_interface.m_worldSelection.m_node->setVisible(true);
    } else if (m_interface.m_worldSelection.m_node->isVisible()) {
        m_interface.m_worldSelection.m_node->setVisible(false);
        m_interface.m_characterSelection.m_node->setVisible(true);
    } else if (m_interface.m_characterCreationNode->isVisible()) {
        m_interface.m_characterCreationNode->setVisible(false);
        m_interface.m_characterSelection.m_node->setVisible(true);
    } else if (m_interface.m_characterSelection.m_node->isVisible()) {
        m_interface.m_characterSelection.m_node->setVisible(false);
        m_interface.m_startMenuNode->setVisible(true);
    }
}

// The world selection's "Delete": removes the selected world's two files and its entry, rewrites the world list and
// refreshes the page.
void Game::onDeleteWorld()
{
    if (m_currentWorld >= 0 && m_currentWorld < (int)m_worlds.size()) {
        std::string worldFile = "Save/world_" + m_worlds[m_currentWorld]->m_name + ".db";
        std::string mapFile = "Save/map_" + m_worlds[m_currentWorld]->m_name + ".db";
        DeleteFileA(worldFile.c_str());
        DeleteFileA(mapFile.c_str());
        delete m_worlds[m_currentWorld];
        m_worlds.erase(m_worlds.begin() + m_currentWorld);
        m_currentWorld = -1;
        BytesIO data;
        int count = (int)m_worlds.size();
        data.write(count);
        m_worldsDatabase.write("num", data);
        for (int i = 0; i < count; i++) {
            saveWorldInfo(i, m_worlds[i], true);
        }
        m_interface.updateWorldList();
    }
}

// The character creation's "Done": names the new character after the name field, makes it the next slot of the
// characters database, saves it and adds a copy to the saved characters.
void Game::onDoneCharacter()
{
    m_interface.m_characterCreationNode->setVisible(false);
    std::string name = NarrowString(m_interface.m_characterName->getText(L"edit", true));
    strcpy(m_world->m_localCreature->m_entityData.name, name.c_str());
    m_world->m_localCreature->m_entityData.level = 1;
    m_world->m_localCreature->m_playerData->discoveredMarkers.insert(std::array<int, 2> {
        {m_world->m_localCreature->m_entityData.portCity[0], m_world->m_localCreature->m_entityData.portCity[1]}});
    m_interface.m_inGame.m_helpWidget->getNode()->setVisible(true);
    BytesIO data;
    int count = (int)m_savedCharacters.size() + 1;
    data.write(count);
    m_charactersDatabase.write("num", data);
    m_world->enterZoneMeshes();
    m_currentCharacterSlot = count - 1;
    saveCharacter();
    m_world->leaveZoneMeshes();
    {
        BytesIO slot;
        slot.write(m_currentCharacterSlot);
        m_charactersDatabase.write("slot", slot);
    }
    Creature *saved = new Creature(0);
    saved->m_entityData = m_world->m_localCreature->m_entityData;
    saved->m_inventory = m_world->getLocalCreature()->m_inventory;
    saved->m_buffs = m_world->m_localCreature->m_buffs;
    saved->m_drawnPosition = saved->m_entityData.position;
    saved->m_playerData = new PlayerData;
    *(CharacterStyle *)saved->m_playerData = *m_world->m_localCreature->m_playerData;
    saved->m_playerData->knownFormulas = m_world->m_localCreature->m_playerData->knownFormulas;
    m_savedCharacters.push_back(saved);
    m_interface.updateSlots();
    m_interface.updateStyle();
    m_interface.refresh();
    updateFormulas();
}

// The world creation page's "Done": appends a world with the entered name and seed to the list, saves it and makes it
// the current world.
void Game::onDoneWorld()
{
    m_interface.m_worldCreationNode->setVisible(false);
    m_targetCameraDistance = 8.0f;
    m_cameraDistance = 8.0f;
    m_targetCameraAngle = plasma::Vector<3, double>(90.0, 0.0, 180.0);
    m_cameraAngle = m_targetCameraAngle;
    BytesIO data;
    int count = (int)m_worlds.size() + 1;
    data.write(count);
    m_worldsDatabase.write("num", data);
    std::wstring name = m_interface.m_worldName->getText(L"edit", true);
    std::wstring seed = m_interface.m_worldSeed->getText(L"edit", true);
    WorldInfo *world = new WorldInfo;
    std::wstringstream(seed) >> world->m_seed;
    world->m_name = NarrowString(name);
    m_worlds.push_back(world);
    m_currentWorld = (int)m_worlds.size() - 1;
    saveWorldInfo(m_currentWorld, world, true);
    changeWorld(m_worlds[m_currentWorld]->m_seed, m_worlds[m_currentWorld]->m_name);
    m_interface.updateWorldList();
}

void Game::onEditCharacter()
{
    m_interface.m_characterSelection.m_deleteCharacterButton->setVisible(true);
}

void Game::onEditWorld()
{
    m_interface.m_worldSelection.m_deleteWorldButton->setVisible(true);
}

void Game::onExit()
{
    m_shutdown = true;
}

void Game::onKeyTyped(unsigned char character)
{
    if (m_engine->getFocusWidget() == nullptr && m_interface.m_inGame.m_chatWidget->m_typeboxActive) {
        m_interface.m_inGame.m_chatWidget->typeCharacter((char)character);
    }
}

void Game::onP2PSessionRequest(P2PSessionRequest_t *request)
{
    int count = SteamFriends()->GetFriendCount(k_EFriendFlagImmediate);
    if (count == -1) {
        count = 0;
    }
    for (int i = 0; i < count; i++) {
        CSteamID friendId = SteamFriends()->GetFriendByIndex(i, k_EFriendFlagImmediate);
        if (friendId.IsValid() && friendId == request->m_steamIDRemote) {
            SteamNetworking()->AcceptP2PSessionWithUser(request->m_steamIDRemote);
        }
    }
}

// The character selection's "Play". A saved slot loads that character and records the slot in the characters database;
// any other selection starts a new character: the local player is reset to a level 1 warrior at a random start cell
// and the character creation page opens.
void Game::onSelectCharacter()
{
    m_interface.m_characterSelection.m_node->setVisible(false);
    m_world->enterZoneMeshes();
    m_currentCharacterSlot = m_interface.m_selectedCharacter;
    if (m_currentCharacterSlot >= 0 && m_currentCharacterSlot < (int)m_savedCharacters.size()) {
        loadCharacter(m_currentCharacterSlot, m_world->getLocalCreature());
        BytesIO data;
        data.write(m_interface.m_selectedCharacter);
        m_charactersDatabase.write("slot", data);
        m_interface.updateStyle();
        m_interface.refresh();
        updateFormulas();
        m_targetCameraDistance = 8.0f;
        m_cameraDistance = 8.0f;
        m_targetCameraAngle = plasma::Vector<3, double>(90.0, 0.0, 180.0);
        m_cameraAngle = m_targetCameraAngle;
    } else {
        m_targetCameraDistance = 8.0f;
        m_cameraDistance = 8.0f;
        m_targetCameraAngle = plasma::Vector<3, double>(90.0, 0.0, 0.0);
        m_cameraAngle = m_targetCameraAngle;
        m_interface.m_characterCreationNode->setVisible(true);
        m_world->m_localCreature->reset();
        m_world->m_localCreature->m_entityData.name[0] = 0;
        m_world->m_localCreature->m_entityData.hostilityType = HostilityPlayer;
        m_world->m_localCreature->m_entityData.rotation = plasma::Vector<3, float>(0, 0, 0);
        m_world->m_localCreature->m_entityData.velocity = plasma::Vector<3, float>(0, 0, 0);
        m_world->m_localCreature->m_entityData.flags = CreatureFlagNone;
        m_world->m_localCreature->m_entityData.level = 1;
        Creature *player = m_world->m_localCreature;
        player->m_entityData.hp = player->maxHP();
        m_world->m_localCreature->m_entityData.equipment.weaponRight.category = ItemCategoryWeapon;
        m_world->m_localCreature->m_entityData.equipment.weaponRight.id = WeaponSword;
        m_world->m_localCreature->m_entityData.equipment.weaponRight.material = ItemMaterialIron;
        m_world->m_localCreature->m_entityData.equipment.weaponLeft.category = ItemCategoryWeapon;
        m_world->m_localCreature->m_entityData.equipment.weaponLeft.id = WeaponSword;
        m_world->m_localCreature->m_entityData.equipment.weaponLeft.material = ItemMaterialIron;
        m_world->m_localCreature->m_entityData.equipment.chest.material = ItemMaterialIron;
        m_world->m_localCreature->m_entityData.hostilityType = HostilityPlayer;
        m_world->m_localCreature->m_entityData.classType = ClassWarrior;
        m_world->getLocalCreature()->m_entityData.portCity = m_world->randomStartCell();
        m_world->m_localCreature->m_entityData.position =
            plasma::Vector<3, Fixed>(Fixed(m_world->m_localCreature->m_entityData.portCity[0] * 16 + 8),
                                     Fixed((m_world->m_localCreature->m_entityData.portCity[1] + 2) * 16), Fixed(0));
        m_world->m_localCreature->m_entityData.position[2] =
            m_world->height((int)m_world->m_localCreature->m_entityData.position[0],
                            (int)m_world->m_localCreature->m_entityData.position[1], false);
        if (m_world->m_localCreature->m_playerData) {
            delete m_world->m_localCreature->m_playerData;
        }
        m_world->m_localCreature->m_playerData = new PlayerData;
        m_interface.m_characterName->setText(L"");
        m_interface.m_characterStyleWidget->reset();
        m_interface.m_characterStyleWidget->updateCreature(false);
    }
    m_world->spawnLocalCreature();
    m_world->leaveZoneMeshes();
}

void Game::onSelectWorld()
{
    m_interface.m_worldSelection.m_node->setVisible(false);
    m_targetCameraDistance = 8.0f;
    m_cameraDistance = 8.0f;
    m_targetCameraAngle = plasma::Vector<3, double>(90.0, 0.0, 180.0);
    m_cameraAngle = m_targetCameraAngle;
    if (m_currentWorld < (int)m_worlds.size()) {
        WorldInfo *world = m_worlds[m_currentWorld];
        changeWorld(world->m_seed, world->m_name);
    } else {
        m_interface.m_worldCreationNode->setVisible(true);
    }
}

void Game::onStartMenu()
{
    saveCharacter();
    disconnect();
    for (int i = 0; i < (int)m_savedCharacters.size(); i++) {
        loadCharacter(i, m_savedCharacters[i]);
    }
    m_interface.m_startMenuNode->setVisible(true);
    m_interface.m_inGame.m_menuOpen = false;
    m_interface.m_mapOpen = false;
    m_interface.m_inGame.m_equipmentInventoryWidget->getNode()->setVisible(false);
    m_interface.m_inGame.m_craftingInventoryWidget->getNode()->setVisible(false);
    m_interface.m_inGame.m_vendorInventoryWidget->getNode()->setVisible(false);
    m_interface.m_inGame.m_enchantWidget->getNode()->setVisible(false);
    m_interface.m_inGame.m_multiplayerWidget->getNode()->setVisible(false);
    m_interface.m_inGame.m_speechWidget->getNode()->setVisible(false);
    m_interface.m_inGame.m_questionWidget->getNode()->setVisible(false);
}

void Game::onToggleWorlds()
{
    m_interface.m_worldSelection.m_online = !m_interface.m_worldSelection.m_online;
    if (m_interface.m_worldSelection.m_online) {
        m_interface.m_worldSelection.m_multiplayerWorldsButton->setText(L"Singleplayer worlds...");
    } else {
        m_interface.m_worldSelection.m_multiplayerWorldsButton->setText(L"Multiplayer worlds...");
    }
    m_interface.updateWorldList();
}

void Game::playSound(int sound)
{
    playSound(sound, m_cameraPosition, 1.0f, 1.0f, true);
}

void Game::playSound(int sound, const plasma::Vector<3, Fixed> &position, float volume, float speed, bool unusedFlag)
{
    if (distanceSquared(position, m_cameraPosition) > 4096.0f) {
        return;
    }
    volume *= 1.0f - distanceSquared(position, m_cameraPosition) / 4096.0f;
    float pan = (m_worldViewMatrix * plasma::Vector<3, float>(position))[0] * -0.1f;
    pan = (std::max)(pan, -1.0f);
    pan = (std::min)(pan, 1.0f);
    if (sound >= SoundFlute1 && sound <= SoundNightingale1) {
        m_musicDuckTime = (std::max)(m_musicDuckTime, 6000);
    }
    m_audio->play(soundId(sound), (float)m_options.soundVolume * volume * 0.01f, pan, speed, false);
}

void Game::processHitLights()
{
    for (const ServerUpdates::HitPacket &hit : m_world->m_serverUpdates.m_hitPacketList) {
        if (!m_world->creatureByID(hit.targetId))
            continue;
        if (m_world->creatureByID(hit.attackerId) && hit.damage >= 0.0f) {
            Creature *attacker = m_world->creatureByID(hit.attackerId);
            attacker->m_damageDealt += hit.damage;
            m_world->creatureByID(hit.attackerId)->m_resetOnHit = 0;
        }
        if (hit.powered && m_world->creatureByID(hit.targetId) != m_world->getLocalCreature())
            m_world->creatureByID(hit.attackerId);
        Creature *local = m_world->getLocalCreature();
        if (hit.targetId == local->m_id && hit.hitType == HitTypeTameness && hit.damage >= 1.0f &&
            local->m_entityData.actionItem.category == ItemCategoryPetFood) {
            Creature *pet = m_world->creatureByID(hit.attackerId);
            if (pet) {
                if (local->m_entityData.equipment.pet.category != ItemCategoryNone)
                    local->m_inventory.add(local->m_entityData.equipment.pet, 1, -1);
                m_world->getLocalCreature()->m_entityData.equipment.pet = Item();
                m_world->getLocalCreature()->m_entityData.currentAbility = AbilityNone;
                m_world->getLocalCreature()->m_petId = pet->m_id;
                m_world->getLocalCreature()->m_entityData.equipment.pet.category = ItemCategoryPet;
                m_world->getLocalCreature()->m_entityData.equipment.pet.id = pet->m_entityData.race;
                m_world->getLocalCreature()->removeItem(m_world->getLocalCreature()->m_entityData.actionItem, 1);
                m_world->getLocalCreature()->m_entityData.actionItem = Item();
                std::cout << "TAMED!" << std::endl;
            }
        }
        if (hit.powered && m_world->creatureByID(hit.attackerId)) {
            std::list<Item *> weapons = m_world->creatureByID(hit.attackerId)->attackWeapons();
            for (Item *weapon : weapons) {
                plasma::Vector<4, float> color = weapon->effectColor();
                if (color[3] > 0.0f) {
                    Light light;
                    light.position = hit.position;
                    light.radius = 20.0f;
                    light.color = plasma::Vector<3, float>(color[0], color[1], color[2]);
                    light.age = 0;
                    light.duration = 400;
                    m_lights.push_back(light);
                }
            }
        }
        if (hit.supportRayLight && m_world->creatureByID(hit.attackerId)) {
            Light light;
            light.position = hit.position;
            light.radius = 16.0f;
            light.color = plasma::Vector<3, float>(0.3f, 0.3f, 0.5f);
            light.age = 0;
            light.duration = 400;
            m_lights.push_back(light);
        }
        if (hit.stunTime != 0) {
            TextFX effect;
            effect.color = plasma::Vector<4, float>(1, 1, 0, 1);
            effect.position = hit.position;
            effect.position[2] += Fixed(2);
            effect.text = L"K.O.";
            effect.pixelSize = 20.0f;
            effect.rise = m_world->creatureByID(hit.targetId) == m_world->getLocalCreature() ? -50.0f : 50.0f;
            effect.textFlags = 1 << plasma::TEXT_CENTER;
            effect.age = 0;
            m_textFx.push_back(effect);
        }
        if (m_world->creatureByID(hit.attackerId) != m_world->getLocalCreature() &&
            m_world->creatureByID(hit.targetId) != m_world->getLocalCreature())
            continue;
        if (hit.hitType == HitTypeDodge || hit.hitType == HitTypeTameness)
            continue;
        float magnitude = (float)fabs((double)hit.damage);
        if (!(magnitude >= 0.1f) && hit.hitType == HitTypeDamage)
            continue;
        TextFX effect;
        effect.position = hit.position;
        effect.position[2] += Fixed(2);
        Creature *target = m_world->creatureByID(hit.targetId);
        effect.position[2] += Fixed(1.0f - target->m_entityData.appearance.size[2] * 0.5f);
        if (hit.damage < 0.0f) {
            effect.color = plasma::Vector<4, float>(0.4f, 1, 0.5f, 1);
            if (m_world->creatureByID(hit.targetId) == m_world->getLocalCreature()) {
                effect.rise = 100.0f;
                effect.position[2] -= Fixed(1);
            } else {
                effect.rise = -100.0f;
            }
        } else if (m_world->creatureByID(hit.targetId) == m_world->getLocalCreature()) {
            effect.color = plasma::Vector<4, float>(1, 0.3f, 0.3f, 1);
            effect.rise = 100.0f;
            effect.position[2] -= Fixed(1);
        } else {
            effect.color = plasma::Vector<4, float>(1, 1, 0.25f, 1);
            if (!hit.powered)
                effect.color[2] = 1.0f;
            effect.rise = -100.0f;
            if (hit.critical)
                effect.rise = -200.0f;
        }
        effect.textFlags = 1 << plasma::TEXT_CENTER;
        effect.age = 0;
        effect.offsetX = 0;
        effect.offsetY = 0;
        effect.pixelSize = 20.0f;
        if (hit.critical)
            effect.pixelSize = 60.0f;
        std::wstringstream text(std::ios::in | std::ios::out);
        if (hit.hitType == HitTypeHeal)
            text << "BLOCK" << std::endl;
        else if (hit.hitType == HitTypeMiss)
            text << "MISS" << std::endl;
        else if (hit.hitType == HitTypeMana)
            text << "ABSORB" << std::endl;
        else if (magnitude < 1.0f)
            text << (float)(int)(magnitude * 10.0f) * 0.1f;
        else
            text << (int)magnitude;
        effect.text = text.str();
        m_textFx.push_back(effect);
    }
}

void Game::processNotices()
{
    if (!m_interface.m_inGame.m_missionTag->isVisible() && m_missionNotices.size() != 0) {
        MissionNotice notice = m_missionNotices.front();
        m_missionNotices.pop_front();
        m_interface.m_inGame.m_missionTag->setText(L"info", notice.info, true);
        m_interface.m_inGame.m_missionTag->setText(L"xp", notice.xp, true);
        m_interface.m_inGame.m_missionTag->play(L"shine", 0);
        playSound(SoundNewQuest, m_cameraPosition, 1.0f, 1.0f, true);
    }
}

void Game::resize(int width, int height)
{
    m_width = width;
    m_height = height;
    m_interface.updateResolution(width, height);
}

float roundTenth(float value)
{
    if (value < 0.0f) {
        float result = -roundTenth(-value);
        if (result == 0.0f) {
            return 0.0f;
        }
        return result;
    }
    return (float)(int)(value * 10.0f + 0.5f) * 0.1f;
}

// Writes the local player to the characters database under the current slot's number: a copy of the creature (its
// data, its buffs, its inventory and its player data) is made while the zones are locked and then serialised and
// compressed. The copy of the world's state is not used.
void Game::saveCharacter()
{
    m_world->enterZoneMeshes();
    int slot = m_currentCharacterSlot;
    if (slot < 0) {
        m_world->leaveZoneMeshes();
        return;
    }
    Creature *creature = new Creature(0);
    creature->m_entityData = m_world->m_localCreature->m_entityData;
    creature->m_petWaterLevel = m_world->m_localCreature->m_petWaterLevel;
    creature->m_unreadSavedInt = m_world->m_localCreature->m_unreadSavedInt;
    creature->m_playerData = new PlayerData(*m_world->m_localCreature->m_playerData);
    creature->m_inventory = m_world->getLocalCreature()->m_inventory;
    creature->m_buffs = m_world->m_localCreature->m_buffs;
    World::WorldState state = m_world->m_state;
    m_world->leaveZoneMeshes();
    BytesIO data;
    Serializer(&data).saveCreature(creature);
    delete creature;
    std::stringstream key;
    key << slot;
    compressBytes(data);
    EnterCriticalSection(&m_characterCriticalSection);
    m_charactersDatabase.write(key.str(), data);
    LeaveCriticalSection(&m_characterCriticalSection);
}

// Writes a world's entry to the worlds database under its index: the name, the seed, the explored area, and the
// preview sprite's size and compressed voxels. The entry of the running world first takes a copy of the map's third
// zoom level as its preview. lock: hold the zones meanwhile.
void Game::saveWorldInfo(int index, WorldInfo *world, bool lock)
{
    if (lock) {
        m_world->enterZoneMeshes();
    }
    if (world->m_name == m_world->m_worldName && world->m_seed == m_world->m_seed &&
        m_worldMap->m_levels[2].previewSprite) {
        delete world->m_preview;
        world->m_preview = new Sprite(*m_worldMap->m_levels[2].previewSprite);
    }
    BytesIO data;
    int length = (int)world->m_name.size();
    data.write(length);
    data.write(world->m_name.c_str(), length);
    data.write(world->m_seed);
    std::cout << "wrote " << world->m_seed << std::endl;
    data.write(world->m_exploredArea);
    plasma::Vector<3, int> size(0, 0, 0);
    if (world->m_preview) {
        size = plasma::Vector<3, int>(world->m_preview->sizeX(), world->m_preview->sizeY(), world->m_preview->sizeZ());
    }
    data.write(size);
    if (size[0] > 0 && size[1] > 0 && size[2] > 0) {
        std::vector<char> voxels(size[0] * size[1] * size[2] * sizeof(plasma::Vector<3, unsigned char>));
        unsigned int voxelsSize = (unsigned int)voxels.size();
        const plasma::Vector<3, unsigned char> *source = world->m_preview->m_voxels;
        memcpy(&voxels[0], source, voxels.size());
        std::vector<char> compressed;
        uLongf compressedSize = (uLongf)(voxelsSize * 1.1) + 12;
        compressed.resize(compressedSize);
        ::compress((Bytef *)&compressed[0], &compressedSize, (const Bytef *)&voxels[0], voxelsSize);
        compressed.resize(compressedSize);
        int count = (int)compressed.size();
        data.write(count);
        data.write(&compressed[0], count);
    }
    if (lock) {
        m_world->leaveZoneMeshes();
    }
    std::stringstream key;
    key << index;
    m_worldsDatabase.write(key.str(), data);
}

void Game::say(Creature *speaker, Creature *listener, const std::vector<Dialog> &dialogs)
{
    if (listener == m_world->m_localCreature) {
        m_interface.m_inGame.m_questionWidget->m_dialogs.clear();
    }
    SpeechWidget *widget;
    bool found = false;
    for (SpeechWidget *candidate : m_interface.m_inGame.m_speechWidgets) {
        widget = candidate;
        if (widget->m_speakerId == speaker->m_id) {
            found = true;
            break;
        }
    }
    if (!found) {
        widget = new SpeechWidget(this, m_interface.m_speech->instance(m_interface.m_inGame.m_node), nullptr, nullptr);
        m_interface.m_inGame.m_speechWidgets.push_back(widget);
    }
    for (const Dialog &dialog : dialogs) {
        if (dialog.answer.size() != 0) {
            m_interface.m_inGame.m_questionWidget->m_dialogs.push_back(dialog);
            m_interface.m_inGame.m_questionWidget->m_speakerId = speaker->m_id;
        } else {
            widget->m_dialogs.push_back(dialog);
        }
    }
    widget->m_anchorPosition = speaker->m_drawnPosition;
    widget->m_speakerId = speaker->m_id;
    if (listener) {
        widget->m_listenerId = listener->m_id;
    } else {
        widget->m_listenerId = 0;
    }
}

void Game::scrollMap(int x, int y)
{
    m_targetMapPosition = plasma::Vector<3, Fixed>(Fixed(x), Fixed(y), Fixed(m_world->height(x, y, false)));
    m_mapZoomChanged = true;
}

void Game::setControl(int control, bool pressed)
{
    if ((unsigned int)control <= Controls::ButtonCount - 1) {
        m_controlStates[control] = pressed;
    }
}

void Game::setStickMove(float x, float y)
{
    m_gamepadWalkVector = plasma::Vector<2, float>(x, y);
    if (m_gamepadWalkVector.squaredLength() < 0.2f * 0.2f) {
        m_gamepadWalkVector = plasma::Vector<2, float>(0.0f, 0.0f);
    }
}

void Game::setStickTurn(float x, float y)
{
    m_gamepadTurnVector = plasma::Vector<2, float>(x, y);
    if (m_gamepadTurnVector.squaredLength() < 0.2f * 0.2f) {
        m_gamepadTurnVector = plasma::Vector<2, float>(0.0f, 0.0f);
    }
}

int Game::soundId(int sound)
{
    std::string path;
    path = "sounds/";
    int id;
    switch (sound) {
    case SoundHit:
        id = m_audio->soundId(path + "hit.wav");
        break;
    case SoundBlade1:
        id = m_audio->soundId(path + "blade1.wav");
        break;
    case SoundBlade2:
        id = m_audio->soundId(path + "blade2.wav");
        break;
    case SoundHitArrow:
        id = m_audio->soundId(path + "hit-arrow.wav");
        break;
    case SoundHitArrowCritical:
        id = m_audio->soundId(path + "hit-arrow-critical.wav");
        break;
    case SoundLongBlade1:
        id = m_audio->soundId(path + "long-blade1.wav");
        break;
    case SoundLongBlade2:
        id = m_audio->soundId(path + "long-blade2.wav");
        break;
    case SoundSmash1:
        id = m_audio->soundId(path + "smash1.wav");
        break;
    case SoundHit1:
        id = m_audio->soundId(path + "hit1.wav");
        break;
    case SoundHit2:
        id = m_audio->soundId(path + "hit2.wav");
        break;
    case SoundPunch1:
        id = m_audio->soundId(path + "punch1.wav");
        break;
    case SoundPunch2:
        id = m_audio->soundId(path + "punch2.wav");
        break;
    case SoundPunch285:
        id = m_audio->soundId(path + "punch2.wav");
        break;
    case SoundSlamGround:
        id = m_audio->soundId(path + "slam-ground.wav");
        break;
    case SoundSmashHit2:
        id = m_audio->soundId(path + "smash-hit2.wav");
        break;
    case SoundSmashJump:
        id = m_audio->soundId(path + "smash-jump.wav");
        break;
    case SoundSwordHit03:
        id = m_audio->soundId(path + "sword-hit03.wav");
        break;
    case SoundBlock:
        id = m_audio->soundId(path + "block.wav");
        break;
    case SoundShieldSlam:
        id = m_audio->soundId(path + "shield-slam.wav");
        break;
    case SoundSwing:
        id = m_audio->soundId(path + "swing.wav");
        break;
    case SoundShieldSwing:
        id = m_audio->soundId(path + "shield-swing.wav");
        break;
    case SoundSwingSlow:
        id = m_audio->soundId(path + "swing-slow.wav");
        break;
    case SoundSwingSlow2:
        id = m_audio->soundId(path + "swing-slow2.wav");
        break;
    case SoundSwirl:
        id = m_audio->soundId(path + "swirl.wav");
        break;
    case SoundSalvo2:
        id = m_audio->soundId(path + "salvo2.wav");
        break;
    case SoundRoll:
        id = m_audio->soundId(path + "roll.wav");
        break;
    case SoundDestroy2:
        id = m_audio->soundId(path + "destroy2.wav");
        break;
    case SoundArrowDestroy:
        id = m_audio->soundId(path + "arrow-destroy.wav");
        break;
    case SoundBlade120:
        id = m_audio->soundId(path + "blade1.wav");
        break;
    case SoundPunch221:
        id = m_audio->soundId(path + "punch2.wav");
        break;
    case SoundCry:
        id = m_audio->soundId(path + "cry.wav");
        break;
    case SoundLevelUp:
        id = m_audio->soundId(path + "level-up.wav");
        break;
    case SoundSkillUp:
        id = m_audio->soundId(path + "skill-up.wav");
        break;
    case SoundQuestComplete:
        id = m_audio->soundId(path + "quest-complete.wav");
        break;
    case SoundQuestProgress:
        id = m_audio->soundId(path + "quest-progress.wav");
        break;
    case SoundNewQuest:
        id = m_audio->soundId(path + "new-quest.wav");
        break;
    case SoundWaterSplash01:
        id = m_audio->soundId(path + "water-splash01.wav");
        break;
    case SoundStep2:
        id = m_audio->soundId(path + "step2.wav");
        break;
    case SoundStepWater:
        id = m_audio->soundId(path + "step-water.wav");
        break;
    case SoundStepWater2:
        id = m_audio->soundId(path + "step-water2.wav");
        break;
    case SoundStepWater3:
        id = m_audio->soundId(path + "step-water3.wav");
        break;
    case SoundChannel2:
        id = m_audio->soundId(path + "channel2.wav");
        break;
    case SoundChannelHit:
        id = m_audio->soundId(path + "channel-hit.wav");
        break;
    case SoundFireball:
        id = m_audio->soundId(path + "fireball.wav");
        break;
    case SoundFireHit:
        id = m_audio->soundId(path + "fire-hit.wav");
        break;
    case SoundAbsorb:
        id = m_audio->soundId(path + "absorb.wav");
        break;
    case SoundManashield:
        id = m_audio->soundId(path + "manashield.wav");
        break;
    case SoundBulwark:
        id = m_audio->soundId(path + "bulwark.wav");
        break;
    case SoundMagic02:
        id = m_audio->soundId(path + "magic02.wav");
        break;
    case SoundWatersplash:
        id = m_audio->soundId(path + "watersplash.wav");
        break;
    case SoundWatersplashHit:
        id = m_audio->soundId(path + "watersplash-hit.wav");
        break;
    case SoundExplosion:
        id = m_audio->soundId(path + "Explosion.wav");
        break;
    case SoundLichScream:
        id = m_audio->soundId(path + "lich-scream.wav");
        break;
    case SoundDrink2:
        id = m_audio->soundId(path + "drink2.wav");
        break;
    case SoundPickup:
        id = m_audio->soundId(path + "pickup.wav");
        break;
    case SoundDisenchant2:
        id = m_audio->soundId(path + "disenchant2.wav");
        break;
    case SoundUpgrade2:
        id = m_audio->soundId(path + "upgrade2.wav");
        break;
    case SoundHumanVoice01:
        id = m_audio->soundId(path + "human-voice01.wav");
        break;
    case SoundHumanVoice02:
        id = m_audio->soundId(path + "human-voice02.wav");
        break;
    case SoundGate:
        id = m_audio->soundId(path + "gate.wav");
        break;
    case SoundLever:
        id = m_audio->soundId(path + "lever.wav");
        break;
    case SoundSpikeTrap:
        id = m_audio->soundId(path + "spike-trap.wav");
        break;
    case SoundFireTrap:
        id = m_audio->soundId(path + "fire-trap.wav");
        break;
    case SoundMagic0259:
        id = m_audio->soundId(path + "magic02.wav");
        break;
    case SoundCharge2:
        id = m_audio->soundId(path + "charge2.wav");
        break;
    case SoundDrop:
        id = m_audio->soundId(path + "drop.wav");
        break;
    case SoundDropCoin:
        id = m_audio->soundId(path + "drop-coin.wav");
        break;
    case SoundDropItem:
        id = m_audio->soundId(path + "drop-item.wav");
        break;
    case SoundMaleGroan:
        id = m_audio->soundId(path + "male-groan.wav");
        break;
    case SoundFemaleGroan:
        id = m_audio->soundId(path + "female-groan.wav");
        break;
    case SoundMaleGroan65:
        id = m_audio->soundId(path + "male-groan.wav");
        break;
    case SoundFemaleGroan66:
        id = m_audio->soundId(path + "female-groan.wav");
        break;
    case SoundGoblinMaleGroan:
        id = m_audio->soundId(path + "goblin-male-groan.wav");
        break;
    case SoundGoblinFemaleGroan:
        id = m_audio->soundId(path + "goblin-female-groan.wav");
        break;
    case SoundLizardMaleGroan:
        id = m_audio->soundId(path + "lizard-male-groan.wav");
        break;
    case SoundLizardFemaleGroan:
        id = m_audio->soundId(path + "lizard-female-groan.wav");
        break;
    case SoundDwarfMaleGroan:
        id = m_audio->soundId(path + "dwarf-male-groan.wav");
        break;
    case SoundDwarfFemaleGroan:
        id = m_audio->soundId(path + "dwarf-female-groan.wav");
        break;
    case SoundOrcMaleGroan:
        id = m_audio->soundId(path + "orc-male-groan.wav");
        break;
    case SoundOrcFemaleGroan:
        id = m_audio->soundId(path + "orc-female-groan.wav");
        break;
    case SoundUndeadMaleGroan:
        id = m_audio->soundId(path + "undead-male-groan.wav");
        break;
    case SoundUndeadFemaleGroan:
        id = m_audio->soundId(path + "undead-female-groan.wav");
        break;
    case SoundFrogmanMaleGroan:
        id = m_audio->soundId(path + "frogman-male-groan.wav");
        break;
    case SoundFrogmanFemaleGroan:
        id = m_audio->soundId(path + "frogman-female-groan.wav");
        break;
    case SoundMonsterGroan:
        id = m_audio->soundId(path + "monster-groan.wav");
        break;
    case SoundTrollGroan:
        id = m_audio->soundId(path + "troll-groan.wav");
        break;
    case SoundZombieGroan:
        id = m_audio->soundId(path + "zombie-groan.wav");
        break;
    case SoundSlimeGroan:
        id = m_audio->soundId(path + "slime-groan.wav");
        break;
    case SoundMoleGroan:
        id = m_audio->soundId(path + "mole-groan.wav");
        break;
    case SoundMenuOpen2:
        id = m_audio->soundId(path + "menu-open2.wav");
        break;
    case SoundMenuClose2:
        id = m_audio->soundId(path + "menu-close2.wav");
        break;
    case SoundMenuSelect:
        id = m_audio->soundId(path + "menu-select.wav");
        break;
    case SoundMenuTab:
        id = m_audio->soundId(path + "menu-tab.wav");
        break;
    case SoundMenuGrabItem:
        id = m_audio->soundId(path + "menu-grab-item.wav");
        break;
    case SoundMenuDropItem:
        id = m_audio->soundId(path + "menu-drop-item.wav");
        break;
    case SoundCraft:
        id = m_audio->soundId(path + "craft.wav");
        break;
    case SoundCraftProc:
        id = m_audio->soundId(path + "craft-proc.wav");
        break;
    case SoundBird1:
        id = m_audio->soundId(path + "bird1.wav");
        break;
    case SoundBird2:
        id = m_audio->soundId(path + "bird2.wav");
        break;
    case SoundBird3:
        id = m_audio->soundId(path + "bird3.wav");
        break;
    case SoundOwl1:
        id = m_audio->soundId(path + "owl1.wav");
        break;
    case SoundOwl2:
        id = m_audio->soundId(path + "owl2.wav");
        break;
    case SoundCricket1:
        id = m_audio->soundId(path + "cricket1.wav");
        break;
    case SoundCricket2:
        id = m_audio->soundId(path + "cricket2.wav");
        break;
    case SoundRiver1:
        id = m_audio->soundId(path + "river1.wav");
        break;
    case SoundRiver2:
        id = m_audio->soundId(path + "river2.wav");
        break;
    case SoundRiver3:
        id = m_audio->soundId(path + "river3.wav");
        break;
    case SoundHeroicShout:
        id = m_audio->soundId(path + "heroic-shout.wav");
        break;
    case SoundMachine:
        id = m_audio->soundId(path + "machine.wav");
        break;
    case SoundFlute1:
        id = m_audio->soundId(path + "flute1.wav");
        break;
    case SoundFlute2:
        id = m_audio->soundId(path + "flute2.wav");
        break;
    case SoundHarp1:
        id = m_audio->soundId(path + "harp1.wav");
        break;
    case SoundHarp2:
        id = m_audio->soundId(path + "harp2.wav");
        break;
    case SoundFiddle1:
        id = m_audio->soundId(path + "fiddle1.wav");
        break;
    case SoundWhistle1:
        id = m_audio->soundId(path + "whistle1.wav");
        break;
    case SoundWhistle2:
        id = m_audio->soundId(path + "whistle2.wav");
        break;
    case SoundBell1:
        id = m_audio->soundId(path + "bell1.wav");
        break;
    case SoundBell2:
        id = m_audio->soundId(path + "bell2.wav");
        break;
    case SoundLute1:
        id = m_audio->soundId(path + "lute1.wav");
        break;
    case SoundNightingale1:
        id = m_audio->soundId(path + "nightingale1.wav");
        break;
    case SoundWind:
        id = m_audio->soundId(path + "wind.wav");
        break;
    case SoundEagle:
        id = m_audio->soundId(path + "eagle.wav");
        break;
    case SoundSpeechEnd:
        id = m_audio->soundId(path + "speech-end.wav");
        break;
    case SoundSpeechNext:
        id = m_audio->soundId(path + "speech-next.wav");
        break;
    case SoundTamersWhistle:
        id = m_audio->soundId(path + "tamers-whistle.wav");
        break;
    default:
        id = -1;
        break;
    }
    return id;
}

// One frame updates input labels and timing, advances the world, processes received events, updates effects, camera,
// interface, location and music, then draws the frame.
void Game::update(double milliseconds)
{
    // Frame time capped at one second.
    double step = milliseconds > 1000.0 ? 1000.0 : milliseconds;
    m_interface.m_buttonCenters.clear();

    // The quick buttons' labels: the gamepad's X, A and Y, or the bound keys.
    if (m_gamepadActive) {
        m_interface.m_inGame.m_quickButtons[QuickSlotItem].button->setText(L"name", L"X", true);
        m_interface.m_inGame.m_quickButtons[QuickSlotItem].button->setTextColor(
            L"name", plasma::Vector<4, float>(0.4f, 0.5f, 0.9f, 1.0f), true);
        m_interface.m_inGame.m_quickButtons[QuickSlotUse].button->setText(L"name", L"A", true);
        m_interface.m_inGame.m_quickButtons[QuickSlotUse].button->setTextColor(
            L"name", plasma::Vector<4, float>(0.4f, 0.8f, 0.3f, 1.0f), true);
        m_interface.m_inGame.m_quickButtons[QuickSlotSkill].button->setText(L"name", L"Y", true);
        m_interface.m_inGame.m_quickButtons[QuickSlotSkill].button->setTextColor(
            L"name", plasma::Vector<4, float>(0.9f, 0.9f, 0.4f, 1.0f), true);
    } else {
        m_interface.m_inGame.m_quickButtons[QuickSlotItem].button->setText(
            L"name", m_keyNames.getShortName(m_controls.buttons[Controls::ButtonQuickItem]), true);
        m_interface.m_inGame.m_quickButtons[QuickSlotUse].button->setText(
            L"name", m_keyNames.getShortName(m_controls.buttons[Controls::ButtonUse]), true);
        m_interface.m_inGame.m_quickButtons[QuickSlotSkill].button->setText(
            L"name", m_keyNames.getShortName(m_controls.buttons[Controls::ButtonUltimateSkill]), true);
        m_interface.m_inGame.m_quickButtons[QuickSlotItem].button->setTextColor(
            L"name", plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f), true);
        m_interface.m_inGame.m_quickButtons[QuickSlotUse].button->setTextColor(
            L"name", plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f), true);
        m_interface.m_inGame.m_quickButtons[QuickSlotSkill].button->setTextColor(
            L"name", plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f), true);
    }

    // The menu buttons' labels: the bound keys, and F1 for help.
    m_interface.m_inGame.m_menuButtons[InGameMenuCrafting]->setText(
        m_keyNames.getShortName(m_controls.buttons[Controls::ButtonCraftingMenu]));
    m_interface.m_inGame.m_menuButtons[InGameMenuInventory]->setText(
        m_keyNames.getShortName(m_controls.buttons[Controls::ButtonInventory]));
    m_interface.m_inGame.m_menuButtons[InGameMenuWorldMap]->setText(
        m_keyNames.getShortName(m_controls.buttons[Controls::ButtonWorldMap]));
    m_interface.m_inGame.m_menuButtons[InGameMenuSystem]->setText(
        m_keyNames.getShortName(m_controls.buttons[Controls::ButtonSystemMenu]));
    m_interface.m_inGame.m_menuButtons[InGameMenuMultiplayer]->setText(
        m_keyNames.getShortName(m_controls.buttons[Controls::ButtonMultiplayerMenu]));
    m_interface.m_inGame.m_menuButtons[InGameMenuHelp]->setText(L"F1");

    if (!m_engine->isButtonDown(0) && !m_engine->isButtonDown(1)) {
        m_pointerMoved = false;
    }

    // The frame's clock (every stage's time goes into the profile at the end), the zone meshes held for the frame, the
    // player's Steam id as its seed, and the updates the frame collects for the host.
    unsigned long start = timeGetTime();
    m_world->enterZoneMeshes();
    if (m_world->m_localCreature->m_entityData.seed == 0) {
        m_world->m_localCreature->m_entityData.seed = SteamUser()->GetSteamID().ConvertToUint64();
    }
    ServerUpdates updates;

    // The menu view (ability 157) while the start menu or the character selection is shown: the camera circles above
    // the character.
    if (!m_interface.m_startMenuNode->isVisible() && !m_interface.m_characterSelection.m_node->isVisible()) {
        if (m_world->m_localCreature->m_entityData.currentAbility == AbilityMenuView) {
            m_world->m_localCreature->m_entityData.currentAbility = AbilityNone;
        }
    } else {
        m_world->m_localCreature->m_entityData.currentAbility = AbilityMenuView;
        m_world->m_localCreature->m_entityData.destination[0] = (int)m_world->m_localCreature->m_entityData.position[0];
        m_world->m_localCreature->m_entityData.destination[1] = (int)m_world->m_localCreature->m_entityData.position[1];
        m_world->m_localCreature->m_entityData.origin = m_world->m_localCreature->m_entityData.position;
        m_targetCameraAngle[2] += step * 0.003f;
        m_targetCameraAngle[0] = 110.0;
        m_targetCameraDistance = 12.0f;
    }
    // In the menus and while flying (ability 156) the lamp follows the time of day: lit before 5:00 and from 19:00.
    if (m_interface.m_startMenuNode->isVisible() || m_interface.m_characterSelection.m_node->isVisible() ||
        m_interface.m_characterCreationNode->isVisible() ||
        m_world->m_localCreature->m_entityData.currentAbility == AbilityEagleFlight) {
        m_world->m_localCreature->m_entityData.setToggles(1 << CreatureToggleLampOn,
                                                          m_world->m_state.m_time < 5 * MillisecondsPerHour ||
                                                              m_world->m_state.m_time >= 19 * MillisecondsPerHour);
    }
    unsigned long afterSetup = timeGetTime();
    if (afterSetup - start > 1) {
        static unsigned long lastReport = afterSetup;
        std::cout << "lag: " << afterSetup - start << " (" << afterSetup - lastReport << ")" << std::endl;
        lastReport = afterSetup;
    }

    // What the frame looks at: no creature, no block, no item.
    m_crosshairCreatureId = 0;
    m_promptCreatureId = 0;
    m_crosshairObject = plasma::Vector<3, int>(0x7fffffff, 0x7fffffff, 0);
    m_crosshairItem = plasma::Vector<3, int>(0x7fffffff, 0x7fffffff, 0);
    m_creatureInView = false;

    // The special-skill quick button: no count, the skill's name, and the seconds of cooldown left while there are
    // any.
    m_interface.m_inGame.m_quickButtons[QuickSlotSkill].button->setText(L"count", L"", true);
    int skill = m_world->m_localCreature->specialSkill();
    if (skill) {
        m_interface.m_inGame.m_quickButtons[QuickSlotSkill].button->setText(
            L"info", m_speech.getText(m_speech.skillKey(skill), L"default", L""), true);
        if (m_world->m_localCreature->m_skillCooldowns[skill] != 0.0f) {
            m_interface.m_inGame.m_quickButtons[QuickSlotSkill].button->setText(
                L"info",
                std::to_string((int)(m_world->m_localCreature->m_skillCooldowns[skill] + 999.0f) / 1000) + " s", true);
        }
    } else {
        m_interface.m_inGame.m_quickButtons[QuickSlotSkill].button->setText(L"info", L"", true);
    }
    m_interface.m_inGame.m_quickButtons[QuickSlotUnused].button->setText(L"info", L"", true);

    // The item quick buttons start the frame empty.
    Item noItem;
    m_interface.m_inGame.m_quickItemSprites[QuickSlotItem]->m_item = noItem;
    m_interface.m_inGame.m_quickButtons[QuickSlotUse].button->setText(L"count", L"", true);
    Item noItem2;
    m_interface.m_inGame.m_quickItemSprites[QuickSlotUse]->m_item = noItem2;
    m_interface.m_inGame.m_quickButtons[QuickSlotUse].button->setText(L"info", L"", true);
    m_interface.m_inGame.m_quickButtons[QuickSlotUse].button->setTextColor(
        L"info", plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f), true);
    if (m_world->m_localCreature->m_entityData.hp <= 0.0f) {
        m_world->m_localCreature->m_entityData.currentAbility = AbilityNone;
    }

    // The map zoom keys, on their press; the zoom stays within the map's levels.
    static bool zoomInWasDown;
    static bool zoomOutWasDown;
    if (m_controlStates[Controls::ButtonZoomIn] && !zoomInWasDown) {
        m_mapZoom--;
        if (m_mapZoom < 0) {
            m_mapZoom = 0;
        }
        if (m_mapZoom >= (int)m_worldMap->m_levels.size()) {
            m_mapZoom = (int)m_worldMap->m_levels.size() - 1;
        }
    }
    zoomInWasDown = m_controlStates[Controls::ButtonZoomIn];
    if (m_controlStates[Controls::ButtonZoomOut] && !zoomOutWasDown) {
        m_mapZoom++;
        if (m_mapZoom < 0) {
            m_mapZoom = 0;
        }
        if (m_mapZoom >= (int)m_worldMap->m_levels.size()) {
            m_mapZoom = (int)m_worldMap->m_levels.size() - 1;
        }
    }
    zoomOutWasDown = m_controlStates[Controls::ButtonZoomOut];
    if (!m_interface.m_mapOpen) {
        m_mapZoomChanged = false;
        m_worldMap->m_flightmasterMode = false;
    }

    // The frame's starting gold, platinum, region and maximum HP, compared after the world step.
    int gold = m_world->m_localCreature->m_inventory.m_gold;
    int platinum = m_world->m_localCreature->platinum();
    plasma::Vector<2, int> region = m_world->m_localCreature->m_entityData.region;
    float maxHP = m_world->m_localCreature->maxHP();
    m_world->m_zonesWithChangedItems.clear();
    m_world->m_zonesWithChangedObjects.clear();
    m_audio->cleanUp();

    // Flying (ability 156): the wing beat twice a second, from the first half second on.
    Creature *player = m_world->m_localCreature;
    if (player->m_entityData.currentAbility == AbilityEagleFlight && player->m_entityData.timeSinceAbility > 500.0f) {
        if ((int)player->m_entityData.timeSinceAbility / 500 !=
            (int)(player->m_entityData.timeSinceAbility + step) / 500) {
            playSound(SoundWind, player->m_entityData.position, (float)rand() * 0.5f / (float)RAND_MAX + 0.5f,
                      (float)rand() * 2.0f / (float)RAND_MAX + 1.0f, true);
        }
    }
    unsigned int afterSounds = timeGetTime();

    // Finds the character's place and whether its mission was already completed. A place uses a 1024-block cell, or
    // two cells when large. The equipment copy and buff lookup are unused.
    Equipment equipment = m_world->m_localCreature->m_entityData.equipment;
    player = m_world->m_localCreature;
    plasma::Vector<2, int> cell(floorDivide((int)player->m_entityData.position[0], BlocksPerPlace),
                                floorDivide((int)player->m_entityData.position[1], BlocksPerPlace));
    if (m_world->isLargePlace(cell[0], cell[1])) {
        cell[0] = floorDivide(cell[0], 2) * 2;
        cell[1] = floorDivide(cell[1], 2) * 2;
    }
    bool completed = player->hasCompletedMission(cell[0], cell[1]);
    m_world->m_localCreature->findBuff(BuffManaabsorption);
    m_world->m_localCreature->m_entityData.setToggles(1 << CreatureToggleDodge,
                                                      m_interface.m_startMenuNode->isVisible() ||
                                                          m_interface.m_characterSelection.m_node->isVisible() ||
                                                          m_interface.m_characterCreationNode->isVisible());

    // The world's step, in slices of at most 16 ms, while the world is the one being played; otherwise only the map
    // overlay is hidden.
    if (m_seed == m_world->m_seed && m_worldName == m_world->m_worldName) {
        double remaining = step;
        if (step > MaxWorldStepMs) {
            do {
                m_world->updateStep(MaxWorldStepMsFloat);
                remaining -= MaxWorldStepMs;
            } while (remaining > MaxWorldStepMs);
        }
        m_world->updateStep((float)remaining);
    } else {
        m_interface.m_inGame.m_mapOverlayWidget->getNode()->setVisible(false);
    }

    // A changed maximum HP (new equipment, a level) keeps the HP's share.
    if (maxHP != m_world->m_localCreature->maxHP()) {
        std::cout << maxHP << "  -> " << m_world->m_localCreature->maxHP() << std::endl;
        player = m_world->m_localCreature;
        player->m_entityData.hp = player->maxHP() / maxHP * player->m_entityData.hp;
        m_world->m_localCreature->m_entityData.hp =
            (std::min)(m_world->m_localCreature->maxHP(), m_world->m_localCreature->m_entityData.hp);
    }

    // A mission completed this frame: the place's story goes to the speech widget, by the kind of the place.
    if (!completed && m_world->m_localCreature->hasCompletedMission(cell[0], cell[1])) {
        m_successMusicTime = 60000;
        switch (Missions(m_world, m_world->m_localCreature->m_entityData.region).kindAt(cell[0], cell[1])) {
        case MissionKindSupplierGnome: {
            std::vector<std::string> script;
            script << "new_page";
            player = m_world->m_localCreature;
            script << "define_land" << m_world->landBiome(player->m_entityData.region)
                   << (int)fastRandom(player->m_entityData.region[0], player->m_entityData.region[1]);
            script << "description_supplier_gnome";
            m_speech.generateFromScript(script, rand() % 10000, m_interface.m_inGame.m_speechWidget->m_dialogs, L"");
            break;
        }
        case MissionKindMagicBarriers:
            if (m_world->m_localCreature->m_entityData.binaryToggles.magicBarriersDisabled) {
                std::vector<std::string> script;
                script << "new_page";
                player = m_world->m_localCreature;
                script << "define_land" << m_world->landBiome(player->m_entityData.region)
                       << (int)fastRandom(player->m_entityData.region[0], player->m_entityData.region[1]);
                script << "description_magic_barriers";
                m_speech.generateFromScript(script, rand() % 10000, m_interface.m_inGame.m_speechWidget->m_dialogs,
                                            L"");
            }
            break;
        case MissionKindFlameOfPower: {
            std::vector<std::string> script;
            script << "new_page";
            player = m_world->m_localCreature;
            script << "define_land" << m_world->landBiome(player->m_entityData.region)
                   << (int)fastRandom(player->m_entityData.region[0], player->m_entityData.region[1]);
            script << "description_flame_of_power";
            m_speech.generateFromScript(script, rand() % 10000, m_interface.m_inGame.m_speechWidget->m_dialogs, L"");
            break;
        }
        case MissionKindDemonPortal: {
            std::vector<std::string> script;
            script << "new_page";
            player = m_world->m_localCreature;
            script << "define_land" << m_world->landBiome(player->m_entityData.region)
                   << (int)fastRandom(player->m_entityData.region[0], player->m_entityData.region[1]);
            script << "description_demon_portal";
            m_speech.generateFromScript(script, rand() % 10000, m_interface.m_inGame.m_speechWidget->m_dialogs, L"");
            break;
        }
        case MissionKindManaPumps: {
            std::vector<std::string> script;
            script << "new_page";
            player = m_world->m_localCreature;
            script << "define_land" << m_world->landBiome(player->m_entityData.region)
                   << (int)fastRandom(player->m_entityData.region[0], player->m_entityData.region[1]);
            if (m_world->m_localCreature->findBuff(BuffManaabsorption)) {
                script << "description_mana_pumps";
            } else {
                script << "description_mana_pump";
            }
            m_speech.generateFromScript(script, rand() % 10000, m_interface.m_inGame.m_speechWidget->m_dialogs, L"");
            break;
        }
        case MissionKindWitchCurse: {
            Missions missions(m_world, m_world->m_localCreature->m_entityData.region);
            std::vector<std::string> script;
            script << "new_page";
            player = m_world->m_localCreature;
            script << "define_land" << m_world->landBiome(player->m_entityData.region)
                   << (int)fastRandom(player->m_entityData.region[0], player->m_entityData.region[1]);
            plasma::Vector<2, int> place = missions.m_village;
            script << "define_location" << place[0] << place[1] << m_world->locationType(place);
            script << "description_witch";
            m_speech.generateFromScript(script, rand() % 10000, m_interface.m_inGame.m_speechWidget->m_dialogs, L"");
            break;
        }
        }
    }

    // The creatures' AI, the map position (following the character unless the map was moved, then at the terrain's
    // height) and the clock after the world.
    m_world->inspectCreatures((float)step);
    m_world->emptyUpdateStep();
    if (!m_mapZoomChanged) {
        m_targetMapPosition = m_world->m_localCreature->m_entityData.position;
        m_mapPosition = m_targetMapPosition;
    } else {
        m_targetMapPosition[2] = Fixed(m_world->height((int)m_targetMapPosition[0], (int)m_targetMapPosition[1], 0));
    }
    blend(m_mapPosition, m_targetMapPosition, step, 0.01f);
    unsigned int afterMap = timeGetTime();

    // Connected: the hits, buffs and bullets of the player go to the host.
    if (m_socket) {
        for (auto &hit : m_world->m_serverUpdates.m_hitPacketList) {
            if (hit.attackerId == m_world->m_localCreature->m_id) {
                m_hitPackets.push_back(hit);
            }
        }
        for (auto &buff : m_world->m_serverUpdates.m_buffPacketList) {
            if (buff.sourceId == m_world->m_localCreature->m_id) {
                m_buffPackets.push_back(buff);
            }
        }
        for (auto &bullet : m_world->m_serverUpdates.m_bulletPacketList) {
            if (bullet.ownerId == m_world->m_localCreature->m_id) {
                m_bulletPackets.push_back(bullet);
            }
        }
    }

    // Applies received block changes and records them in their zones. Computes the affected extent without using it.
    if (m_serverUpdates.m_cubeChangeList.size()) {
        std::cout << "CUBE CHANGES 1" << std::endl;
        int minX = m_serverUpdates.m_cubeChangeList.front().position[0];
        int minY = m_serverUpdates.m_cubeChangeList.front().position[1];
        int maxX = minX;
        int maxY = minY;
        std::cout << "CUBE CHANGES 2" << std::endl;
        for (std::list<CubeChange>::iterator it = m_serverUpdates.m_cubeChangeList.begin();
             it != m_serverUpdates.m_cubeChangeList.end(); ++it) {
            if (it->newBlock.type == BlockAir && it->newBlock.flags == 0) {
                m_world->destroyBlock(it->position[0], it->position[1], it->position[2]);
            } else {
                m_world->setBlock(it->position[0], it->position[1], it->position[2], it->newBlock);
            }
            if (it->position[0] < minX) {
                minX = it->position[0];
            }
            if (it->position[0] > maxX) {
                maxX = it->position[0];
            }
            if (it->position[1] < minY) {
                minY = it->position[1];
            }
            if (it->position[1] > maxY) {
                maxY = it->position[1];
            }
            Zone *zone =
                m_world->zone(floorDivide(it->position[0], BlocksPerZone), floorDivide(it->position[1], BlocksPerZone));
            if (zone) {
                zone->m_cubeChanges.push_back(*it);
            }
        }
        std::cout << "CUBE CHANGES 3" << std::endl;
        std::cout << "CUBE CHANGES 4" << std::endl;
    }
    // Zones the host sends anew, their items and their static entities.
    if (m_serverUpdates.m_deliveredZoneList.size()) {
        for (std::list<plasma::Vector<2, int>>::iterator it = m_serverUpdates.m_deliveredZoneList.begin();
             it != m_serverUpdates.m_deliveredZoneList.end(); ++it) {
            Zone *zone = m_world->zone((*it)[0], (*it)[1]);
            if (zone) {
                zone->m_awaitingData = false;
            }
        }
    }
    if (m_serverUpdates.m_itemListPacketList.size()) {
        for (std::list<ServerUpdates::ItemListPacket>::iterator it = m_serverUpdates.m_itemListPacketList.begin();
             it != m_serverUpdates.m_itemListPacketList.end(); it++) {
            Zone *zone = m_world->zone(it->zone[0], it->zone[1]);
            if (zone) {
                zone->m_items.clear();
                zone->m_items.insert(zone->m_items.begin(), it->items.begin(), it->items.end());
            }
        }
    }
    if (m_serverUpdates.m_staticEntityPacketList.size()) {
        for (std::list<ServerUpdates::Packet6>::iterator it = m_serverUpdates.m_staticEntityPacketList.begin();
             it != m_serverUpdates.m_staticEntityPacketList.end(); it++) {
            Zone *zone = m_world->zone(it->zone[0], it->zone[1]);
            if (zone) {
                zone->m_staticEntities.clear();
                zone->m_staticEntities.insert(zone->m_staticEntities.begin(), it->objects.begin(), it->objects.end());
                m_world->m_zonesWithChangedObjects.insert(zone);
                zone->updateLists();
                zone->updateOccupancy();
            }
        }
    }
    // Buffs and aggro received for other creatures, the zones, the kills.
    for (auto &buff : m_serverUpdates.m_buffPacketList) {
        if (buff.sourceId != m_world->m_localCreature->m_id || buff.targetId != m_world->m_localCreature->m_id) {
            Creature *target = m_world->creatureByID(buff.targetId);
            if (target) {
                target->setBuff(buff.buff, buff.replace);
            }
        }
    }
    for (auto &aggro : m_serverUpdates.m_aggroPacketList) {
        Creature *creature = m_world->creatureByID(aggro.creatureId);
        if (creature) {
            creature->m_aggro[aggro.attackerId] = aggro.aggro;
        }
    }
    m_world->updateZones();
    for (auto &kill : m_serverUpdates.m_killPacketList) {
        m_world->m_state.m_deadCreatures.insert(kill.targetId);
    }
    unsigned int afterPackets = timeGetTime();

    // The chat: the last received line, with its speaker's name, then the host's messages.
    if (m_chatMessages.size()) {
        if (m_chatMessages.back().creatureId >= 0 && m_world->creatureByID(m_chatMessages.back().creatureId)) {
            m_interface.m_inGame.m_chatWidget->printMessage(
                WidenString(std::string(m_world->creatureByID(m_chatMessages.back().creatureId)->m_entityData.name)) +
                    L": ",
                plasma::Vector<3, unsigned char>(0, 255, 255));
        }
        m_interface.m_inGame.m_chatWidget->printMessage(m_chatMessages.back().text + L"\n",
                                                        plasma::Vector<3, unsigned char>(255, 255, 255));
        m_chatMessages.pop_back();
    }
    for (auto &message : m_world->m_serverUpdates.m_systemMessageList) {
        m_interface.m_inGame.m_chatWidget->printMessage(message + L"\n",
                                                        plasma::Vector<3, unsigned char>(255, 200, 200));
    }

    // What the host sent this frame joins the world's own updates.
    m_world->m_serverUpdates.m_soundPacketList.insert(m_world->m_serverUpdates.m_soundPacketList.end(),
                                                      m_serverUpdates.m_soundPacketList.begin(),
                                                      m_serverUpdates.m_soundPacketList.end());
    m_world->m_serverUpdates.m_particlePacketList.insert(m_world->m_serverUpdates.m_particlePacketList.end(),
                                                         m_serverUpdates.m_particlePacketList.begin(),
                                                         m_serverUpdates.m_particlePacketList.end());
    m_world->m_serverUpdates.m_cubeChangeList.insert(m_world->m_serverUpdates.m_cubeChangeList.end(),
                                                     m_serverUpdates.m_cubeChangeList.begin(),
                                                     m_serverUpdates.m_cubeChangeList.end());
    m_world->m_serverUpdates.m_itemPickupPacketList.insert(m_world->m_serverUpdates.m_itemPickupPacketList.end(),
                                                           m_serverUpdates.m_itemPickupPacketList.begin(),
                                                           m_serverUpdates.m_itemPickupPacketList.end());
    m_world->m_serverUpdates.m_killPacketList.insert(m_world->m_serverUpdates.m_killPacketList.end(),
                                                     m_serverUpdates.m_killPacketList.begin(),
                                                     m_serverUpdates.m_killPacketList.end());
    m_world->m_serverUpdates.m_passivePacketList.insert(m_world->m_serverUpdates.m_passivePacketList.end(),
                                                        m_serverUpdates.m_passivePacketList.begin(),
                                                        m_serverUpdates.m_passivePacketList.end());
    m_world->m_serverUpdates.m_talkingCreatureList.insert(m_world->m_serverUpdates.m_talkingCreatureList.end(),
                                                          m_serverUpdates.m_talkingCreatureList.begin(),
                                                          m_serverUpdates.m_talkingCreatureList.end());
    m_world->m_serverUpdates.m_worldEventList.insert(m_world->m_serverUpdates.m_worldEventList.end(),
                                                     m_serverUpdates.m_worldEventList.begin(),
                                                     m_serverUpdates.m_worldEventList.end());
    static int countdown;
    countdown = (int)(countdown - step);
    if (countdown < 0) {
        countdown = 0;
    }

    // A feat of the player: a floating "FEAT!", a chat line and sound 31.
    for (auto &passive : m_world->m_serverUpdates.m_passivePacketList) {
        if (passive.creatureId == m_world->m_localCreature->m_id) {
            TextFX fx;
            fx.color = plasma::Vector<4, float>(0.7f, 0.2f, 1.0f, 1.0f);
            fx.position = m_world->m_localCreature->m_entityData.position;
            fx.age = 0;
            fx.duration = 3000;
            fx.offsetX = 5.0f;
            fx.offsetY = -100.0f;
            fx.pixelSize = 30.0f;
            fx.rise = -100.0f;
            fx.text = getFeatText(passive.unused08);
            m_textFx.push_back(fx);
            m_interface.m_inGame.m_chatWidget->printMessage(fx.text + L"\n",
                                                            plasma::Vector<3, unsigned char>(200, 100, 255));
            playSound(SoundQuestComplete);
        }
    }
    // Processes hit packets. Blocked hits push the player and consume block power while charging mana and block
    // charge; type 5 drains type-6 buffs. Other hits reduce stealth and HP and apply stun, playing sound 27 on death.
    // Other creatures only flash. Collects killed creatures for death effects.
    std::set<Creature *> hitCreatures;
    for (auto &hit : m_serverUpdates.m_hitPacketList) {
        Creature *target = m_world->creatureByID(hit.targetId);
        if (!target) {
            continue;
        }
        if (m_world->creatureByID(hit.targetId)->m_entityData.hp <= 0.0f &&
            m_world->creatureByID(hit.targetId)->m_entityData.race != RaceWaterBubble) {
            hitCreatures.insert(m_world->creatureByID(hit.targetId));
        }
        if (target == m_world->m_localCreature) {
            if (hit.hitType == HitTypeHeal && hit.damage > 0.0f) {
                m_world->m_localCreature->m_entityData.velocity = hit.direction;
                m_world->m_localCreature->m_hitFlash = 1.0f;
            }
            if (hit.hitType == HitTypeHeal) {
                m_world->m_localCreature->m_entityData.velocity = hit.direction;
                float cost = hit.damage / m_world->m_localCreature->blockPower();
                player = m_world->m_localCreature;
                if (player->m_entityData.currentAbility == AbilityShellSpin ||
                    player->m_entityData.currentAbility == AbilityShellBlock ||
                    player->m_entityData.currentAbility == AbilityRestHeal) {
                    cost = 0.0f;
                }
                if (player->m_entityData.currentAbility == AbilityCyclone) {
                    cost *= 0.5f;
                }
                std::cout << "mana cost " << cost << std::endl;
                float share = (std::max)(0.0f, (1.0f - cost) * 0.5f);
                float gain = m_world->m_localCreature->manaGeneration() * share;
                if (m_world->m_localCreature->isBlocking()) {
                    player = m_world->m_localCreature;
                    player->m_entityData.mp += gain;
                    if (player->m_entityData.currentAbility != AbilityCyclone) {
                        player->m_entityData.charge += gain;
                    }
                }
                player = m_world->m_localCreature;
                if (player->m_entityData.mp > 1.0f) {
                    player->m_entityData.mp = 1.0f;
                }
                player = m_world->m_localCreature;
                if (player->m_entityData.charge > player->m_entityData.mp) {
                    player->m_entityData.charge = player->m_entityData.mp;
                }
                player = m_world->m_localCreature;
                player->m_entityData.blockPower -= cost;
                if (player->m_entityData.blockPower < -1.0f) {
                    player->m_entityData.blockPower = -1.0f;
                }
            } else if (hit.hitType == HitTypeMana) {
                for (std::list<Creature::Buff>::iterator buff = m_world->m_localCreature->m_buffs.begin();
                     buff != m_world->m_localCreature->m_buffs.end(); buff++) {
                    if (buff->type == BuffManashield) {
                        buff->strength -= hit.damage;
                        if (buff->strength <= 0.0f) {
                            buff->strength = 0.0f;
                            buff->timeRemaining = 0.0f;
                        }
                        ServerUpdates::BuffPacket packet;
                        packet.sourceId = m_world->m_localCreature->m_id;
                        packet.targetId = m_world->m_localCreature->m_id;
                        packet.buff = *buff;
                        m_world->m_serverUpdates.m_buffPacketList.push_back(packet);
                    }
                }
            } else if (m_world->m_localCreature->m_entityData.hp > 0.0f && hit.hitType != HitTypeTameness) {
                player = m_world->m_localCreature;
                if (hit.damage > 0.0f) {
                    player->m_entityData.stealth -= 0.25f;
                    if (player->m_entityData.stealth < 0.0f) {
                        player->m_entityData.stealth = 0.0f;
                    }
                    target->m_hitFlash = 1.0f;
                }
                if (hit.stunTime > 0) {
                    player = m_world->m_localCreature;
                    player->m_entityData.stunTime = (std::max)(player->m_entityData.stunTime, (float)hit.stunTime);
                }
                player = m_world->m_localCreature;
                player->m_entityData.hp -= hit.damage;
                if (player->m_entityData.hp < 0.0f) {
                    player->m_entityData.hp = 0.0f;
                    ServerUpdates::SoundPacket sound;
                    sound.position = m_world->m_localCreature->m_entityData.position;
                    sound.speed = 1.0f;
                    sound.sound = SoundDestroy2;
                    sound.sourceId = m_world->m_localCreature->m_id;
                    m_world->m_serverUpdates.m_soundPacketList.push_back(sound);
                    hitCreatures.insert(m_world->getLocalCreature());
                }
                if (m_world->m_localCreature->m_entityData.hp > m_world->m_localCreature->maxHP()) {
                    player = m_world->m_localCreature;
                    player->m_entityData.hp = player->maxHP();
                }
            }
        } else if (hit.damage > 0.0f && hit.attackerId != m_world->m_localCreature->m_id) {
            target->m_hitFlash = 1.0f;
        }
    }

    // The death effects of the creatures killed that were drawn and are in view: their model parts burst into
    // particles, and a race-144 creature leaves a light.
    for (Creature *creature : hitCreatures) {
        if (!creature->m_animationState.m_spritesInitialized) {
            continue;
        }
        if (!m_renderer->isSphereVisible(creature->m_drawnPosition,
                                         (std::max)((std::max)(creature->m_entityData.appearance.size[0],
                                                               creature->m_entityData.appearance.size[1]),
                                                    creature->m_entityData.appearance.size[2]) *
                                             0.5f,
                                         100.0f)) {
            continue;
        }
        createParticles(creature->m_animationState.m_headSprite, creature->m_animationState.m_headMatrix,
                        plasma::Vector<3, float>(), creature->m_entityData.velocity, 1.0f,
                        Fixed(creature->m_animationState.m_originX).toFloat(),
                        Fixed(creature->m_animationState.m_originY).toFloat(),
                        plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f), 0);
        if (!(creature->m_entityData.appearance.appearanceFlags & AppearanceFlagSuppressHairDebris)) {
            createParticles(creature->m_animationState.m_hairSprite, creature->m_animationState.m_headMatrix,
                            plasma::Vector<3, float>(), creature->m_entityData.velocity, 1.0f,
                            Fixed(creature->m_animationState.m_originX).toFloat(),
                            Fixed(creature->m_animationState.m_originY).toFloat(),
                            plasma::Vector<4, float>((float)creature->m_entityData.appearance.hairColor[0] / 255.0f,
                                                     (float)creature->m_entityData.appearance.hairColor[1] / 255.0f,
                                                     (float)creature->m_entityData.appearance.hairColor[2] / 255.0f,
                                                     1.0f),
                            0);
        }
        createParticles(creature->m_animationState.m_chestSprite, creature->m_animationState.m_chestMatrix,
                        plasma::Vector<3, float>(), creature->m_entityData.velocity, 1.0f,
                        Fixed(creature->m_animationState.m_originX).toFloat(),
                        Fixed(creature->m_animationState.m_originY).toFloat(),
                        plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f), 0);
        createParticles(creature->m_animationState.m_handsSprite, creature->m_animationState.m_leftHandMatrix,
                        plasma::Vector<3, float>(), creature->m_entityData.velocity, 1.0f,
                        Fixed(creature->m_animationState.m_originX).toFloat(),
                        Fixed(creature->m_animationState.m_originY).toFloat(),
                        plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f), 0);
        createParticles(creature->m_animationState.m_handsSprite, creature->m_animationState.m_rightHandMatrix,
                        plasma::Vector<3, float>(), creature->m_entityData.velocity, 1.0f,
                        Fixed(creature->m_animationState.m_originX).toFloat(),
                        Fixed(creature->m_animationState.m_originY).toFloat(),
                        plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f), 0);
        createParticles(creature->m_animationState.m_shouldersSprite, creature->m_animationState.m_leftShoulderMatrix,
                        plasma::Vector<3, float>(), creature->m_entityData.velocity, 1.0f,
                        Fixed(creature->m_animationState.m_originX).toFloat(),
                        Fixed(creature->m_animationState.m_originY).toFloat(),
                        plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f), 0);
        createParticles(creature->m_animationState.m_shouldersSprite, creature->m_animationState.m_rightShoulderMatrix,
                        plasma::Vector<3, float>(), creature->m_entityData.velocity, 1.0f,
                        Fixed(creature->m_animationState.m_originX).toFloat(),
                        Fixed(creature->m_animationState.m_originY).toFloat(),
                        plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f), 0);
        createParticles(creature->m_animationState.m_feetSprite, creature->m_animationState.m_leftFootMatrix,
                        plasma::Vector<3, float>(), creature->m_entityData.velocity, 1.0f,
                        Fixed(creature->m_animationState.m_originX).toFloat(),
                        Fixed(creature->m_animationState.m_originY).toFloat(),
                        plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f), 0);
        createParticles(creature->m_animationState.m_feetSprite, creature->m_animationState.m_rightFootMatrix,
                        plasma::Vector<3, float>(), creature->m_entityData.velocity, 1.0f,
                        Fixed(creature->m_animationState.m_originX).toFloat(),
                        Fixed(creature->m_animationState.m_originY).toFloat(),
                        plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f), 0);
        createParticles(creature->m_animationState.m_rightWeaponSprite, creature->m_animationState.m_rightWeaponMatrix,
                        plasma::Vector<3, float>(), creature->m_entityData.velocity, 1.0f,
                        Fixed(creature->m_animationState.m_originX).toFloat(),
                        Fixed(creature->m_animationState.m_originY).toFloat(),
                        plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f), 0);
        if (creature->m_entityData.race == RaceBomb) {
            Light light;
            light.position = creature->m_drawnPosition;
            light.radius = 16.0f;
            light.color = plasma::Vector<3, float>(1.0f, 0.8f, 0.5f);
            light.age = 0;
            light.duration = 800;
            m_lights.push_back(light);
        }
    }

    // The host's bullets and the hits of other creatures join the world's; the zones' pending updates too, when there
    // is a collector.
    for (std::list<ServerUpdates::BulletPacket>::iterator it = m_serverUpdates.m_bulletPacketList.begin();
         it != m_serverUpdates.m_bulletPacketList.end(); it++) {
        if (it->ownerId != m_world->m_localCreature->m_id) {
            m_world->m_projectiles.push_back(*it);
        }
    }
    for (std::list<ServerUpdates::HitPacket>::iterator it = m_serverUpdates.m_hitPacketList.begin();
         it != m_serverUpdates.m_hitPacketList.end(); it++) {
        if (it->attackerId != m_world->m_localCreature->m_id) {
            m_world->m_serverUpdates.m_hitPacketList.push_back(*it);
        }
    }
    m_world->beforeServerUpdates();
    if (m_server) {
        m_server->addServerUpdates(m_world->m_serverUpdates);
    }

    // The creatures the host makes talk: their script becomes a dialog seeded by their id, then the interaction with
    // them.
    for (auto &creatureId : m_world->m_serverUpdates.m_talkingCreatureList) {
        Creature *creature = m_world->creatureByID(creatureId);
        if (creature) {
            std::vector<std::string> script = m_world->talkScript(creature);
            if (!script.empty()) {
                std::vector<Dialog> dialogs;
                m_speech.generateFromScript(script, (int)((unsigned long long)creature->m_id % 100000), dialogs, L"");
                say(creature, nullptr, dialogs);
            }
            interact(creature);
        }
    }
    m_serverUpdates = ServerUpdates {};

    // The living creatures within 160 blocks that the renderer can see, nearest first.
    std::vector<NearbyCreature> nearby;
    for (Creature *creature : m_world->m_creatures) {
        if (creature->m_entityData.hp <= 0.0f) {
            continue;
        }
        float squaredDistance =
            plasma::Vector<3, float>(creature->m_drawnPosition - m_world->m_localCreature->m_entityData.position)
                .squaredLength();
        if (!(squaredDistance <= NearbyCreatureRangeSquared)) {
            continue;
        }
        if (m_renderer->isSphereVisible(creature->m_drawnPosition,
                                        (std::max)(creature->m_entityData.appearance.size[2],
                                                   (std::max)(creature->m_entityData.appearance.size[0],
                                                              creature->m_entityData.appearance.size[1]) *
                                                       0.6f),
                                        NearbyCreatureRange)) {
            NearbyCreature entry;
            entry.creature = creature;
            entry.squaredDistance = squaredDistance;
            entry.inClearView = false;
            nearby.push_back(entry);
        }
    }
    std::sort(nearby.begin(), nearby.end());

    // The player's actions, while a world is played, the character is not flying and no menu is shown.
    if (!m_world->m_worldName.empty() && m_world->m_localCreature->m_entityData.currentAbility != AbilityEagleFlight &&
        m_world->m_localCreature->m_entityData.currentAbility != AbilityMenuView &&
        !m_interface.m_startMenuNode->isVisible() && !m_interface.m_characterSelection.m_node->isVisible() &&
        !m_interface.m_characterCreationNode->isVisible()) {
        controlCharacter(step, nearby);
    }

    // The debug switches: cruising 200 blocks above the land at 20 blocks a second along the character's heading; a
    // flight (ability 156) to a random start cell with full HP.
    if (s_debugCruising) {
        player = m_world->m_localCreature;
        float height = (std::max)(0.0f, m_world->biomeElevationAt((int)player->m_entityData.position[0],
                                                                  (int)player->m_entityData.position[1]) +
                                            200.0f);
        blend(m_world->m_localCreature->m_entityData.position[2], Fixed(height), step, 0.05f);
        plasma::Matrix<float> heading;
        heading.identity();
        player = m_world->m_localCreature;
        heading.rotateZ(player->m_entityData.rotation[2]);
        player->m_entityData.acceleration = heading.transformDirection(plasma::Vector<3, float>(0.0f, 20.0f, 0.0f));
        m_world->m_localCreature->m_entityData.velocity = m_world->m_localCreature->m_entityData.acceleration;
    }
    if (s_debugFlying) {
        player = m_world->m_localCreature;
        if (player->m_entityData.hp <= 0.0f) {
            player->m_entityData.hp = player->maxHP();
        }
        m_world->m_localCreature->m_entityData.stunTime = -3000.0f;
        player = m_world->m_localCreature;
        if (player->m_entityData.currentAbility != AbilityEagleFlight &&
            (player->m_entityData.flags & (CreatureFlagOnGround | CreatureFlagInWater))) {
            player->m_entityData.currentAbility = AbilityEagleFlight;
            m_world->m_localCreature->m_entityData.timeSinceAbility = 0.0f;
            player = m_world->m_localCreature;
            player->m_entityData.destination = m_world->randomStartCell() * 16;
            m_world->m_localCreature->m_entityData.origin = m_world->m_localCreature->m_entityData.position;
        }
    }

    // The quick item buttons, the quick buttons' "info" texts and the quick bar (hidden with the map), and the
    // creature the speech widget shows: the character faces it, with toggle 2 set.
    updateUsableItems();
    for (auto &quickButton : m_interface.m_inGame.m_quickButtons) {
        quickButton.button->setVisible(!quickButton.button->getText(L"info", true).empty() && !m_interface.m_mapOpen);
    }
    m_interface.m_inGame.m_quickBar->setVisible(!m_interface.m_mapOpen);
    if (m_interface.m_inGame.m_speechWidget->getNode()->isVisible()) {
        m_crosshairCreatureId = m_interface.m_inGame.m_speechWidget->m_speakerId;
        Creature *speaker = m_world->creatureByID(m_interface.m_inGame.m_speechWidget->m_speakerId);
        if (speaker) {
            player = m_world->m_localCreature;
            player->m_entityData.attackRotation = speaker->m_entityData.position - player->m_entityData.position;
            m_world->m_localCreature->m_entityData.setToggles(1 << CreatureToggleFaceAim, true);
        }
    }
    unsigned int afterSpeech = timeGetTime();

    // The camera turns slower near the creature looked at: the gamepad's turn, cubed, moves the voxel widget's model
    // when it is shown, else the camera.
    float cameraSpeed = 0.4f;
    if (m_creatureInView) {
        Creature *creature = m_world->creatureByID(m_crosshairCreatureId);
        if (creature && creature->m_entityData.hp > 0.0f) {
            float distance = plasma::Vector<3, float>(creature->m_drawnPosition - m_cameraPosition).length() / 50.0f;
            cameraSpeed = (std::max)(0.3f, 1.0f - distance) * 0.4f;
        }
    }
    if (m_interface.m_inGame.m_voxelWidget->getNode()->isVisible()) {
        m_interface.m_inGame.m_voxelWidget->m_rotation[2] += powf(m_gamepadTurnVector[0], 3.0f) * cameraSpeed * step;
        m_interface.m_inGame.m_voxelWidget->m_rotation[0] += powf(m_gamepadTurnVector[1], 3.0f) * cameraSpeed * step;
    } else {
        m_targetCameraAngle[2] -= powf(m_gamepadTurnVector[0], 3.0f) * cameraSpeed * step;
        m_targetCameraAngle[0] -= powf(m_gamepadTurnVector[1], 3.0f) * cameraSpeed * step;
    }

    // With the map overlay: the distance from the character to the map's cursor, in metres (two blocks), as the
    // cursor's text.
    if (m_interface.m_inGame.m_mapOverlayWidget->getNode()->isVisible() && m_worldMap->m_cursorPosition[2] >= 0) {
        const plasma::Vector<3, Fixed> &offset =
            m_world->m_localCreature->m_entityData.position - m_worldMap->m_cursorPosition;
        plasma::Vector<2, float> unused(plasma::Vector<2, Fixed>(offset[0], offset[1]));
        const plasma::Vector<3, Fixed> &delta =
            m_world->m_localCreature->m_entityData.position - m_worldMap->m_cursorPosition;
        plasma::Vector<2, float> flat(plasma::Vector<2, Fixed>(delta[0], delta[1]));
        m_interface.m_cursor->setText(getFlatDistanceText((int)(flat.length() * 0.5)));
    } else {
        m_interface.m_cursor->setText(L"");
    }
    updateCreatureLabels(nearby);
    updateSpeech(step);

    // The particle packets: count particles each, spread around the position by up to a block, with a random rotation
    // and spin; type 1 particles are red to yellow, type 5 ones start at a random age.
    for (ServerUpdates::ParticlePacket packet : m_world->m_serverUpdates.m_particlePacketList) {
        for (int i = 0; i < packet.count; i++) {
            Particle particle;
            particle.type = packet.type;
            particle.age = 0;
            plasma::Vector<3, float> offset((float)rand() / (float)RAND_MAX - 0.5f,
                                            (float)rand() / (float)RAND_MAX - 0.5f, (float)rand() / (float)RAND_MAX);
            particle.position = packet.position + plasma::Vector<3, Fixed>(offset);
            particle.acceleration =
                packet.acceleration + packet.spread * 2.0f *
                                          plasma::Vector<3, float>((float)rand() / (float)RAND_MAX - 0.5f,
                                                                   (float)rand() / (float)RAND_MAX - 0.5f,
                                                                   (float)rand() / (float)RAND_MAX - 0.5f);
            particle.rotation = plasma::Vector<3, float>((float)rand() * 360.0f / (float)RAND_MAX,
                                                         (float)rand() * 360.0f / (float)RAND_MAX,
                                                         (float)rand() * 360.0f / (float)RAND_MAX);
            particle.spin = 100.0f * plasma::Vector<3, float>((float)rand() / (float)RAND_MAX - 0.5f,
                                                              (float)rand() / (float)RAND_MAX - 0.5f,
                                                              (float)rand() / (float)RAND_MAX - 0.5f);
            particle.color = packet.color;
            if (packet.type == ParticleFire) {
                particle.color = plasma::Vector<4, float>(1.0f, (float)rand() / (float)RAND_MAX, 0.0f, 1.0f);
            }
            if (particle.type == ParticleDebris) {
                particle.collidesWithBlocks = true;
                particle.age = rand() % 500;
            }
            particle.size = packet.size;
            particle.matrix.identity();
            m_particles.push_front(particle);
        }
    }

    // The sound packets; an explosion (84, 85) shakes the screen by how close it is within 50 blocks.
    for (auto &sound : m_world->m_serverUpdates.m_soundPacketList) {
        playSound(sound.sound, sound.position, sound.volume, sound.speed, true);
        if (sound.sound == SoundExplosion || sound.sound == SoundPunch285) {
            float proximity =
                1.0f - plasma::Vector<3, float>(sound.position - m_world->m_localCreature->m_entityData.position)
                               .squaredLength() /
                           2500.0f;
            float shake = std::max(0.0f, proximity);
            if (shake > m_screenShake) {
                m_screenShake = shake;
            }
        }
    }
    // The item pickups: the item as the creature gets it, into the player's inventory (platinum coins by region; an
    // item with a count as gold; a block's item only once), with the inventory widgets refreshed, the 16-block cell of
    // a category-30 item recorded, a dialog for the artifacts, books and pets, and the notice for everyone's pickups.
    for (auto &pickup : m_world->m_serverUpdates.m_itemPickupPacketList) {
        Creature *creature = m_world->creatureByID(pickup.creatureId);
        if (!creature) {
            continue;
        }
        Item item = pickup.item.transformed(creature);
        if (creature == m_world->m_localCreature) {
            if (pickup.flags & ZoneItemFlagPerPlayer) {
                IntTriple block = {
                    {pickup.position[0].toBlock(), pickup.position[1].toBlock(), pickup.position[2].toBlock()}};
                if (m_world->m_localCreature->m_playerData->collectedPlacedItems.find(block) !=
                    m_world->m_localCreature->m_playerData->collectedPlacedItems.end()) {
                    continue;
                }
                m_world->m_localCreature->m_playerData->collectedPlacedItems.insert(block);
            }
            if (item.category == ItemCategoryCoin && item.material == ItemMaterialPlatinum) {
                // Temporary rvalue key used for lookup.
                creature->m_playerData
                    ->platinum[std::array<int, 2> {{m_world->m_localCreature->m_entityData.region[0],
                                                    m_world->m_localCreature->m_entityData.region[1]}}] +=
                    pickup.quantity;
            } else if (item.count() != 0) {
                creature->m_inventory.m_gold += item.count();
            } else {
                creature->m_inventory.add(item, pickup.quantity, -1);
            }
            updateSorting();
            m_interface.m_inGame.m_equipmentInventoryWidget->updateRows();
            m_interface.m_inGame.m_equipmentInventoryWidget->updateScrollButton();
            if (pickup.item.category == ItemCategoryGeneratedItem) {
                m_world->m_localCreature->m_playerData->discoveredMarkers.insert(
                    std::array<int, 2> {{tileCoordinate(pickup.position[0]), tileCoordinate(pickup.position[1])}});
            }
            if (pickup.item.category == ItemCategoryGeneratedItem || item.category == ItemCategoryArtifact ||
                item.category == ItemCategorySpecial || item.category == ItemCategoryGlyph) {
                std::vector<std::string> script;
                script << "define_item" << item;
                if (item.category == ItemCategoryArtifact || item.category == ItemCategorySpecial ||
                    (item.isEquipment() && item.rarity > ItemRarityBlue)) {
                    script << "you_received_the_item";
                } else {
                    script << "you_received_an_item";
                }
                if (item.category == ItemCategorySpecial) {
                    script << "new_page";
                    script << "define_land" << m_world->landBiome(item.region)
                           << (int)fastRandom(item.region[0], item.region[1]);
                    switch (item.id) {
                    case SpecialItemTreasureSpirit:
                        script << "description_treasure_spirit";
                        break;
                    case SpecialItemHangGlider:
                        script << "description_hang_glider";
                        break;
                    case SpecialItemBoat:
                        script << "description_boat";
                        break;
                    case SpecialItemClimbingSpikes:
                        script << "description_climbing_spikes";
                        break;
                    case SpecialItemReins:
                        script << "description_reins";
                        break;
                    case SpecialItemHarp:
                        script << "description_harp";
                        break;
                    case SpecialItemWhistle:
                        script << "description_whistle";
                        break;
                    case SpecialItemBell:
                        script << "description_bell";
                        break;
                    case SpecialItemMagicCraftingBook:
                        script << "description_crafting_book1";
                        break;
                    case SpecialItemRareCraftingBook:
                        script << "description_crafting_book2";
                        break;
                    case SpecialItemEpicCraftingBook:
                        script << "description_crafting_book3";
                        break;
                    case SpecialItemLegendaryCraftingBook:
                        script << "description_crafting_book4";
                        break;
                    }
                }
                m_speech.generateFromScript(script, rand() % 10000, m_interface.m_inGame.m_speechWidget->m_dialogs,
                                            L"");
                // The character shows the item (abilities 120, 121 and 124) unless it is still attacking.
                player = m_world->m_localCreature;
                if (player->m_entityData.currentAbility == AbilityNone ||
                    player->m_entityData.timeSinceAbility > (float)player->attackDuration()) {
                    if (item.category == ItemCategorySpecial && item.id == SpecialItemBoat) {
                        if (m_world->m_localCreature->m_entityData.flags & CreatureFlagInWater) {
                            m_world->m_localCreature->m_entityData.currentAbility = AbilitySailBoat;
                            m_world->m_localCreature->m_entityData.actionItem = item;
                            m_world->m_localCreature->m_entityData.timeSinceAbility = 0.0f;
                        }
                    } else {
                        if (item.category == ItemCategorySpecial && item.id == SpecialItemHangGlider) {
                            m_world->m_localCreature->m_entityData.currentAbility = AbilityHangGlide;
                        } else {
                            m_world->m_localCreature->m_entityData.currentAbility = AbilityPresentItem;
                        }
                        m_world->m_localCreature->m_entityData.actionItem = item;
                        m_world->m_localCreature->m_entityData.timeSinceAbility = 0.0f;
                    }
                    m_successMusicTime = 60000;
                }
            }
        }
        if (creature == m_world->m_localCreature &&
            m_interface.m_inGame.m_craftingInventoryWidget->getNode()->isVisible()) {
            updateFormulas();
        }
        notifyItem(item, pickup.quantity, creature);
    }
    processHitLights();
    processNotices();
    unsigned int afterPickups = timeGetTime();

    // The zones within four of the character (of the flight's target while flying) that the host has filled, for the
    // client to report.
    m_requestedZones.clear();
    if (m_world->m_isClient) {
        player = m_world->m_localCreature;
        int y = (int)player->m_entityData.position[1];
        int x = (int)player->m_entityData.position[0];
        if (player->m_entityData.currentAbility == AbilityEagleFlight) {
            x = player->m_entityData.destination[0];
            y = player->m_entityData.destination[1];
        }
        int minZoneX = World::zoneIndex(x - 256);
        int minZoneY = World::zoneIndex(y - 256);
        int maxZoneX = World::zoneIndex(x + 256);
        int maxZoneY = World::zoneIndex(y + 256);
        for (int zoneX = minZoneX; zoneX <= maxZoneX; zoneX++) {
            for (int zoneY = minZoneY; zoneY <= maxZoneY; zoneY++) {
                Zone *zone = m_world->zone(zoneX, zoneY);
                if (zone && zone->m_awaitingData) {
                    m_requestedZones.push_back(plasma::Vector<2, int>(zoneX, zoneY));
                }
            }
        }
    }
    m_client.update();
    unsigned int afterZones = timeGetTime();

    // The per-frame updates of the effects, the camera and the sounds, the kills, and the XP and level before the
    // texts below compare them.
    int stepMs = (int)step;
    updateFx(stepMs);
    updateLights(stepMs);
    updateTextFx(stepMs);
    updateCrafting(stepMs);
    updateCamera(step);
    updateClimateSounds(stepMs);
    int xp = m_world->m_localCreature->totalXP();
    int level = m_world->m_localCreature->m_entityData.level;
    for (auto &kill : m_world->m_serverUpdates.m_killPacketList) {
        m_world->onKill(kill);
    }

    // The menu keys, on their press, while the chat is not typed into and no start menu is shown; the equipment slots
    // follow the equipment inventory and the formula details the crafting inventory.
    if (!m_interface.m_inGame.m_chatWidget->m_typeboxActive && !m_interface.m_startMenuNode->isVisible() &&
        !m_interface.m_characterSelection.m_node->isVisible() && !m_interface.m_characterCreationNode->isVisible()) {
        static bool worldMapWasDown;
        static bool inventoryWasDown;
        static bool craftingWasDown;
        static bool ringMenuWasDown;
        static bool multiplayerWasDown;
        static bool systemWasDown;
        if (!worldMapWasDown && m_controlStates[Controls::ButtonWorldMap]) {
            m_interface.toggleWorldMap();
        }
        worldMapWasDown = m_controlStates[Controls::ButtonWorldMap];
        if (!inventoryWasDown && m_controlStates[Controls::ButtonInventory]) {
            m_interface.toggleInventoryWidget();
        }
        inventoryWasDown = m_controlStates[Controls::ButtonInventory];
        for (SpriteWidget *slot : m_interface.m_inGame.m_equipmentSlots) {
            slot->getNode()->setVisible(m_interface.m_inGame.m_equipmentInventoryWidget->getNode()->isVisible());
        }
        if (!craftingWasDown && m_controlStates[Controls::ButtonCraftingMenu]) {
            m_interface.toggleCraftingWidget();
        }
        craftingWasDown = m_controlStates[Controls::ButtonCraftingMenu];
        m_interface.m_inGame.m_formulaDetailsWidget->getNode()->setVisible(
            m_interface.m_inGame.m_craftingInventoryWidget->getNode()->isVisible());
        if (!ringMenuWasDown && m_controlStates[Controls::ButtonRingMenu]) {
            m_interface.m_inGame.m_selector->setVisible(!m_interface.m_inGame.m_selector->isVisible());
        }
        ringMenuWasDown = m_controlStates[Controls::ButtonRingMenu];
        if (!multiplayerWasDown && m_controlStates[Controls::ButtonMultiplayerMenu]) {
            m_interface.toggleMultiplayerWidget();
        }
        multiplayerWasDown = m_controlStates[Controls::ButtonMultiplayerMenu];
        if (!systemWasDown && m_controlStates[Controls::ButtonSystemMenu]) {
            m_interface.toggleSystemWidget();
        }
        systemWasDown = m_controlStates[Controls::ButtonSystemMenu];
    }

    // The map dragged with the gamepad: the pointer follows the overlay and the stick moves the map, faster when
    // zoomed out. Otherwise walking (or the stick) closes every menu and the map.
    if (m_interface.m_mapOpen && m_gamepadActive) {
        m_engine->movePointer((float)m_width * m_mapOverlayPosition[0],
                              (1.0f - m_mapOverlayPosition[1]) * (float)m_height);
        m_mapZoomChanged = true;
        plasma::Matrix<float> heading;
        heading.identity();
        heading.rotateZ((float)m_cameraAngle[2]);
        plasma::Vector<3, float> stick(-m_gamepadWalkVector[0], -m_gamepadWalkVector[1], 0.0f);
        plasma::Vector<3, float> move =
            heading.transformDirection(powf(2.0f, (float)m_mapZoom) * 0.5f * (float)step * stick);
        m_targetMapPosition += plasma::Vector<3, Fixed>(move);
    } else if (((m_controlStates[Controls::ButtonUp] || m_controlStates[Controls::ButtonLeft] ||
                 m_controlStates[Controls::ButtonRight] || m_controlStates[Controls::ButtonDown]) &&
                !m_interface.m_inGame.m_chatWidget->m_typeboxActive) ||
               m_gamepadWalkVector.squaredLength() > 0.1f * 0.1f) {
        m_interface.m_mapOpen = false;
        m_interface.m_inGame.m_equipmentInventoryWidget->getNode()->setVisible(false);
        m_interface.m_inGame.m_vendorInventoryWidget->getNode()->setVisible(false);
        m_interface.m_inGame.m_craftingInventoryWidget->getNode()->setVisible(false);
        m_interface.m_inGame.m_formulaDetailsWidget->getNode()->setVisible(false);
        m_interface.m_inGame.m_menuOpen = false;
        m_interface.m_inGame.m_systemWidget->getNode()->setVisible(false);
        m_interface.m_inGame.m_voxelWidget->getNode()->setVisible(false);
        m_interface.m_inGame.m_enchantWidget->getNode()->setVisible(false);
        m_interface.m_inGame.m_adaptionWidget->getNode()->setVisible(false);
        m_interface.m_inGame.m_multiplayerWidget->getNode()->setVisible(false);
        if (m_interface.m_inGame.m_questionWidget->getNode()->isVisible()) {
            m_interface.m_inGame.m_questionWidget->m_dialogs.clear();
        }
    }

    // The question widget shows its dialogs unless a speech widget speaks to the character; the ring menu closes on a
    // forward or backward move; the map overlay eases to its place and scale.
    QuestionWidget *question = m_interface.m_inGame.m_questionWidget;
    if (question->m_dialogs.empty()) {
        question->getNode()->setVisible(false);
    } else {
        bool speaking = false;
        for (SpeechWidget *speech : m_interface.m_inGame.m_speechWidgets) {
            if (speech->m_listenerId == m_world->m_localCreature->m_id) {
                speaking = true;
                break;
            }
        }
        question->getNode()->setVisible(!speaking);
        if (m_interface.m_inGame.m_questionWidget->getNode()->isVisible()) {
            m_interface.m_inGame.m_questionWidget->fitToDialogs();
        }
    }
    if (m_interface.m_inGame.m_selector->isVisible()) {
        if (m_controlStates[Controls::ButtonUp] || m_gamepadWalkVector[1] > 0.25f ||
            m_controlStates[Controls::ButtonDown] || m_gamepadWalkVector[1] < -0.25f) {
            m_interface.m_inGame.m_selector->setVisible(false);
        }
    }
    if (m_interface.m_mapOpen) {
        blend(m_mapOverlayPosition, plasma::Vector<2, float>(0.5f, 0.6f), step, 0.01f);
        blend(m_mapOverlayScale, 1.8f, step, 0.01f);
    } else {
        if (m_width > 0) {
            blend(m_mapOverlayPosition,
                  plasma::Vector<2, float>(1.0f - (float)m_height * 0.3f / (float)m_width * m_options.mapScale,
                                           1.0f - m_options.mapScale * 0.2f),
                  step, 0.01f);
        }
        blend(m_mapOverlayScale, m_options.mapScale, step, 0.01f);
    }
    unsigned int afterOverlay = timeGetTime();
    m_interface.update((float)step);
    if (!m_interface.m_choosingShrine) {
        m_mapItemStack = nullptr;
    }

    // A large or tinted (AppearanceFlagTinted) hostile creature with aggro on the character calls for the boss music.
    bool bossNear = false;
    for (Creature *creature : m_world->m_creatures) {
        for (auto &aggro : creature->m_aggro) {
            if (aggro.first == m_world->m_localCreature->m_id &&
                creature->m_entityData.hostilityType == HostilityEnemy &&
                (creature->isLarge() || creature->isBoosted())) {
                bossNear = true;
            }
        }
    }

    // The location texts: the land's name, and the place's (a dungeon's inside a structure, a named building's, else
    // the place name when the character is well inside it), or the arrival time while flying. A changed place name
    // plays the location banner. How long the current music file has played.
    static int musicTime;
    static std::string lastMusicFile;
    std::string music;
    player = m_world->m_localCreature;
    int regionX = player->m_entityData.region[0];
    int regionY = player->m_entityData.region[1];
    int biome = m_world->landBiome(regionX, regionY);
    player = m_world->m_localCreature;
    int placeX = World::placeIndex((int)player->m_entityData.position[0]);
    int placeY = World::placeIndex((int)player->m_entityData.position[1]);
    bool large = m_world->isLargePlace(placeX, placeY);
    int type = m_world->locationType(placeX, placeY, large);
    plasma::Vector<2, int> center = m_world->placeCenter(placeX, placeY, large);
    player = m_world->m_localCreature;
    if (m_world
            ->getBlockAtDot(player->m_entityData.position[0], player->m_entityData.position[1],
                            player->m_entityData.position[2])
            .isInDungeon()) {
        type = m_world->dungeonType(placeX, placeY, type, large);
    } else {
        player = m_world->m_localCreature;
        int blockY = player->m_entityData.position[1].toBlock();
        int blockX = player->m_entityData.position[0].toBlock();
        if (m_world->placeInfluence(
                type, large, m_world->placeSquaredDistance(m_world->placeCenter(placeX, placeY), blockX, blockY)) <
            0.5f) {
            type = PlaceNone;
        }
    }
    player = m_world->m_localCreature;
    int building = m_world->zoneBuildingType(tileCoordinate(player->m_entityData.position[0]),
                                             tileCoordinate(player->m_entityData.position[1]));
    m_interface.m_inGame.m_infoTextLarge->setText(L"");
    m_interface.m_inGame.m_infoTextMedium->setText(L"");
    player = m_world->m_localCreature;
    if (player->m_entityData.currentAbility == AbilityEagleFlight) {
        // Truncates the flight's start position to blocks and reads the local creature's elapsed ability time before
        // computing flightDuration.
        plasma::Vector<3, int> origin(player->m_entityData.origin);
        plasma::Vector<2, int> from(origin[0], origin[1]);
        Creature *local = m_world->m_localCreature;
        int seconds = (int)((float)m_world->flightDuration(from, player->m_entityData.destination) -
                            local->m_entityData.timeSinceAbility) /
                      1000;
        m_interface.m_inGame.m_infoTextMedium->setText(L"Arrival in " + std::to_wstring(seconds) + L" s");
    } else {
        std::wstring text = m_speech.getText(m_speech.getLandName(biome), L"named",
                                             randomName((int)fastRandom(regionX, regionY), RaceNone));
        m_interface.m_inGame.m_infoTextLarge->setText(text);
        bool named = building != BuildingNone && !m_speech.staticKey(building).empty();
        if (named) {
            text = m_speech.getText(m_speech.staticKey(building), L"default", L"");
            m_interface.m_inGame.m_infoTextMedium->setText(text);
        } else if (type) {
            text = m_speech.findPlaceName(placeX, placeY, type, large);
            m_interface.m_inGame.m_infoTextMedium->setText(text);
        }
        static std::wstring lastLocation;
        if (text.empty()) {
            lastLocation = L"";
        } else if (text != lastLocation && !m_interface.m_inGame.m_location->isPlaying()) {
            lastLocation = text;
            m_interface.m_inGame.m_location->setText(L"name", text, true);
            m_interface.m_inGame.m_location->setText(L"detail", L"", true);
            m_interface.m_inGame.m_location->play(L"location", 0);
        }
    }

    // The music for where the character is: the main theme at the world's first place, the shrine inside a structure,
    // else the land's (and the beach or ocean by the terrain height) overridden by the place's.
    bool loop;
    if (placeX == 0 && placeY == 0 && type) {
        music = "maintheme.ogg";
        loop = false;
    } else {
        player = m_world->m_localCreature;
        if (m_world
                ->getBlockAtDot(player->m_entityData.position[0], player->m_entityData.position[1],
                                player->m_entityData.position[2])
                .isInDungeon()) {
            music = "shrine.ogg";
            loop = false;
        } else {
            music = "greenlands.ogg";
            switch (biome) {
            case BiomeDarkWoods:
            case BiomeDeadlands:
                music = "deadlands.ogg";
                break;
            case BiomeFirelands:
                music = "cult of doom.ogg";
                break;
            case BiomeDesert:
                music = "desert.ogg";
                break;
            case BiomeSnowForest:
            case BiomeSnowlands:
                music = "winter.ogg";
                break;
            case BiomeWoodlands:
                music = "woodlands.ogg";
                break;
            case BiomeJungle:
                music = "jungle.ogg";
                break;
            case BiomeSavannah:
                music = "savannah.ogg";
                break;
            case BiomeMountains:
                music = "mountains.ogg";
                break;
            }
            player = m_world->m_localCreature;
            float height =
                m_world->height((int)player->m_entityData.position[0], (int)player->m_entityData.position[1], 0);
            if (height < 10.0f) {
                music = "beach.ogg";
            }
            if (height < 0.0f) {
                music = "ocean.ogg";
            }
            switch (type) {
            case PlaceVillage: {
                World::BiomeSample sample;
                m_world->biomeSample(center[0], center[1], sample);
                if (sample.undead(type, 1.0f) > 0.5f) {
                    music = "unholy pact.ogg";
                } else if (fastRandom(placeX, placeY) & 1) {
                    music = "village.ogg";
                } else {
                    music = "village2.ogg";
                }
                break;
            }
            case PlaceCrypt:
            case PlaceGraveyard:
                music = "unholy pact.ogg";
                break;
            case PlaceCastle:
                music = "order of the light.ogg";
                break;
            case PlaceLake:
            case PlaceTree:
            case PlaceGrove:
                music = "druids of mana.ogg";
                break;
            case PlaceForest:
                music = "woodlands.ogg";
                break;
            case PlaceDarkForest:
                music = "deadlands.ogg";
                break;
            case PlaceEnchantedForest:
                music = "enchanted-forest.ogg";
                break;
            case PlaceManaPump:
                music = "steel empire.ogg";
                break;
            case PlaceMountain:
                music = "mountains.ogg";
                break;
            case PlaceCrater:
            case PlaceAltar:
                music = "cult of doom.ogg";
                break;
            }
            loop = true;
        }
    }
    // The spirit world (buff 19), a boss, a success (m_successMusicTime counts down from a mission's completion) and
    // the menus override it.
    if (m_world->m_localCreature->findBuff(BuffSpiritworld)) {
        m_interface.m_inGame.m_infoTextMedium->setText(m_speech.getText(L"SpiritWorld", L"default", L""));
    }
    if (m_world->m_localCreature->findBuff(BuffSpiritworld)) {
        music = "deadlands.ogg";
        if (music != lastMusicFile) {
            s_nextMusicTime = 100;
        }
    }
    if (bossNear) {
        music = "boss.ogg";
        loop = false;
    }
    m_successMusicTime = (int)(m_successMusicTime - step);
    if (m_successMusicTime < 0) {
        m_successMusicTime = 0;
    }
    m_musicDuckTime = (int)(m_musicDuckTime - step);
    if (m_musicDuckTime < 0) {
        m_musicDuckTime = 0;
    }
    int success = m_successMusicTime;
    if (success > 0) {
        music = "success.ogg";
        if (music != lastMusicFile) {
            s_nextMusicTime = 500;
        }
    }
    if (m_interface.m_startMenuNode->isVisible() || m_interface.m_characterCreationNode->isVisible() ||
        m_interface.m_characterSelection.m_node->isVisible()) {
        music = "maintheme.ogg";
        loop = false;
        if (music != lastMusicFile) {
            s_nextMusicTime = 500;
        }
    }
    bool fade = m_options.musicLoop == 0 ? loop : false;
    if (m_audio->isMusicPlaying()) {
        musicTime = (int)(musicTime + step);
        if (music != lastMusicFile) {
            if (s_musicPaused) {
                if (bossNear || success > 0) {
                    s_nextMusicTime = musicTime + 500;
                } else if (musicTime < 25000) {
                    s_nextMusicTime = 30000;
                } else {
                    s_nextMusicTime = musicTime + 5000;
                }
                s_musicPaused = false;
            }
        } else {
            s_nextMusicTime = fade ? 300000 : musicTime + 300000;
            s_musicPaused = true;
        }
        // The volume eases to its target: nearly silent while m_musicDuckTime runs, the option's while the file is
        // due, else down to silence.
        EnterCriticalSection(&m_musicCriticalSection);
        float target;
        float rate;
        if (m_musicDuckTime > 0) {
            target = 0.015f;
            rate = 0.005f;
        } else if (musicTime > s_nextMusicTime) {
            target = 0.0f;
            rate = bossNear || success > 0 ? 0.005f : 0.0004f;
        } else {
            target = (float)m_options.musicVolume * 0.01f;
            rate = 0.001f;
        }
        blend(m_musicVolume, target, step, rate);
        LeaveCriticalSection(&m_musicCriticalSection);
    } else {
        if (music != lastMusicFile) {
            lastMusicFile = music;
            EnterCriticalSection(&m_musicCriticalSection);
            m_currentMusicFile = "music/" + music;
            LeaveCriticalSection(&m_musicCriticalSection);
        }
        s_nextMusicTime = success > 0 ? 10000 : 300000;
        EnterCriticalSection(&m_musicCriticalSection);
        m_musicVolume = (float)m_options.musicVolume * 0.01f;
        LeaveCriticalSection(&m_musicCriticalSection);
        musicTime = 0;
        s_musicPaused = true;
    }

    // The first quick button holds the selected food slot's item, with its count and name.
    m_interface.m_inGame.m_quickButtons[QuickSlotItem].button->setText(L"count", L"", true);
    m_interface.m_inGame.m_quickButtons[QuickSlotItem].button->setText(L"info", L"", true);
    std::vector<plasma::Vector<2, int>> foodSlots = getFoodSlots();
    if (!foodSlots.empty()) {
        int selected = m_ringSelection;
        if (selected < 0) {
            selected = (int)(((size_t)-selected / foodSlots.size() + 1) * foodSlots.size() + selected);
        }
        plasma::Vector<2, int> slot = foodSlots[(size_t)selected % foodSlots.size()];
        Creature *creature = m_world->m_localCreature;
        m_interface.m_inGame.m_quickItemSprites[QuickSlotItem]->m_item =
            creature->m_inventory.m_inventoryTabs[slot[0]][slot[1]].item;
        if (itemIsStackable(m_world->m_localCreature->m_inventory.m_inventoryTabs[slot[0]][slot[1]].item)) {
            m_interface.m_inGame.m_quickButtons[QuickSlotItem].button->setText(
                L"count",
                std::to_string(m_world->m_localCreature->m_inventory.m_inventoryTabs[slot[0]][slot[1]].quantity), true);
        }
        creature = m_world->m_localCreature;
        m_interface.m_inGame.m_quickButtons[QuickSlotItem].button->setText(
            L"info",
            m_speech.getText(m_speech.getItemKey(creature->m_inventory.m_inventoryTabs[slot[0]][slot[1]].item.category,
                                                 creature->m_inventory.m_inventoryTabs[slot[0]][slot[1]].item.id),
                             L"default", L""),
            true);
    }
    unsigned int afterRingMenu = timeGetTime();

    // The ring menu: the usable items (the food slots) around the selected one, turned by left and right (sound 88
    // every 200 ms), its count, name and colour, and its rotation eased unless it is far round. Whether both turn
    // inputs were released since the last turn, and the cooldowns of the two directions.
    static bool ringMenuReleased;
    static int ringMenuLeftCooldown;
    static int ringMenuRightCooldown;
    std::vector<MenuEntry> usableItems;
    if (m_interface.m_inGame.m_selector->isVisible()) {
        if (foodSlots.empty()) {
            {
                std::string count("");
                m_interface.m_inGame.m_selector->setText(L"count", count, true);
            }
            m_interface.m_inGame.m_selector->setText(L"name", L"You don't have any usable items.", true);
            m_interface.m_inGame.m_selector->setTextColor(L"name", plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f),
                                                          true);
            m_ringRadius = 0.0f;
            m_ringRotation = 0.0f;
        } else {
            for (auto &slot : foodSlots) {
                MenuEntry entry;
                entry.item = &m_world->m_localCreature->m_inventory.m_inventoryTabs[slot[0]][slot[1]].item;
                usableItems.push_back(entry);
            }
            int selected = m_ringSelection;
            if (selected < 0) {
                selected = (int)(((size_t)-selected / foodSlots.size() + 1) * foodSlots.size() + selected);
            }
            plasma::Vector<2, int> slot = foodSlots[(size_t)selected % foodSlots.size()];
            {
                std::wstringstream text;
                Creature *creature = m_world->m_localCreature;
                if (itemIsStackable(creature->m_inventory.m_inventoryTabs[slot[0]][slot[1]].item)) {
                    text << creature->m_inventory.m_inventoryTabs[slot[0]][slot[1]].quantity;
                }
                m_interface.m_inGame.m_selector->setText(L"count", text.str(), true);
                text.str(L"");
                text << m_speech.createItemName(
                    m_world->m_localCreature->m_inventory.m_inventoryTabs[slot[0]][slot[1]].item);
                m_interface.m_inGame.m_selector->setText(L"name", text.str(), true);
                creature = m_world->m_localCreature;
                m_interface.m_inGame.m_selector->setTextColor(
                    L"name",
                    creature->m_inventory.m_inventoryTabs[slot[0]][slot[1]].item.textColor(
                        creature->m_entityData.region),
                    true);
            }
            bool left = m_controlStates[Controls::ButtonLeft] || m_gamepadWalkVector[0] < 0.0f;
            bool right = m_controlStates[Controls::ButtonRight] || m_gamepadWalkVector[0] > 0.0f;
            if (!left) {
                ringMenuLeftCooldown = 0;
            }
            if (!right) {
                ringMenuRightCooldown = 0;
            }
            if (!left && !right) {
                ringMenuReleased = true;
            }
            if ((!left && !right) || ringMenuReleased) {
                if (left && ringMenuLeftCooldown <= 0) {
                    m_ringSelection--;
                    ringMenuLeftCooldown = 200;
                    playSound(SoundMenuSelect, m_cameraPosition, 1.0f, 1.0f, true);
                }
                if (right && ringMenuRightCooldown <= 0) {
                    m_ringSelection++;
                    ringMenuRightCooldown = 200;
                    playSound(SoundMenuSelect, m_cameraPosition, 1.0f, 1.0f, true);
                }
                ringMenuLeftCooldown = (int)(ringMenuLeftCooldown - step);
                if (ringMenuLeftCooldown < 0) {
                    ringMenuLeftCooldown = 0;
                }
                ringMenuRightCooldown = (int)(ringMenuRightCooldown - step);
                if (ringMenuRightCooldown < 0) {
                    ringMenuRightCooldown = 0;
                }
            }
            float rotation = (float)m_ringSelection * -360.0f / (float)(int)usableItems.size();
            if (m_ringRadius > 0.0f && fabsf(m_ringRotation - rotation) < 270.0f) {
                blend(m_ringRotation, rotation, step, 0.01f);
            } else {
                m_ringRotation = rotation;
            }
            if (m_ringRadius == 0.0f) {
                m_ringRotation = rotation;
            }
            blend(m_ringRadius, m_interface.m_scale * 200.0f, step, 0.01f);
        }
    } else {
        ringMenuReleased = false;
        m_ringRadius = 0.0f;
        m_ringRotation = 0.0f;
    }
    if (m_interface.m_inGame.m_selector->isVisible()) {
        std::list<plasma::Node *> brackets;
        m_interface.m_inGame.m_selector->findNodes(brackets, L"brackets");
        for (plasma::Node *bracket : brackets) {
            bracket->setVisible(!usableItems.empty());
        }
    }

    // What the frame gained: XP, gold, platinum (in the region it started in) and a level, each as a floating text; XP
    // and the level also in the chat.
    int newXP = m_world->m_localCreature->totalXP();
    if (newXP > xp) {
        TextFX fx;
        fx.color = plasma::Vector<4, float>(0.65f, 0.4f, 1.0f, 1.0f);
        float offsetY = (float)rand() * 2.0f / (float)RAND_MAX - 1.0f;
        float offsetX = (float)rand() * 2.0f / (float)RAND_MAX - 1.0f;
        float offsetZ = (float)rand() / (float)RAND_MAX;
        plasma::Vector<3, Fixed> offset(plasma::Vector<3, float>(offsetX, offsetY, offsetZ));
        fx.position = m_world->m_localCreature->m_entityData.position + offset;
        fx.age = 0;
        fx.duration = 3000;
        fx.offsetX = 5.0f;
        fx.offsetY = -100.0f;
        fx.pixelSize = 16.0f;
        fx.rise = -100.0f;
        std::wstringstream text;
        text << "+" << newXP - xp << " XP";
        fx.text = text.str();
        m_textFx.push_back(fx);
        text.str(L"");
        text << L"You gain " << newXP - xp << L" XP." << std::endl;
        m_interface.m_inGame.m_chatWidget->printMessage(text.str(), plasma::Vector<3, unsigned char>(150, 128, 255));
    }
    int newGold = m_world->m_localCreature->m_inventory.m_gold;
    if (newGold > gold) {
        TextFX fx;
        fx.color = plasma::Vector<4, float>(1.0f, 0.9f, 0.5f, 1.0f);
        float offsetY = (float)rand() * 2.0f / (float)RAND_MAX - 1.0f;
        float offsetX = (float)rand() * 2.0f / (float)RAND_MAX - 1.0f;
        float offsetZ = (float)rand() / (float)RAND_MAX;
        plasma::Vector<3, Fixed> offset(plasma::Vector<3, float>(offsetX, offsetY, offsetZ));
        fx.position = m_world->m_localCreature->m_entityData.position + offset;
        fx.age = 0;
        fx.duration = 3000;
        fx.offsetX = 5.0f;
        fx.offsetY = -100.0f;
        fx.pixelSize = 16.0f;
        fx.rise = -100.0f;
        std::wstringstream text;
        text << "+" << newGold - gold << " GOLD";
        fx.text = text.str();
        m_textFx.push_back(fx);
    }
    int newPlatinum = m_world->m_localCreature->platinum();
    if (newPlatinum > platinum && region == m_world->m_localCreature->m_entityData.region) {
        TextFX fx;
        fx.color = plasma::Vector<4, float>(0.8f, 0.9f, 0.9f, 1.0f);
        float offsetY = (float)rand() * 2.0f / (float)RAND_MAX - 1.0f;
        float offsetX = (float)rand() * 2.0f / (float)RAND_MAX - 1.0f;
        float offsetZ = (float)rand() / (float)RAND_MAX;
        plasma::Vector<3, Fixed> offset(plasma::Vector<3, float>(offsetX, offsetY, offsetZ));
        fx.position = m_world->m_localCreature->m_entityData.position + offset;
        fx.age = 0;
        fx.duration = 3000;
        fx.offsetX = 5.0f;
        fx.offsetY = -100.0f;
        fx.pixelSize = 16.0f;
        fx.rise = -100.0f;
        std::wstringstream text;
        text << "+" << newPlatinum - platinum << " PLATINUM";
        fx.text = text.str();
        m_textFx.push_back(fx);
    }
    int newLevel = m_world->m_localCreature->m_entityData.level;
    if (newLevel > level) {
        TextFX fx;
        fx.color = plasma::Vector<4, float>(1.0f, 0.9f, 0.5f, 1.0f);
        float offsetY = (float)rand() * 2.0f / (float)RAND_MAX - 1.0f;
        float offsetX = (float)rand() * 2.0f / (float)RAND_MAX - 1.0f;
        float offsetZ = (float)rand() / (float)RAND_MAX;
        plasma::Vector<3, Fixed> offset(plasma::Vector<3, float>(offsetX, offsetY, offsetZ));
        fx.position = m_world->m_localCreature->m_entityData.position + offset;
        fx.age = 0;
        fx.duration = 3000;
        fx.offsetX = 5.0f;
        fx.offsetY = -150.0f;
        fx.pixelSize = 16.0f;
        fx.rise = -100.0f;
        std::wstringstream text;
        text << "LEVEL UP!";
        playSound(SoundLevelUp);
        fx.text = text.str();
        m_textFx.push_back(fx);
        text.str(L"");
        text << L"You reached level " << newLevel << "!" << std::endl;
        m_interface.m_inGame.m_chatWidget->printMessage(text.str(), plasma::Vector<3, unsigned char>(255, 220, 128));
    }

    // The frame drawn: the renderer stops when the world shown is not the one played; the draw flags; the menu open
    // and close sounds (86, 87).
    if (m_seed != m_world->m_seed || m_worldName != m_world->m_worldName) {
        m_renderer->resetViewDistance();
    }
    int flags = 0;
    if (s_debugDrawCreatureBlocks)
        flags = gfx::RenderFlagDebugCreatureBlocks;
    if (s_debugDrawOnlyPlayer) {
        flags |= gfx::RenderFlagOnlyPlayer;
    }
    if (!m_engine->root()->isVisible()) {
        flags |= gfx::RenderFlagHideHud;
    }
    // Whether a menu was shown in the last frame.
    static bool menuWasShown;
    bool menuShown = isInMenu() || m_interface.m_inGame.m_selector->isVisible();
    if (!menuShown) {
        if (menuWasShown) {
            playSound(SoundMenuClose2, m_cameraPosition, 1.0f, 1.0f, true);
        }
    } else if (!menuWasShown) {
        playSound(SoundMenuOpen2, m_cameraPosition, 1.0f, 1.0f, true);
    }
    if (m_engine->isKeyDown(VK_CONTROL)) {
        flags |= gfx::RenderFlagMapModifier;
    }
    if (m_interface.m_mapOpen) {
        flags |= gfx::RenderFlagMapOpen;
    }
    if (m_interface.m_startMenuNode->isVisible() || m_interface.m_characterCreationNode->isVisible() ||
        m_interface.m_characterSelection.m_node->isVisible()) {
        flags |= gfx::RenderFlagHideHud;
    }
    bool startMenuShown = m_interface.m_startMenuNode->isVisible();
    menuWasShown = menuShown;
    unsigned int beforeDraw = timeGetTime();
    int zoneRadius = (std::max)(3, 30 * m_options.renderDistance / 100);
    plasma::Vector<3, float> cameraAngle(m_cameraAngle);
    if (startMenuShown) {
        flags |= gfx::RenderFlagLogo;
    }
    m_renderer->render(m_cameraPosition, m_cameraFocusPosition, cameraAngle, m_width, m_height, (float)step,
                       m_worldViewMatrix, m_viewMatrix, m_projectionMatrix, zoneRadius, m_mapPosition, m_mapZoom,
                       m_interface.m_scale, m_particles, m_lights, usableItems, m_ringRadius, m_ringRotation,
                       &m_crosshairItem[0], m_mapOverlayPosition, m_options.mapScale * m_mapOverlayScale, flags);
    unsigned int afterDraw = timeGetTime();

    // Counts frames and sums stage times, resetting after 20 frames. zoneTime is never reset; no code reads this
    // profile.
    static int frameCount;
    static int setupTime;
    static int soundTime;
    static int worldTime;
    static int packetTime;
    static int speechTime;
    static int pickupTime;
    static int zoneTime;
    static int overlayTime;
    static int ringMenuTime;
    static int updateTime;
    static int drawTime;
    frameCount++;
    setupTime += afterSetup - start;
    soundTime += afterSounds - afterSetup;
    worldTime += afterMap - afterSounds;
    packetTime += afterPackets - afterMap;
    speechTime += afterSpeech - afterPackets;
    pickupTime += afterPickups - afterSpeech;
    zoneTime += afterZones - afterPickups;
    overlayTime += afterOverlay - afterZones;
    ringMenuTime += afterRingMenu - afterOverlay;
    updateTime += beforeDraw - start;
    drawTime += afterDraw - beforeDraw;
    if ((unsigned int)frameCount > 20) {
        frameCount = 0;
        setupTime = 0;
        soundTime = 0;
        worldTime = 0;
        packetTime = 0;
        speechTime = 0;
        pickupTime = 0;
        overlayTime = 0;
        ringMenuTime = 0;
        updateTime = 0;
        drawTime = 0;
    }

    // The world's updates go to the host and start afresh.
    m_client.addServerUpdates(m_world->m_serverUpdates);
    m_world->m_serverUpdates = ServerUpdates();
    m_world->leaveZoneMeshes();
}

void Game::moveCharacter(double step)
{
    float force = 80.0f;
    Creature *local = m_world->m_localCreature;
    if (local->isGliding()) {
        plasma::Vector<3, float> horizontal(local->m_entityData.velocity[0], local->m_entityData.velocity[1], 0.0f);
        if (horizontal.squaredLength() > 0.0f) {
            horizontal.normalize();
            plasma::Vector<3, float> side = plasma::Vector<3, float>(0.0f, 0.0f, 1.0f).cross(horizontal);
            float factor = blendFactor(step, 0.001f);
            local->m_entityData.acceleration[0] +=
                (horizontal[0] * 10.0f - local->m_entityData.acceleration[0]) * factor;
            local = m_world->m_localCreature;
            local->m_entityData.acceleration[1] +=
                (horizontal[1] * 10.0f - local->m_entityData.acceleration[1]) * factor;
            local = m_world->m_localCreature;
            plasma::Vector<3, float> correction = side * local->m_entityData.rotation[1] * 0.05f;
            local->m_entityData.acceleration = local->m_entityData.acceleration - correction;
        }
        local = m_world->m_localCreature;
        if (local->m_stamina < 0.001f) {
            local->m_entityData.setToggles(1 << CreatureToggleFastMovement, false);
            local = m_world->m_localCreature;
            local->m_entityData.acceleration[2] +=
                blendFactor(step, 0.002f) * (0.0f - local->m_entityData.acceleration[2]);
            return;
        }
    } else {
        if (local->m_entityData.currentAbility == AbilitySailBoat) {
            force = 5.0f;
            if (local->m_entityData.acceleration.squaredLength() < 225.0f && !m_controlStates[Controls::ButtonUp] &&
                !m_controlStates[Controls::ButtonDown] && !m_controlStates[Controls::ButtonLeft] &&
                !m_controlStates[Controls::ButtonRight] && m_gamepadWalkVector.squaredLength() < 0.01f) {
                local->m_entityData.acceleration = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
            }
            local = m_world->m_localCreature;
            if (local->m_entityData.acceleration.squaredLength() > 1600.0f) {
                local->m_entityData.acceleration.normalize();
                m_world->m_localCreature->m_entityData.acceleration *= 40.0f;
            }
        } else {
            local->m_entityData.acceleration = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
        }
    }
    local = m_world->m_localCreature;
    if (local->isGliding()) {
        bool braking =
            m_controlStates[Controls::ButtonJump] && local->m_stamina > 0.0f && local->m_entityData.velocity[2] <= 5.0f;
        if (braking) {
            // <windows.h>'s max and std::min(maxss, then minss).
            float glide = std::max(0.0f, local->m_glideTime * 0.001f - 0.5f);
            float factor = std::min(1.0f, glide) * 0.002f;
            local->m_entityData.acceleration[2] =
                (float)((double)factor * step * (double)std::max(-local->m_entityData.velocity[2], 0.0f) +
                        (double)local->m_entityData.acceleration[2]);
        } else {
            float forward = m_controlStates[Controls::ButtonUp] ? 1.0f : m_gamepadWalkVector[1];
            if (forward > 0.0f) {
                float glide = std::max(0.0f, local->m_glideTime * 0.001f - 0.5f);
                float factor = std::min(1.0f, glide) * forward * 0.0015f;
                local->m_entityData.acceleration[2] =
                    (float)((double)factor * step * (double)std::max(-local->m_entityData.velocity[2], 0.0f) +
                            (double)local->m_entityData.acceleration[2]);
            }
            local = m_world->m_localCreature;
            local->m_entityData.acceleration[2] +=
                blendFactor(step, 0.002f) * (0.0f - local->m_entityData.acceleration[2]);
        }
        local = m_world->m_localCreature;
        if (local->m_entityData.velocity[2] > 0.0f && local->m_entityData.acceleration[2] > 12.0f)
            local->m_entityData.acceleration[2] = 12.0f;
    }
    if (m_engine->getFocusWidget() || m_interface.m_inGame.m_chatWidget->m_typeboxActive ||
        m_interface.m_inGame.m_selector->isVisible() || (m_interface.m_mapOpen && m_gamepadActive)) {
        return;
    }
    local = m_world->m_localCreature;
    if (local->isGliding())
        force = (float)(step * 4.0);
    plasma::Matrix<float> yaw;
    yaw.identity();
    yaw.rotateZ((float)m_cameraAngle[2]);
    bool dodge = !m_interface.isInMenu() && m_controlStates[Controls::ButtonDodge] &&
                 (local->m_entityData.flags & CreatureFlagOnGround) != 0 && local->m_entityData.rollTime == 0.0f &&
                 local->m_stamina >= StaminaMoveCost;
    if (m_controlStates[Controls::ButtonUp]) {
        local = m_world->m_localCreature;
        unsigned char ability = local->m_entityData.currentAbility;
        unsigned int flags = local->m_entityData.flags;
        if (ability == AbilityClimb && (flags & CreatureFlagAtWall) != 0) {
            local->m_entityData.acceleration[2] += force * 0.2f;
        } else if ((flags & CreatureFlagInWater) != 0 && ability != AbilitySailBoat) {
            local->m_entityData.acceleration +=
                m_cameraMatrix.transformDirection(plasma::Vector<3, float>(0.0f, 0.0f, 1.0f)) * force;
        } else if (ability != AbilityHangGlide || (flags & CreatureFlagOnGround) != 0 || local->m_stamina > 0.0f) {
            local->m_entityData.acceleration +=
                yaw.transformDirection(plasma::Vector<3, float>(0.0f, -1.0f, 0.0f)) * force;
        }
    }
    if (m_controlStates[Controls::ButtonDown]) {
        local = m_world->m_localCreature;
        unsigned char ability = local->m_entityData.currentAbility;
        unsigned int flags = local->m_entityData.flags;
        if (ability == AbilityClimb && (flags & CreatureFlagAtWall) != 0) {
            local->m_entityData.acceleration[2] -= force * 0.2f;
        } else if ((flags & CreatureFlagInWater) != 0 && ability != AbilitySailBoat) {
            local->m_entityData.acceleration +=
                m_cameraMatrix.transformDirection(plasma::Vector<3, float>(0.0f, 0.0f, -1.0f)) * force;
        } else if (ability != AbilityHangGlide || (flags & CreatureFlagOnGround) != 0 || local->m_stamina > 0.0f) {
            plasma::Vector<3, float> forward =
                yaw.transformDirection(plasma::Vector<3, float>(0.0f, -1.0f, 0.0f)) * force;
            local->m_entityData.acceleration = local->m_entityData.acceleration - forward;
        }
    }
    if (m_controlStates[Controls::ButtonRight]) {
        local = m_world->m_localCreature;
        unsigned char ability = local->m_entityData.currentAbility;
        unsigned int flags = local->m_entityData.flags;
        if (ability == AbilityClimb && (flags & CreatureFlagAtWall) != 0) {
            plasma::Vector<3, float> side =
                local->m_wallNormal.cross(plasma::Vector<3, float>(0.0f, 0.0f, 1.0f)) * (force * 0.2f);
            local->m_entityData.acceleration = local->m_entityData.acceleration - side;
        } else if ((flags & CreatureFlagInWater) != 0 && ability != AbilitySailBoat) {
            local->m_entityData.acceleration +=
                m_cameraMatrix.transformDirection(plasma::Vector<3, float>(-1.0f, 0.0f, 0.0f)) * force;
        } else if (ability != AbilityHangGlide || (flags & CreatureFlagOnGround) != 0 || local->m_stamina > 0.0f) {
            plasma::Vector<3, float> right = yaw.transformDirection(plasma::Vector<3, float>(1.0f, 0.0f, 0.0f)) * force;
            local->m_entityData.acceleration = local->m_entityData.acceleration - right;
        }
    }
    if (m_controlStates[Controls::ButtonLeft]) {
        local = m_world->m_localCreature;
        unsigned char ability = local->m_entityData.currentAbility;
        unsigned int flags = local->m_entityData.flags;
        if (ability == AbilityClimb && (flags & CreatureFlagAtWall) != 0) {
            local->m_entityData.acceleration +=
                local->m_wallNormal.cross(plasma::Vector<3, float>(0.0f, 0.0f, 1.0f)) * (force * 0.2f);
        } else if ((flags & CreatureFlagInWater) != 0 && ability != AbilitySailBoat) {
            local->m_entityData.acceleration +=
                m_cameraMatrix.transformDirection(plasma::Vector<3, float>(1.0f, 0.0f, 0.0f)) * force;
        } else if (ability != AbilityHangGlide || (flags & CreatureFlagOnGround) != 0 || local->m_stamina > 0.0f) {
            local->m_entityData.acceleration +=
                yaw.transformDirection(plasma::Vector<3, float>(1.0f, 0.0f, 0.0f)) * force;
        }
    }
    if (m_gamepadWalkVector.squaredLength() != 0.0f) {
        local = m_world->m_localCreature;
        unsigned char ability = local->m_entityData.currentAbility;
        unsigned int flags = local->m_entityData.flags;
        if (ability == AbilityClimb && (flags & CreatureFlagAtWall) != 0) {
            local->m_entityData.acceleration +=
                local->m_wallNormal.cross(plasma::Vector<3, float>(0.0f, 0.0f, -m_gamepadWalkVector[0])) *
                (force * 0.2f);
            m_world->m_localCreature->m_entityData.acceleration[2] += force * 0.2f * m_gamepadWalkVector[1];
        } else if ((flags & CreatureFlagInWater) != 0) {
            local->m_entityData.acceleration += m_cameraMatrix.transformDirection(plasma::Vector<3, float>(
                                                    -m_gamepadWalkVector[0], 0.0f, m_gamepadWalkVector[1])) *
                                                force;
        } else if (ability != AbilityHangGlide || (flags & CreatureFlagOnGround) != 0 || local->m_stamina > 0.0f) {
            local->m_entityData.acceleration += yaw.transformDirection(plasma::Vector<3, float>(
                                                    -m_gamepadWalkVector[0], -m_gamepadWalkVector[1], 0.0f)) *
                                                force;
        }
    }
    local = m_world->m_localCreature;
    if (local->m_entityData.currentAbility == Ability_54)
        local->m_entityData.currentAbility = AbilityNone;
    if (dodge) {
        local = m_world->m_localCreature;
        if (local->m_entityData.acceleration.squaredLength() > 0.0f && local->m_stamina >= StaminaMoveCost &&
            local->m_entityData.rollTime <= 0.0f) {
            local->m_entityData.currentAbility = AbilityNone;
            local = m_world->m_localCreature;
            plasma::Vector<3, float> direction(local->m_entityData.acceleration[0], local->m_entityData.acceleration[1],
                                               0.0f);
            direction.normalize();
            local->m_entityData.velocity = direction * 15.0f + plasma::Vector<3, float>(0.0f, 0.0f, 5.0f);
            m_world->m_localCreature->m_entityData.rollTime = DodgeRollDurationMs;
            m_world->m_localCreature->m_stamina -= StaminaMoveCost;
        }
    }

    static bool jumpWasDown; // zero-initialized
    static int jumpHeldTime; // zero-initialized
    static int jumpRiseTime; // zero-initialized
    int riseDuration = 100;
    local = m_world->m_localCreature;
    if (local->m_entityData.currentAbility == AbilityCarryCreature) {
        Creature *target = m_world->creatureByID(local->m_entityData.focusId);
        if (target && (target->m_entityData.appearance.appearanceFlags & AppearanceFlagFlying) != 0)
            riseDuration = 500;
    }
    bool jumping = m_controlStates[Controls::ButtonJump];
    if (jumping && (local->m_entityData.currentAbility != AbilityTeleport ||
                    local->m_entityData.timeSinceAbility > (float)local->attackDuration())) {
        local = m_world->m_localCreature;
        if (local->m_entityData.currentAbility == AbilityClimb && !jumpWasDown) {
            local->m_entityData.currentAbility = AbilityNone;
            local = m_world->m_localCreature;
            if ((local->m_entityData.flags & CreatureFlagWallAboveFeet) != 0) {
                local->m_entityData.velocity = local->m_wallNormal * 15.0f;
                local = m_world->m_localCreature;
                local->m_entityData.acceleration = local->m_wallNormal * 15.0f;
                local = m_world->m_localCreature;
                local->m_entityData.retreat = local->m_wallNormal * 15.0f;
                m_world->m_localCreature->m_entityData.rollTime = DodgeRollDurationMs;
                local = m_world->m_localCreature;
                local->m_entityData.rotation[2] =
                    interpolateAngle(local->m_entityData.rotation[2], local->m_entityData.rotation[2] + 180.0f, 1.0f);
                m_world->m_localCreature->m_stamina -= 25.0f;
                local = m_world->m_localCreature;
                if (local->m_stamina <= 0.0f)
                    local->m_stamina = 0.0f;
            }
        }
        jumpHeldTime = (int)((double)jumpHeldTime + step);
        if (jumpHeldTime > 0) {
            if (jumpHeldTime < 200) {
                local = m_world->m_localCreature;
                if (((local->m_entityData.flags & CreatureFlagOnGround) != 0 || local->m_entityData.rollTime != 0.0f) &&
                    local->m_entityData.velocity[2] <= 0.0f) {
                    unsigned char ability = local->m_entityData.currentAbility;
                    bool smallJump = ability == AbilityFloat ||
                                     ((ability == AbilityHealingStream || ability == AbilityCenteredHealingStream) &&
                                      (float)local->attackDuration() > local->m_entityData.timeSinceAbility);
                    m_world->m_localCreature->m_entityData.velocity[2] = smallJump ? 5.0f : 10.0f;
                    if (m_controlStates[Controls::ButtonClassSkill]) {
                        local = m_world->m_localCreature;
                        if (local->m_stamina >= StaminaMoveCost &&
                            ((local->m_entityData.classType == ClassRogue &&
                              local->m_entityData.specialization == SpecializationNinja) ||
                             (local->m_entityData.classType == ClassRanger &&
                              local->m_entityData.specialization == SpecializationScout))) {
                            local->m_entityData.rollTime = DodgeRollDurationMs;
                            m_world->m_localCreature->m_entityData.velocity[2] = 20.0f;
                            m_world->m_localCreature->m_stamina -= StaminaMoveCost;
                        }
                        if (m_controlStates[Controls::ButtonClassSkill]) {
                            local = m_world->m_localCreature;
                            if (local->m_entityData.currentAbility != AbilityWarriorJumpAttack &&
                                local->m_stamina >= StaminaMoveCost && local->m_entityData.classType == ClassWarrior) {
                                local->m_entityData.currentAbility = AbilityWarriorJumpAttack;
                                m_world->m_localCreature->m_entityData.timeSinceAbility = 0.0f;
                                m_world->m_localCreature->m_entityData.velocity[2] = 20.0f;
                                m_world->m_localCreature->m_stamina -= StaminaMoveCost;
                            }
                        }
                    }
                    jumpRiseTime = riseDuration;
                    m_world->m_localCreature->m_entityData.retreat = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
                }
            }
            if (jumpHeldTime > 0) {
                local = m_world->m_localCreature;
                if ((local->m_entityData.flags & CreatureFlagInWater) != 0 && local->m_entityData.velocity[2] < 10.0f) {
                    jumpRiseTime = riseDuration;
                    local->m_entityData.velocity[2] =
                        (float)(step * 0.0010000000474974513 + (double)local->m_entityData.velocity[2]);
                }
            }
        }
    } else {
        jumpHeldTime = 0;
    }
    if (jumpRiseTime > 0 && m_controlStates[Controls::ButtonJump]) {
        local = m_world->m_localCreature;
        unsigned char ability = local->m_entityData.currentAbility;
        if (ability != AbilityFloat && !((ability == AbilityHealingStream || ability == AbilityCenteredHealingStream) &&
                                         (float)local->attackDuration() > local->m_entityData.timeSinceAbility)) {
            local = m_world->m_localCreature;
            if (local->m_entityData.velocity[2] < 10.0f) {
                local->m_entityData.velocity[2] =
                    (float)(step * 0.07999999821186066 + (double)local->m_entityData.velocity[2]);
                local = m_world->m_localCreature;
                if (local->m_entityData.velocity[2] >= 10.0f)
                    local->m_entityData.velocity[2] = 10.0f;
            }
        }
    }
    jumpRiseTime = (std::max)((int)((double)jumpRiseTime - step), 0);
    jumpWasDown = m_controlStates[Controls::ButtonJump];
}

void Game::controlCharacter(double step, std::vector<NearbyCreature> &creatures)
{
    // The mod's alpha classes have no class skill (Shift) and no ultimate (R): the keys 1 to 3 start the class's skills.
    bool alphaClasses = mod::alphaClasses();
    if (alphaClasses) {
        // R talks and opens, as in the alpha; the release's use control (E) does that and picks up.
        if (m_controlStates[Controls::ButtonUltimateSkill]) {
            m_controlStates[Controls::ButtonUse] = true;
        }
        m_controlStates[Controls::ButtonClassSkill] = false;
        m_controlStates[Controls::ButtonUltimateSkill] = false;
    }
    m_interface.m_inGame.m_crosshair->setText(L"");
    static bool mousePressed; // previous mouse button 0 state
    if (m_engine->isButtonDown(0) && !mousePressed) {
        if (m_world->m_localCreature->m_inventory.m_cursorCount != 0 &&
            m_interface.m_inGame.m_equipmentInventoryWidget->getNode()->isVisible() && !m_engine->getPointerNode()) {
            Interaction interaction;
            interaction.type = InteractionDrop;
            interaction.item = std::move(m_world->m_localCreature->m_inventory.m_cursorItem);
            m_world->m_localCreature->m_interactions.push_back(interaction);
            Creature *player = m_world->m_localCreature;
            if (--player->m_inventory.m_cursorCount <= 0) {
                player->m_inventory.m_cursorCount = 0;
                player->m_inventory.m_cursorItem.category = ItemCategoryNone;
                player->m_inventory.m_cursorItem.id = NoneCleared;
            }
        }
    }
    mousePressed = m_engine->isButtonDown(0) != 0;
    Creature *player = m_world->m_localCreature;
    if (player->m_entityData.currentAbility == AbilityClimb && !(player->m_entityData.flags & CreatureFlagAtWall) &&
        (player->m_entityData.flags & (CreatureFlagOnGround | CreatureFlagInWater)))
        player->m_entityData.currentAbility = AbilityNone;

    const plasma::Matrix<float> &camera = m_cameraMatrix;
    plasma::Vector<3, float> direction(camera[4] * 0.0f + camera[0] * 0.0f + camera[8],
                                       camera[5] * 0.0f + camera[1] * 0.0f + camera[9],
                                       camera[6] * 0.0f + camera[2] * 0.0f + camera[10]);
    plasma::Vector<3, Fixed> lookPoint = m_cameraFocusPosition + plasma::Vector<3, Fixed>(direction * 100.0f);
    float rayDistance = m_world->rayDistance(m_cameraFocusPosition, direction, 60.0f, false, true, nullptr);
    if (rayDistance >= 0.0f) {
        lookPoint = m_cameraFocusPosition + plasma::Vector<3, Fixed>(direction * rayDistance);
    }
    long long closestCreatureId = 0;
    plasma::Vector<3, int> closestEntity(INT_MAX, INT_MAX, 0);
    plasma::Vector<3, int> closestItem(INT_MAX, INT_MAX, 0);
    float itemDistance = -1.0f;
    float crosshairDistance = -1.0f;
    float attackDistance = -1.0f;
    float nearbyDistance = -1.0f;
    float visibleDistance = -1.0f;
    long long friendlyCreatureId = 0;
    float friendlyDistance = -1.0f;
    bool harmful =
        m_world->m_localCreature->m_entityData.timeSinceAbility <= m_world->m_localCreature->attackDuration() &&
        isHarmfulSkill(m_world->m_localCreature->m_entityData.currentAbility);
    bool healing =
        m_world->m_localCreature->m_entityData.timeSinceAbility <= m_world->m_localCreature->attackDuration() &&
        isHealingSkill(m_world->m_localCreature->m_entityData.currentAbility);
    for (NearbyCreature &nearby : creatures) {
        player = m_world->m_localCreature;
        if (nearby.creature == player ||
            (player->m_entityData.currentAbility == AbilityRidePet && nearby.creature->m_id == player->m_petId))
            continue;
        if (player->m_entityData.currentAbility == AbilityHeal &&
            player->m_entityData.timeSinceAbility < player->attackDuration() &&
            (nearby.creature->m_entityData.hostilityType == HostilityEnemy ||
             nearby.creature->m_entityData.hostilityType == HostilityAttackableObject))
            continue;
        if (nearby.creature->isGate() && nearby.creature->m_entityData.interactionState)
            continue;
        if (nearby.creature->m_entityData.race == RaceWaterBubble ||
            (nearby.creature->m_entityData.race == RaceClimbingPlant &&
             nearby.creature->m_entityData.interactionState) ||
            (harmful && !m_world->m_localCreature->canDoDamage(nearby.creature)) ||
            (healing && m_world->m_localCreature->canDoDamage(nearby.creature)))
            continue;
        plasma::Vector<3, Fixed> loweredPosition =
            nearby.creature->m_drawnPosition +
            plasma::Vector<3, Fixed>(plasma::Vector<3, float>(0.0f, 0.0f, -nearby.creature->m_stepOffset));
        if (!m_renderer->isSphereVisible(loweredPosition, nearby.creature->m_entityData.appearance.size[0], 256.0f))
            continue;
        plasma::Vector<3, Fixed> focusOffset = nearby.creature->m_drawnPosition - m_cameraFocusPosition;
        bool left = false, below = false, right = false, above = false;
        for (int x = 0; x < 2; ++x) {
            for (int y = 0; y < 2; ++y) {
                for (int z = 0; z < 2; ++z) {
                    plasma::Vector<3, Fixed> corner(
                        Fixed((nearby.creature->m_entityData.appearance.size[0] + 1.0f) * (float)x -
                              (nearby.creature->m_entityData.appearance.size[0] + 1.0f) * 0.5f),
                        Fixed((nearby.creature->m_entityData.appearance.size[1] + 1.0f) * (float)y -
                              (nearby.creature->m_entityData.appearance.size[1] + 1.0f) * 0.5f),
                        Fixed((nearby.creature->m_entityData.appearance.size[2] + 1.0f) * (float)z -
                              (nearby.creature->m_entityData.appearance.size[2] + 1.0f) * 0.5f));
                    plasma::Vector<3, Fixed> relative =
                        nearby.creature->m_drawnPosition +
                        plasma::Vector<3, Fixed>(plasma::Vector<3, float>(0.0f, 0.0f, -nearby.creature->m_stepOffset)) +
                        corner + plasma::Vector<3, Fixed>(m_drawOriginX, m_drawOriginY, Fixed(0));
                    plasma::Vector<3, float> point((float)relative[0], (float)relative[1], (float)relative[2]);
                    point = m_viewMatrix * point;
                    if (point[2] > 0.0f) {
                        point = m_projectionMatrix * point;
                        if (point[0] < 0.0f)
                            left = true;
                        if (point[1] < 0.0f)
                            below = true;
                        if (point[0] > 0.0f)
                            right = true;
                        if (point[1] > 0.0f)
                            above = true;
                    }
                }
            }
        }
        plasma::Vector<3, Fixed> delta =
            m_world->m_localCreature->m_entityData.position - nearby.creature->m_drawnPosition;
        // Signed 64-bit square products and their sum can overflow.
        float squaredDistance = (float)delta.squaredLength();
        if (squaredDistance < 16.0f && (nearbyDistance < 0.0f || squaredDistance < nearbyDistance)) {
            nearbyDistance = squaredDistance;
            closestCreatureId = nearby.creature->m_id;
        }
        if (!left || !below || !right)
            continue;
        if (above)
            m_creatureInView = true;
        if (!above)
            continue;
        loweredPosition =
            nearby.creature->m_drawnPosition +
            plasma::Vector<3, Fixed>(plasma::Vector<3, float>(0.0f, 0.0f, -nearby.creature->m_stepOffset));
        if (!m_world->inRange(m_cameraPosition, loweredPosition, false, 256.0f, nearby.creature))
            continue;
        nearby.inClearView = true;
        plasma::Vector<3, Fixed> cameraDelta = nearby.creature->m_drawnPosition - m_cameraPosition;
        plasma::Vector<3, float> blocks((float)cameraDelta[0], (float)cameraDelta[1], (float)cameraDelta[2]);
        float alongRay = blocks[1] * direction[1] + blocks[0] * direction[0] + blocks[2] * direction[2];
        plasma::Vector<3, Fixed> projected = m_cameraPosition + plasma::Vector<3, Fixed>(direction * alongRay);
        if (alongRay < 0.0f)
            continue;
        plasma::Vector<3, Fixed> miss = projected - nearby.creature->m_drawnPosition;
        plasma::Vector<3, float> missBlocks((float)miss[0], (float)miss[1], (float)miss[2]);
        float missDistance =
            sqrtf(missBlocks[0] * missBlocks[0] + missBlocks[1] * missBlocks[1] + missBlocks[2] * missBlocks[2]);
        if (crosshairDistance < 0.0f || missDistance < crosshairDistance) {
            crosshairDistance = missDistance;
            m_crosshairCreatureId = nearby.creature->m_id;
        }
        bool attackHostile = m_world->m_localCreature->m_entityData.currentAbility != AbilityHealingStream &&
                             m_world->m_localCreature->m_entityData.currentAbility != AbilityCenteredHealingStream;
        if (m_world->m_localCreature->canDoDamage(nearby.creature) == attackHostile &&
            (attackDistance < 0.0f || alongRay < attackDistance))
            attackDistance = alongRay;
        if (visibleDistance < 0.0f || squaredDistance < visibleDistance)
            visibleDistance = squaredDistance;
        if (((nearby.creature->m_entityData.hostilityType == HostilityEnemy ||
              nearby.creature->m_entityData.hostilityType == HostilityAttackableObject) &&
             friendlyDistance < 0.0f) ||
            squaredDistance < friendlyDistance) {
            friendlyDistance = squaredDistance;
            friendlyCreatureId = nearby.creature->m_id;
        }
    }
    if (!m_world->m_localCreature->isGliding() &&
        m_world->m_localCreature->m_entityData.currentAbility != AbilitySailBoat)
        m_world->m_localCreature->m_entityData.acceleration = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
    if (m_world->m_localCreature->m_entityData.stunTime <= 0.0f &&
        m_world->m_localCreature->m_entityData.actionLockTime <= 0.0f)
        moveCharacter(step);
    if (m_engine->getFocusWidget() || m_interface.m_inGame.m_chatWidget->m_typeboxActive)
        return;
    if (m_world->m_localCreature->isGliding() && m_world->m_localCreature->m_stamina < 0.001f)
        return;
    player = m_world->m_localCreature;
    if (player->m_entityData.currentAbility == Ability_125 && player->m_entityData.timeSinceAbility < 2000.0f)
        return;
    if (player->m_entityData.currentAbility == AbilityHangGlide &&
        !(player->m_entityData.flags & CreatureFlagOnGround)) {
        player->m_entityData.setToggles(1 << CreatureToggleFastMovement,
                                        player->m_stamina > 0.0f && m_controlStates[Controls::ButtonJump]);
    } else {
        player->m_entityData.setToggles(1 << CreatureToggleFastMovement, !m_controlStates[Controls::ButtonWalk]);
    }
    static bool togglePressed; // control 12 rising edge
    if (!togglePressed && m_controlStates[Controls::ButtonLamp])
        m_world->m_localCreature->m_entityData.setToggles(1 << CreatureToggleLampOn,
                                                          !m_world->m_localCreature->m_entityData.binaryToggles.lampOn);
    togglePressed = m_controlStates[Controls::ButtonLamp];
    player = m_world->m_localCreature;
    // Keeps indefinite abilities active; ends other abilities ten seconds after they start.
    if (player->m_entityData.currentAbility != AbilityFlightToPoint &&
        player->m_entityData.currentAbility != AbilityEagleFlight &&
        player->m_entityData.currentAbility != AbilityMenuView && player->m_entityData.currentAbility != AbilitySit &&
        player->m_entityData.currentAbility != AbilityRidePet &&
        player->m_entityData.currentAbility != AbilityCarryCreature &&
        player->m_entityData.currentAbility != AbilitySailBoat &&
        player->m_entityData.currentAbility != AbilityHangGlide &&
        player->m_entityData.currentAbility != AbilityClimb && player->m_entityData.currentAbility != AbilitySprint &&
        player->m_entityData.currentAbility != AbilityCyclone && player->m_entityData.currentAbility != AbilityFloat &&
        player->m_entityData.currentAbility != AbilityFlameRush &&
        player->m_entityData.currentAbility != AbilityOfferPetFood &&
        player->m_entityData.currentAbility != AbilitySleep && player->m_entityData.currentAbility != AbilityBlock &&
        player->m_entityData.currentAbility != Ability_36 && player->m_entityData.currentAbility != AbilitySneak &&
        player->m_entityData.currentAbility != AbilitySprint && player->m_entityData.currentAbility != AbilityFloat &&
        player->m_entityData.currentAbility != AbilitySupportRay && player->m_entityData.currentAbility != Ability_30 &&
        player->m_entityData.currentAbility != AbilityFireballCharge &&
        !m_controlStates[Controls::ButtonSpecialAttack] &&
        player->m_entityData.timeSinceAbility > player->attackDuration() &&
        m_world->m_localCreature->m_entityData.timeSinceAbility > 10000.0f)
        m_world->m_localCreature->m_entityData.currentAbility = AbilityNone;
    std::vector<int> attackSkills;
    player = m_world->m_localCreature;
    int rightWeaponId = player->m_entityData.equipment.weaponRight.id;
    if (rightWeaponId == WeaponStaff) {
        if (player->m_entityData.specialization == SpecializationWater) {
            {
                int skill = AbilityWaterSwirl;
                attackSkills.push_back(std::move(skill));
            }
            {
                int skill = AbilityWaterVortex;
                attackSkills.push_back(std::move(skill));
            }
        } else {
            {
                int skill = AbilityFireSwirl;
                attackSkills.push_back(std::move(skill));
            }
            {
                int skill = AbilityFireVortex;
                attackSkills.push_back(std::move(skill));
            }
        }
    } else if (rightWeaponId == WeaponBracelet) {
        if (player->m_entityData.specialization == SpecializationWater) {
            {
                int skill = AbilityBraceletWaterbolt;
                attackSkills.push_back(std::move(skill));
            }
            {
                int skill = AbilityWaterball;
                attackSkills.push_back(std::move(skill));
            }
        } else {
            {
                int skill = AbilityBraceletFirebolt;
                attackSkills.push_back(std::move(skill));
            }
            {
                int skill = AbilityFireball;
                attackSkills.push_back(std::move(skill));
            }
        }
    } else if (rightWeaponId == WeaponWand) {
        if (player->m_entityData.specialization == SpecializationWater) {
            {
                int skill = AbilityWaterBeam;
                attackSkills.push_back(std::move(skill));
            }
            {
                int skill = AbilityWaterRay;
                attackSkills.push_back(std::move(skill));
            }
        } else {
            {
                int skill = AbilityFireBeam;
                attackSkills.push_back(std::move(skill));
            }
            {
                int skill = AbilityFireRay;
                attackSkills.push_back(std::move(skill));
            }
        }
    } else {
        attackSkills.push_back(player->nextAttackSkill(false));
        player = m_world->m_localCreature;
        int skill;
        if (player->m_entityData.equipment.weaponRight.id == WeaponLongsword)
            skill = AbilityPerforate;
        else if (player->m_entityData.equipment.weaponRight.id == WeaponDagger)
            skill = AbilityAmbush;
        else if (player->m_entityData.equipment.weaponRight.id == WeaponFist)
            skill = AbilityKick;
        else if (player->m_entityData.equipment.weaponRight.id == WeaponBow ||
                 player->m_entityData.equipment.weaponLeft.id == WeaponBow)
            skill = AbilitySalvo;
        else if (player->m_entityData.equipment.weaponRight.id == WeaponCrossbow ||
                 player->m_entityData.equipment.weaponLeft.id == WeaponCrossbow)
            skill = AbilityChargedShot;
        else if (player->m_entityData.equipment.weaponRight.id == WeaponBoomerang)
            skill = AbilityChargedBoomerang;
        else if (player->hasTwoHandedWeapon())
            skill = AbilityCrush;
        else if (m_world->m_localCreature->m_entityData.equipment.weaponLeft.id == WeaponShield)
            skill = AbilityBlock;
        else
            skill = AbilitySwirl;
        attackSkills.push_back(std::move(skill));
    }
    if (m_highlightedCreatureId == 0 && attackDistance >= 0.0f &&
        m_world->m_localCreature->m_entityData.currentAbility != AbilityTeleport) {
        plasma::Vector<3, Fixed> attackPoint = m_cameraPosition + plasma::Vector<3, Fixed>(direction * attackDistance);
        plasma::Vector<3, Fixed> attackDelta = attackPoint - m_world->m_localCreature->m_entityData.position;
        float alongRay = (float)attackDelta[1] * direction[1] + (float)attackDelta[0] * direction[0] +
                         (float)attackDelta[2] * direction[2];
        if (alongRay > 0.0f)
            lookPoint = attackPoint;
    }
    if (m_world->m_localCreature->m_entityData.currentAbility == AbilityAssassinate)
        m_world->m_localCreature->attackDuration();
    player = m_world->m_localCreature;
    if (player->m_entityData.currentAbility == AbilityFireSwirl ||
        player->m_entityData.currentAbility == AbilityFireVortex ||
        player->m_entityData.currentAbility == AbilityWaterSwirl ||
        player->m_entityData.currentAbility == AbilityWaterVortex) {
        double previousTime = (double)player->m_entityData.timeSinceAbility - step;
        if (player->anticipationTime(AbilityUseCurrent) >= previousTime)
            m_world->m_localCreature->m_abilityTargetPosition = lookPoint;
        else
            lookPoint = m_world->m_localCreature->m_abilityTargetPosition;
    }
    m_world->m_localCreature->m_entityData.binaryToggles.playerAi = 0;
    plasma::Vector<3, Fixed> aimDelta = lookPoint - m_world->m_localCreature->m_entityData.position;
    m_world->m_localCreature->m_entityData.attackRotation = aimDelta;
    static bool specialPressed; // control 3 rising edge
    if (m_controlStates[Controls::ButtonUltimateSkill] && !specialPressed &&
        m_world->m_localCreature->specialSkill() != 0) {
        player = m_world->m_localCreature;
        if (player->canUseSkill(player->specialSkill(), true)) {
            int special = m_world->m_localCreature->specialSkill();
            if (special == AbilityCamouflage || special == AbilityNinjutsu) {
                player = m_world->m_localCreature;
                m_world->applyBuffSkill(player, player->specialSkill());
            } else {
                player = m_world->m_localCreature;
                player->m_entityData.currentAbility = (unsigned char)player->specialSkill();
                m_world->m_localCreature->m_entityData.timeSinceAbility = 0.0f;
                m_world->m_localCreature->m_entityData.rollTime = 0.0f;
            }
            int ability = m_world->m_localCreature->m_entityData.currentAbility;
            player = m_world->m_localCreature;
            float &cooldown = player->m_skillCooldowns[ability];
            cooldown = (float)player->cooldown(player->m_entityData.currentAbility);
        }
    }
    m_highlightedCreatureId = 0;
    specialPressed = m_controlStates[Controls::ButtonUltimateSkill];
    if (m_crosshairCreatureId &&
        m_gamepadWalkVector[0] * m_gamepadWalkVector[0] + m_gamepadWalkVector[1] * m_gamepadWalkVector[1] > 0.1f) {
        Creature *target = m_world->creatureByID(m_crosshairCreatureId);
        if (target && target->m_entityData.hp > 0.0f) {
            plasma::Vector<3, Fixed> targetDelta = target->m_drawnPosition - m_cameraFocusPosition;
            plasma::Vector<3, float> targetDirection((float)targetDelta[0], (float)targetDelta[1],
                                                     (float)targetDelta[2]);
            targetDirection[2] += target->m_entityData.appearance.size[2] * 0.45f;
            float projectileSpeed = 150.0f;
            if (!m_world->m_localCreature->isAttackingFromRange())
                projectileSpeed = 500.0f;
            plasma::Vector<3, Fixed> playerDelta =
                m_world->m_localCreature->m_entityData.position - target->m_drawnPosition;
            plasma::Vector<3, float> playerBlocks((float)playerDelta[0], (float)playerDelta[1], (float)playerDelta[2]);
            float travelTime = sqrtf(playerBlocks[0] * playerBlocks[0] + playerBlocks[1] * playerBlocks[1] +
                                     playerBlocks[2] * playerBlocks[2]) /
                               projectileSpeed;
            targetDirection += target->m_entityData.velocity * travelTime;
            plasma::Vector<3, float> targetFlat(targetDirection[0], targetDirection[1], 0.0f);
            plasma::Vector<3, float> cameraFlat(direction[0], direction[1], 0.0f);
            plasma::Vector<3, float> targetAngle(m_cameraAngle);
            float horizontalLengthSquared =
                targetDirection[1] * targetDirection[1] + targetDirection[0] * targetDirection[0];
            if (horizontalLengthSquared + 0.0f > 0.0f &&
                direction[1] * direction[1] + direction[0] * direction[0] + 0.0f > 0.0f) {
                targetFlat.normalize();
                cameraFlat.normalize();
                float angle = asinf(cameraFlat[0] * targetFlat[1] - cameraFlat[1] * targetFlat[0]);
                targetAngle[2] += angle / Pi * 180.0;
            }
            plasma::Vector<3, float> targetUnit(targetDirection);
            float lengthSquared = targetDirection[2] * targetDirection[2] + horizontalLengthSquared;
            if (lengthSquared > 0.0f) {
                targetUnit.normalize();
                float pitch = acosf(targetUnit[2]);
                targetAngle[0] = (float)((double)pitch / Pi * 180.0);
            }
            targetUnit = targetDirection;
            plasma::Vector<3, float> cameraUnit(direction);
            if (lengthSquared > 0.0f &&
                direction[1] * direction[1] + direction[0] * direction[0] + direction[2] * direction[2] > 0.0f) {
                targetUnit.normalize();
                cameraUnit.normalize();
                float crossZ = cameraUnit[1] * targetUnit[0] - cameraUnit[0] * targetUnit[1];
                float crossY = cameraUnit[0] * targetUnit[2] - targetUnit[0] * cameraUnit[2];
                float crossX = targetUnit[1] * cameraUnit[2] - cameraUnit[1] * targetUnit[2];
                float crossLength = sqrtf(crossX * crossX + crossY * crossY + crossZ * crossZ);
                float speed = 0.015f;
                if (m_world->m_localCreature->m_entityData.rollTime != 0.0f)
                    speed = 0.03f;
                float factor = (1.0f - expf((float)(-((double)speed * step)))) * expf(crossLength * -20.0f);
                m_targetCameraAngle[2] =
                    (double)interpolateAngle((float)m_targetCameraAngle[2], targetAngle[2], factor);
            }
        }
    }
    player = m_world->m_localCreature;
    if (player->m_entityData.currentAbility == AbilityFeedClimbingPlant) {
        player->m_entityData.binaryToggles.faceAim = 0;
        if (m_world->m_localCreature->attackDuration() > m_world->m_localCreature->m_entityData.timeSinceAbility) {
            player = m_world->m_localCreature;
            Creature *target = m_world->creatureByID(player->m_entityData.ownerId);
            if (target) {
                plasma::Vector<3, Fixed> delta = target->m_drawnPosition - player->m_entityData.position;
                m_world->m_localCreature->m_entityData.attackRotation = delta;
                m_world->m_localCreature->m_entityData.binaryToggles.faceAim = 1;
            }
        }
    }
    player = m_world->m_localCreature;
    int firstZoneX = zoneCoordinate(player->m_entityData.position[0] - Fixed(8));
    int lastZoneX = zoneCoordinate(player->m_entityData.position[0] + Fixed(8));
    int firstZoneY = zoneCoordinate(player->m_entityData.position[1] - Fixed(8));
    int lastZoneY = zoneCoordinate(player->m_entityData.position[1] + Fixed(8));
    float entityCrosshairDistance = -1.0f;
    float entityNearbyDistance = -1.0f;
    for (int zoneX = firstZoneX; zoneX <= lastZoneX; ++zoneX) {
        for (int zoneY = firstZoneY; zoneY <= lastZoneY; ++zoneY) {
            Zone *zone = m_world->zone(zoneX, zoneY);
            if (!zone)
                continue;
            for (int index = 0; index < (int)zone->m_staticEntities.size(); ++index) {
                StaticEntity &entity = zone->m_staticEntities[index];
                // The types with an interaction.
                bool hasInteraction =
                    entity.type == StaticEntityGoddess2 || entity.type == StaticEntityDoor ||
                    entity.type == StaticEntityBigDoor || entity.type == StaticEntityBigIronDoor ||
                    entity.type == StaticEntityWindow || entity.type == StaticEntityChestBase02 ||
                    entity.type == StaticEntityRewardChest || entity.type == StaticEntityStoneStool ||
                    entity.type == StaticEntityStool || entity.type == StaticEntitySandstoneStool ||
                    entity.type == StaticEntityBench || entity.type == StaticEntityBenchBlack ||
                    entity.type == StaticEntityBed || entity.type == StaticEntityRunestone ||
                    entity.type == StaticEntityCampfire || entity.type == StaticEntityCageBase ||
                    entity.type == StaticEntityCage2Base || entity.type == StaticEntityCage3Base ||
                    entity.type == StaticEntityCage4Base || entity.type == StaticEntityBeachTowel ||
                    entity.type == StaticEntityWoodMat || entity.type == StaticEntityFurnace ||
                    entity.type == StaticEntityAnvil || entity.type == StaticEntitySpinningwheel ||
                    entity.type == StaticEntityLoom || entity.type == StaticEntitySawbench ||
                    entity.type == StaticEntityWorkbench || entity.type == StaticEntityCustomizationBench ||
                    entity.type == StaticEntityDruidChair || entity.type == StaticEntityPedestal ||
                    entity.type == StaticEntityStoneTablet1 || entity.type == StaticEntityStoneTablet2 ||
                    entity.type == StaticEntityStoneTablet3 || entity.type == StaticEntityStoneTablet4 ||
                    entity.type == StaticEntityStoneTablet5 || entity.type == StaticEntityStoneTablet6 ||
                    entity.type == StaticEntityInscribedTablet || entity.type == StaticEntityBrazier ||
                    entity.type == StaticEntitySarcophagus;
                if (!hasInteraction && entity.item.category == ItemCategoryNone)
                    continue;
                // Signed 64-bit square products and their sum can overflow.
                plasma::Vector<3, Fixed> delta = m_world->m_localCreature->m_entityData.position - entity.position;
                float squaredDistance = (float)delta.squaredLength();
                if (squaredDistance > 16.0f)
                    continue;
                float halfHeight = entity.scaledSize()[2] * 0.5f;
                plasma::Vector<3, Fixed> cameraDelta =
                    m_cameraPosition - entity.position -
                    plasma::Vector<3, Fixed>(plasma::Vector<3, float>(0.0f, 0.0f, halfHeight));
                float cameraDistance = (float)(cameraDelta[0] * cameraDelta[0] + cameraDelta[1] * cameraDelta[1] +
                                               cameraDelta[2] * cameraDelta[2]);
                if (entityNearbyDistance < 0.0f || squaredDistance < entityNearbyDistance) {
                    halfHeight = entity.scaledSize()[2] * 0.5f;
                    plasma::Vector<3, Fixed> center =
                        entity.position + plasma::Vector<3, Fixed>(plasma::Vector<3, float>(0.0f, 0.0f, halfHeight));
                    if (m_world->inRange(m_world->m_localCreature->m_entityData.position, center, false, 8.0f,
                                         nullptr)) {
                        entityNearbyDistance = squaredDistance;
                        closestCreatureId = 0;
                        closestEntity = plasma::Vector<3, int>(zoneX, zoneY, index);
                    }
                }
                if (!(entityCrosshairDistance < 0.0f || cameraDistance < entityCrosshairDistance))
                    continue;
                halfHeight = entity.scaledSize()[2] * 0.5f;
                plasma::Vector<3, Fixed> center =
                    entity.position + plasma::Vector<3, Fixed>(plasma::Vector<3, float>(0.0f, 0.0f, halfHeight));
                if (!m_world->inRange(m_world->m_localCreature->m_entityData.position, center, false, 8.0f, nullptr))
                    continue;
                plasma::Vector<3, float> size = entity.scaledSize();
                if (entity.orientation & 1)
                    std::swap(size[0], size[1]);
                bool left = false, right = false, below = false, above = false;
                for (int x = 0; x < 2; ++x) {
                    for (int y = 0; y < 2; ++y) {
                        for (int z = 0; z < 2; ++z) {
                            plasma::Vector<3, Fixed> corner(Fixed((float)x * size[0] - size[0] * 0.5f),
                                                            Fixed((float)y * size[1] - size[1] * 0.5f),
                                                            Fixed((float)z * size[2]));
                            plasma::Vector<3, Fixed> relative =
                                entity.position + corner +
                                plasma::Vector<3, Fixed>(m_drawOriginX, m_drawOriginY, Fixed(0));
                            plasma::Vector<3, float> point((float)relative[0], (float)relative[1], (float)relative[2]);
                            point = m_viewMatrix * point;
                            if (point[2] > 0.0f) {
                                point = m_projectionMatrix * point;
                                if (point[0] < 0.0f)
                                    left = true;
                                if (point[0] > 0.0f)
                                    right = true;
                                if (point[1] < 0.0f)
                                    below = true;
                                if (point[1] > 0.0f)
                                    above = true;
                            }
                        }
                    }
                }
                if (left && right && below && above) {
                    entityCrosshairDistance = cameraDistance;
                    m_crosshairCreatureId = 0;
                    m_crosshairObject = plasma::Vector<3, int>(zoneX, zoneY, index);
                }
            }
        }
    }
    if (m_world->m_localCreature->m_entityData.hp <= 0.0f) {
        m_crosshairCreatureId = 0;
        m_crosshairObject[0] = INT_MIN;
        m_crosshairObject[1] = INT_MIN;
        m_crosshairObject[2] = 0;
    }
    for (int zoneX = firstZoneX; zoneX <= lastZoneX; ++zoneX) {
        for (int zoneY = firstZoneY; zoneY <= lastZoneY; ++zoneY) {
            Zone *zone = m_world->zone(zoneX, zoneY);
            if (!zone)
                continue;
            for (int index = 0; index < (int)zone->m_items.size(); ++index) {
                ZoneItem &zoneItem = zone->m_items[index];
                if (!m_world->isGroundItemAvailable(zoneItem))
                    continue;
                float squaredDistance =
                    plasma::Vector<3, float>(m_world->m_localCreature->m_entityData.position - zoneItem.position)
                        .squaredLength();
                if (squaredDistance > 16.0f)
                    continue;
                if ((zoneItem.item.category == ItemCategoryCoin || zoneItem.item.category == ItemCategoryManaCube) &&
                    !(zoneItem.flags & ZoneItemFlagPerPlayer)) {
                    if (zoneItem.dropTime != 0.0f)
                        continue;
                    Interaction pickup;
                    pickup.type = InteractionPickUp;
                    pickup.position = plasma::Vector<3, int>(zoneX, zoneY, index);
                    pickup.item = std::move(zoneItem.item);
                    bool queued = false;
                    for (const Interaction &interaction : m_world->m_localCreature->m_interactions) {
                        if (interaction.type == pickup.type && interaction.position == pickup.position &&
                            interaction.item == pickup.item) {
                            queued = true;
                            break;
                        }
                    }
                    if (!queued)
                        m_world->m_localCreature->m_interactions.push_back(pickup);
                    continue;
                }
                if (plasma::Vector<3, float>(m_world->m_localCreature->m_entityData.position - zoneItem.position)
                        .squaredLength() > 100.0f)
                    continue;
                float cameraDistance = plasma::Vector<3, float>(m_cameraPosition - zoneItem.position).squaredLength();
                if (!m_world->m_localCreature->canTakeItem(zoneItem.item)) {
                    squaredDistance += 16.0f;
                    cameraDistance += 16.0f;
                }
                if (itemDistance < 0.0f || squaredDistance < itemDistance) {
                    itemDistance = squaredDistance;
                    closestCreatureId = 0;
                    closestEntity = plasma::Vector<3, int>(INT_MAX, INT_MAX, 0);
                    closestItem = plasma::Vector<3, int>(zoneX, zoneY, index);
                }
                if (!(entityCrosshairDistance < 0.0f || cameraDistance < entityCrosshairDistance) ||
                    !m_world->inRange(m_cameraPosition, zoneItem.position, false, 200.0f, nullptr))
                    continue;
                Sprite *sprite = m_world->m_spriteManager->itemSprite(&zoneItem.item);
                if (!sprite)
                    continue;
                plasma::Matrix<float> itemMatrix;
                itemMatrix.identity();
                itemMatrix.translate((float)(m_drawOriginX + zoneItem.position[0]),
                                     (float)(m_drawOriginY + zoneItem.position[1]), (float)zoneItem.position[2]);
                float scaleZ = zoneItem.item.scale();
                float scaleY = zoneItem.item.scale();
                float scaleX = zoneItem.item.scale();
                itemMatrix.scale(scaleX, scaleY, scaleZ);
                unsigned char category = zoneItem.item.category;
                if (category >= ItemCategoryWeapon && category <= ItemCategoryChest) {
                    itemMatrix.rotateZ(zoneItem.rotation);
                    itemMatrix.translate(0.0f, 0.0f, (float)sprite->m_size[0] * 0.5f);
                    itemMatrix.rotateY(90.0f);
                    itemMatrix.translate((float)sprite->m_size[0] * -0.5f, (float)sprite->m_size[1] * -0.5f,
                                         (float)sprite->m_size[2] * -0.5f);
                } else if (category == ItemCategoryCoin || category == ItemCategoryPlatinumCoin) {
                    itemMatrix.translate(0.0f, 0.0f, (float)sprite->m_size[0] * 0.5f);
                    itemMatrix.rotateY(90.0f);
                    itemMatrix.translate((float)sprite->m_size[0] * -0.5f, (float)sprite->m_size[1] * -0.5f,
                                         (float)sprite->m_size[2] * -0.5f);
                } else {
                    itemMatrix.rotateZ(zoneItem.rotation);
                    itemMatrix.translate((float)sprite->m_size[0] * -0.5f, (float)sprite->m_size[1] * -0.5f, 0.0f);
                }
                bool left = false, right = false, below = false, above = false;
                for (int x = 0; x < 2; ++x) {
                    for (int y = 0; y < 2; ++y) {
                        for (int z = 0; z < 2; ++z) {
                            plasma::Vector<3, float> corner((float)(sprite->m_size[0] * x),
                                                            (float)(sprite->m_size[1] * y),
                                                            (float)(sprite->m_size[2] * z));
                            plasma::Vector<3, float> point = itemMatrix * corner;
                            point = m_viewMatrix * point;
                            if (point[2] > 0.0f) {
                                point = m_projectionMatrix * point;
                                if (point[0] < 0.0f)
                                    left = true;
                                if (point[0] > 0.0f)
                                    right = true;
                                if (point[1] < 0.0f)
                                    below = true;
                                if (point[1] > 0.0f)
                                    above = true;
                            }
                        }
                    }
                }
                if (left && right && below && above) {
                    entityCrosshairDistance = cameraDistance;
                    m_crosshairItem = plasma::Vector<3, int>(zoneX, zoneY, index);
                    m_crosshairCreatureId = 0;
                    m_crosshairObject = plasma::Vector<3, int>(INT_MAX, INT_MAX, 0);
                }
            }
        }
    }
    if (m_crosshairCreatureId == 0 && m_crosshairObject[0] == INT_MAX) {
        if (closestCreatureId) {
            Creature *target = m_world->creatureByID(closestCreatureId);
            if (target && target->m_entityData.hostilityType != HostilityEnemy &&
                target->m_entityData.hostilityType != HostilityAttackableObject)
                m_crosshairCreatureId = closestCreatureId;
        }
        if (closestEntity[0] != INT_MAX) {
            m_crosshairObject = closestEntity;
        }
    }
    if (m_crosshairItem[0] == INT_MAX && closestItem[0] != INT_MAX) {
        m_crosshairItem = closestItem;
    }
    if (m_crosshairCreatureId) {
        Creature *target = m_world->creatureByID(m_crosshairCreatureId);
        if (target && target->m_entityData.hostilityType != HostilityPlayer &&
            (target->m_entityData.hostilityType != HostilityFriendlyNpc ||
             target->m_entityData.classType == ClassShopSupplier ||
             (target->m_entityData.appearance.appearanceFlags & AppearanceFlagCaptive)) &&
            target->m_entityData.destination[0] != INT_MAX &&
            !m_world->m_localCreature->knowsMission(target->m_entityData.destination[0],
                                                    target->m_entityData.destination[1])) {
            m_world->m_localCreature->m_playerData->knownMissions.insert(
                std::array<int, 2> {target->m_entityData.destination[0], target->m_entityData.destination[1]});
            m_world->m_localCreature->m_playerData->discoveredPlaces.insert(
                std::array<int, 2> {target->m_entityData.destination[0], target->m_entityData.destination[1]});
            m_world->m_localCreature->m_playerData->missionTimers[std::array<int, 2> {
                target->m_entityData.destination[0], target->m_entityData.destination[1]}] = 180000.0f;
        }
    }
    static bool usePressed; // control 11 rising edge
    if (interactionPrompt()) {
        if (!m_interface.m_inGame.m_selector->isVisible()) {
            m_crosshairItem = plasma::Vector<3, int>(INT_MAX, INT_MAX, INT_MAX);
            if ((m_controlStates[Controls::ButtonUse] && !usePressed) &&
                m_world->m_localCreature->m_entityData.hp > 0.0f && m_crosshairObject[0] >= 0 &&
                !m_interface.m_inGame.m_speechWidget->getNode()->isVisible()) {
                m_world->object(m_crosshairObject);
            }
        }
    } else if (m_interface.m_inGame.m_selector->isVisible()) {
        if ((m_controlStates[Controls::ButtonUse] && !usePressed)) {
            std::vector<plasma::Vector<2, int>> food = getFoodSlots();
            if (!food.empty()) {
                long long selected = m_ringSelection;
                if (selected < 0) {
                    selected = (selected + ((unsigned long long)-m_ringSelection / food.size() + 1) * food.size()) %
                               food.size();
                }
                plasma::Vector<2, int> slot = food[(unsigned long long)(long long)(int)selected % food.size()];
                useItem(m_world->m_localCreature->m_inventory.m_inventoryTabs[slot[0]][slot[1]]);
                m_interface.m_inGame.m_selector->setVisible(false);
            }
        }
    } else if (m_crosshairItem[0] != INT_MAX) {
        ZoneItem *zoneItem = m_world->item(m_crosshairItem);
        if (zoneItem) {
            bool reserved =
                zoneItem->ownerSeed != 0 && zoneItem->ownerSeed != m_world->m_localCreature->m_entityData.seed;
            m_interface.m_inGame.m_quickItemSprites[QuickSlotUse]->m_item = std::move(zoneItem->item);
            if (reserved) {
                m_interface.m_inGame.m_quickButtons[QuickSlotUse].button->setTextColor(
                    L"info", plasma::Vector<4, float>(1.0f, 0.1f, 0.1f, 1.0f), true);
                m_interface.m_inGame.m_quickButtons[QuickSlotUse].button->setText(L"info", L"Reserved", true);
            } else {
                if (m_world->m_localCreature->canTakeItem(zoneItem->item) &&
                    m_world->m_localCreature->m_inventory.canAdd(zoneItem->item, 1)) {
                    m_interface.m_inGame.m_quickButtons[QuickSlotUse].button->setText(L"info", L"Pick up", true);
                } else {
                    m_interface.m_inGame.m_quickButtons[QuickSlotUse].button->setTextColor(
                        L"info", plasma::Vector<4, float>(1.0f, 0.3f, 0.3f, 1.0f), true);
                    m_interface.m_inGame.m_quickButtons[QuickSlotUse].button->setText(L"info", L"Full", true);
                }
                if ((m_controlStates[Controls::ButtonUse] && !usePressed)) {
                    player = m_world->m_localCreature;
                    int tab = player->m_inventory.itemTab(zoneItem->item);
                    if (!player->canTakeItem(zoneItem->item)) {
                        m_interface.m_inGame.m_chatWidget->printMessage(
                            L"You can't carry more of these items.\n",
                            plasma::Vector<4, float>(1.0f, 0.2f, 0.2f, 1.0f));
                        playSound(SoundHumanVoice01, m_world->m_localCreature->m_entityData.position, 1.0f, 1.0f, true);
                    } else if (!m_world->m_localCreature->m_inventory.canAdd(zoneItem->item, 1)) {
                        switch (tab) {
                        case InventoryTabPets:
                            m_interface.m_inGame.m_chatWidget->printMessage(
                                L"Your pet bag is full.\n", plasma::Vector<4, float>(1.0f, 0.2f, 0.2f, 1.0f));
                            break;
                        case InventoryTabCrafting:
                            m_interface.m_inGame.m_chatWidget->printMessage(
                                L"Your ingredient bag is full.\n", plasma::Vector<4, float>(1.0f, 0.2f, 0.2f, 1.0f));
                            break;
                        case InventoryTabConsumables:
                            m_interface.m_inGame.m_chatWidget->printMessage(
                                L"Your item bag is full.\n", plasma::Vector<4, float>(1.0f, 0.2f, 0.2f, 1.0f));
                            break;
                        case InventoryTabEquipment:
                            m_interface.m_inGame.m_chatWidget->printMessage(
                                L"Your equipment bag is full.\n", plasma::Vector<4, float>(1.0f, 0.2f, 0.2f, 1.0f));
                            break;
                        }
                        playSound(SoundHumanVoice01, m_world->m_localCreature->m_entityData.position, 1.0f, 1.0f, true);
                    } else {
                        Interaction pickup;
                        pickup.type = InteractionPickUp;
                        pickup.item = std::move(zoneItem->item);
                        pickup.position = m_crosshairItem;
                        m_world->m_localCreature->m_interactions.push_front(pickup);
                    }
                }
            }
        }
    } else if (!m_interface.m_inGame.m_speechWidget->getNode()->isVisible()) {
        player = m_world->m_localCreature;
        if (player->m_playerData->usableSpecialItem.category != ItemCategoryNone) {
            m_interface.m_inGame.m_quickItemSprites[QuickSlotUse]->m_item =
                std::move(player->m_playerData->usableSpecialItem);
            player = m_world->m_localCreature;
            if ((player->m_entityData.currentAbility == AbilityHangGlide &&
                 player->m_playerData->usableSpecialItem.id == SpecialItemHangGlider) ||
                (player->m_entityData.currentAbility == AbilitySailBoat &&
                 player->m_playerData->usableSpecialItem.id == SpecialItemBoat)) {
                m_interface.m_inGame.m_quickButtons[QuickSlotUse].button->setText(L"info", L"Cancel", true);
            } else {
                std::wstring argument = L"";
                std::wstring variant = L"default";
                std::wstring tag = L"info";
                std::wstring itemKey =
                    m_speech.getItemKey(m_world->m_localCreature->m_playerData->usableSpecialItem.category,
                                        m_world->m_localCreature->m_playerData->usableSpecialItem.id);
                m_interface.m_inGame.m_quickButtons[QuickSlotUse].button->setText(
                    tag, m_speech.getText(itemKey, variant, argument), true);
            }
            m_crosshairItem = plasma::Vector<3, int>(INT_MAX, INT_MAX, INT_MAX);
        }
        if ((m_controlStates[Controls::ButtonUse] && !usePressed)) {
            player = m_world->m_localCreature;
            Item &specialItem = player->m_playerData->usableSpecialItem;
            unsigned char ability = player->m_entityData.currentAbility;
            if ((ability == AbilityHangGlide && specialItem.id == SpecialItemHangGlider) ||
                (ability == AbilitySailBoat && specialItem.id == SpecialItemBoat) ||
                (ability == AbilityRidePet && specialItem.id == SpecialItemReins &&
                 player->m_entityData.acceleration[0] * player->m_entityData.acceleration[0] +
                         player->m_entityData.acceleration[1] * player->m_entityData.acceleration[1] +
                         player->m_entityData.acceleration[2] * player->m_entityData.acceleration[2] <
                     0.001f) ||
                (ability == AbilityClimb && specialItem.category == ItemCategoryNone)) {
                player->m_entityData.currentAbility = AbilityNone;
            } else if (specialItem.category != ItemCategoryNone) {
                player->m_entityData.actionItem = std::move(specialItem);
                player = m_world->m_localCreature;
                if (player->m_playerData->usableSpecialItem.id == SpecialItemHangGlider)
                    player->m_entityData.currentAbility = AbilityHangGlide;
                else if (player->m_playerData->usableSpecialItem.id == SpecialItemBoat)
                    player->m_entityData.currentAbility = AbilitySailBoat;
                else
                    player->m_entityData.currentAbility = AbilityUseSpecialItem;
                m_world->m_localCreature->m_entityData.timeSinceAbility = 0.0f;
            }
        }
    }
    usePressed = m_controlStates[Controls::ButtonUse];
    static bool petPressed; // control 14 rising edge
    if (m_controlStates[Controls::ButtonCallPet] && !petPressed &&
        m_world->m_localCreature->m_entityData.timeSinceAbility > m_world->m_localCreature->attackDuration()) {
        player = m_world->m_localCreature;
        if (player->m_entityData.currentAbility == AbilityRidePet) {
            player->m_entityData.currentAbility = AbilityNone;
        } else {
            Creature *pet = m_world->creatureByID(player->m_petId);
            bool mount = false;
            if (pet) {
                plasma::Vector<3, Fixed> delta = pet->m_entityData.position - player->m_entityData.position;
                plasma::Vector<3, float> blocks((float)delta[0], (float)delta[1], (float)delta[2]);
                if (blocks[0] * blocks[0] + blocks[1] * blocks[1] + blocks[2] * blocks[2] < 36.0f &&
                    m_world->m_localCreature->hasSpecialItem(SpecialItemReins)) {
                    m_world->m_localCreature->m_entityData.currentAbility = AbilityRidePet;
                    mount = true;
                }
            }
            if (!mount) {
                m_world->m_localCreature->m_entityData.actionItem.category = ItemCategoryPetCall;
                m_world->m_localCreature->m_entityData.currentAbility = AbilityCallPet;
                m_world->m_localCreature->m_entityData.timeSinceAbility = 0.0f;
                playSound(SoundTamersWhistle, m_cameraPosition, 1.0f, 1.0f, true);
                Interaction interaction;
                interaction.type = InteractionWhistle;
                m_world->m_localCreature->m_interactions.push_back(interaction);
            }
        }
    }
    petPressed = m_controlStates[Controls::ButtonCallPet];
    static bool foodPressed; // control 4 rising edge
    if (m_controlStates[Controls::ButtonQuickItem] && !foodPressed) {
        std::vector<plasma::Vector<2, int>> food = getFoodSlots();
        if (!food.empty()) {
            long long selected = m_ringSelection;
            if (selected < 0) {
                selected =
                    (selected + ((unsigned long long)-m_ringSelection / food.size() + 1) * food.size()) % food.size();
            }
            plasma::Vector<2, int> slot = food[(unsigned long long)(long long)(int)selected % food.size()];
            useItem(m_world->m_localCreature->m_inventory.m_inventoryTabs[slot[0]][slot[1]]);
            m_interface.m_inGame.m_selector->setVisible(false);
        }
    }
    foodPressed = m_controlStates[Controls::ButtonQuickItem];
    m_interface.refresh();
    if (m_interface.isInMenu())
        return;
    static bool attackPressed; // control 0 snapshot
    bool attack = m_controlStates[Controls::ButtonAttack];
    if (m_controlStates[Controls::ButtonWalk] && attackPressed)
        attack = false;
    attackPressed = m_controlStates[Controls::ButtonAttack];
    if (m_controlStates[Controls::ButtonSpecialAttack]) {
        if (m_world->m_localCreature->performSpecialAttack(m_controlStates[Controls::ButtonClassSkill]))
            m_world->m_localCreature->m_entityData.focusId = friendlyCreatureId;
    } else {
        player = m_world->m_localCreature;
        switch (player->m_entityData.currentAbility) {
        case AbilityCrush:
            if (player->m_entityData.charge < 0.01f) {
                player->m_entityData.charge = 0.0f;
                m_world->m_localCreature->m_entityData.currentAbility = AbilityNone;
            } else if (player->m_entityData.specialization == SpecializationGuardian) {
                player->m_entityData.currentAbility = AbilityGuardianCrushStrike;
                m_world->m_localCreature->m_entityData.timeSinceAbility = 0.0f;
            } else {
                player->m_entityData.currentAbility = AbilityCrushStrike;
                player = m_world->m_localCreature;
                player->m_entityData.timeSinceAbility = (float)player->anticipationTime(AbilityUseCurrent);
            }
            break;
        case AbilitySwirl:
            if (player->m_entityData.charge < 0.01f) {
                player->m_entityData.charge = 0.0f;
                m_world->m_localCreature->m_entityData.currentAbility = AbilityNone;
            } else {
                player->m_entityData.currentAbility = AbilitySwirlStrike;
                player = m_world->m_localCreature;
                player->m_entityData.timeSinceAbility = (float)player->anticipationTime(AbilityUseCurrent);
            }
            break;
        case AbilityBlock:
            if (player->m_entityData.charge < 0.01f) {
                player->m_entityData.charge = 0.0f;
                m_world->m_localCreature->m_entityData.currentAbility = AbilityNone;
            } else {
                player->m_entityData.currentAbility = AbilityBlockStrike;
                m_world->m_localCreature->m_entityData.timeSinceAbility = 0.0f;
            }
            break;
        case AbilityMonsterCharge:
            player->m_entityData.currentAbility = AbilityMonsterChargeStrike;
            player = m_world->m_localCreature;
            player->m_entityData.timeSinceAbility = (float)player->anticipationTime(AbilityUseCurrent);
            break;
        case AbilityChargedShot:
            player->m_entityData.currentAbility = AbilityChargedShotRelease;
            m_world->m_localCreature->m_entityData.timeSinceAbility = 0.0f;
            break;
        case AbilitySalvo:
            player->m_entityData.currentAbility = AbilitySalvoRelease;
            m_world->m_localCreature->m_entityData.timeSinceAbility = 0.0f;
            break;
        case AbilityChargedBoomerang:
            player->m_entityData.currentAbility = AbilityThrowBoomerang;
            m_world->m_localCreature->m_entityData.timeSinceAbility = 0.0f;
            break;
        case AbilityRockFistCharge:
            player->m_entityData.currentAbility = AbilityRockFist;
            m_world->m_localCreature->m_entityData.timeSinceAbility = 0.0f;
            break;
        case AbilityFireballCharge:
            player->m_entityData.currentAbility = AbilityFireball;
            m_world->m_localCreature->m_entityData.timeSinceAbility = 0.0f;
            break;
        case Ability_36:
            player->m_entityData.currentAbility = AbilityNone;
            m_world->m_localCreature->m_entityData.timeSinceAbility = 0.0f;
            break;
        default:
            if (attack && player->performAttack(m_controlStates[Controls::ButtonClassSkill])) {
                player = m_world->m_localCreature;
                if (player->m_entityData.currentAbility == AbilitySmash) {
                    player->m_entityData.velocity = player->m_entityData.attackRotation * 2.0f;
                    const plasma::Vector<3, float> &velocity = player->m_entityData.velocity;
                    float duration =
                        sqrtf(velocity[0] * velocity[0] + velocity[1] * velocity[1] + velocity[2] * velocity[2]) /
                        LeapSpeed * 1000.0f;
                    player->m_leapDuration = duration;
                    player = m_world->m_localCreature;
                    if (player->m_entityData.velocity[0] * player->m_entityData.velocity[0] +
                            player->m_entityData.velocity[1] * player->m_entityData.velocity[1] +
                            player->m_entityData.velocity[2] * player->m_entityData.velocity[2] >
                        LeapSpeedSquared) {
                        player->m_entityData.velocity.normalize();
                        m_world->m_localCreature->m_entityData.velocity *= LeapSpeed;
                    }
                    m_world->m_localCreature->m_entityData.velocity[2] = 15.0f;
                }
            }
            break;
        }
    }
    if (!m_controlStates[Controls::ButtonClassSkill]) {
        player = m_world->m_localCreature;
        if (player->m_entityData.currentAbility == AbilitySneak &&
            player->m_entityData.acceleration[0] * player->m_entityData.acceleration[0] +
                    player->m_entityData.acceleration[1] * player->m_entityData.acceleration[1] +
                    player->m_entityData.acceleration[2] * player->m_entityData.acceleration[2] >
                0.0f)
            player->m_entityData.currentAbility = AbilityNone;
        player = m_world->m_localCreature;
        if (player->m_entityData.currentAbility == AbilitySprint)
            player->m_entityData.currentAbility = AbilityNone;
        player = m_world->m_localCreature;
        if (player->m_entityData.currentAbility == AbilityCyclone && player->m_entityData.timeSinceAbility > 1000.0f)
            player->m_entityData.currentAbility = AbilityNone;
        player = m_world->m_localCreature;
        if (player->m_entityData.currentAbility == AbilityFloat)
            player->m_entityData.currentAbility = AbilityNone;
        player = m_world->m_localCreature;
        if (player->m_entityData.currentAbility == AbilityFlameRush)
            player->m_entityData.currentAbility = AbilityNone;
    }
    static bool classControlPressed;     // previous control 2 state
    static bool movementSkillPressed;    // previous control 10 state
    static unsigned int spellPressedAt;  // written on the class 2/4 control edge
    static unsigned int rangerPressedAt; // written on the class 3 control edge
    if (alphaClasses) {
        mod::controlBar(this);
    } else if (!m_controlStates[Controls::ButtonAttack] && !m_controlStates[Controls::ButtonSpecialAttack]) {
        player = m_world->m_localCreature;
        if (((player->m_entityData.classType == ClassRogue &&
              player->m_entityData.specialization == SpecializationAssassin) ||
             (player->m_entityData.classType == ClassRanger &&
              player->m_entityData.specialization == SpecializationSniper)) &&
            m_controlStates[Controls::ButtonClassSkill] && player->skillLevel(AbilitySneak) > 0 &&
            m_world->m_localCreature->m_entityData.timeSinceAbility > m_world->m_localCreature->attackDuration() &&
            (m_world->m_localCreature->m_entityData.flags & CreatureFlagOnGround) &&
            m_world->m_localCreature->m_entityData.currentAbility != AbilityIntercept)
            m_world->m_localCreature->m_entityData.currentAbility = AbilitySneak;
        player = m_world->m_localCreature;
        if (((player->m_entityData.classType == ClassRogue &&
              player->m_entityData.specialization == SpecializationNinja) ||
             (player->m_entityData.classType == ClassRanger &&
              player->m_entityData.specialization == SpecializationScout &&
              player->m_entityData.currentAbility != AbilityHangGlide)) &&
            m_controlStates[Controls::ButtonClassSkill]) {
            if (!classControlPressed)
                spellPressedAt = timeGetTime();
            if (m_world->m_localCreature->m_entityData.timeSinceAbility > m_world->m_localCreature->attackDuration() &&
                m_world->m_localCreature->m_entityData.currentAbility != AbilityIntercept &&
                m_world->m_localCreature->skillLevel(AbilitySprint) != 0)
                m_world->m_localCreature->m_entityData.currentAbility = AbilitySprint;
        }
        player = m_world->m_localCreature;
        if (player->m_entityData.classType == ClassWarrior && m_controlStates[Controls::ButtonClassSkill] &&
            player->m_stamina >= StaminaMoveCost && player->m_entityData.timeSinceAbility > player->attackDuration() &&
            m_world->m_localCreature->m_entityData.currentAbility != AbilityCyclone) {
            m_world->m_localCreature->m_entityData.currentAbility = AbilityCyclone;
            m_world->m_localCreature->m_entityData.timeSinceAbility = 0.0f;
        }
        player = m_world->m_localCreature;
        if (player->m_entityData.classType == ClassMage) {
            timeGetTime();
            if (m_controlStates[Controls::ButtonClassSkill]) {
                if (!classControlPressed)
                    rangerPressedAt = timeGetTime();
                if (m_world->m_localCreature->m_entityData.timeSinceAbility >
                        m_world->m_localCreature->attackDuration() &&
                    m_world->m_localCreature->m_entityData.rollTime == 0.0f &&
                    m_world->m_localCreature->m_entityData.currentAbility != AbilityFloat) {
                    m_world->m_localCreature->m_entityData.currentAbility = AbilityFloat;
                    player = m_world->m_localCreature;
                    if (player->m_entityData.velocity[2] < -0.2f)
                        player->m_entityData.velocity[2] = -0.2f;
                    player->m_entityData.retreat = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
                }
            }
        }
        player = m_world->m_localCreature;
        if (player->m_entityData.classType == ClassMage && player->canUseSkill(AbilityTeleport, true) &&
            m_world->m_localCreature->skillLevel(AbilityTeleport) != 0 && m_controlStates[Controls::ButtonDodge] &&
            !movementSkillPressed &&
            m_world->m_localCreature->m_entityData.acceleration[0] *
                        m_world->m_localCreature->m_entityData.acceleration[0] +
                    m_world->m_localCreature->m_entityData.acceleration[1] *
                        m_world->m_localCreature->m_entityData.acceleration[1] +
                    m_world->m_localCreature->m_entityData.acceleration[2] *
                        m_world->m_localCreature->m_entityData.acceleration[2] <
                0.01f) {
            m_world->m_localCreature->m_entityData.currentAbility = AbilityTeleport;
            m_world->m_localCreature->m_entityData.timeSinceAbility = 0.0f;
            m_world->m_localCreature->m_entityData.rollTime = 0.0f;
        }
        player = m_world->m_localCreature;
        if (player->m_entityData.classType == ClassWarrior && player->canUseSkill(AbilitySmash, true) &&
            m_world->m_localCreature->skillLevel(AbilitySmash) != 0 && m_controlStates[Controls::ButtonDodge] &&
            !movementSkillPressed &&
            m_world->m_localCreature->m_entityData.acceleration[0] *
                        m_world->m_localCreature->m_entityData.acceleration[0] +
                    m_world->m_localCreature->m_entityData.acceleration[1] *
                        m_world->m_localCreature->m_entityData.acceleration[1] +
                    m_world->m_localCreature->m_entityData.acceleration[2] *
                        m_world->m_localCreature->m_entityData.acceleration[2] <
                0.01f) {
            m_world->m_localCreature->m_entityData.currentAbility = AbilitySmash;
            m_world->m_localCreature->m_entityData.timeSinceAbility = 0.0f;
            m_world->m_localCreature->m_entityData.rollTime = 0.0f;
            player = m_world->m_localCreature;
            player->m_entityData.velocity = player->m_entityData.attackRotation * 2.0f;
            float duration = player->m_entityData.velocity.length() / LeapSpeed * 1000.0f;
            player->m_leapDuration = duration;
            player = m_world->m_localCreature;
            if (player->m_entityData.velocity[0] * player->m_entityData.velocity[0] +
                    player->m_entityData.velocity[1] * player->m_entityData.velocity[1] +
                    player->m_entityData.velocity[2] * player->m_entityData.velocity[2] >
                LeapSpeedSquared) {
                player->m_entityData.velocity.normalize();
                m_world->m_localCreature->m_entityData.velocity *= LeapSpeed;
            }
            m_world->m_localCreature->m_entityData.velocity[2] = 15.0f;
        }
        player = m_world->m_localCreature;
        if (player->m_entityData.classType == ClassRanger &&
            player->m_entityData.specialization == SpecializationScout &&
            player->canUseSkill(AbilityRangerKick, true) &&
            m_world->m_localCreature->skillLevel(AbilityRangerKick) != 0 && m_controlStates[Controls::ButtonDodge] &&
            !movementSkillPressed &&
            m_world->m_localCreature->m_entityData.acceleration[0] *
                        m_world->m_localCreature->m_entityData.acceleration[0] +
                    m_world->m_localCreature->m_entityData.acceleration[1] *
                        m_world->m_localCreature->m_entityData.acceleration[1] +
                    m_world->m_localCreature->m_entityData.acceleration[2] *
                        m_world->m_localCreature->m_entityData.acceleration[2] <
                0.01f) {
            m_world->m_localCreature->m_entityData.currentAbility = AbilityRangerKick;
            m_world->m_localCreature->m_entityData.timeSinceAbility = 0.0f;
            m_world->m_localCreature->m_entityData.rollTime = 0.0f;
        }
        player = m_world->m_localCreature;
        if (player->m_entityData.classType == ClassRogue &&
            player->m_entityData.specialization == SpecializationAssassin &&
            player->canUseSkill(AbilityThrowPoisonVial, true) &&
            m_world->m_localCreature->skillLevel(AbilityRangerKick) != 0 && m_controlStates[Controls::ButtonDodge] &&
            !movementSkillPressed &&
            m_world->m_localCreature->m_entityData.acceleration[0] *
                        m_world->m_localCreature->m_entityData.acceleration[0] +
                    m_world->m_localCreature->m_entityData.acceleration[1] *
                        m_world->m_localCreature->m_entityData.acceleration[1] +
                    m_world->m_localCreature->m_entityData.acceleration[2] *
                        m_world->m_localCreature->m_entityData.acceleration[2] <
                0.01f) {
            m_world->m_localCreature->m_entityData.currentAbility = AbilityThrowPoisonVial;
            m_world->m_localCreature->m_entityData.timeSinceAbility = 0.0f;
            m_world->m_localCreature->m_entityData.rollTime = 0.0f;
        }
        player = m_world->m_localCreature;
        if (player->m_entityData.classType == ClassRanger &&
            player->m_entityData.specialization == SpecializationSniper && player->canUseSkill(AbilityRetreat, true) &&
            m_world->m_localCreature->skillLevel(AbilityRetreat) != 0 && m_controlStates[Controls::ButtonDodge] &&
            !movementSkillPressed &&
            m_world->m_localCreature->m_entityData.acceleration[0] *
                        m_world->m_localCreature->m_entityData.acceleration[0] +
                    m_world->m_localCreature->m_entityData.acceleration[1] *
                        m_world->m_localCreature->m_entityData.acceleration[1] +
                    m_world->m_localCreature->m_entityData.acceleration[2] *
                        m_world->m_localCreature->m_entityData.acceleration[2] <
                0.01f) {
            m_world->m_localCreature->m_entityData.currentAbility = AbilityRetreat;
            m_world->m_localCreature->m_entityData.timeSinceAbility = 0.0f;
            m_world->m_localCreature->m_entityData.rollTime = 0.0f;
            player = m_world->m_localCreature;
            if (player->m_entityData.acceleration[0] * player->m_entityData.acceleration[0] +
                    player->m_entityData.acceleration[1] * player->m_entityData.acceleration[1] +
                    player->m_entityData.acceleration[2] * player->m_entityData.acceleration[2] >
                0.0f)
                player->m_entityData.retreat = player->m_entityData.acceleration;
            else
                player->m_entityData.retreat = -player->m_entityData.attackRotation;
            m_world->m_localCreature->m_entityData.rollTime = DodgeRollDurationMs;
            player = m_world->m_localCreature;
            player->m_entityData.retreat[2] = 0.0f;
            if (player->m_entityData.retreat[0] * player->m_entityData.retreat[0] +
                    player->m_entityData.retreat[1] * player->m_entityData.retreat[1] +
                    player->m_entityData.retreat[2] * player->m_entityData.retreat[2] >
                0.0f) {
                player->m_entityData.retreat.normalize();
                player = m_world->m_localCreature;
                player->m_entityData.retreat[0] *= 20.0f;
                player->m_entityData.retreat[1] *= 20.0f;
                player->m_entityData.retreat[2] = 15.0f;
                player->m_entityData.velocity[0] += player->m_entityData.retreat[0];
                player->m_entityData.velocity[1] += player->m_entityData.retreat[1];
            }
        }
    }
    classControlPressed = m_controlStates[Controls::ButtonClassSkill];
    movementSkillPressed = m_controlStates[Controls::ButtonDodge];
    if (!m_controlStates[Controls::ButtonSpecialAttack] && attackSkills[1] == AbilitySupportRay &&
        m_world->m_localCreature->m_entityData.currentAbility == AbilitySupportRay)
        m_world->m_localCreature->m_entityData.currentAbility = AbilityNone;
    if (!m_controlStates[Controls::ButtonAttack] && attackSkills[0] == Ability_30 &&
        m_world->m_localCreature->m_entityData.currentAbility == Ability_30)
        m_world->m_localCreature->m_entityData.currentAbility = AbilityNone;
}

bool Game::interactionPrompt()
{
    static bool useWasDown = false;
    bool usePressed = m_controlStates[Controls::ButtonUse] && !useWasDown;
    useWasDown = m_controlStates[Controls::ButtonUse];
    if (m_controlStates[Controls::ButtonDodge] || m_controlStates[Controls::ButtonWalk] ||
        m_interface.m_inGame.m_speechWidget->m_node->isVisible()) {
        return false;
    }
    Creature *player = m_world->m_localCreature;
    if (player->m_entityData.currentAbility == AbilityClimb) {
        m_interface.m_inGame.m_quickButtons[QuickSlotUse].button->setText(L"info", L"Fall", true);
        if (usePressed) {
            m_world->m_localCreature->m_entityData.currentAbility = AbilityNone;
        }
        return true;
    }
    if (player->m_entityData.currentAbility != AbilityClimb &&
        player->m_entityData.timeSinceAbility > player->attackDuration() &&
        m_world->m_localCreature->m_entityData.rollTime == 0.0f &&
        m_world->m_localCreature->m_entityData.currentAbility != AbilityHangGlide &&
        m_world->m_localCreature->m_entityData.currentAbility != AbilityRidePet &&
        m_world->m_localCreature->m_entityData.currentAbility != AbilitySailBoat &&
        m_world->m_localCreature->m_entityData.acceleration.squaredLength() > 0.1f &&
        (m_world->m_localCreature->m_entityData.flags & CreatureFlagAtWall) &&
        !(m_world->m_localCreature->m_entityData.flags & CreatureFlagOnGround) &&
        m_world->m_localCreature->m_stamina > 0.0f) {
        m_interface.m_inGame.m_quickButtons[QuickSlotUse].button->setText(L"info", L"Climb", true);
        if (usePressed) {
            m_world->m_localCreature->m_entityData.currentAbility = AbilityClimb;
        }
        return true;
    }

    World *world = m_world;
    Creature *target = world->creatureByID(m_crosshairCreatureId);
    player = world->m_localCreature;
    if (player->m_entityData.currentAbility == AbilityMarkCreature &&
        player->m_entityData.timeSinceAbility > player->anticipationTime(AbilityUseCurrent)) {
        m_world->m_localCreature->m_entityData.currentAbility = AbilityNone;
        m_world->m_localCreature->m_entityData.timeSinceAbility = 0.0f;
        Creature *marked = m_world->creatureByID(m_world->m_localCreature->m_entityData.focusId);
        if (marked) {
            m_worldMap->markCreature(marked);
            playSound(SoundPickup, m_cameraPosition, 1.0f, 1.0f, true);
            m_worldMap->m_markedCreatureId = marked->m_id;
            std::vector<Creature *> following = getFollowingCreatures();
            m_markedCreatureIndex = 0;
            for (std::vector<Creature *>::iterator it = following.begin(); it != following.end(); ++it) {
                if ((*it)->m_id == marked->m_id) {
                    break;
                }
                ++m_markedCreatureIndex;
            }
        }
    }
    player = m_world->m_localCreature;
    if (player->m_entityData.currentAbility == AbilityUnmarkCreature &&
        player->m_entityData.timeSinceAbility > player->anticipationTime(AbilityUseCurrent)) {
        m_world->m_localCreature->m_entityData.currentAbility = AbilityNone;
        m_world->m_localCreature->m_entityData.timeSinceAbility = 0.0f;
        WorldMap *map = m_worldMap;
        Creature *marked = map->lookupCreature(m_world->m_localCreature->m_entityData.focusId);
        if (marked) {
            map->unmarkCreature(marked);
            playSound(SoundDrop);
            Creature *next = markedCreature();
            m_worldMap->m_markedCreatureId = next ? next->m_id : 0;
        }
    }
    player = m_world->m_localCreature;
    if (player->m_entityData.hp <= 0.0f) {
        m_interface.m_inGame.m_quickButtons[QuickSlotUse].button->setText(L"info", L"Revive", true);
        if (usePressed) {
            m_world->m_localCreature->stopActions();
            player = m_world->m_localCreature;
            player->m_playerData->deathPosition = player->m_entityData.position;
            m_world->m_localCreature->m_playerData->deathMarkerTime = 300000.0f;
            player = m_world->m_localCreature;
            player->m_entityData.position = m_world->nearestShrine(player->m_entityData.position);
            m_world->m_localCreature->m_entityData.revivalPenalty = -0.5f;
            player = m_world->m_localCreature;
            player->m_entityData.hp = player->maxHP();
        }
        return true;
    }
    if (player->m_entityData.currentAbility == AbilityCarryCreature &&
        m_world->creatureByID(player->m_entityData.ownerId)) {
        m_interface.m_inGame.m_quickButtons[QuickSlotUse].button->setText(L"info", L"Throw", true);
        if (usePressed) {
            m_world->m_localCreature->m_entityData.currentAbility = AbilityThrowCreature;
            m_world->m_localCreature->m_entityData.timeSinceAbility = 0.0f;
        }
        return true;
    }
    if (target) {
        if ((target->m_entityData.appearance.appearanceFlags & AppearanceFlagPassive) ||
            (target->m_entityData.hostilityType == HostilityEnemy &&
             target->m_entityData.currentAbility == AbilityRestHeal) ||
            (target->m_id == player->m_petId && !isPet(target->m_entityData.race))) {
            plasma::Vector<3, float> offset(target->m_entityData.position -
                                            m_world->m_localCreature->m_entityData.position);
            if (offset.squaredLength() < 25.0f) {
                m_interface.m_inGame.m_quickButtons[QuickSlotUse].button->setText(L"info", L"Pick up", true);
                m_promptCreatureId = target->m_id;
                if (usePressed) {
                    m_world->m_localCreature->m_entityData.ownerId = target->m_id;
                    m_world->m_localCreature->m_entityData.currentAbility = AbilityCarryCreature;
                    m_world->m_localCreature->m_entityData.timeSinceAbility = 0.0f;
                }
                return true;
            }
        }
        if (target->m_entityData.hostilityType == HostilityFriendlyNpc) {
            plasma::Vector<3, float> offset(target->m_entityData.position -
                                            m_world->m_localCreature->m_entityData.position);
            float distanceSquared = offset.squaredLength();
            if (target->interactionRange() * target->interactionRange() > distanceSquared) {
                if (target->m_entityData.appearance.appearanceFlags & AppearanceFlagPetrified) {
                    m_interface.m_inGame.m_quickButtons[QuickSlotUse].button->setText(L"info", L"Examine", true);
                    if (usePressed) {
                        std::vector<std::string> script;
                        script << std::string("define_model") << target->m_entityData.race;
                        script << std::string("he_is_petrified");
                        m_speech.generateFromScript(script, (unsigned long long)target->m_id % 100000,
                                                    m_interface.m_inGame.m_speechWidget->m_dialogs, L"");
                    }
                    return true;
                }
                m_interface.m_inGame.m_quickButtons[QuickSlotUse].button->setText(L"info", L"Talk", true);
                m_promptCreatureId = target->m_id;
                if (usePressed) {
                    unsigned char classType = target->m_entityData.classType;
                    if ((unsigned char)(classType + 128) <= 2 || classType == ClassGemTrader) {
                        m_interface.m_inGame.m_vendorInventoryWidget->m_node->setVisible(true);
                        m_interface.showTab(InterfacePanelEquipment);
                        m_vendorId = target->m_id;
                        m_vendorClass = target->m_entityData.classType;
                        updateVendor();
                    } else if (classType == ClassAnalyzer) {
                        m_interface.m_inGame.m_enchantWidget->m_node->setVisible(true);
                        m_interface.showTab(InterfacePanelEquipment);
                    } else if (classType == ClassFlightMaster) {
                        m_interface.m_mapOpen = true;
                        m_worldMap->m_flightmasterMode = true;
                    } else {
                        std::vector<SpeechWidget *>::iterator widget = m_interface.m_inGame.m_speechWidgets.begin();
                        for (; widget != m_interface.m_inGame.m_speechWidgets.end(); ++widget) {
                            if ((*widget)->m_speakerId == target->m_id) {
                                break;
                            }
                        }
                        if (widget != m_interface.m_inGame.m_speechWidgets.end()) {
                            SpeechWidget *speech = *widget;
                            if (speech->m_readTime >= speech->getDialogDuration()) {
                                speech->m_readTime += 100000.0f;
                            } else {
                                speech->m_readTime = (float)speech->getDialogDuration();
                            }
                        } else {
                            std::vector<std::string> script = m_world->talkScript(target);
                            if (!script.empty()) {
                                std::vector<Dialog> dialogs;
                                m_speech.generateFromScript(script, (unsigned long long)target->m_id % 100000, dialogs,
                                                            L"");
                                say(target, m_world->m_localCreature, dialogs);
                            }
                            interact(target);
                        }
                    }
                }
                return true;
            }
        }
        if ((target->m_entityData.hostilityType == HostilityNeutral ||
             target->m_entityData.hostilityType == HostilityEnemy) &&
            speciesType(target->m_entityData.race) == SpeciesAnimal) {
            player = m_world->m_localCreature;
            if (player->m_entityData.timeSinceAbility > player->attackDuration() &&
                target->m_entityData.currentAbility == AbilityWatchPlayer) {
                plasma::Vector<3, float> offset(target->m_entityData.position -
                                                m_world->m_localCreature->m_entityData.position);
                float distanceSquared = offset.squaredLength();
                if (target->interactionRange() * target->interactionRange() > distanceSquared) {
                    m_interface.m_inGame.m_quickButtons[QuickSlotUse].button->setText(L"info", L"Pet", true);
                    m_promptCreatureId = target->m_id;
                    if (usePressed) {
                        m_world->m_localCreature->m_entityData.currentAbility = AbilityPetAnimal;
                        m_world->m_localCreature->m_entityData.timeSinceAbility = 0.0f;
                        interact(target);
                    }
                    return true;
                }
            }
        }
        if (target->m_entityData.hostilityType == HostilityNeutral) {
            plasma::Vector<3, float> offset(target->m_entityData.position -
                                            m_world->m_localCreature->m_entityData.position);
            float distanceSquared = offset.squaredLength();
            if (target->interactionRange() * target->interactionRange() > distanceSquared) {
                m_interface.m_inGame.m_quickButtons[QuickSlotUse].button->setText(L"info", L"Interact", true);
                m_promptCreatureId = target->m_id;
                if (usePressed) {
                    interact(target);
                }
                return true;
            }
        }
        if (target->isChest()) {
            plasma::Vector<3, float> offset(target->m_entityData.position -
                                            m_world->m_localCreature->m_entityData.position);
            float distanceSquared = offset.squaredLength();
            if (target->interactionRange() * target->interactionRange() > distanceSquared) {
                m_interface.m_inGame.m_quickButtons[QuickSlotUse].button->setText(L"info", L"Open", true);
                m_promptCreatureId = target->m_id;
                if (usePressed) {
                    interact(target);
                }
                return true;
            }
        }
        if (target->m_entityData.race == RaceLockedGate) {
            plasma::Vector<3, float> offset(target->m_entityData.position -
                                            m_world->m_localCreature->m_entityData.position);
            float distanceSquared = offset.squaredLength();
            if (target->interactionRange() * target->interactionRange() > distanceSquared) {
                m_interface.m_inGame.m_quickButtons[QuickSlotUse].button->setText(L"info", L"Open", true);
                m_promptCreatureId = target->m_id;
                if (usePressed) {
                    interact(target);
                }
                return true;
            }
        }
        if (target->isCage() || target->m_entityData.race == RaceTortureRack ||
            target->m_entityData.race == RaceLegIrons) {
            plasma::Vector<3, float> offset(target->m_entityData.position -
                                            m_world->m_localCreature->m_entityData.position);
            float distanceSquared = offset.squaredLength();
            if (target->interactionRange() * target->interactionRange() > distanceSquared) {
                m_interface.m_inGame.m_quickButtons[QuickSlotUse].button->setText(L"info", L"Open", true);
                m_promptCreatureId = target->m_id;
                if (usePressed) {
                    interact(target);
                }
                return true;
            }
        }
        if (target->m_entityData.appearance.appearanceFlags & AppearanceFlagUsable) {
            plasma::Vector<3, float> offset(target->m_entityData.position -
                                            m_world->m_localCreature->m_entityData.position);
            float distanceSquared = offset.squaredLength();
            if (target->interactionRange() * target->interactionRange() > distanceSquared) {
                m_interface.m_inGame.m_quickButtons[QuickSlotUse].button->setText(L"info", L"Use", true);
                m_promptCreatureId = target->m_id;
                if (usePressed) {
                    interact(target);
                }
                return true;
            }
        }
        if (target->m_entityData.race == RaceRunePortal) {
            plasma::Vector<3, float> offset(target->m_entityData.position -
                                            m_world->m_localCreature->m_entityData.position);
            float distanceSquared = offset.squaredLength();
            if (target->interactionRange() * target->interactionRange() > distanceSquared) {
                m_interface.m_inGame.m_quickButtons[QuickSlotUse].button->setText(L"info", L"Use", true);
                m_promptCreatureId = target->m_id;
                if (usePressed) {
                    m_interface.m_mapOpen = true;
                    m_interface.m_choosingShrine = true;
                }
                return true;
            }
        }
        if (target->m_entityData.race == RaceCampfire) {
            plasma::Vector<3, float> offset(target->m_entityData.position -
                                            m_world->m_localCreature->m_entityData.position);
            float range = target->m_entityData.appearance.size[0] * 0.5f + 5.0f;
            if (range * range > offset.squaredLength()) {
                m_interface.m_inGame.m_quickButtons[QuickSlotUse].button->setText(L"info", L"Use", true);
                m_promptCreatureId = target->m_id;
                if (usePressed) {
                    m_interface.m_inGame.m_craftingInventoryWidget->m_node->setVisible(
                        !m_interface.m_inGame.m_craftingInventoryWidget->m_node->isVisible());
                    if (m_interface.m_inGame.m_craftingInventoryWidget->m_tabIndex != CraftingTabCooking) {
                        m_interface.m_inGame.m_craftingInventoryWidget->activateTab(CraftingTabCooking);
                    }
                    updateFormulas();
                }
                return true;
            }
        }
        if ((unsigned int)(target->m_entityData.race - RaceSleepingMat) <= 1) {
            plasma::Vector<3, float> offset(target->m_entityData.position -
                                            m_world->m_localCreature->m_entityData.position);
            float range = target->m_entityData.appearance.size[0] * 0.5f + 5.0f;
            if (range * range > offset.squaredLength() && target->m_entityData.focusId == 0) {
                m_interface.m_inGame.m_quickButtons[QuickSlotUse].button->setText(L"info", L"Sleep", true);
                m_promptCreatureId = target->m_id;
                if (usePressed) {
                    interact(target);
                }
                return true;
            }
        }
        if (target->m_entityData.hostilityType == HostilityInteractiveObject) {
            plasma::Vector<3, float> offset(target->m_entityData.position -
                                            m_world->m_localCreature->m_entityData.position);
            float range = target->m_entityData.appearance.size[0] * 0.5f + 5.0f;
            if (range * range > offset.squaredLength()) {
                m_interface.m_inGame.m_quickButtons[QuickSlotUse].button->setText(L"info", L"Examine", true);
                m_promptCreatureId = target->m_id;
                if (usePressed) {
                    interact(target);
                }
                return true;
            }
        }
    }
    plasma::Vector<3, int> objectId(m_crosshairObject[0], m_crosshairObject[1], m_crosshairObject[2]);
    StaticEntity *object = m_world->object(objectId);
    if (!object) {
        return false;
    }
    if (object->item.category && object->active) {
        if (m_world->m_localCreature->hasItem(object->item, object->itemCount)) {
            m_interface.m_inGame.m_quickItemSprites[QuickSlotUse]->m_item = object->item;
            if (object->itemCount > 1) {
                std::wstring name(L"info");
                m_interface.m_inGame.m_quickButtons[QuickSlotUse].button->setText(
                    name, std::to_wstring(object->itemCount) + L" x " + m_speech.createItemName(object->item), true);
            } else {
                std::wstring name(L"info");
                m_interface.m_inGame.m_quickButtons[QuickSlotUse].button->setText(
                    name, m_speech.createItemName(object->item), true);
            }
            if (usePressed) {
                if (object->type != StaticEntityLockedGate) {
                    m_world->m_localCreature->removeItem(object->item, object->itemCount);
                }
                playSound(SoundDropItem);
                Interaction interaction;
                interaction.type = InteractionTalk;
                interaction.position = m_crosshairObject;
                interaction.item = object->item;
                m_world->m_localCreature->m_interactions.push_back(interaction);
            }
            return true;
        }
        m_interface.m_inGame.m_quickItemSprites[QuickSlotUse]->m_item = object->item;
        if (object->itemCount > 1) {
            std::wstring name(L"info");
            m_interface.m_inGame.m_quickButtons[QuickSlotUse].button->setText(
                name, std::to_wstring(object->itemCount) + L" x " + m_speech.createItemName(object->item), true);
        } else {
            std::wstring name(L"info");
            m_interface.m_inGame.m_quickButtons[QuickSlotUse].button->setText(
                name, m_speech.createItemName(object->item), true);
        }
        m_interface.m_inGame.m_quickButtons[QuickSlotUse].button->setTextColor(
            L"info", plasma::Vector<4, float>(1.0f, 0.3f, 0.3f, 1.0f), true);
        return true;
    }
    switch (object->type) {
    case StaticEntityDoor:
    case StaticEntityBigDoor:
    case StaticEntityBigIronDoor:
    case StaticEntityWindow:
    case StaticEntityChestBase02:
    case StaticEntityCageBase:
    case StaticEntityCage2Base:
    case StaticEntityCage3Base:
    case StaticEntityCage4Base:
    case StaticEntityTortureRack:
    case StaticEntityLegIrons:
    case StaticEntityHatch:
        if (object->active) {
            m_interface.m_inGame.m_quickButtons[QuickSlotUse].button->setText(L"info", L"Open", true);
        } else {
            m_interface.m_inGame.m_quickButtons[QuickSlotUse].button->setText(L"info", L"Close", true);
        }
        if (usePressed) {
            Interaction interaction;
            interaction.type = InteractionTalk;
            interaction.position = m_crosshairObject;
            m_world->m_localCreature->m_interactions.push_back(interaction);
        }
        return true;
    case StaticEntityRewardChest: {
        if (!object->active) {
            return true;
        }
        int x = tileCoordinate(object->position[0]);
        int y = tileCoordinate(object->position[1]);
        std::array<int, 2> marker = {{x, y}};
        world = m_world;
        const std::set<std::array<int, 2>> &markers = world->m_localCreature->m_playerData->discoveredMarkers;
        if (markers.find(marker) != markers.end()) {
            return true;
        }
        Zone *zone = world->zone(m_crosshairObject[0], m_crosshairObject[1]);
        if (!zone) {
            return true;
        }
        std::map<int, Item>::iterator reward = zone->m_objectRewards.find(m_crosshairObject[2]);
        if (reward == zone->m_objectRewards.end() || !reward->second.category) {
            return true;
        }
        m_interface.m_inGame.m_quickButtons[QuickSlotUse].button->setText(L"info", L"Open", true);
        if (usePressed) {
            m_world->m_localCreature->m_playerData->discoveredMarkers.insert(marker);
            Item item(reward->second);
            if (item.category == ItemCategoryGeneratedItem) {
                item = item.transformed(m_world->m_localCreature);
            }
            std::vector<std::string> script;
            script << std::string("define_item") << item;
            if ((unsigned char)(item.category - ItemCategoryArtifact) <= 1 ||
                (item.isEquipment() && item.rarity > ItemRarityBlue)) {
                script << std::string("you_received_the_item");
            } else {
                script << std::string("you_received_an_item");
            }
            m_speech.generateFromScript(script, rand() % 10000, m_interface.m_inGame.m_speechWidget->m_dialogs, L"");
            player = m_world->m_localCreature;
            if (!player->m_entityData.currentAbility ||
                player->m_entityData.timeSinceAbility > player->attackDuration()) {
                m_world->m_localCreature->m_entityData.currentAbility = AbilityPresentItem;
                m_world->m_localCreature->m_entityData.timeSinceAbility = 0.0f;
                m_world->m_localCreature->m_entityData.actionItem = item;
                m_successMusicTime = 60000;
            }
            if (item.count()) {
                player = m_world->m_localCreature;
                player->m_inventory.m_gold += item.count();
            } else {
                m_world->m_localCreature->m_inventory.add(item, 1, -1);
                notifyItem(item, 1, m_world->m_localCreature);
            }
        }
        return true;
    }
    case StaticEntityStool:
    case StaticEntityBench:
    case StaticEntityBenchBlack:
    case StaticEntityDruidChair:
        if (object->occupantId != m_world->m_localCreature->m_id) {
            m_interface.m_inGame.m_quickButtons[QuickSlotUse].button->setText(L"info", L"Sit", true);
        }
        if (usePressed) {
            Interaction interaction;
            interaction.type = InteractionTalk;
            interaction.position = m_crosshairObject;
            m_world->m_localCreature->m_interactions.push_back(interaction);
        }
        return true;
    case StaticEntityBed:
    case StaticEntityBeachTowel:
    case StaticEntityWoodMat:
        if (object->occupantId != m_world->m_localCreature->m_id) {
            m_interface.m_inGame.m_quickButtons[QuickSlotUse].button->setText(L"info", L"Sleep", true);
        }
        if (usePressed) {
            Interaction interaction;
            interaction.type = InteractionTalk;
            interaction.position = m_crosshairObject;
            m_world->m_localCreature->m_interactions.push_back(interaction);
        }
        return true;
    case StaticEntityPedestal:
    case StaticEntityStoneTablet1:
    case StaticEntityStoneTablet2:
    case StaticEntityStoneTablet3:
    case StaticEntityStoneTablet4:
    case StaticEntityStoneTablet5:
    case StaticEntityStoneTablet6:
    case StaticEntityInscribedTablet:
    case StaticEntityBrazier:
    case StaticEntitySarcophagus:
        m_interface.m_inGame.m_quickButtons[QuickSlotUse].button->setText(L"info", L"Examine", true);
        if (usePressed) {
            int y = tileCoordinate(object->position[1]);
            int x = tileCoordinate(object->position[0]);
            int radius = 0;
            if (!m_world->hasLore(x, y))
                radius = 1;
            for (int dx = -radius; dx <= radius; ++dx) {
                for (int dy = -radius; dy <= radius; ++dy) {
                    int loreX = tileCoordinate(object->position[0]) + dx;
                    int loreY = tileCoordinate(object->position[1]) + dy;
                    if (m_world->hasLore(loreX, loreY)) {
                        {
                            std::wstring argument(L"");
                            std::vector<std::string> script = m_world->storyScript(loreX, loreY);
                            m_speech.generateFromScript(script, fastRandom(loreX, loreY),
                                                        m_interface.m_inGame.m_speechWidget->m_dialogs, argument);
                        }
                        if (!m_interface.m_inGame.m_speechWidget->m_dialogs.empty()) {
                            m_interface.m_inGame.m_speechWidget->m_dialogs.back().revealedLoreCells.push_back(
                                {{loreX, loreY}});
                        }
                        break;
                    }
                }
                if (!m_interface.m_inGame.m_speechWidget->m_dialogs.empty()) {
                    return true;
                }
            }
        }
        return true;
    case StaticEntityRunestone:
        if (object->occupantId != m_world->m_localCreature->m_id) {
            m_interface.m_inGame.m_quickButtons[QuickSlotUse].button->setText(L"info", L"Teleport", true);
        }
        return true;
    case StaticEntityGoddess2:
        if (m_world->m_localCreature->hasSpecialItem(SpecialItemFlute)) {
            return false;
        }
        m_interface.m_inGame.m_quickButtons[QuickSlotUse].button->setText(L"info", L"Examine", true);
        if (usePressed) {
            Speech::Context context;
            m_speech.generateDialogs(L"gift_of_the_goddess", context, 0, m_interface.m_inGame.m_speechWidget->m_dialogs,
                                     L"");
            m_interface.m_inGame.m_speechWidget->m_dialogs.back().item.category = ItemCategorySpecial;
            m_interface.m_inGame.m_speechWidget->m_dialogs.back().item.id = SpecialItemFlute;
            m_interface.m_inGame.m_speechWidget->m_dialogs.back().givenCount = 1;
        }
        return true;
    case StaticEntityFurnace:
        if (object->occupantId != m_world->m_localCreature->m_id) {
            m_interface.m_inGame.m_quickButtons[QuickSlotUse].button->setText(L"info", L"Furnace", true);
            if (usePressed) {
                InventoryWidget *inventory = m_interface.m_inGame.m_craftingInventoryWidget;
                inventory->m_node->setVisible(!inventory->m_node->isVisible());
                if (m_interface.m_inGame.m_craftingInventoryWidget->m_tabIndex != CraftingTabIngredients) {
                    m_interface.m_inGame.m_craftingInventoryWidget->activateTab(CraftingTabIngredients);
                }
                updateFormulas();
                if (m_interface.m_inGame.m_craftingInventoryWidget->m_node->isVisible()) {
                    m_interface.m_inGame.m_craftingInventoryWidget->updateRows();
                    m_interface.m_inGame.m_craftingInventoryWidget->updateScrollButton();
                }
            }
        }
        return true;
    case StaticEntityAnvil:
        if (object->occupantId != m_world->m_localCreature->m_id) {
            m_interface.m_inGame.m_quickButtons[QuickSlotUse].button->setText(L"info", L"Anvil", true);
            if (usePressed) {
                InventoryWidget *inventory = m_interface.m_inGame.m_craftingInventoryWidget;
                inventory->m_node->setVisible(!inventory->m_node->isVisible());
                if (m_interface.m_inGame.m_craftingInventoryWidget->m_tabIndex != CraftingTabWeapons) {
                    m_interface.m_inGame.m_craftingInventoryWidget->activateTab(CraftingTabWeapons);
                }
                updateFormulas();
            }
        }
        return true;
    case StaticEntitySpinningwheel:
        if (object->occupantId != m_world->m_localCreature->m_id) {
            m_interface.m_inGame.m_quickButtons[QuickSlotUse].button->setText(L"info", L"Spinning Wheel", true);
            if (usePressed) {
                InventoryWidget *inventory = m_interface.m_inGame.m_craftingInventoryWidget;
                inventory->m_node->setVisible(!inventory->m_node->isVisible());
                if (m_interface.m_inGame.m_craftingInventoryWidget->m_tabIndex != CraftingTabIngredients) {
                    m_interface.m_inGame.m_craftingInventoryWidget->activateTab(CraftingTabIngredients);
                }
                updateFormulas();
            }
        }
        return true;
    case StaticEntityLoom:
        if (object->occupantId != m_world->m_localCreature->m_id) {
            m_interface.m_inGame.m_quickButtons[QuickSlotUse].button->setText(L"info", L"Loom", true);
            if (usePressed) {
                InventoryWidget *inventory = m_interface.m_inGame.m_craftingInventoryWidget;
                inventory->m_node->setVisible(!inventory->m_node->isVisible());
                if (m_interface.m_inGame.m_craftingInventoryWidget->m_tabIndex != CraftingTabArmor) {
                    m_interface.m_inGame.m_craftingInventoryWidget->activateTab(CraftingTabArmor);
                }
                updateFormulas();
            }
        }
        return true;
    case StaticEntitySawbench:
        if (object->occupantId != m_world->m_localCreature->m_id) {
            m_interface.m_inGame.m_quickButtons[QuickSlotUse].button->setText(L"info", L"Saw", true);
            if (usePressed) {
                InventoryWidget *inventory = m_interface.m_inGame.m_craftingInventoryWidget;
                inventory->m_node->setVisible(!inventory->m_node->isVisible());
                m_interface.m_inGame.m_craftingInventoryWidget->activateTab(CraftingTabIngredients);
                updateFormulas();
            }
        }
        return true;
    case StaticEntityWorkbench:
        if (object->occupantId != m_world->m_localCreature->m_id) {
            m_interface.m_inGame.m_quickButtons[QuickSlotUse].button->setText(L"info", L"Workbench", true);
            if (usePressed) {
                InventoryWidget *inventory = m_interface.m_inGame.m_craftingInventoryWidget;
                inventory->m_node->setVisible(!inventory->m_node->isVisible());
                if (m_interface.m_inGame.m_craftingInventoryWidget->m_tabIndex != CraftingTabWeapons) {
                    m_interface.m_inGame.m_craftingInventoryWidget->activateTab(CraftingTabWeapons);
                }
                updateFormulas();
            }
        }
        return true;
    case StaticEntityCustomizationBench:
        if (object->occupantId != m_world->m_localCreature->m_id) {
            m_interface.m_inGame.m_quickButtons[QuickSlotUse].button->setText(L"info", L"Customization Bench", true);
            if (usePressed) {
                m_interface.m_inGame.m_voxelWidget->cancelUpgrade();
                plasma::Node *node = m_interface.m_inGame.m_voxelWidget->m_node;
                node->setVisible(!node->isVisible());
            }
        }
        return true;
    case StaticEntityCampfire:
        if (object->occupantId != m_world->m_localCreature->m_id) {
            m_interface.m_inGame.m_quickButtons[QuickSlotUse].button->setText(L"info", L"Campfire", true);
            if (usePressed) {
                InventoryWidget *inventory = m_interface.m_inGame.m_craftingInventoryWidget;
                inventory->m_node->setVisible(!inventory->m_node->isVisible());
                if (m_interface.m_inGame.m_craftingInventoryWidget->m_tabIndex != CraftingTabCooking) {
                    m_interface.m_inGame.m_craftingInventoryWidget->activateTab(CraftingTabCooking);
                }
                updateFormulas();
            }
        }
        return true;
    default:
        return false;
    }
}

void Game::updateCamera(double step)
{
    float angleRate = std::pow(0.005f, (float)m_options.cameraSmoothness * 0.01f);
    float angleBlend = blendFactor(step, angleRate);
    m_cameraAngle += (m_targetCameraAngle - m_cameraAngle) * (double)angleBlend;
    m_cameraDistance += blendFactor(step, 0.004f) * (m_targetCameraDistance - m_cameraDistance);
    m_screenShake += blendFactor(step, 0.005f) * (0.0f - m_screenShake);

    Creature *local = m_world->m_localCreature;
    plasma::Vector<3, Fixed> focus = local->m_entityData.position;
    focus[2] += Fixed(-local->m_stepOffset);
    unsigned char ability = local->m_entityData.currentAbility;
    if (ability == AbilitySit || ability == AbilitySleep) {
        focus[2] += Fixed(local->m_entityData.appearance.size[0] * 0.5f);
    } else if (ability == AbilityHangGlide || !m_interface.m_characterCreationNode->isVisible()) {
        focus[2] += Fixed(local->m_entityData.appearance.size[2] * 0.5f + 0.5f);
    }
    if (ability == AbilityRidePet) {
        Creature *mount = m_world->creatureByID(local->m_petId);
        if (mount && mount->m_entityData.hp > 0.0f) {
            plasma::Vector<3, float> offset = riderOffset(mount->m_entityData.race);
            focus[2] += Fixed(mount->m_entityData.appearance.size[2] * 0.75f + offset[2]);
        } else {
            local->m_entityData.currentAbility = AbilityNone;
        }
    }
    if (m_world->m_localCreature->m_entityData.currentAbility == AbilitySailBoat)
        focus[2] += Fixed(2);

    for (;;) {
        Block block = m_world->getBlock(focus[0].toBlock(), focus[1].toBlock(), focus[2].toBlock());
        if (block.isSolid() || ((block.flags & BlockFlagBarrier) != 0 && (block.flags & BlockFlagSpiritPassable) == 0))
            break;
        Block again = m_world->getBlock(focus[0].toBlock(), focus[1].toBlock(), focus[2].toBlock());
        if (!again.isSolid())
            break;
        focus[2] += Fixed(1);
    }
    for (int x = 0; x < 2; ++x) {
        Fixed offsetX(((float)x - 0.5f) * 0.5f);
        for (int y = 0; y < 2; ++y) {
            Fixed sampleX = focus[0] + offsetX;
            Fixed sampleY = focus[1] + Fixed(((float)y - 0.5f) * 0.5f);
            for (int z = 0; z < 2; ++z) {
                Fixed sampleZ = focus[2] + Fixed(((float)z - 0.5f) * 0.5f);
                Block block = m_world->getBlockAtDot(sampleX, sampleY, sampleZ);
                if (block.isSolid()) {
                    focus = m_world->m_localCreature->m_entityData.position;
                    break;
                }
            }
        }
    }
    local = m_world->m_localCreature;
    ability = local->m_entityData.currentAbility;
    if (ability == AbilityEagleFlight) {
        m_cameraFocusPosition = local->m_entityData.position;
    } else if (ability == AbilityMenuView) {
        m_cameraFocusPosition = local->m_entityData.position + plasma::Vector<3, Fixed>(0, 0, 1);
    } else if (ability == AbilityFlightToPoint) {
        m_cameraFocusPosition += (focus - m_cameraFocusPosition) * Fixed(blendFactor(step, 0.25f));
    } else if (ability == AbilitySailBoat) {
        m_cameraFocusPosition += (focus - m_cameraFocusPosition) * Fixed(blend(step, 0.05f));
    } else {
        const plasma::Vector<3, float> &velocity = local->m_entityData.velocity;
        float speedFactor =
            std::exp(-(velocity[0] * velocity[0] + velocity[1] * velocity[1] + velocity[2] * velocity[2]));
        float rate = (float)((double)(1.0f - speedFactor) * 0.5 + 0.02500000037252903);
        Fixed factor(blendFactor(step, rate) + 0.0f);
        m_cameraFocusPosition[0] += factor * (focus[0] - m_cameraFocusPosition[0]);
        m_cameraFocusPosition[1] += factor * (focus[1] - m_cameraFocusPosition[1]);
        float verticalRate = (float)fabs((double)m_world->m_localCreature->m_stepOffset) * 0.005f + 0.0035f;
        m_cameraFocusPosition[2] += (focus[2] - m_cameraFocusPosition[2]) * blendFactor(step, verticalRate);
    }
    plasma::Vector<3, float> fromCreature;
    for (int axis = 0; axis < 3; ++axis)
        fromCreature[axis] =
            (float)(m_cameraFocusPosition[axis] - m_world->m_localCreature->m_entityData.position[axis]);
    if (fromCreature.squaredLength() > 2500.0f)
        m_cameraFocusPosition = m_world->m_localCreature->m_entityData.position;
    m_cameraPosition = m_cameraFocusPosition;

    plasma::Matrix<float> rotation;
    rotation.identity();
    rotation.rotateZ((float)m_cameraAngle[2]);
    rotation.rotateY((float)m_cameraAngle[1]);
    rotation.rotateX((float)m_cameraAngle[0]);
    plasma::Vector<3, float> direction = rotation.transformDirection(plasma::Vector<3, float>(0.0f, 0.0f, 1.0f));
    for (int axis = 0; axis < 3; ++axis)
        m_cameraPosition[axis] -= Fixed(direction[axis] * m_cameraDistance);

    local = m_world->m_localCreature;
    if (local->m_entityData.currentAbility != AbilityEagleFlight) {
        bool aboveWater =
            local->m_entityData.position[2] > Fixed(2) ||
            (local->m_entityData.flags & (CreatureFlagInWater | CreatureFlagPartlyAboveWater)) == CreatureFlagInWater;
        bool clear = true;
        for (int x = 0; x < 2; ++x) {
            for (int y = 0; y < 2; ++y) {
                for (int z = 0; z < 2; ++z) {
                    plasma::Vector<3, Fixed> sample(m_cameraPosition[0] + Fixed(((float)x - 0.5f) * 0.5f),
                                                    m_cameraPosition[1] + Fixed(((float)y - 0.5f) * 0.5f),
                                                    m_cameraPosition[2] + Fixed(((float)z - 0.5f) * 0.5f));
                    if (!m_world->inClearLine(sample, m_world->m_localCreature->m_entityData.position, aboveWater, true,
                                              200.0f, nullptr))
                        clear = false;
                }
            }
        }
        if (!clear) {
            plasma::Vector<3, float> towardCamera = direction * -1.0f;
            for (int x = 0; x < 2; ++x) {
                for (int y = 0; y < 2; ++y) {
                    for (int z = 0; z < 2; ++z) {
                        plasma::Vector<3, Fixed> sample(m_cameraFocusPosition[0] + Fixed(((float)x - 0.5f) * 0.5f),
                                                        m_cameraFocusPosition[1] + Fixed(((float)y - 0.5f) * 0.5f),
                                                        m_cameraFocusPosition[2] + Fixed(((float)z - 0.5f) * 0.5f));
                        float distance =
                            m_world->cameraRay(sample, towardCamera, m_cameraDistance, aboveWater, true, nullptr);
                        if (distance >= 0.0f)
                            m_cameraDistance = distance;
                    }
                }
            }
            for (int axis = 0; axis < 3; ++axis)
                m_cameraPosition[axis] = m_cameraFocusPosition[axis] - Fixed(m_cameraDistance * direction[axis]);
        }
        Block block = m_world->getBlockAtDot(m_cameraPosition[0], m_cameraPosition[1], m_cameraPosition[2]);
        if (block.isSolid()) {
            m_cameraPosition = m_world->m_localCreature->m_entityData.position;
            m_cameraFocusPosition = m_world->m_localCreature->m_entityData.position;
            m_cameraDistance = 0.0f;
        }
    }

    m_cameraMatrix.identity();
    m_cameraMatrix.identity();
    m_cameraMatrix.translate((float)m_cameraPosition[0], (float)m_cameraPosition[1], (float)m_cameraPosition[2]);
    m_cameraMatrix.rotateZ((float)m_cameraAngle[2]);
    m_cameraMatrix.rotateY((float)m_cameraAngle[1]);
    m_cameraMatrix.rotateX((float)m_cameraAngle[0]);

    m_worldViewMatrix.identity();
    m_worldViewMatrix.rotateX((float)-m_cameraAngle[0]);
    m_worldViewMatrix.rotateY((float)-m_cameraAngle[1]);
    m_worldViewMatrix.rotateZ((float)-m_cameraAngle[2]);
    m_worldViewMatrix.translate((float)(-m_cameraPosition[0]), (float)(-m_cameraPosition[1]),
                                (float)(-m_cameraPosition[2]));
    m_drawOriginX = -m_cameraFocusPosition[0];
    m_drawOriginY = -m_cameraFocusPosition[1];

    m_viewMatrix.identity();
    float shake = m_screenShake * 0.5f;
    float time = m_world->m_clock;
    float shakeY = std::cos(time * 0.04f) * shake;
    float shakeX = std::cos(time * 0.03f) * shake;
    m_viewMatrix.translate(shakeX, shakeY);
    m_viewMatrix.rotateX((float)-m_cameraAngle[0]);
    m_viewMatrix.rotateY((float)-m_cameraAngle[1]);
    m_viewMatrix.rotateZ((float)-m_cameraAngle[2]);
    m_viewMatrix.translate((float)(-m_cameraPosition[0] - m_drawOriginX), (float)(-m_cameraPosition[1] - m_drawOriginY),
                           (float)(-m_cameraPosition[2]));
    // Zoomed in while leaning forward (aiming).
    float factor;
    float target;
    if (m_world->m_localCreature->m_entityData.binaryToggles.leanForwardPose) {
        factor = blendFactor(step, 0.01f);
        target = 0.52359879f;
    } else {
        factor = blendFactor(step, 0.002f);
        target = 0.87266469f;
    }
    s_cameraFieldOfView += factor * (target - s_cameraFieldOfView);
    gfx::perspectiveFov(m_projectionMatrix, s_cameraFieldOfView, -((float)m_width / (float)m_height), 0.05f, 1920.0f);
}

// Whether a plant's sound period ends in this frame: the world time is shifted by the plant's place (in blocks, y by
// 333) so that plants do not sound together.
static inline bool soundPeriodEnds(float time, int milliseconds, const Fixed &x, Fixed y, int span, int period)
{
    // Phase arithmetic uses truncating division of fixed-point positions and signed integer sums.
    int oldPhase =
        ((int)time - ((int)x / span + (int)(y + Fixed(333)) / span) * span + (int)x + (int)(y + Fixed(333))) / period;
    int newPhase = ((int)time + (int)x % span + (int)(y + Fixed(333)) % span + milliseconds) / period;
    return oldPhase != newPhase;
}

void Game::updateClimateSounds(int milliseconds)
{
    unsigned int now = timeGetTime();
    std::set<std::pair<int, int>> changedZones;
    for (const CubeChange &change : m_world->m_serverUpdates.m_cubeChangeList) {
        for (int dx = -CubeChangeRemeshMargin; dx <= CubeChangeRemeshMargin; dx += CubeChangeRemeshMargin) {
            for (int dy = -CubeChangeRemeshMargin; dy <= CubeChangeRemeshMargin; dy += CubeChangeRemeshMargin) {
                changedZones.insert(std::pair<int, int>(floorDivide(change.position[0] + dx, BlocksPerZone),
                                                        floorDivide(change.position[1] + dy, BlocksPerZone)));
            }
        }
    }
    for (const std::pair<int, int> &position : changedZones) {
        Zone *zone = m_world->zone(position.first, position.second);
        if (zone && !zone->m_chunk.needsRemesh) {
            zone->m_chunk.needsRemesh = true;
            zone->m_chunk.remeshTime = now;
        }
    }

    Creature *local = m_world->m_localCreature;
    std::pair<int, int> center(floorDivide(local->m_entityData.position[0].toBlock(), BlocksPerZone),
                               floorDivide(local->m_entityData.position[1].toBlock(), BlocksPerZone));
    if (local->m_entityData.currentAbility == AbilityEagleFlight) {
        center = std::pair<int, int>(floorDivide(local->m_entityData.destination[0], BlocksPerZone),
                                     floorDivide(local->m_entityData.destination[1], BlocksPerZone));
    }
    for (int x = center.first - 1; x <= center.first + 1; ++x) {
        for (int y = center.second - 1; y <= center.second + 1; ++y) {
            // Only the zones within 64 blocks of the camera (the squared distance the renderer keeps).
            Zone *zone = m_world->zone(x, y);
            if (!zone || !(zone->m_chunk.squaredCameraDistance < 4096.0f)) {
                continue;
            }
            int plantIndex = 0;
            for (const Zone::Plant &plant : zone->m_soundPlants) {
                ++plantIndex;
                if (plant.type == PlantLeafCanopy) {
                    if ((int)m_world->m_clock / 1000 != ((int)m_world->m_clock + milliseconds) / 1000 &&
                        rand() % 16 == 0) {
                        Particle particle;
                        particle.motionDisabled = false;
                        particle.age = 0;
                        particle.position = plant.position;
                        float velocityY = (float)rand() / (float)RAND_MAX - 0.5f;
                        float velocityX = (float)rand() / (float)RAND_MAX - 0.5f;
                        particle.acceleration = plasma::Vector<3, float>(velocityX * 4.0f, velocityY * 4.0f, -2.0f);
                        particle.rotation = plasma::Vector<3, float>(0.0f, 0.0f, 0.0f);
                        float spinZ = (float)rand() / (float)RAND_MAX - 0.5f;
                        float spinY = (float)rand() / (float)RAND_MAX - 0.5f;
                        float spinX = (float)rand() / (float)RAND_MAX - 0.5f;
                        particle.spin = plasma::Vector<3, float>(spinX * 10.0f, spinY * 10.0f, spinZ * 10.0f);
                        particle.color = plasma::Vector<4, float>(plant.color[0], plant.color[1], plant.color[2], 1.0f);
                        particle.size = 0.5f;
                        particle.type = ParticleLeaf;
                        particle.matrix.identity();
                        particle.lifetime = 20000;
                        particle.collidesWithBlocks = true;
                        m_particles.push_front(particle);
                    }
                    continue;
                }

                if (plant.type == PlantRiverSound) {
                    if (soundPeriodEnds(m_world->m_clock, milliseconds, Fixed(plant.position[0].m_raw * 13),
                                        Fixed(plant.position[1].m_raw * 13), 1000, 500) &&
                        rand() % 4 == 0) {
                        float speed = (float)rand() * 0.4f / (float)RAND_MAX + 0.8f;
                        int sound = rand() % 3 + 104;
                        playSound(sound, plant.position, 0.1f, speed, false);
                    }
                    continue;
                }

                float daytime = m_world->m_state.m_time;
                if (daytime > 6 * MillisecondsPerHour && daytime < 21 * MillisecondsPerHour &&
                    plant.type == PlantBirdAndOwlSounds) {
                    if (soundPeriodEnds(m_world->m_clock, milliseconds, plant.position[0], plant.position[1], 1000,
                                        1000) &&
                        rand() % 10 == 0) {
                        float speed = (float)rand() * 0.2f / (float)RAND_MAX + 0.9f;
                        playSound(SoundBird1, plant.position, 0.1f, speed, false);
                    }
                } else if (daytime > 6 * MillisecondsPerHour && daytime < 21 * MillisecondsPerHour &&
                           plant.type == PlantBird2Sound) {
                    if (soundPeriodEnds(m_world->m_clock, milliseconds, plant.position[0], plant.position[1], 1000,
                                        1000) &&
                        rand() % 10 == 0) {
                        float speed = (float)rand() * 0.2f / (float)RAND_MAX + 0.9f;
                        playSound(SoundBird2, plant.position, 0.1f, speed, false);
                    }
                } else if (daytime > 6 * MillisecondsPerHour && daytime < 21 * MillisecondsPerHour &&
                           plant.type == PlantBird3Sound) {
                    if (soundPeriodEnds(m_world->m_clock, milliseconds, plant.position[0], plant.position[1], 1000,
                                        1000) &&
                        rand() % 10 == 0) {
                        float speed = (float)rand() * 0.2f / (float)RAND_MAX + 0.9f;
                        playSound(SoundBird3, plant.position, 0.1f, speed, false);
                    }
                } else if (!(daytime > 6 * MillisecondsPerHour && daytime < 21 * MillisecondsPerHour) &&
                           plant.type == PlantBirdAndOwlSounds) {
                    if (soundPeriodEnds(m_world->m_clock, milliseconds, plant.position[0], plant.position[1], 1000,
                                        1000) &&
                        rand() % 10 == 0) {
                        float speed = (float)rand() * 0.2f / (float)RAND_MAX + 0.5f;
                        int sound = rand() % 2 + 102;
                        playSound(sound, plant.position, 0.1f, speed, false);
                    }
                } else if (daytime > 12 * MillisecondsPerHour && daytime < MillisecondsPerDay && plantIndex % 20 == 0) {
                    if (plant.type == PlantGrass || plant.type == PlantGrass2 || plant.type == PlantGrass3 ||
                        plant.type == PlantGrassDry1 || plant.type == PlantGrassDry2) {
                        if (soundPeriodEnds(m_world->m_clock, milliseconds, plant.position[0], plant.position[1], 5000,
                                            5000) &&
                            rand() % 70 == 0) {
                            float speed = (float)rand() * 0.2f / (float)RAND_MAX + 0.9f;
                            playSound(SoundCricket1, plant.position, 0.05f, speed, false);
                        }
                    } else if ((unsigned int)plant.type <= PlantFlowers2 || plant.type == PlantSavannahPlant1 ||
                               plant.type == PlantSavannahPlant2) {
                        if (soundPeriodEnds(m_world->m_clock, milliseconds, plant.position[0], plant.position[1], 5000,
                                            5000) &&
                            rand() % 70 == 0) {
                            float speed = (float)rand() * 0.2f / (float)RAND_MAX + 0.9f;
                            playSound(SoundCricket2, plant.position, 0.05f, speed, false);
                        }
                    }
                }
            }
        }
    }
}

void Game::updateCrafting(int milliseconds)
{
    if (m_interface.m_inGame.m_formulaDetailsWidget->getNode()->isVisible()) {
        if (m_interface.m_inGame.m_formulaDetailsWidget->m_craftButtonHovered && m_engine->isButtonDown(0)) {
            if (m_craftingProgress < 0.0f) {
                m_craftingProgress = 0.0f;
                m_craftingFormula = m_interface.m_inGame.m_formulaDetailsWidget->m_formula;
                m_craftingCell = m_interface.m_inGame.m_formulaDetailsWidget->m_formulaCell;
                if (m_craftingFormula.result.hasRegion()) {
                    m_craftingFormula.result.region = m_world->getLocalCreature()->m_entityData.region;
                }
            }
        } else {
            m_craftingSpeed = 0.001f;
        }
        if (m_craftingProgress >= 0.0f) {
            m_craftingProgress += (float)milliseconds * m_craftingSpeed;
            if (m_craftingProgress >= 1.0f) {
                m_craftingProgress = -1.0f;
                m_craftingSpeed *= 1.2f;
                if (m_craftingSpeed > 0.005f) {
                    m_craftingSpeed = 0.005f;
                }
                craft();
            }
        }
        m_interface.m_inGame.m_formulaDetailsWidget->setProgress(m_craftingProgress);
    } else {
        m_craftingProgress = -1.0f;
        m_craftingFormula = Formula();
        m_craftingCell = plasma::Vector<2, int>(-1, -1);
    }
}

// Rebuilds what the crafting inventory shows. The formulas the character knows go to the tabs by what they make
// (weapons 0, food 3, potions and elixirs 3 or 4, materials 5); every crafting rank the character holds in its region
// adds the class's weapons (tab 0), armour (tab 1) and jewellery (tab 2) in that rarity, the same ones each time
// because the modifiers come from rand() seeded with the region. Tab 6 and the two maps of the rows that stand for the
// player's own items are emptied.
void Game::updateCreatureLabels(std::vector<NearbyCreature> &creatures)
{
    m_interface.m_lifeBars->destroyChildren();
    m_interface.m_inGame.m_lockedTarget->m_display->m_visibility.value() = 0;
    m_interface.m_inGame.m_highlightedTarget->m_display->m_visibility.value() = 0;
    if (m_interface.isInMenu())
        return;
    for (std::vector<NearbyCreature>::reverse_iterator it = creatures.rbegin(); it != creatures.rend(); ++it) {
        Creature *local = m_world->m_localCreature;
        if (it->creature == local ||
            (local->m_entityData.currentAbility == AbilityRidePet && it->creature->m_entityData.ownerId == local->m_id))
            continue;
        if (it->creature->m_entityData.race == RaceWaterBubble ||
            (it->creature->isGate() && it->creature->m_entityData.interactionState != 0))
            continue;
        plasma::Vector<3, Fixed> position = it->creature->m_drawnPosition;
        if (it->creature->m_entityData.currentAbility == AbilityRidePet) {
            Creature *mount = m_world->creatureByID(it->creature->m_petId);
            if (mount) {
                position[2] += Fixed(mount->m_entityData.appearance.size[2] * 0.75f);
                position[1] -= Fixed(0.25f);
                position += plasma::Vector<3, Fixed>(riderOffset(mount->m_entityData.race));
            }
        }
        if (!m_world->inRange(m_cameraPosition, position, false, 200.0f, it->creature))
            continue;
        plasma::Vector<3, float> view =
            m_viewMatrix *
            plasma::Vector<3, float>(
                position + plasma::Vector<3, Fixed>(m_drawOriginX, m_drawOriginY, 0) +
                plasma::Vector<3, Fixed>(plasma::Vector<3, float>(0.0f, 0.0f,
                                                                  it->creature->m_entityData.appearance.size[2] * 0.5f -
                                                                      0.25f - it->creature->m_stepOffset)));
        if (it->creature->m_entityData.currentAbility == AbilitySailBoat ||
            it->creature->m_entityData.currentAbility == AbilityRidePet)
            view[2] += 2.0f;
        if (it->creature->m_entityData.ownerId && it->creature->m_entityData.race != RaceManaMech) {
            Creature *owner = m_world->creatureByID(it->creature->m_entityData.ownerId);
            if (owner && owner->m_entityData.currentAbility == AbilityRidePet)
                continue;
        }
        if (!(view[2] > 0.0f))
            continue;
        plasma::Vector<3, float> projection = m_projectionMatrix * view;
        float screenX =
            (float)m_engine->viewportWidth() * 0.5f * projection[0] + (float)m_engine->viewportWidth() * 0.5f;
        float screenY =
            (float)-m_engine->viewportHeight() * 0.5f * projection[1] + (float)m_engine->viewportHeight() * 0.5f;
        if (screenY < m_interface.m_scale * 50.0f)
            screenY = m_interface.m_scale * 50.0f;
        plasma::Node *label;
        if (it->creature->m_id == m_crosshairCreatureId) {
            plasma::Node *prototype;
            if (it->creature->m_entityData.hostilityType == HostilityEnemy)
                prototype = it->creature->isMarked() ? m_interface.m_neutralLifeBar : m_interface.m_enemyLifeBar;
            else if (it->creature->m_entityData.hostilityType == HostilityAttackableObject)
                prototype = m_interface.m_staticLifeBar;
            else
                prototype = m_interface.m_friendLifeBar;
            label = prototype->instance(m_interface.m_lifeBars);
            if (it->creature->m_entityData.hostilityType == HostilityInteractiveObject) {
                for (std::list<plasma::Node *>::iterator child = label->m_children.begin();
                     child != label->m_children.end(); ++child) {
                    if ((*child)->getName() != L"info") {
                        (*child)->setDisplay((*child)->m_display->clone());
                        (*child)->m_display->m_visibility.value() = 0;
                    }
                }
            }
        } else {
            if (!(it->creature->m_entityData.hp < it->creature->maxHP()) || !(it->squaredDistance < 1600.0f) ||
                it->creature->m_entityData.hostilityType == HostilityInteractiveObject)
                continue;
            plasma::Node *prototype;
            if (it->creature->m_entityData.hostilityType == HostilityEnemy)
                prototype =
                    it->creature->isMarked() ? m_interface.m_neutralLifeBarSmall : m_interface.m_enemyLifeBarSmall;
            else if (it->creature->m_entityData.hostilityType == HostilityAttackableObject)
                prototype = m_interface.m_staticLifeBarSmall;
            else
                prototype = m_interface.m_friendLifeBarSmall;
            label = prototype->instance(m_interface.m_lifeBars);
            if (it->creature->m_entityData.hostilityType == HostilityInteractiveObject) {
                for (std::list<plasma::Node *>::iterator child = label->m_children.begin();
                     child != label->m_children.end(); ++child) {
                    if ((*child)->getName() != L"info") {
                        (*child)->setDisplay((*child)->m_display->clone());
                        (*child)->m_display->m_visibility.value() = 0;
                    }
                }
            }
        }
        if (!label)
            continue;
        if (it->creature->m_id != m_crosshairCreatureId) {
            for (std::list<plasma::Node *>::iterator child = label->m_children.begin();
                 child != label->m_children.end(); ++child) {
                if ((*child)->getName() == L"rarename" || (*child)->getName() == L"info" ||
                    (*child)->getName() == L"faction") {
                    (*child)->setDisplay((*child)->m_display->clone());
                    (*child)->m_display->m_visibility.value() = 0;
                }
            }
        }
        label->setTransformation(label->m_transformation->clone());
        label->setDisplay(label->m_display->clone());
        std::list<plasma::Node *> textNodes;
        label->findNodes(textNodes, L"info");
        label->findNodes(textNodes, L"rarename");
        label->findNodes(textNodes, L"text");
        label->findNodes(textNodes, L"faction");
        for (plasma::Node *textNode : textNodes) {
            if (textNode->m_shape)
                textNode->setShape(textNode->m_shape->clone());
        }
        plasma::Node *bar = label->findNode(L"bar");
        bar->setTransformation(bar->m_transformation->clone());
        plasma::Node *bar2 = label->findNode(L"bar2");
        bar2->setTransformation(bar2->m_transformation->clone());
        label->setPosition(screenX, screenY, 0.0f, 0.0f);

        std::wstringstream text(std::ios::in | std::ios::out);
        if (strlen(it->creature->m_entityData.name) > 0)
            text << WidenString(std::string(it->creature->m_entityData.name));
        else if (it->creature->m_entityData.appearance.appearanceFlags & AppearanceFlagTinted)
            text << randomName((int)it->creature->m_id, it->creature->m_entityData.race);
        else if (it->creature->m_entityData.hostilityType == HostilityFriendlyNpc)
            text << randomName((int)it->creature->m_id, it->creature->m_entityData.race);
        label->setText(L"rarename", text.str(), true);
        text.str(L"");
        if (it->creature->m_entityData.factionKind)
            text << L"["
                 << m_speech.factionName(it->creature->m_entityData.factionKind, it->creature->m_entityData.factionSeed,
                                         it->creature->m_entityData.factionVariant)
                 << L"]";
        switch (it->creature->m_entityData.classType) {
        case ClassItemVendor:
            text << L"[Item Vendor]";
            break;
        case ClassWeaponVendor:
            text << L"[Weapon Vendor]";
            break;
        case ClassArmorVendor:
            text << L" [Armor Vendor]";
            break;
        case ClassAnalyzer:
            text << L"[Analyzer]";
            break;
        case ClassInnkeeper:
            text << L"[Innkeeper]";
            break;
        case ClassArchaeologist:
            text << L"[Archaeologist]";
            break;
        case ClassAlchemist:
            text << L"[Alchemist]";
            break;
        case ClassSmith:
            text << L"[Smith]";
            break;
        case ClassCarpenter:
            text << L"[Carpenter]";
            break;
        case ClassTailor:
            text << L"[Tailor]";
            break;
        case ClassFlightMaster:
            text << L"[Flight Master]";
            break;
        case ClassGuildReceptionist:
            text << L"[Guild Receptionist]";
            break;
        case ClassGemTrader:
            text << L"[Gem Trader]";
            break;
        case ClassShopSupplier:
            text << L"[Shop Supplier]";
            break;
        case ClassMasterSailor:
            text << L"[Master Sailor]";
            break;
        case ClassMasterGlider:
            text << L"[Master Glider]";
            break;
        }
        if (it->creature->m_entityData.appearance.appearanceFlags & AppearanceFlagClassMaster) {
            switch (it->creature->m_entityData.classType) {
            case ClassWarrior:
                text << L"[Warrior Master]";
                break;
            case ClassRanger:
                text << L"[Ranger Master]";
                break;
            case ClassMage:
                text << L"[Mage Master]";
                break;
            case ClassRogue:
                text << L"[Rogue Master]";
                break;
            }
        }
        if (!text.str().empty()) {
            std::list<plasma::Node *> shiftedNodes;
            label->findNodes(shiftedNodes, L"info");
            label->findNodes(shiftedNodes, L"rarename");
            label->findNodes(shiftedNodes, L"faction");
            for (plasma::Node *shifted : shiftedNodes) {
                shifted->setTransformation(shifted->m_transformation->clone());
                shifted->m_transformation->m_translation.value()[1] -= 12.0f;
                shifted->m_transformation->attributesChanged(true);
            }
        }
        label->setText(L"faction", text.str(), true);
        text.str(L"");

        if (it->creature->m_entityData.hostilityType == HostilityPlayer)
            text << L"LV " << it->creature->m_entityData.level << L" ";
        std::list<plasma::Node *> infoNodes;
        label->findNodes(infoNodes, L"info");
        plasma::Vector<4, float> color(1.0f, 1.0f, 1.0f, 1.0f);
        if (it->creature->m_entityData.appearance.appearanceFlags & AppearanceFlagPromotedResident)
            text << L"Possessed ";
        text << m_speech.getText(m_speech.creatureKey(it->creature->m_entityData.race), L"default", L"");
        if (it->creature->m_entityData.hostilityType == HostilityPlayer) {
            color = plasma::Vector<4, float>(0.0f, 0.8f, 1.0f, 1.0f);
        } else if (it->creature->m_entityData.hostilityType == HostilityEnemy ||
                   it->creature->m_entityData.hostilityType == HostilityAttackableObject) {
            color = rarityColor(it->creature->m_entityData.level);
            if (m_options.showRarity) {
                text << L" ";
                for (int rarity = 0; rarity <= it->creature->m_entityData.level; ++rarity)
                    text << L"\u00b1";
            }
        }
        label->setTextColor(L"info", color, true);
        label->setText(L"info", text.str(), true);
        text.str(L"");
        text << getFoldedCountText((int)it->creature->m_entityData.hp) << "/"
             << getFoldedCountText((int)it->creature->maxHP());
        label->setText(L"text", text.str(), true);
        bar->m_transformation->m_deformation.value().identity();
        bar->m_transformation->m_deformation.value().scale(it->creature->m_entityData.hp / it->creature->maxHP(), 1.0f,
                                                           1.0f);
        bar->m_transformation->attributesChanged(true);
        bar2->m_transformation->m_deformation.value().identity();
        bar2->m_transformation->m_deformation.value().scale(
            1.0f - it->creature->m_entityData.hp / it->creature->maxHP(), 1.0f, 1.0f);
        bar2->m_transformation->attributesChanged(true);
        if (m_highlightedCreatureId && it->creature->m_id == m_highlightedCreatureId) {
            m_interface.m_inGame.m_highlightedTarget->m_display->m_visibility.value() = 1;
            float offsetY = cosf((float)m_engine->getCaretClock() * 0.01f) * 5.0f - 60.0f;
            m_interface.m_inGame.m_highlightedTarget->setPosition(plasma::Vector<2, float>(screenX, screenY),
                                                                  plasma::Vector<2, float>(0.0f, offsetY));
        }
    }
}

void Game::updateFormulas()
{
    if (m_world->m_localCreature->m_inventory.m_inventoryTabs.empty())
        return;
    std::vector<Formula> weapons;
    std::vector<Formula> armor;
    std::vector<Formula> jewelry;
    std::vector<Formula> food;
    std::vector<Formula> potions;
    std::vector<Formula> materials;
    for (std::list<Item>::iterator it = m_world->m_localCreature->m_playerData->knownFormulas.begin();
         it != m_world->m_localCreature->m_playerData->knownFormulas.end(); ++it) {
        Formula formula;
        if (m_world->groundedFormula(*it, formula, false)) {
            switch (it->category) {
            case ItemCategoryConsumable:
                if ((it->id >= ConsumableLifePotion && it->id <= ConsumableEssenceOfWisdom) ||
                    it->id == ConsumableGreaterLifePotion)
                    potions.push_back(formula);
                else
                    food.push_back(formula);
                break;
            case ItemCategoryWeapon:
                weapons.push_back(formula);
                break;
            case ItemCategoryMaterial:
                materials.push_back(formula);
                break;
            case ItemCategoryElixir:
                if (it->id >= ElixirLemonade && it->id <= ElixirGreenSmoothie)
                    food.push_back(formula);
                else
                    potions.push_back(formula);
                break;
            }
        }
    }
    srand(random(m_world->m_localCreature->m_entityData.region[0] + 62969,
                 m_world->m_localCreature->m_entityData.region[1]));
    for (int rank = 0; rank < 5; rank++) {
        if (!m_world->hasCraftingRank(rank))
            continue;
        srand(random(m_world->m_localCreature->m_entityData.region[0] + 62969,
                     m_world->m_localCreature->m_entityData.region[1]) +
              rank);
        Formula formula;
        Item item;
        item.rarity = rank;
        item.region = m_world->m_localCreature->m_entityData.region;
        item.category = ItemCategoryWeapon;
        switch (m_world->m_localCreature->m_entityData.classType) {
        case ClassWarrior:
            item.id = WeaponSword + rand() % 3;
            item.material = ItemMaterialIron;
            item.modifier = rand();
            if (m_world->groundedFormula(item, formula, false))
                weapons.push_back(formula);
            item.id = WeaponShield;
            item.modifier = rand();
            if (m_world->groundedFormula(item, formula, false))
                weapons.push_back(formula);
            item.id = WeaponGreatsword + rand() % 3;
            item.modifier = rand();
            if (m_world->groundedFormula(item, formula, false))
                weapons.push_back(formula);
            break;
        case ClassRanger:
            item.id = WeaponBow;
            item.material = ItemMaterialWood;
            item.modifier = rand();
            if (m_world->groundedFormula(item, formula, false))
                weapons.push_back(formula);
            item.id = WeaponCrossbow;
            item.modifier = rand();
            if (m_world->groundedFormula(item, formula, false))
                weapons.push_back(formula);
            item.id = WeaponBoomerang;
            item.modifier = rand();
            if (m_world->groundedFormula(item, formula, false))
                weapons.push_back(formula);
            break;
        case ClassMage:
            item.id = WeaponWand;
            item.material = ItemMaterialWood;
            item.modifier = rand();
            if (m_world->groundedFormula(item, formula, false))
                weapons.push_back(formula);
            item.id = WeaponStaff;
            item.modifier = rand();
            if (m_world->groundedFormula(item, formula, false))
                weapons.push_back(formula);
            item.id = WeaponBracelet;
            item.material = rand() % 2 ? ItemMaterialSilver : ItemMaterialGold;
            item.modifier = rand();
            if (m_world->groundedFormula(item, formula, false))
                weapons.push_back(formula);
            break;
        case ClassRogue:
            item.id = WeaponDagger;
            item.material = ItemMaterialIron;
            item.modifier = rand();
            if (m_world->groundedFormula(item, formula, false))
                weapons.push_back(formula);
            item.id = WeaponFist;
            item.modifier = rand();
            if (m_world->groundedFormula(item, formula, false))
                weapons.push_back(formula);
            item.id = WeaponLongsword;
            item.modifier = rand();
            if (m_world->groundedFormula(item, formula, false))
                weapons.push_back(formula);
            break;
        }
        // The armour is of the class's material.
        switch (m_world->m_localCreature->m_entityData.classType) {
        case ClassWarrior:
            item.material = ItemMaterialIron;
            break;
        case ClassRanger:
            item.material = ItemMaterialLinen;
            break;
        case ClassMage:
            item.material = ItemMaterialSilk;
            break;
        case ClassRogue:
            item.material = ItemMaterialCotton;
            break;
        }
        item.id = WeaponSword;
        item.category = ItemCategoryChest;
        item.modifier = rand();
        if (m_world->groundedFormula(item, formula, false))
            armor.push_back(formula);
        item.category = ItemCategoryShoulder;
        item.modifier = rand();
        if (m_world->groundedFormula(item, formula, false))
            armor.push_back(formula);
        item.category = ItemCategoryHands;
        item.modifier = rand();
        if (m_world->groundedFormula(item, formula, false))
            armor.push_back(formula);
        item.category = ItemCategoryFeet;
        item.modifier = rand();
        if (m_world->groundedFormula(item, formula, false))
            armor.push_back(formula);
        item.category = ItemCategoryNeck;
        item.material = rand() % 2 ? ItemMaterialSilver : ItemMaterialGold;
        item.modifier = rand();
        if (m_world->groundedFormula(item, formula, false))
            jewelry.push_back(formula);
        item.category = ItemCategoryRing;
        item.material = rand() % 2 ? ItemMaterialSilver : ItemMaterialGold;
        item.modifier = rand();
        if (m_world->groundedFormula(item, formula, false))
            jewelry.push_back(formula);
    }
    updateFormulaTab(CraftingTabWeapons, weapons);
    updateFormulaTab(CraftingTabArmor, armor);
    updateFormulaTab(CraftingTabAmulets, jewelry);
    updateFormulaTab(CraftingTabCooking, food);
    updateFormulaTab(CraftingTabAlchemy, potions);
    updateFormulaTab(CraftingTabIngredients, materials);
    if (m_craftingInventory.m_inventoryTabs.size() < CraftingTabRepair + 1)
        m_craftingInventory.m_inventoryTabs.resize(CraftingTabRepair + 1);
    m_craftingInventory.m_inventoryTabs[CraftingTabRepair].clear();
    m_equippedCraftingItems.clear();
    m_craftingSources.clear();
    // Nothing is done with the player's stacks: the loop over them is empty.
    for (int tab = 0; tab < (int)m_world->m_localCreature->m_inventory.m_inventoryTabs.size(); tab++) {
        for (int slot = 0; slot < (int)m_world->m_localCreature->m_inventory.m_inventoryTabs[tab].size(); slot++) {
        }
    }
    m_interface.m_inGame.m_craftingInventoryWidget->updateRows();
    m_interface.m_inGame.m_craftingInventoryWidget->updateScrollButton();
}

// Fills one tab of the crafting inventory: a stack per formula whose count is how often the player's inventory covers
// the ingredients (0 for a formula without ingredients). The formula in the selected cell is shown again in the
// formula details.
void Game::updateFormulaTab(int tab, std::vector<Formula> &formulas)
{
    std::vector<std::vector<ItemStack>> &tabs = m_craftingInventory.m_inventoryTabs;
    if ((int)tabs.size() < tab + 1)
        tabs.resize(tab + 1);
    tabs[tab].clear();
    int row = 0;
    for (Formula &formula : formulas) {
        int count = -1;
        std::vector<std::vector<ItemStack>> &playerTabs = m_world->m_localCreature->m_inventory.m_inventoryTabs;
        for (Ingredient &ingredient : formula.ingredients) {
            int owned = 0;
            for (std::vector<ItemStack> &playerStacks : playerTabs) {
                for (ItemStack &stack : playerStacks) {
                    if (stack.item == ingredient.item)
                        owned += stack.quantity;
                }
            }
            int times = owned / ingredient.quantity;
            if (count < 0 || times < count)
                count = times;
        }
        ItemStack stack;
        stack.item = formula.result;
        stack.quantity = formula.ingredients.empty() ? 0 : count;
        tabs[tab].push_back(stack);
        if (tab == m_interface.m_inGame.m_craftingInventoryWidget->getSelectedCell()[0] &&
            row == m_interface.m_inGame.m_craftingInventoryWidget->getSelectedCell()[1]) {
            m_interface.m_inGame.m_formulaDetailsWidget->m_formula = formula;
            m_interface.m_inGame.m_formulaDetailsWidget->m_formulaCell =
                m_interface.m_inGame.m_craftingInventoryWidget->getSelectedCell();
        }
        row++;
    }
}

void Game::updateFriends()
{
    int count = SteamFriends()->GetFriendCount(k_EFriendFlagImmediate);
    if (count == -1) {
        count = 0;
    }
    m_interface.m_inGame.m_multiplayerWidget->m_friends.clear();
    for (int i = 0; i < count; i++) {
        CSteamID friendId = SteamFriends()->GetFriendByIndex(i, k_EFriendFlagImmediate);
        if (!friendId.IsValid()) {
            continue;
        }
        m_interface.m_inGame.m_multiplayerWidget->m_friends.push_back(
            std::make_pair(friendId, WidenString(SteamFriends()->GetFriendPersonaName(friendId))));
    }
    m_interface.m_inGame.m_multiplayerWidget->updateScrollbarPosition();
}

void Game::updateLights(int milliseconds)
{
    std::list<std::list<Light>::iterator> expired;
    for (std::list<Light>::iterator it = m_lights.begin(); it != m_lights.end(); ++it) {
        it->age += milliseconds;
        if (it->age > it->duration) {
            expired.push_back(it);
        }
    }
    for (std::list<std::list<Light>::iterator>::iterator it = expired.begin(); it != expired.end(); ++it) {
        m_lights.erase(*it);
    }
}

void Game::updateFx(int milliseconds)
{
    std::list<std::list<Particle>::iterator> expired;
    if ((int)m_particles.size() > 1000)
        m_particles.resize(1000);
    for (std::list<Particle>::iterator it = m_particles.begin(); it != m_particles.end(); ++it) {
        Particle &particle = *it;
        particle.age += milliseconds;
        if (particle.age >= 0 && !particle.motionDisabled && particle.type != ParticleStationary) {
            if (particle.type != ParticleMusicNote) {
                if (particle.type != ParticleLeaf && particle.type != ParticleShrinkingCube)
                    particle.acceleration[2] -= ((float)milliseconds * 0.001f) * 30.0f;
                particle.rotation += particle.spin * ((float)milliseconds * 0.001f);
            }
            if (particle.type == ParticleWaterSplash && particle.acceleration[2] < 0.0f) {
                plasma::Vector<3, float> displacement = particle.acceleration * ((float)milliseconds * 0.001f);
                int steps = (int)sqrt(displacement[1] * displacement[1] + displacement[0] * displacement[0] +
                                      displacement[2] * displacement[2]) +
                            1;
                if (steps > 10) {
                    steps = 10;
                }
                displacement = displacement * (1.0f / (float)steps);
                plasma::Vector<3, Fixed> dots(displacement);
                for (int step = 0; step < steps; ++step) {
                    particle.position += dots;
                    Block block =
                        m_world->getBlockAtDot(particle.position[0], particle.position[1], particle.position[2]);
                    if (block.type != BlockAir) {
                        expired.push_back(it);
                        break;
                    }
                }
            } else if (particle.collidesWithBlocks) {
                plasma::Vector<3, float> displacement = particle.acceleration * ((float)milliseconds * 0.001f);
                int steps = (int)displacement.length() + 1;
                if (steps > 10) {
                    steps = 10;
                }
                displacement = displacement * (1.0f / (float)steps);
                for (int step = 0; step < steps; ++step) {
                    for (int axis = 0; axis < 3; ++axis) {
                        float delta = displacement[axis];
                        Fixed dots(delta);
                        particle.position[axis] += dots;
                        Block block =
                            m_world->getBlockAtDot(particle.position[0], particle.position[1], particle.position[2]);
                        if (block.isSolid() &&
                            !(particle.type == ParticleLeaf && (block.type == BlockTree || block.type == BlockLeaves ||
                                                                block.type == BlockSnowLeaves))) {
                            particle.position[axis] -= Fixed(delta);
                            particle.acceleration[axis] = 0;
                            if (axis == 2) {
                                particle.acceleration = plasma::Vector<3, float>(0, 0, 0);
                                particle.spin = plasma::Vector<3, float>(0, 0, 0);
                            }
                        }
                    }
                }
            } else {
                plasma::Vector<3, float> displacement = particle.acceleration * ((float)milliseconds * 0.001f);
                particle.position += plasma::Vector<3, Fixed>(displacement);
                particle.rotation += particle.spin * ((float)milliseconds * 0.001f);
            }
        }
        if (particle.age > particle.lifetime)
            expired.push_back(it);
    }
    for (std::list<Particle>::iterator it : expired)
        m_particles.erase(it);
}

void Game::updateSorting()
{
    // Empty stacks sort last and are cut off below. The original tested b's emptiness first, so two empty stacks each
    // sorted before the other; MSVC's std::sort tolerates that, libstdc++'s runs past the end of the vector.
    // Equipment: items of the shown region first, then what the player can equip, then by power.
    std::sort(m_world->m_localCreature->m_inventory.m_inventoryTabs[0].begin(),
              m_world->m_localCreature->m_inventory.m_inventoryTabs[0].end(),
              [this](const ItemStack &a, const ItemStack &b) -> bool {
                  if (a.quantity == 0) {
                      return false;
                  }
                  if (b.quantity == 0) {
                      return true;
                  }
                  bool aLocal = m_interface.m_inGame.m_equipmentInventoryWidget->m_region == a.item.region;
                  bool bLocal = m_interface.m_inGame.m_equipmentInventoryWidget->m_region == b.item.region;
                  bool aUsable = m_world->getLocalCreature()->canEquipItem(a.item);
                  bool bUsable = m_world->getLocalCreature()->canEquipItem(b.item);
                  return (aLocal && !bLocal) ||
                         (aLocal == bLocal &&
                          ((aUsable && !bUsable) ||
                           (aUsable == bUsable && a.item.power(m_world->getLocalCreature(), 0) >
                                                      b.item.power(m_world->getLocalCreature(), 0))));
              });
    for (int i = 2; i < (int)m_world->m_localCreature->m_inventory.m_inventoryTabs.size(); i++) {
        std::sort(m_world->m_localCreature->m_inventory.m_inventoryTabs[i].begin(),
                  m_world->m_localCreature->m_inventory.m_inventoryTabs[i].end(),
                  [](const ItemStack &a, const ItemStack &b) -> bool {
                      if (a.quantity == 0) {
                          return false;
                      }
                      if (b.quantity == 0) {
                          return true;
                      }
                      if (a.item.category < b.item.category ||
                          (a.item.category == b.item.category && a.item.id < b.item.id)) {
                          return true;
                      }
                      return false;
                  });
    }
    std::sort(m_world->m_localCreature->m_inventory.m_inventoryTabs[1].begin(),
              m_world->m_localCreature->m_inventory.m_inventoryTabs[1].end(),
              [this](const ItemStack &a, const ItemStack &b) -> bool {
                  if (a.quantity == 0) {
                      return false;
                  }
                  if (b.quantity == 0) {
                      return true;
                  }
                  bool aLocal = m_interface.m_inGame.m_equipmentInventoryWidget->m_region == a.item.region;
                  bool bLocal = m_interface.m_inGame.m_equipmentInventoryWidget->m_region == b.item.region;
                  return (aLocal && !bLocal) ||
                         (aLocal == bLocal && (a.item.category < b.item.category ||
                                               (a.item.category == b.item.category && a.item.id < b.item.id)));
              });
    for (std::vector<ItemStack> &tab : m_world->m_localCreature->m_inventory.m_inventoryTabs) {
        int count = 0;
        for (ItemStack &stack : tab) {
            if (stack.quantity != 0) {
                count++;
            }
        }
        tab.resize(count);
    }
}

static inline float speechDuration(SpeechWidget *bubble)
{
    int duration = (int)((double)bubble->getDialogDuration() * 1.75);
    int minimum = bubble->getDialogDuration() + 1000;
    return (float)(std::max)(minimum, duration);
}

void Game::updateSpeech(double step)
{
    std::vector<SpeechWidget *> active;
    for (SpeechWidget *bubble : m_interface.m_inGame.m_speechWidgets) {
        float duration = speechDuration(bubble);
        if (bubble->m_readTime > duration && !bubble->m_dialogs.empty() &&
            bubble->m_dialogIndex < (int)bubble->m_dialogs.size()) {
            Creature *speaker = m_world->creatureByID(bubble->m_speakerId);
            if (speaker) {
                plasma::Vector<4, float> speakerColor(0.2f, 1.0f, 1.0f, 1.0f);
                if (speaker->m_entityData.hostilityType == HostilityPlayer || strlen(speaker->m_entityData.name) > 1) {
                    m_interface.m_inGame.m_chatWidget->printMessage(
                        WidenString(std::string(speaker->m_entityData.name)) + L": ", speakerColor);
                } else if (speaker->m_entityData.hostilityType == HostilityFriendlyNpc ||
                           (speaker->m_entityData.hostilityType == HostilityEnemy &&
                            (speaker->m_entityData.appearance.appearanceFlags & AppearanceFlagTinted))) {
                    m_interface.m_inGame.m_chatWidget->printMessage(
                        randomName((int)speaker->m_id, speaker->m_entityData.race) + L": ", speakerColor);
                } else {
                    m_interface.m_inGame.m_chatWidget->printMessage(
                        m_speech.getText(m_speech.creatureKey(speaker->m_entityData.race), L"default", L"") + L": ",
                        speakerColor);
                }
                bool first = true;
                for (const Dialog::Phrase &phrase : bubble->m_dialogs[bubble->m_dialogIndex].phrases) {
                    if (phrase.text.size() > 1 && phrase.text[0] == L'!')
                        continue;
                    if (phrase.text != L"." && phrase.text != L"-" && phrase.text != L"," && phrase.text != L";" &&
                        phrase.text != L"!" && phrase.text != L"?" && phrase.text != L":" && phrase.text != L"'" &&
                        !first)
                        m_interface.m_inGame.m_chatWidget->printMessage(
                            L" ", plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f));
                    m_interface.m_inGame.m_chatWidget->printMessage(phrase.text, phrase.color);
                    first = false;
                }
                m_interface.m_inGame.m_chatWidget->printMessage(L"\n",
                                                                plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f));
            }
            ++bubble->m_dialogIndex;
            bubble->m_readTime = 0.0f;
            bubble->m_textUncovered = 0.0f;
        }
        if (bubble->m_dialogIndex < (int)bubble->m_dialogs.size()) {
            active.push_back(bubble);
        } else {
            Creature *speaker = m_world->creatureByID(bubble->m_speakerId);
            if (speaker) {
                Interaction interaction;
                if (speaker == m_world->m_localCreature && bubble->m_item.category) {
                    interaction.type = InteractionUseSpecialItem;
                    interaction.creatureId = speaker->m_id;
                    interaction.item = bubble->m_item;
                } else {
                    interaction.type = InteractionSpeech;
                    interaction.creatureId = speaker->m_id;
                }
                m_world->m_localCreature->m_interactions.push_back(interaction);
            }
            m_engine->destroyNode(bubble->getNode());
        }
    }
    m_interface.m_inGame.m_speechWidgets = active;

    for (SpeechWidget *bubble : m_interface.m_inGame.m_speechWidgets) {
        Creature *speaker = m_world->creatureByID(bubble->m_speakerId);
        if (!speaker)
            continue;
        Fixed share(blendFactor(step, 0.01f));
        plasma::Vector<3, Fixed> target = speaker->m_drawnPosition;
        target[2] += Fixed(-speaker->m_stepOffset);
        bubble->m_anchorPosition += (target - bubble->m_anchorPosition) * share;
        plasma::Vector<3, float> view =
            m_viewMatrix * plasma::Vector<3, float>(
                               bubble->m_anchorPosition +
                               plasma::Vector<3, Fixed>(m_drawOriginX, m_drawOriginY,
                                                        Fixed(speaker->m_entityData.appearance.size[2] * 0.5f + 1.0f)));
        if (view[2] > 0.0f) {
            bubble->getNode()->m_display->m_visibility.value() = 1;
            // The view position becomes the screen position, y downwards.
            plasma::Vector<3, float> projection = m_projectionMatrix * view;
            view = projection * plasma::Vector<3, float>((float)m_engine->viewportWidth() * 0.5f,
                                                         (float)-m_engine->viewportHeight() * 0.5f, 0.5f) +
                   plasma::Vector<3, float>((float)m_engine->viewportWidth() * 0.5f,
                                            (float)m_engine->viewportHeight() * 0.5f, 0.0f);
            if (view[1] < m_interface.m_scale * 180.0f)
                view[1] = m_interface.m_scale * 180.0f;
            if (view[1] > (float)m_engine->viewportHeight())
                view[1] = (float)m_engine->viewportHeight();
            bubble->getNode()->setPosition(view[0], view[1], -125.0f, -170.0f);
        } else {
            bubble->getNode()->m_display->m_visibility.value() = 0;
        }
        bool emote = false;
        if (bubble->m_dialogIndex >= 0 && bubble->m_dialogIndex < (int)bubble->m_dialogs.size() &&
            !bubble->m_dialogs[bubble->m_dialogIndex].phrases.empty()) {
            int index = -1;
            if (bubble->m_dialogs[bubble->m_dialogIndex].phrases.front().text == L"!alert")
                index = SpriteEmoteAlert;
            else if (bubble->m_dialogs[bubble->m_dialogIndex].phrases.front().text == L"!question")
                index = SpriteEmoteQuestion;
            else if (bubble->m_dialogs[bubble->m_dialogIndex].phrases.front().text == L"!love")
                index = SpriteHeartSlot3568;
            else if (bubble->m_dialogs[bubble->m_dialogIndex].phrases.front().text == L"!angry")
                index = SpriteAngry;
            else if (bubble->m_dialogs[bubble->m_dialogIndex].phrases.front().text == L"!sweat")
                index = SpriteEmoteSweat;
            if (index >= 0) {
                Sprite *sprite = m_world->m_spriteManager->sprite(index);
                speaker->m_animationState.m_emoteSprite = sprite;
                speaker->m_animationState.m_emoteTime = 0;
                emote = true;
            }
        }
        if (speaker->m_animationState.m_emoteSprite)
            bubble->getNode()->m_display->m_visibility.value() = 0;
        if (bubble->m_readTime == 0.0f && !bubble->m_dialogs.empty() && bubble->m_dialogIndex >= 0 &&
            bubble->m_dialogIndex < (int)bubble->m_dialogs.size()) {
            Dialog &dialog = bubble->m_dialogs[bubble->m_dialogIndex];
            if (dialog.takenCount)
                m_world->m_localCreature->m_inventory.remove(dialog.takenItem, dialog.takenCount);
            if (dialog.givenCount && m_world->m_localCreature->canTakeItem(dialog.item)) {
                m_world->m_localCreature->m_inventory.add(dialog.item, dialog.givenCount, -1);
                notifyItem(dialog.item, dialog.givenCount, m_world->m_localCreature);
                m_successMusicTime = 60000;
                if (dialog.givenCount == 1 && dialog.item.category == ItemCategorySpecial) {
                    std::vector<int> commands;
                    commands << SpeechCommand_DefineItem << dialog.item << ExpressionYouReceivedTheItem;
                    m_speech.generateFromCommands(commands, rand() % 10000,
                                                  m_interface.m_inGame.m_speechWidget->m_dialogs, L"");
                    Creature *local = m_world->m_localCreature;
                    if (!local->m_entityData.currentAbility ||
                        local->m_entityData.timeSinceAbility > (float)local->attackDuration()) {
                        m_world->m_localCreature->m_entityData.timeSinceAbility = 0.0f;
                        m_world->m_localCreature->m_entityData.currentAbility = AbilityPresentItem;
                        m_world->m_localCreature->m_entityData.actionItem = dialog.item;
                    }
                }
            }

            for (const std::array<int, 2> &cell : dialog.revealedLoreCells) {
                std::array<int, 2> key = {{cell[0], cell[1]}};
                m_world->m_localCreature->m_playerData->discoveredMarkers.insert(key);
                if (m_world->hasLore(cell[0], cell[1])) {
                    int x = floorDivide(cell[0] * BlocksPerCell, BlocksPerPlace);
                    int y = floorDivide(cell[1] * BlocksPerCell, BlocksPerPlace);
                    unsigned realmId = m_world->placeRealm(x, y);
                    if (m_world->m_localCreature->m_playerData->loreByRealm[realmId].find(key) ==
                        m_world->m_localCreature->m_playerData->loreByRealm[realmId].end()) {
                        m_world->m_localCreature->m_playerData->loreByRealm[realmId].insert(key);
                        Realm realm(realmId);
                        m_interface.m_inGame.m_chatWidget->printMessage(
                            m_speech.realmName(realm.form(), realmId),
                            plasma::Vector<4, float>(1.0f, 0.8f, 0.5f, 1.0f));
                        m_interface.m_inGame.m_chatWidget->printMessage(
                            L" lore increased to " +
                                std::to_wstring(10 *
                                                m_world->m_localCreature->m_playerData->loreByRealm[realmId].size()) +
                                L"%.\n",
                            plasma::Vector<4, float>(0.6f, 1.0f, 1.0f, 1.0f));
                        playSound(SoundQuestProgress, m_cameraPosition, 1.0f, 1.0f, true);
                    }
                }
            }
            for (const std::array<int, 2> &place : dialog.markedLocations) {
                if (!m_world->m_localCreature->knowsMission(place[0], place[1])) {
                    std::array<int, 2> key = {{place[0], place[1]}};
                    m_world->m_localCreature->m_playerData->knownMissions.insert(key);
                    m_world->m_localCreature->m_playerData->discoveredPlaces.insert(key);
                    m_world->m_localCreature->m_playerData->missionTimers[key] = 180000.0f;
                    playSound(SoundQuestProgress, m_cameraPosition, 1.0f, 1.0f, true);
                }
            }
        }
        if (emote) {
            ++bubble->m_dialogIndex;
            bubble->m_readTime = 0.0f;
        }
    }
    for (Creature *creature : m_world->m_creatures) {
        if (creature->m_animationState.m_emoteSprite) {
            if (creature->m_animationState.m_emoteTime == 0)
                playSound(SoundDrop, creature->m_entityData.position, 1.0f, 1.0f, true);
            double elapsed = (double)creature->m_animationState.m_emoteTime + step;
            creature->m_animationState.m_emoteTime = (int)elapsed;
            if (creature->m_animationState.m_emoteTime > 2000) {
                creature->m_animationState.m_emoteTime = 0;
                creature->m_animationState.m_emoteSprite = nullptr;
            }
        }
    }

    SpeechWidget *panel = m_interface.m_inGame.m_speechWidget;
    if (panel->m_dialogs.empty()) {
        panel->getNode()->m_display->m_visibility.value() = 0;
        m_interface.m_inGame.m_speechWidget->m_dialogIndex = 0;
        m_interface.m_inGame.m_speechWidget->m_readTime = 0.0f;
        m_interface.m_inGame.m_speechWidget->m_textUncovered = 0.0f;
        m_interface.m_inGame.m_speechWidget->m_speakerId = 0;
    } else {
        if (panel->m_readTime == 0.0f && !panel->m_dialogs.empty() && panel->m_dialogIndex >= 0 &&
            panel->m_dialogIndex < (int)panel->m_dialogs.size()) {
            Dialog &dialog = panel->m_dialogs[panel->m_dialogIndex];
            if (dialog.givenCount) {
                if (m_world->m_localCreature->canTakeItem(dialog.item)) {
                    m_world->m_localCreature->m_inventory.add(dialog.item, dialog.givenCount, -1);
                    notifyItem(dialog.item, dialog.givenCount, m_world->m_localCreature);
                    m_successMusicTime = 60000;
                    if (dialog.givenCount == 1 && dialog.item.category == ItemCategorySpecial) {
                        std::vector<std::string> script;
                        {
                            std::string received("you_received_the_item");
                            std::string define("define_item");
                            script << define << dialog.item << received;
                        }
                        m_speech.generateFromScript(script, rand() % 10000,
                                                    m_interface.m_inGame.m_speechWidget->m_dialogs, L"");
                        Creature *local = m_world->m_localCreature;
                        if (!local->m_entityData.currentAbility ||
                            local->m_entityData.timeSinceAbility > (float)local->attackDuration()) {
                            m_world->m_localCreature->m_entityData.timeSinceAbility = 0.0f;
                            m_world->m_localCreature->m_entityData.currentAbility = AbilityPresentItem;
                            m_world->m_localCreature->m_entityData.actionItem = dialog.item;
                        }
                    }
                }
                if (dialog.takenCount)
                    m_world->m_localCreature->m_inventory.remove(dialog.takenItem, dialog.takenCount);
            }

            for (const std::array<int, 2> &cell : dialog.revealedLoreCells) {
                std::array<int, 2> key = {{cell[0], cell[1]}};
                m_world->m_localCreature->m_playerData->discoveredMarkers.insert(key);
                if (m_world->hasLore(cell[0], cell[1])) {
                    int x = floorDivide(cell[0] * BlocksPerCell, BlocksPerPlace);
                    int y = floorDivide(cell[1] * BlocksPerCell, BlocksPerPlace);
                    unsigned realmId = m_world->mortalRealm(x, y, m_world->isLargePlace(x, y));
                    if (m_world->m_localCreature->m_playerData->loreByRealm[realmId].find(key) ==
                        m_world->m_localCreature->m_playerData->loreByRealm[realmId].end()) {
                        m_world->m_localCreature->m_playerData->loreByRealm[realmId].insert(key);
                        Realm realm(realmId);
                        m_interface.m_inGame.m_chatWidget->printMessage(
                            m_speech.realmName(realm.form(), realmId),
                            plasma::Vector<4, float>(1.0f, 0.8f, 0.5f, 1.0f));
                        m_interface.m_inGame.m_chatWidget->printMessage(
                            L" lore increased to " +
                                std::to_wstring(10 *
                                                m_world->m_localCreature->m_playerData->loreByRealm[realmId].size()) +
                                L"%.\n",
                            plasma::Vector<4, float>(0.6f, 1.0f, 1.0f, 1.0f));
                    }
                }
            }
            for (const std::array<int, 2> &place : dialog.markedLocations) {
                if (!m_world->m_localCreature->knowsMission(place[0], place[1])) {
                    std::array<int, 2> key = {{place[0], place[1]}};
                    m_world->m_localCreature->m_playerData->knownMissions.insert(key);
                    m_world->m_localCreature->m_playerData->discoveredPlaces.insert(key);
                    m_world->m_localCreature->m_playerData->missionTimers[key] = 180000.0f;
                    playSound(SoundQuestProgress, m_cameraPosition, 1.0f, 1.0f, true);
                }
            }
        }
        m_interface.m_inGame.m_speechWidget->m_readTime =
            (float)((double)m_interface.m_inGame.m_speechWidget->m_readTime + step);
        m_interface.m_inGame.m_speechWidget->getNode()->m_display->m_visibility.value() = 1;
        static bool primaryWasDown = false;
        static bool nextWasDown = false;
        if (m_interface.m_inGame.m_speechWidget->m_readTime >
            (float)(m_interface.m_inGame.m_speechWidget->getDialogDuration() + 100)) {
            if ((!m_controlStates[Controls::ButtonAttack] && primaryWasDown) ||
                (!m_controlStates[Controls::ButtonUse] && nextWasDown)) {
                SpeechWidget *current = m_interface.m_inGame.m_speechWidget;
                if (!current->m_dialogs.empty() && current->m_dialogIndex < (int)current->m_dialogs.size()) {
                    ++current->m_dialogIndex;
                    m_interface.m_inGame.m_speechWidget->m_readTime = 0.0f;
                    m_interface.m_inGame.m_speechWidget->m_textUncovered = 0.0f;
                    int sound = m_interface.m_inGame.m_speechWidget->m_dialogIndex ==
                                        (int)m_interface.m_inGame.m_speechWidget->m_dialogs.size()
                                    ? SoundSpeechEnd
                                    : SoundSpeechNext;
                    playSound(sound, m_cameraPosition, 1.0f, 1.0f, true);
                }
            }
            primaryWasDown = m_controlStates[Controls::ButtonAttack];
            nextWasDown = m_controlStates[Controls::ButtonUse];
        } else {
            primaryWasDown = false;
            nextWasDown = false;
        }
        if (m_interface.m_inGame.m_speechWidget->m_dialogIndex >=
            (int)m_interface.m_inGame.m_speechWidget->m_dialogs.size()) {
            m_interface.m_inGame.m_speechWidget->m_dialogs.clear();
            m_interface.m_inGame.m_speechWidget->getNode()->m_display->m_visibility.value() = 0;
            m_interface.m_inGame.m_speechWidget->m_dialogIndex = 0;
            m_interface.m_inGame.m_speechWidget->m_readTime = 0.0f;
            m_interface.m_inGame.m_speechWidget->m_textUncovered = 0.0f;
        }
    }
    for (SpeechWidget *bubble : m_interface.m_inGame.m_speechWidgets) {
        Creature *speaker = m_world->creatureByID(bubble->m_speakerId);
        if (speaker && speaker->m_animationState.m_emoteSprite)
            bubble->m_readTime = 0.0f;
        else
            bubble->m_readTime = (float)((double)bubble->m_readTime + step);
    }
}

void Game::updateTextFx(int milliseconds)
{
    m_interface.m_textFx->destroyChildren();
    std::list<std::list<TextFX>::iterator> expired;
    for (std::list<TextFX>::iterator it = m_textFx.begin(); it != m_textFx.end(); ++it) {
        TextFX &effect = *it;
        plasma::Vector<3, float> position((float)(effect.position[0] + m_drawOriginX),
                                          (float)(effect.position[1] + m_drawOriginY), (float)effect.position[2]);
        plasma::Vector<3, float> view = m_viewMatrix * position;
        if (view[2] > 0.0f) {
            plasma::Vector<3, float> projected = m_projectionMatrix * view;
            int width = m_engine->viewportWidth();
            int height = m_engine->viewportHeight();
            float x = (float)width * 0.5f * projected[0] + (float)width * 0.5f;
            float y = (float)-height * 0.5f * projected[1] + (float)height * 0.5f;
            float progress = (float)effect.age / (float)effect.duration;
            plasma::TextShape *shadow = m_engine->createTextShape(effect.text, L"");
            shadow->m_color.value() = effect.color * plasma::Vector<4, float>(0, 0, 0, 0);
            shadow->m_pixelSize = effect.pixelSize;
            shadow->m_strokeColor.value() = effect.color * plasma::Vector<4, float>(0.1f, 0.1f, 0.1f, 1.0f - progress);
            shadow->m_strokeRadius = (std::max)(3.0f, effect.pixelSize * 0.125f);
            shadow->m_fontName = L"resource1.dat";
            shadow->m_flags = effect.textFlags;
            shadow->attributesChanged(true);
            plasma::Node *shadowNode = m_engine->createNode(nullptr, shadow, nullptr, m_interface.m_textFx, L"");
            shadowNode->setPosition(
                plasma::Vector<2, float>(x + effect.offsetX, y + progress * effect.rise + effect.offsetY),
                plasma::Vector<2, float>(0, 0));
            plasma::TextShape *text = m_engine->createTextShape(effect.text, L"");
            text->m_color.value() = effect.color * plasma::Vector<4, float>(1, 1, 1, 1.0f - progress);
            text->m_pixelSize = effect.pixelSize;
            text->m_fontName = L"resource1.dat";
            text->m_flags = effect.textFlags;
            text->attributesChanged(true);
            plasma::Node *textNode = m_engine->createNode(nullptr, text, nullptr, m_interface.m_textFx, L"");
            textNode->setPosition(
                plasma::Vector<2, float>(x + effect.offsetX, y + progress * effect.rise + effect.offsetY),
                plasma::Vector<2, float>(0, 0));
        }
        effect.age += milliseconds;
        if (effect.age > effect.duration)
            expired.push_back(it);
    }
    for (std::list<TextFX>::iterator it : expired)
        m_textFx.erase(it);
}

void Game::updateUsableItems()
{
    ZoneItem *zoneItem = nullptr;
    int count = 1;
    if (m_crosshairItem[0] != 0x7fffffff) {
        zoneItem = m_world->item(m_crosshairItem);
        if (zoneItem)
            count = zoneItem->quantity;
    }
    Item *item = zoneItem ? &zoneItem->item : nullptr;
    plasma::Widget *hoveredWidget = nullptr;
    bool hovered = false;
    if (m_interface.m_inGame.m_selector->isVisible()) {
        std::vector<Item *> items;
        std::vector<plasma::Vector<2, int>> slots = getFoodSlots();
        for (const plasma::Vector<2, int> &slot : slots)
            items.push_back(&m_world->getLocalCreature()->m_inventory.m_inventoryTabs[slot[0]][slot[1]].item);
        if (!slots.empty()) {
            int index = m_ringSelection;
            if (index < 0) {
                index = (int)((index + ((size_t)-index / slots.size() + 1) * slots.size()) % slots.size());
            }
            const plasma::Vector<2, int> &slot = slots[(size_t)index % slots.size()];
            item = &m_world->getLocalCreature()->m_inventory.m_inventoryTabs[slot[0]][slot[1]].item;
        }
    } else {
        MapOverlayWidget *map = m_interface.m_inGame.m_mapOverlayWidget;
        if (map->m_hoveredItem.category != ItemCategoryNone) {
            item = &map->m_hoveredItem;
            hovered = true;
        } else {
            plasma::Vector<2, int> slot = getHoveredSlot();
            if (slot[0] >= 0 && slot[1] >= 0) {
                item = &m_world->getLocalCreature()->m_inventory.m_inventoryTabs[slot[0]][slot[1]].item;
                hoveredWidget = m_interface.m_inGame.m_equipmentInventoryWidget;
                hovered = true;
            } else if ((item = getHoveredEquipment()) != nullptr && item->category != ItemCategoryNone) {
                hoveredWidget = m_interface.m_inGame.m_equipmentInventoryWidget;
                hovered = true;
            } else if ((item = getHoveredFormula()) != nullptr) {
                hoveredWidget = m_interface.m_inGame.m_craftingInventoryWidget;
                hovered = true;
            } else if ((item = getHoveredVendorItem()) != nullptr) {
                hoveredWidget = m_interface.m_inGame.m_vendorInventoryWidget;
                hovered = true;
            } else if ((item = getHoveredIngredient()) != nullptr) {
                hoveredWidget = m_interface.m_inGame.m_craftingInventoryWidget;
                hovered = true;
            } else if ((item = getHoveredEnchantItem()) != nullptr) {
                hoveredWidget = m_interface.m_inGame.m_enchantWidget;
                hovered = true;
            } else {
                item = zoneItem ? &zoneItem->item : nullptr;
            }
        }
    }
    m_interface.m_inGame.m_previewWidget->getNode()->setVisible(item != nullptr);
    m_interface.m_inGame.m_previewWidget->m_skill = -1;
    m_interface.m_inGame.m_previewWidget->m_specialization = -1;
    m_interface.m_inGame.m_previewWidget->m_item = item;
    m_interface.m_inGame.m_previewWidget->m_zoneItem = hovered ? nullptr : zoneItem;
    m_interface.m_inGame.m_previewWidget->m_count = count;
    m_interface.m_inGame.m_previewWidget->m_equippedItem = nullptr;
    m_interface.m_inGame.m_previewWidget->m_flightPoint = m_worldMap->m_selectedFlightPoint;
    m_interface.m_inGame.m_previewWidget->m_artifactRegion = m_interface.m_inGame.m_mapOverlayWidget->m_hoveredRegion;
    if (m_worldMap->m_selectedFlightPoint[0] != 0x7fffffff)
        m_interface.m_inGame.m_previewWidget->getNode()->setVisible(true);
    if (m_interface.m_inGame.m_mapOverlayWidget->m_hoveredRegion[0] != 0x7fffffff)
        m_interface.m_inGame.m_previewWidget->getNode()->setVisible(true);
    if (item) {
        Item shown = item->transformed(m_world->getLocalCreature());
        if (item != &m_world->getLocalCreature()->m_entityData.equipment.weaponRight &&
            item != &m_world->getLocalCreature()->m_entityData.equipment.weaponLeft &&
            shown.category == m_world->getLocalCreature()->m_entityData.equipment.weaponRight.category) {
            if (shown.id != WeaponShield ||
                m_world->getLocalCreature()->m_entityData.equipment.weaponRight.isTwoHanded())
                m_interface.m_inGame.m_previewWidget->m_equippedItem =
                    &m_world->getLocalCreature()->m_entityData.equipment.weaponRight;
        }
        if ((m_keyStates[VK_SHIFT] || !m_interface.m_inGame.m_previewWidget->m_equippedItem) &&
            item != &m_world->getLocalCreature()->m_entityData.equipment.weaponRight &&
            item != &m_world->getLocalCreature()->m_entityData.equipment.weaponLeft &&
            shown.category == m_world->getLocalCreature()->m_entityData.equipment.weaponLeft.category)
            m_interface.m_inGame.m_previewWidget->m_equippedItem =
                &m_world->getLocalCreature()->m_entityData.equipment.weaponLeft;
        if (item != &m_world->getLocalCreature()->m_entityData.equipment.pet &&
            shown.category == m_world->getLocalCreature()->m_entityData.equipment.pet.category)
            m_interface.m_inGame.m_previewWidget->m_equippedItem =
                &m_world->getLocalCreature()->m_entityData.equipment.pet;
        if (item != &m_world->getLocalCreature()->m_entityData.equipment.shoulder &&
            shown.category == m_world->getLocalCreature()->m_entityData.equipment.shoulder.category)
            m_interface.m_inGame.m_previewWidget->m_equippedItem =
                &m_world->getLocalCreature()->m_entityData.equipment.shoulder;
        if (item != &m_world->getLocalCreature()->m_entityData.equipment.chest &&
            shown.category == m_world->getLocalCreature()->m_entityData.equipment.chest.category)
            m_interface.m_inGame.m_previewWidget->m_equippedItem =
                &m_world->getLocalCreature()->m_entityData.equipment.chest;
        if (item != &m_world->getLocalCreature()->m_entityData.equipment.hands &&
            shown.category == m_world->getLocalCreature()->m_entityData.equipment.hands.category)
            m_interface.m_inGame.m_previewWidget->m_equippedItem =
                &m_world->getLocalCreature()->m_entityData.equipment.hands;
        if (item != &m_world->getLocalCreature()->m_entityData.equipment.feet &&
            shown.category == m_world->getLocalCreature()->m_entityData.equipment.feet.category)
            m_interface.m_inGame.m_previewWidget->m_equippedItem =
                &m_world->getLocalCreature()->m_entityData.equipment.feet;
        if (item != &m_world->getLocalCreature()->m_entityData.equipment.neck &&
            shown.category == m_world->getLocalCreature()->m_entityData.equipment.neck.category)
            m_interface.m_inGame.m_previewWidget->m_equippedItem =
                &m_world->getLocalCreature()->m_entityData.equipment.neck;
        if (item != &m_world->getLocalCreature()->m_entityData.equipment.ringRight &&
            item != &m_world->getLocalCreature()->m_entityData.equipment.ringLeft &&
            shown.category == m_world->getLocalCreature()->m_entityData.equipment.ringRight.category)
            m_interface.m_inGame.m_previewWidget->m_equippedItem =
                &m_world->getLocalCreature()->m_entityData.equipment.ringRight;
        if ((m_keyStates[VK_SHIFT] || !m_interface.m_inGame.m_previewWidget->m_equippedItem) &&
            item != &m_world->getLocalCreature()->m_entityData.equipment.ringRight &&
            item != &m_world->getLocalCreature()->m_entityData.equipment.ringLeft &&
            shown.category == m_world->getLocalCreature()->m_entityData.equipment.ringLeft.category)
            m_interface.m_inGame.m_previewWidget->m_equippedItem =
                &m_world->getLocalCreature()->m_entityData.equipment.ringLeft;
    }
    if (m_interface.isInMenu() && !m_interface.m_inGame.m_previewWidget->m_zoneItem) {
        plasma::Vector<2, float> pointer = m_engine->m_pointer;
        m_interface.m_inGame.m_previewWidget->getNode()->setPosition(pointer[0], pointer[1], 30.0f, 30.0f);
        if (hoveredWidget) {
            float pointerX = m_engine->m_pointer[0];
            if (hoveredWidget->getContentPosition()[0] + hoveredWidget->screenSize()[0] * 0.5f > pointerX) {
                plasma::Vector<2, float> position = hoveredWidget->getViewPosition();
                float pointerY = m_engine->m_pointer[1];
                m_interface.m_inGame.m_previewWidget->getNode()->setPosition(
                    0, pointerY, position[0] - 10.0f - m_interface.m_inGame.m_previewWidget->getContentWidth(), 30.0f);
            } else {
                float x = hoveredWidget->getViewPosition()[0] + hoveredWidget->screenSize()[0] + 10.0f;
                m_interface.m_inGame.m_previewWidget->getNode()->setPosition(0, m_engine->m_pointer[1], x, 30.0f);
            }
        }
    } else {
        int width = m_width;
        m_interface.m_inGame.m_previewWidget->getNode()->setPosition(
            (float)(width / 2), (float)m_height - m_interface.m_inGame.m_previewWidget->getViewHeight(), 210.0f,
            -23.0f);
    }
}

void Game::updateVendor()
{
    m_vendorInventory.m_inventoryTabs.clear();
    m_vendorInventory.m_inventoryTabs.resize(2);
    if (m_interface.m_inGame.m_vendorInventoryWidget->m_tabIndex == VendorTabBuyback) {
        m_vendorInventory.m_inventoryTabs[1].clear();
        for (std::list<ItemStack>::reverse_iterator it = m_soldItems.rbegin(); it != m_soldItems.rend(); ++it) {
            m_vendorInventory.m_inventoryTabs[1].push_back(*it);
        }
    } else {
        Creature *vendor = m_world->creatureByID(m_vendorId);
        if (vendor) {
            switch (m_vendorClass) {
            case ClassWeaponVendor:
                m_world->fillWeaponShop(m_vendorInventory, vendor);
                break;
            case ClassArmorVendor:
                m_world->fillArmorShop(m_vendorInventory, vendor);
                break;
            case ClassGemTrader:
                m_world->fillFormulaShop(m_vendorInventory, m_vendorId);
                break;
            default:
                m_world->fillGeneralShop(m_vendorInventory, m_vendorId);
                break;
            }
        }
    }
    m_vendorInventory.m_inventoryTabs.resize(2);
    m_interface.m_inGame.m_vendorInventoryWidget->updateRows();
    m_interface.m_inGame.m_vendorInventoryWidget->updateScrollButton();
}

// Uses the item of an inventory stack. Formulas, quest items and category 28: formula 9 and quest items are handed to
// the host as an interaction, formula 13 opens the map (and stays), anything else is consumed by ability 84 (formulas
// 1 to 3 and 12, category 28) or 85. Category 24 (of the player's region only) toggles the abilities 120 and 121 or
// starts 137.
void Game::useItem(ItemStack &stack)
{
    if ((stack.item.category == ItemCategoryConsumable || stack.item.category == ItemCategoryQuest ||
         stack.item.category == ItemCategoryElixir) &&
        m_world->m_localCreature->m_entityData.stunTime <= 0.0f) {
        if ((stack.item.category == ItemCategoryConsumable && stack.item.id == ConsumableBomb) ||
            stack.item.category == ItemCategoryQuest) {
            Interaction interaction;
            interaction.type = InteractionUseItem;
            interaction.item = stack.item;
            m_world->m_localCreature->m_interactions.push_back(interaction);
            if (stack.item.category == ItemCategoryConsumable) {
                stack.quantity--;
                if (stack.quantity <= 0) {
                    stack.item.id = NoneCleared;
                    stack.item.category = ItemCategoryNone;
                    stack.quantity = 0;
                }
            }
        } else if (stack.item.category == ItemCategoryConsumable && stack.item.id == ConsumablePortalStone) {
            m_interface.m_mapOpen = true;
            m_interface.m_choosingShrine = true;
            Interaction interaction;
            interaction.type = InteractionUseItem;
            interaction.item = stack.item;
            m_world->m_localCreature->m_interactions.push_back(interaction);
            m_mapItemStack = &stack;
        } else {
            m_world->m_localCreature->m_entityData.actionItem = stack.item;
            if ((stack.item.category == ItemCategoryConsumable &&
                 ((stack.item.id >= ConsumableLifePotion && stack.item.id <= ConsumableEssenceOfWisdom) ||
                  stack.item.id == ConsumableGreaterLifePotion)) ||
                stack.item.category == ItemCategoryElixir) {
                m_world->m_localCreature->m_entityData.currentAbility = AbilityDrink;
            } else {
                m_world->m_localCreature->m_entityData.currentAbility = AbilityEat;
            }
            m_world->m_localCreature->m_entityData.timeSinceAbility = 0;
            m_world->m_localCreature->m_entityData.rollTime = 0;
            stack.quantity--;
            if (stack.quantity <= 0) {
                stack.quantity = 0;
                stack.item.category = ItemCategoryNone;
                stack.item.id = NoneCleared;
            }
        }
    }
    if (stack.item.category == ItemCategorySpecial && m_world->m_localCreature->m_entityData.stunTime <= 0.0f) {
        if (m_world->m_localCreature->m_entityData.region != stack.item.region) {
            playSound(SoundHumanVoice01, m_cameraPosition, 1.0f, 1.0f, true);
            return;
        }
        m_world->m_localCreature->m_entityData.actionItem = stack.item;
        if (stack.item.id == SpecialItemHangGlider) {
            if (m_world->m_localCreature->m_entityData.currentAbility == AbilityHangGlide) {
                m_world->m_localCreature->m_entityData.currentAbility = AbilityNone;
            } else {
                m_world->m_localCreature->m_entityData.currentAbility = AbilityHangGlide;
                m_world->m_localCreature->m_entityData.timeSinceAbility = 0;
            }
        } else if (stack.item.id == SpecialItemBoat) {
            if (m_world->m_localCreature->m_entityData.currentAbility == AbilitySailBoat) {
                m_world->m_localCreature->m_entityData.currentAbility = AbilityNone;
            } else {
                m_world->m_localCreature->m_entityData.currentAbility = AbilitySailBoat;
                m_world->m_localCreature->m_entityData.timeSinceAbility = 0;
            }
        } else {
            m_world->m_localCreature->m_entityData.currentAbility = AbilityUseSpecialItem;
            m_world->m_localCreature->m_entityData.timeSinceAbility = 0;
        }
    }
}

void Game::craft()
{
    Item item = m_craftingFormula.result;
    int extra = 0;
    // A formula of tab 6 changes an item the player has: the one the crafting row stands for, if it still equals the
    // row's item.
    Item *target = nullptr;
    if (m_craftingCell[0] == CraftingTabRepair && m_craftingInventory.m_inventoryTabs.size() >= CraftingTabRepair + 1) {
        std::vector<ItemStack> &tab = m_craftingInventory.m_inventoryTabs[CraftingTabRepair];
        int row = m_craftingCell[1];
        if (row >= 0 && row < (int)tab.size()) {
            ItemStack &cell = tab[row];
            std::map<int, Item *>::iterator equipped = m_equippedCraftingItems.find(m_craftingCell[1]);
            if (equipped != m_equippedCraftingItems.end() && *equipped->second == cell.item) {
                target = equipped->second;
            } else {
                std::map<int, plasma::Vector<2, int>>::iterator place = m_craftingSources.find(m_craftingCell[1]);
                if (place != m_craftingSources.end()) {
                    int tab = place->second[0];
                    if (tab >= 0 && tab < (int)m_world->m_localCreature->m_inventory.m_inventoryTabs.size()) {
                        int slot = place->second[1];
                        if (slot >= 0 &&
                            slot < (int)m_world->m_localCreature->m_inventory.m_inventoryTabs[tab].size() &&
                            m_world->m_localCreature->m_inventory.m_inventoryTabs[tab][slot].item == cell.item)
                            target = &m_world->m_localCreature->m_inventory.m_inventoryTabs[tab][slot].item;
                    }
                }
            }
        }
    }
    if (!target && !m_world->m_localCreature->canTakeItem(item)) {
        m_interface.m_inGame.m_chatWidget->printMessage(L"You can't carry more of these items.\n",
                                                        plasma::Vector<4, float>(1.0f, 0.2f, 0.2f, 1.0f));
        playSound(SoundHumanVoice01, m_world->m_localCreature->m_entityData.position, 1.0f, 1.0f, true);
        return;
    }
    // The ingredients leave the inventory.
    for (Ingredient &ingredient : m_craftingFormula.ingredients) {
        int needed = ingredient.quantity;
        for (int tab = 0; tab < (int)m_world->m_localCreature->m_inventory.m_inventoryTabs.size(); tab++) {
            for (ItemStack &stack : m_world->m_localCreature->m_inventory.m_inventoryTabs[tab]) {
                if (stack.item == ingredient.item) {
                    if (needed < stack.quantity) {
                        stack.quantity -= needed;
                        break;
                    }
                    // Empties the stack before it subtracts the stack's count, so `needed` never falls and every
                    // further stack of the ingredient is emptied too.
                    stack.quantity = 0;
                    stack.item = Item();
                    needed -= stack.quantity;
                    if (needed == 0)
                        break;
                }
            }
        }
    }
    playSound(SoundCraft, m_cameraPosition, 1.0f, 1.0f, true);
    // Materials other than id 26 come out in up to three extra pieces, each with one chance in five.
    if (item.category == ItemCategoryMaterial && item.id != MaterialItemWaterFlask) {
        if (rand() % 5 == 0) {
            extra = 1;
            if (rand() % 5 == 0) {
                extra = 2;
                if (rand() % 5 == 0)
                    extra = 3;
            }
            playSound(SoundCraftProc, m_cameraPosition, 1.0f, 1.0f, true);
        }
    }
    if (target) {
        *target = item;
    } else {
        m_world->m_localCreature->m_inventory.add(item, extra + 1, -1);
        notifyItem(item, extra + 1, m_world->m_localCreature);
    }
    m_interface.m_inGame.m_equipmentInventoryWidget->updateRows();
    m_interface.m_inGame.m_equipmentInventoryWidget->updateScrollButton();
    updateFormulas();
}

// The seed of a region's generated land name: an integer hash of the region (the same hash seeds it in
// InventoryWidget::draw).
static int regionNameSeed(const plasma::Vector<2, int> &region)
{
    int n = region[0] + 57 * region[1];
    n = (n << 13) ^ n;
    return 100000 - ((n * (n * n * 60493 + 19990303) + 1376312589) & 0x7fffffff) / 21474;
}

// The item's description panel, one line of the widget's text at a time from (x, y) downwards: the name (in the colour
// of its rarity), the realm of an artifact, the stars and the named land of an item that belongs to a region, the
// effect of an artifact, the rank of a crafting book, the classes that can use an item the player's class cannot,
// "Ridable" for a pet, then the statistics (damage, armour, resistance, HP, haste, critical chance, regeneration),
// each with the difference to compareItem in green or red, the effect of a potion and of an elixir. The statistics are
// rounded to one decimal; the percentages are shown times 100. Returns the position below the last line.
plasma::Vector<2, float> Game::writeItemInfo(BaseWidget *widget, Item *item, int count, int x, int y, float scale,
                                             int width, bool showName, bool showClasses, bool unusedFlag,
                                             ZoneItem *zoneItem, Item *compareItem)
{
    plasma::Vector<2, float> position((float)x, (float)y);
    std::wstringstream text;
    text.str(L"");
    text << m_speech.createItemName(*item);
    if (item->isRenowned()) {
        text << L"+";
    }
    widget->setFont(L"resource1.dat");
    widget->setTextLineSpacing(1.0f);
    widget->setTextSize(scale * 12.0f);
    widget->setTextStrokeRadius(3.0f);
    widget->setTextStrokeColor(plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f));
    widget->setTextFlags(1 << plasma::TEXT_WRAP); // TEXT_WRAP flag from plasma/Font.h.
    widget->setTextWrapWidth((float)width);
    if (showName) {
        widget->setTextSize(scale * 15.0f);
        widget->setTextStrokeRadius(4.0f);
        widget->setTextColor(item->textColor(m_world->m_localCreature->m_entityData.region));
        plasma::Vector<2, float> end = widget->renderText(text.str(), position[0], position[1]);
        position[1] = end[1] + scale * 24.0f;
    }
    widget->setTextSize(scale * 12.0f);
    widget->setTextStrokeRadius(3.0f);

    // An artifact belongs to a realm.
    if (item->category == ItemCategoryArtifact) {
        int realm = item->formulaCategory;
        widget->setTextColor(plasma::Vector<4, float>(1.0f, 0.9f, 0.5f, 1.0f));
        widget->renderText(m_speech.realmName(Realm(realm).form(), realm), position[0], position[1]);
        position[1] += scale * 24.0f;
    }

    // The rarity of equipment as stars (the item's own over grey ones), and the named land the item comes from, gold
    // when it is at full rarity there.
    if (item->hasRegion()) {
        text.str(L"");
        if ((item->category >= ItemCategoryWeapon && item->category <= ItemCategoryRing) ||
            item->category == ItemCategoryEyePatch) {
            for (int i = 0; i <= item->rarity; i++) {
                text << L"\xb1"; // the font's star glyph
            }
            widget->setTextSize(scale * 18.0f);
            widget->setTextColor(plasma::Vector<4, float>(0.3f, 0.3f, 0.3f, 1.0f));
            widget->renderText(text.str(), position[0], position[1]);
            text.str(L"");
            for (int i = 0; i <= item->getRarity(m_world->m_localCreature->m_entityData.region); i++) {
                text << L"\xb1";
            }
            plasma::Vector<4, float> starColor =
                rarityColor(item->getRarity(m_world->m_localCreature->m_entityData.region));
            widget->setTextColor(starColor);
            widget->renderText(text.str(), position[0], position[1]);
            widget->setTextStrokeColor(plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f));
            position[1] += scale * 24.0f;
        }
        widget->setTextSize(scale * 12.0f);
        text.str(L"");
        plasma::Vector<4, float> color(1.0f, 1.0f, 1.0f, 1.0f);
        if (item->getRarity(m_world->m_localCreature->m_entityData.region) == item->rarity) {
            color = plasma::Vector<4, float>(1.0f, 0.9f, 0.5f, 1.0f);
        } else {
            color = plasma::Vector<4, float>(0.75f, 0.75f, 0.75f, 1.0f);
        }
        widget->setTextColor(color);
        text << m_speech.getText(m_speech.getLandName(m_world->landBiome(item->region[0], item->region[1])), L"named",
                                 randomName(regionNameSeed(item->region), RaceNone));
        if (item->isRenowned()) {
            text << L"+";
        }
        widget->renderText(text.str(), position[0], position[1]);
        position[1] += scale * 24.0f;
    }

    // What an artifact does (no line advance).
    if (item->category == ItemCategoryArtifact) {
        text.str(L"");
        switch (item->artifactType()) {
        case ArtifactEffect_HangGlidingSpeed:
            text << L"Increases hang gliding speed.";
            break;
        case ArtifactEffect_ClimbingSpeed:
            text << L"You can climb faster.";
            break;
        case ArtifactEffect_SailingSpeed:
            text << L"Increases sailing speed.";
            break;
        case ArtifactEffect_SwimmingSpeed:
            text << L"Increases swimming speed.";
            break;
        case ArtifactEffect_DivingStamina:
            text << L"You consume less stamina when diving.";
            break;
        case ArtifactEffect_RidingSpeed:
            text << L"Increases riding speed.";
            break;
        case ArtifactEffect_LampRadius:
            text << L"Increases the radius of your lamp.";
            break;
        }
        widget->setTextColor(plasma::Vector<4, float>(0.2f, 1.0f, 0.6f, 1.0f));
        widget->renderText(text.str(), position[0], position[1]);
    }

    // The rank of a crafting book (no line advance).
    if (item->category == ItemCategorySpecial && item->id == SpecialItemFlute) {
        text.str(L"");
        text << L"RANK " << item->modifier + 1;
        widget->setTextColor(plasma::Vector<4, float>(1.0f, 0.9f, 0.6f, 1.0f));
        widget->renderText(text.str(), position[0], position[1]);
    }

    // The owner of an item lying in the world: the player creature whose seed the item carries, white for the local
    // player, red for another. The line itself is empty.
    text.str(L"");
    if (zoneItem && zoneItem->ownerSeed) {
        Creature *owner = nullptr;
        for (Creature *creature : m_world->m_creatures) {
            if (creature->m_entityData.hostilityType == HostilityPlayer &&
                zoneItem->ownerSeed == creature->m_entityData.seed) {
                owner = creature;
            }
        }
        plasma::Vector<4, float> color;
        if (owner == m_world->m_localCreature) {
            color = plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f);
        } else {
            color = plasma::Vector<4, float>(1.0f, 0.0f, 0.0f, 1.0f);
        }
        widget->setTextColor(color);
        widget->setTextSize(scale * 9.0f);
        widget->setTextStrokeRadius(2.0f);
        widget->renderText(text.str(), position[0], position[1]);
        text.str(L"");
        position[1] += scale * 20.0f;
    }

    // The classes that can use an item the player's class cannot.
    if (showClasses) {
        widget->setTextSize(scale * 12.0f);
        widget->setTextStrokeRadius(3.0f);
        if (!item->fitsClass(m_world->m_localCreature->m_entityData.classType)) {
            text.str(L"");
            if (item->fitsClass(ClassWarrior)) {
                text << L"Warrior ";
            }
            if (item->fitsClass(ClassRanger)) {
                text << L"Ranger ";
            }
            if (item->fitsClass(ClassMage)) {
                text << L"Mage ";
            }
            if (item->fitsClass(ClassRogue)) {
                text << L"Rogue ";
            }
            widget->setTextColor(plasma::Vector<4, float>(1.0f, 0.0f, 0.0f, 1.0f));
            widget->renderText(text.str(), position[0], position[1]);
            position[1] += scale * 20.0f;
        }
    }

    // A pet that can be ridden.
    if (item->category == ItemCategoryPet) {
        text.str(L"");
        if (isPet(item->id)) {
            text.str(L"");
            text << L"Ridable";
            widget->setTextColor(plasma::Vector<4, float>(0.75f, 0.5f, 1.0f, 1.0f));
            widget->renderText(text.str(), position[0], position[1]);
            position[1] += scale * 14.0f;
        }
    }

    // A weapon's damage, and what its spirits add.
    if (item->category == ItemCategoryWeapon) {
        text.str(L"");
        text << "DAMAGE " << roundTenth(item->weaponDamage(m_world->getLocalCreature()));
        widget->setTextColor(plasma::Vector<4, float>(0.0f, 1.0f, 1.0f, 1.0f));
        widget->setTextSize(scale * 15.0f);
        plasma::Vector<2, float> end = widget->renderText(text.str(), position[0], position[1]);
        if (compareItem) {
            widget->setTextSize(scale * 12.0f);
            text.str(L"");
            Creature *creature = m_world->getLocalCreature();
            float difference =
                roundTenth(roundTenth(item->weaponDamage(creature)) - roundTenth(compareItem->weaponDamage(creature)));
            if (difference > 0.0f) {
                text << L" +" << difference;
                widget->setTextColor(plasma::Vector<4, float>(0.3f, 1.0f, 0.3f, 1.0f));
                end = widget->renderText(text.str(), end[0], position[1]);
            } else if (difference < 0.0f) {
                text << L" " << difference;
                widget->setTextColor(plasma::Vector<4, float>(1.0f, 0.3f, 0.3f, 1.0f));
                end = widget->renderText(text.str(), end[0], position[1]);
            }
        }
        position[1] += scale * 20.0f;
        float upgrade = roundTenth(item->upgradeDamage(m_world->getLocalCreature()));
        if (upgrade > 0.0f) {
            text.str(L"");
            text << "+DAMAGE " << upgrade;
            position[1] -= scale * 2.0f;
            widget->setTextColor(plasma::Vector<4, float>(1.0f, 0.2f, 1.0f, 1.0f));
            widget->setTextSize(scale * 12.0f);
            widget->renderText(text.str(), position[0], position[1]);
            position[1] += scale * 20.0f;
        }
    }

    // Armour.
    if (item->armor(m_world->getLocalCreature()) >= 0.1f ||
        (compareItem && compareItem->armor(m_world->getLocalCreature()) >= 0.1f)) {
        text.str(L"");
        text << "ARMOR " << roundTenth(item->armor(m_world->getLocalCreature()));
        widget->setTextSize(scale * 15.0f);
        widget->setTextColor(plasma::Vector<4, float>(0.0f, 1.0f, 1.0f, 1.0f));
        plasma::Vector<2, float> end = widget->renderText(text.str(), position[0], position[1]);
        if (compareItem) {
            widget->setTextSize(scale * 12.0f);
            text.str(L"");
            Creature *creature = m_world->getLocalCreature();
            float difference = roundTenth(roundTenth(item->armor(creature)) - roundTenth(compareItem->armor(creature)));
            if (difference > 0.0f) {
                text << L" +" << difference;
                widget->setTextColor(plasma::Vector<4, float>(0.3f, 1.0f, 0.3f, 1.0f));
                end = widget->renderText(text.str(), end[0], position[1]);
            } else if (difference < 0.0f) {
                text << L" " << difference;
                widget->setTextColor(plasma::Vector<4, float>(1.0f, 0.3f, 0.3f, 1.0f));
                end = widget->renderText(text.str(), end[0], position[1]);
            }
        }
        position[1] += scale * 20.0f;
    }

    // Resistance.
    if (item->resistance(m_world->getLocalCreature()) >= 0.1f ||
        (compareItem && compareItem->resistance(m_world->getLocalCreature()) >= 0.1f)) {
        text.str(L"");
        text << "RESISTANCE " << roundTenth(item->resistance(m_world->getLocalCreature()));
        widget->setTextSize(scale * 15.0f);
        widget->setTextColor(plasma::Vector<4, float>(0.0f, 1.0f, 1.0f, 1.0f));
        plasma::Vector<2, float> end = widget->renderText(text.str(), position[0], position[1]);
        if (compareItem) {
            widget->setTextSize(scale * 12.0f);
            text.str(L"");
            Creature *creature = m_world->getLocalCreature();
            float difference =
                roundTenth(roundTenth(item->resistance(creature)) - roundTenth(compareItem->resistance(creature)));
            if (difference > 0.0f) {
                text << L" +" << difference;
                widget->setTextColor(plasma::Vector<4, float>(0.3f, 1.0f, 0.3f, 1.0f));
                end = widget->renderText(text.str(), end[0], position[1]);
            } else if (difference < 0.0f) {
                text << L" " << difference;
                widget->setTextColor(plasma::Vector<4, float>(1.0f, 0.3f, 0.3f, 1.0f));
                end = widget->renderText(text.str(), end[0], position[1]);
            }
        }
        position[1] += scale * 20.0f;
    }

    // HP.
    if (item->hp(m_world->getLocalCreature()) >= 0.1f ||
        (compareItem && compareItem->hp(m_world->getLocalCreature()) >= 0.1f)) {
        text.str(L"");
        text << "HP " << roundTenth(item->hp(m_world->getLocalCreature()));
        widget->setTextSize(scale * 15.0f);
        widget->setTextColor(plasma::Vector<4, float>(0.75f, 0.5f, 1.0f, 1.0f));
        plasma::Vector<2, float> end = widget->renderText(text.str(), position[0], position[1]);
        if (compareItem) {
            widget->setTextSize(scale * 12.0f);
            text.str(L"");
            Creature *creature = m_world->getLocalCreature();
            float difference = roundTenth(roundTenth(item->hp(creature)) - roundTenth(compareItem->hp(creature)));
            if (difference > 0.0f) {
                text << L" +" << difference;
                widget->setTextColor(plasma::Vector<4, float>(0.3f, 1.0f, 0.3f, 1.0f));
                end = widget->renderText(text.str(), end[0], position[1]);
            } else if (difference < 0.0f) {
                text << L" " << difference;
                widget->setTextColor(plasma::Vector<4, float>(1.0f, 0.3f, 0.3f, 1.0f));
                end = widget->renderText(text.str(), end[0], position[1]);
            }
        }
        position[1] += scale * 16.0f;
    }

    // Haste (the tempo share), as a percentage.
    if (item->tempo(m_world->getLocalCreature()) >= 0.001f ||
        (compareItem && compareItem->tempo(m_world->getLocalCreature()) >= 0.001f)) {
        text.str(L"");
        text << "HASTE " << roundTenth(item->tempo(m_world->getLocalCreature()) * 100.0f) << "%";
        widget->setTextSize(scale * 12.0f);
        widget->setTextColor(plasma::Vector<4, float>(0.75f, 0.5f, 1.0f, 1.0f));
        plasma::Vector<2, float> end = widget->renderText(text.str(), position[0], position[1]);
        if (compareItem) {
            widget->setTextSize(scale * 12.0f);
            text.str(L"");
            Creature *creature = m_world->getLocalCreature();
            float difference = roundTenth(roundTenth(item->tempo(creature) * 100.0f) -
                                          roundTenth(compareItem->tempo(creature) * 100.0f));
            if (difference > 0.0f) {
                text << L" +" << difference;
                widget->setTextColor(plasma::Vector<4, float>(0.3f, 1.0f, 0.3f, 1.0f));
                end = widget->renderText(text.str(), end[0], position[1]);
            } else if (difference < 0.0f) {
                text << L" " << difference;
                widget->setTextColor(plasma::Vector<4, float>(1.0f, 0.3f, 0.3f, 1.0f));
                end = widget->renderText(text.str(), end[0], position[1]);
            }
        }
        position[1] += scale * 16.0f;
    }

    // Critical chance, as a percentage.
    if (item->critical(m_world->getLocalCreature()) >= 0.001f ||
        (compareItem && compareItem->critical(m_world->getLocalCreature()) >= 0.001f)) {
        text.str(L"");
        text << "CRITICAL " << roundTenth(item->critical(m_world->getLocalCreature()) * 100.0f) << "%";
        widget->setTextSize(scale * 12.0f);
        widget->setTextColor(plasma::Vector<4, float>(0.75f, 0.5f, 1.0f, 1.0f));
        plasma::Vector<2, float> end = widget->renderText(text.str(), position[0], position[1]);
        if (compareItem) {
            widget->setTextSize(scale * 12.0f);
            text.str(L"");
            Creature *creature = m_world->getLocalCreature();
            float difference = roundTenth(roundTenth(item->critical(creature) * 100.0f) -
                                          roundTenth(compareItem->critical(creature) * 100.0f));
            if (difference > 0.0f) {
                text << L" +" << difference;
                widget->setTextColor(plasma::Vector<4, float>(0.3f, 1.0f, 0.3f, 1.0f));
                end = widget->renderText(text.str(), end[0], position[1]);
            } else if (difference < 0.0f) {
                text << L" " << difference;
                widget->setTextColor(plasma::Vector<4, float>(1.0f, 0.3f, 0.3f, 1.0f));
                end = widget->renderText(text.str(), end[0], position[1]);
            }
        }
        position[1] += scale * 16.0f;
    }

    // Regeneration, as a percentage.
    if (item->regeneration(m_world->getLocalCreature()) >= 0.001f ||
        (compareItem && compareItem->regeneration(m_world->getLocalCreature()) >= 0.001f)) {
        text.str(L"");
        text << "REGENERATION " << roundTenth(item->regeneration(m_world->getLocalCreature()) * 100.0f) << "%";
        widget->setTextSize(scale * 12.0f);
        widget->setTextColor(plasma::Vector<4, float>(0.75f, 0.5f, 1.0f, 1.0f));
        plasma::Vector<2, float> end = widget->renderText(text.str(), position[0], position[1]);
        if (compareItem) {
            widget->setTextSize(scale * 12.0f);
            text.str(L"");
            Creature *creature = m_world->getLocalCreature();
            float difference = roundTenth(roundTenth(item->regeneration(creature) * 100.0f) -
                                          roundTenth(compareItem->regeneration(creature) * 100.0f));
            if (difference > 0.0f) {
                text << L" +" << difference;
                widget->setTextColor(plasma::Vector<4, float>(0.3f, 1.0f, 0.3f, 1.0f));
                end = widget->renderText(text.str(), end[0], position[1]);
            } else if (difference < 0.0f) {
                text << L" " << difference;
                widget->setTextColor(plasma::Vector<4, float>(1.0f, 0.3f, 0.3f, 1.0f));
                end = widget->renderText(text.str(), end[0], position[1]);
            }
        }
        position[1] += scale * 16.0f;
    }

    // A healing potion.
    widget->setTextSize(scale * 12.0f);
    if (item->category == ItemCategoryConsumable && item->effectValue() >= 0.1f) {
        text.str(L"");
        text << "Restores health over " << item->cooldown() / 1000 << L" seconds";
        widget->setTextSize(scale * 12.0f);
        widget->setTextColor(plasma::Vector<4, float>(0.25f, 1.0f, 0.25f, 1.0f));
        widget->renderText(text.str(), position[0], position[1]);
        position[1] += scale * 16.0f;
    }

    // The bonus against demons.
    if (item->demonDamageBonus() > 0.0f) {
        text.str(L"");
        text << "Increases damage against demons by " << roundTenth(item->demonDamageBonus() * 100.0f) << L"%";
        widget->setTextSize(scale * 12.0f);
        widget->setTextColor(plasma::Vector<4, float>(1.0f, 0.75f, 0.5f, 1.0f));
        widget->renderText(text.str(), position[0], position[1]);
        position[1] += scale * 16.0f;
    }

    // An elixir.
    if (item->category == ItemCategoryElixir) {
        text.str(L"");
        switch (item->id) {
        case ElixirElixirOfLife:
            text << "HP +20%";
            break;
        case ElixirElixirOfPower:
            text << "DAMAGE +20%";
            break;
        case ElixirElixirOfToughness:
            text << "ARMOR +20%";
            break;
        case ElixirElixirOfSanity:
            text << "RESISTANCE +20%";
            break;
        case ElixirLemonade:
            text << "A refreshing drink that helps against extreme heat.";
            break;
        case ElixirHotChocolate:
            text << "A warming drink that helps against extreme coldness.";
            break;
        case ElixirGreenSmoothie:
            text << "A very healthy drink that helps against toxic environments.";
            break;
        }
        widget->setTextSize(scale * 12.0f);
        widget->setTextColor(plasma::Vector<4, float>(0.6f, 0.5f, 1.0f, 1.0f));
        plasma::Vector<2, float> end = widget->renderText(text.str(), position[0], position[1]);
        position[1] = end[1] + scale * 20.0f;
        widget->setTextColor(plasma::Vector<4, float>(0.0f, 1.0f, 1.0f, 1.0f));
        widget->renderText(L"Effect lasts 10 Minutes", position[0], position[1]);
        position[1] += scale * 16.0f;
    }
    return position;
}

}
