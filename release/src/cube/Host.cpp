#include "plasma/Utf16.h"
#include <windows.h>
#include <mmsystem.h>
// Host packet protocol. Incoming types: 1 joins and receives a creature ID in 1..10; 2 leaves; 3 supplies compressed
// entity changes; 9 supplies an interaction; 10, 11, 12 and 13 supply hits, sounds, buffs and projectiles; 14 queues a
// length-prefixed chat line for every player; 15 requests a zone; 24 refuses an invitation and supplies a protocol
// version. All except 1, 2 and 24 require a joined player. Outgoing types: 0 invites with the protocol version every
// five seconds; 2 closes or withdraws; 3 sends compressed entity changes; 4 assigns a creature ID; 6 ends an entity
// update; 7 sends compressed world-event lists; 8 sends day and time; 14 sends a chat line with its speaker ID.
#include <algorithm>
#include <array>
#include <iostream>
#include <set>
#include <vector>
#include "cube/Host.h"
#include "cube/PacketIds.h"
#include "cube/Creature.h"
#include "cube/SpriteManager.h"
#include "cube/World.h"
#include "cube/Zone.h"
#include "cube/CreatureIds.h"
#include "cube/ServerUpdateIds.h"
#include "plasma/EditFilter.h"

namespace cube {

// The user's own Steam ID: the address of the local client.
static CSteamID g_localSteamId;

// A zone the generation thread may generate next, with the squared distance (in blocks) of the nearest player to its
// square.
struct ZoneCandidate {
    std::array<int, 2> zone;
    int distance;
};

Host::Host(SpriteManager *spriteManager)
    : m_world(spriteManager),
      m_running(true),
      m_localClientConnection(nullptr)
{
    g_localSteamId = SteamUser()->GetSteamID();
    InitializeCriticalSectionAndSpinCount(&m_receivedPacketsLock, 16);
    InitializeCriticalSectionAndSpinCount(&m_connectionsLock, 16);
    m_world.init(123, " world");
    // The generation thread: keeps the zones around the players loaded (at most four new zones per round, nearest
    // first) and saves the world state once a minute and when the host is destroyed.
    m_thread = std::thread([this] {
        timeGetTime();
        unsigned int lastSaveTime = timeGetTime();
        while (m_running) {
            // The block position of every player, or the destination of one who is flying (ability 156).
            m_world.enterZoneMeshes();
            std::list<std::array<int, 2>> positions;
            for (Creature *creature : m_world.m_creatures) {
                if (creature->m_entityData.hostilityType == HostilityPlayer) {
                    if (creature->m_entityData.currentAbility == AbilityEagleFlight) {
                        positions.push_back(
                            {{creature->m_entityData.destination[0], creature->m_entityData.destination[1]}});
                    } else {
                        positions.push_back(
                            {{(int)creature->m_entityData.position[0], (int)creature->m_entityData.position[1]}});
                    }
                }
            }
            m_world.leaveZoneMeshes();
            unsigned int time = timeGetTime();

            // The zones within 270 blocks and five zones of a player.
            std::vector<ZoneCandidate> candidates;
            std::set<std::array<int, 2>> zones;
            for (const std::array<int, 2> &position : positions) {
                int minX = floorDivide(position[0] - 270, BlocksPerZone);
                int minY = floorDivide(position[1] - 270, BlocksPerZone);
                int maxX = floorDivide(position[0] + 270, BlocksPerZone);
                int maxY = floorDivide(position[1] + 270, BlocksPerZone);
                int zoneX = floorDivide(position[0], BlocksPerZone);
                int zoneY = floorDivide(position[1], BlocksPerZone);
                for (int x = minX; x <= maxX; ++x) {
                    for (int y = minY; y <= maxY; ++y) {
                        int dx = x - zoneX;
                        int dy = y - zoneY;
                        if (abs(dx) > 5 || abs(dy) > 5)
                            continue;
                        if ((double)dx * (double)dx + (double)dy * (double)dy > 25.0)
                            continue;
                        std::array<int, 2> zone = {x, y};
                        zones.insert(zone);
                    }
                }
            }
            m_world.requireZones(zones);

            // The zones not generated yet, with the squared distance of the nearest player to the zone's square.
            for (const std::array<int, 2> &zone : zones) {
                if (m_world.zone(zone[0], zone[1]))
                    continue;
                int nearest = -1;
                for (const std::array<int, 2> &position : positions) {
                    int x0 = zone[0] * BlocksPerZone;
                    int x1 = x0 + BlocksPerZone;
                    int y0 = zone[1] * BlocksPerZone;
                    int y1 = y0 + BlocksPerZone;
                    int x = position[0];
                    int y = position[1];
                    if (x >= x0 && x <= x1 && y >= y0 && y <= y1) {
                        nearest = 0;
                        break;
                    }
                    int dx = x - x0;
                    if (abs(dx) > 384)
                        continue;
                    int dy = y - y0;
                    if (abs(dy) > 384)
                        continue;
                    {
                        int distance;
                        if (x >= x0 && x <= x1) {
                            if (y < y0)
                                distance = (y0 - y) * (y0 - y);
                            else
                                distance = (y - y1) * (y - y1);
                        } else if (y >= y0 && y <= y1) {
                            if (x < x0)
                                distance = (x0 - x) * (x0 - x);
                            else
                                distance = (x - x1) * (x - x1);
                        } else if (x <= x0 && y <= y0) {
                            distance = dx * dx + dy * dy;
                        } else if (x <= x0 && y >= y1) {
                            distance = dx * dx + (y - y1) * (y - y1);
                        } else if (x >= x1 && y <= y0) {
                            distance = (x - x1) * (x - x1) + dy * dy;
                        } else {
                            distance = (x - x1) * (x - x1) + (y - y1) * (y - y1);
                        }
                        if (nearest < 0 || distance < nearest)
                            nearest = distance;
                    }
                }
                if (nearest >= 0) {
                    ZoneCandidate candidate = {zone, nearest};
                    candidates.push_back(candidate);
                }
            }

            if (candidates.empty()) {
                Sleep(5);
            } else {
                std::sort(candidates.begin(), candidates.end(),
                          [](const ZoneCandidate &a, const ZoneCandidate &b) { return a.distance < b.distance; });
                if (candidates.size() > 4)
                    candidates.resize(4);
                for (const ZoneCandidate &candidate : candidates)
                    m_world.generateZone(candidate.zone[0], candidate.zone[1]);
            }
            Sleep(0);

            if (time - lastSaveTime > 60000) {
                if (m_running) {
                    m_world.enterZoneMeshes();
                    World::WorldState state = m_world.m_state;
                    m_world.leaveZoneMeshes();
                    m_world.saveState(state);
                    lastSaveTime = time;
                }
            }
        }
        m_world.enterZoneMeshes();
        World::WorldState state = m_world.m_state;
        m_world.leaveZoneMeshes();
        m_world.saveState(state);
    });
}

Host::~Host()
{
    m_running = false;
    std::cout << "[HOST] waiting for generation thread..." << std::endl;
    m_thread.join();
    DeleteCriticalSection(&m_receivedPacketsLock);
    DeleteCriticalSection(&m_connectionsLock);
    for (std::pair<const CSteamID, Connection *> &entry : m_connections) {
        if (entry.first != g_localSteamId) {
            int packet = 2;
            SteamNetworking()->SendP2PPacket(entry.first, &packet, 4, k_EP2PSendReliable, 0);
        }
        delete entry.second;
    }
    for (std::pair<CSteamID, BytesIO *> entry : m_receivedPackets)
        delete entry.second;
}

void Host::addPacket(const CSteamID &sender, const BytesIO &packet)
{
    std::pair<CSteamID, BytesIO *> entry(sender, new BytesIO(packet));
    EnterCriticalSection(&m_receivedPacketsLock);
    m_receivedPackets.push_back(entry);
    LeaveCriticalSection(&m_receivedPacketsLock);
}

void Host::addServerUpdates(const ServerUpdates &serverUpdates)
{
    for (std::pair<const CSteamID, Connection *> &entry : m_connections) {
        Connection *connection = entry.second;
        connection->m_serverUpdates.m_cubeChangeList.insert(connection->m_serverUpdates.m_cubeChangeList.end(),
                                                            serverUpdates.m_cubeChangeList.begin(),
                                                            serverUpdates.m_cubeChangeList.end());
        connection->m_serverUpdates.m_hitPacketList.insert(connection->m_serverUpdates.m_hitPacketList.end(),
                                                           serverUpdates.m_hitPacketList.begin(),
                                                           serverUpdates.m_hitPacketList.end());

        // Particles and sounds only within 256 blocks of the player, and not the player's own sounds.
        Creature *creature = m_world.creatureByID(connection->m_id);
        if (creature) {
            for (std::list<ServerUpdates::ParticlePacket>::const_iterator particle =
                     serverUpdates.m_particlePacketList.begin();
                 particle != serverUpdates.m_particlePacketList.end(); ++particle) {
                if (!distanceExceeds(particle->position, creature->m_entityData.position, 256.0f))
                    connection->m_serverUpdates.m_particlePacketList.push_back(*particle);
            }
            for (std::list<ServerUpdates::SoundPacket>::const_iterator sound = serverUpdates.m_soundPacketList.begin();
                 sound != serverUpdates.m_soundPacketList.end(); ++sound) {
                if (sound->sound == SoundQuestComplete)
                    std::cout << "SoundMissionComplete????" << std::endl;
                if (!distanceExceeds(sound->position, creature->m_entityData.position, 256.0f) &&
                    sound->sourceId != connection->m_id) {
                    if (sound->sound == SoundDestroy2)
                        std::cout << "SoundDie SENT!" << std::endl;
                    if (sound->sound == SoundQuestComplete)
                        std::cout << "SoundMissionComplete SENT!" << std::endl;
                    connection->m_serverUpdates.m_soundPacketList.push_back(*sound);
                } else if (sound->sound == SoundDestroy2) {
                    std::cout << "SoundDie NOT SENT!" << std::endl;
                } else if (sound->sound == SoundQuestComplete) {
                    std::cout << "SoundMissionComplete NOT SENT!" << std::endl;
                }
            }
        }

        connection->m_serverUpdates.m_bulletPacketList.insert(connection->m_serverUpdates.m_bulletPacketList.end(),
                                                              serverUpdates.m_bulletPacketList.begin(),
                                                              serverUpdates.m_bulletPacketList.end());
        connection->m_serverUpdates.m_itemListPacketList.insert(connection->m_serverUpdates.m_itemListPacketList.end(),
                                                                serverUpdates.m_itemListPacketList.begin(),
                                                                serverUpdates.m_itemListPacketList.end());
        connection->m_serverUpdates.m_staticEntityPacketList.insert(
            connection->m_serverUpdates.m_staticEntityPacketList.end(), serverUpdates.m_staticEntityPacketList.begin(),
            serverUpdates.m_staticEntityPacketList.end());
        connection->m_serverUpdates.m_deliveredZoneList.insert(connection->m_serverUpdates.m_deliveredZoneList.end(),
                                                               serverUpdates.m_deliveredZoneList.begin(),
                                                               serverUpdates.m_deliveredZoneList.end());
        connection->m_serverUpdates.m_itemPickupPacketList.insert(
            connection->m_serverUpdates.m_itemPickupPacketList.end(), serverUpdates.m_itemPickupPacketList.begin(),
            serverUpdates.m_itemPickupPacketList.end());
        connection->m_serverUpdates.m_killPacketList.insert(connection->m_serverUpdates.m_killPacketList.end(),
                                                            serverUpdates.m_killPacketList.begin(),
                                                            serverUpdates.m_killPacketList.end());
        connection->m_serverUpdates.m_passivePacketList.insert(connection->m_serverUpdates.m_passivePacketList.end(),
                                                               serverUpdates.m_passivePacketList.begin(),
                                                               serverUpdates.m_passivePacketList.end());
        connection->m_serverUpdates.m_aggroPacketList.insert(connection->m_serverUpdates.m_aggroPacketList.end(),
                                                             serverUpdates.m_aggroPacketList.begin(),
                                                             serverUpdates.m_aggroPacketList.end());
        connection->m_serverUpdates.m_buffPacketList.insert(connection->m_serverUpdates.m_buffPacketList.end(),
                                                            serverUpdates.m_buffPacketList.begin(),
                                                            serverUpdates.m_buffPacketList.end());
        connection->m_serverUpdates.m_worldEventList.insert(connection->m_serverUpdates.m_worldEventList.end(),
                                                            serverUpdates.m_worldEventList.begin(),
                                                            serverUpdates.m_worldEventList.end());
        connection->m_serverUpdates.m_talkingCreatureList.insert(
            connection->m_serverUpdates.m_talkingCreatureList.end(), serverUpdates.m_talkingCreatureList.begin(),
            serverUpdates.m_talkingCreatureList.end());

        // The contents of the zones the player asked for, where loaded.
        for (const plasma::Vector<2, int> &position : connection->m_zones) {
            Zone *zone = m_world.zone(position[0], position[1]);
            if (zone) {
                if (zone->m_cubeChanges.size())
                    connection->m_serverUpdates.m_cubeChangeList.insert(
                        connection->m_serverUpdates.m_cubeChangeList.end(), zone->m_cubeChanges.begin(),
                        zone->m_cubeChanges.end());
                if (zone->m_itemsChanged)
                    connection->m_serverUpdates.m_itemListPacketList.push_back(ServerUpdates::ItemListPacket(*zone));
                if (zone->m_staticEntitiesChanged) {
                    connection->m_serverUpdates.m_staticEntityPacketList.push_back(ServerUpdates::Packet6(*zone));
                }
                connection->m_serverUpdates.m_deliveredZoneList.push_back(position);
            }
        }
        connection->m_zones.clear();
    }
}

void Host::disconnect(CSteamID steamId)
{
    EnterCriticalSection(&m_connectionsLock);
    m_invitations.erase(steamId);
    std::map<CSteamID, Connection *>::iterator it = m_connections.find(steamId);
    if (it != m_connections.end()) {
        Creature *creature = m_world.creatureByID(it->second->m_id);
        if (creature)
            m_world.removeCreature(creature);
        delete it->second;
        m_connections.erase(it);
    }
    LeaveCriticalSection(&m_connectionsLock);
}

void Host::getServerUpdates(ServerUpdates &serverUpdates)
{
    serverUpdates.m_hitPacketList.insert(serverUpdates.m_hitPacketList.end(), m_hitPacketList.begin(),
                                         m_hitPacketList.end());
    m_hitPacketList.clear();
    serverUpdates.m_soundPacketList.insert(serverUpdates.m_soundPacketList.end(), m_soundPacketList.begin(),
                                           m_soundPacketList.end());
    m_soundPacketList.clear();
    serverUpdates.m_buffPacketList.insert(serverUpdates.m_buffPacketList.end(), m_buffPacketList.begin(),
                                          m_buffPacketList.end());
    m_buffPacketList.clear();
    serverUpdates.m_bulletPacketList.insert(serverUpdates.m_bulletPacketList.end(), m_bulletPacketList.begin(),
                                            m_bulletPacketList.end());
    m_bulletPacketList.clear();
}

bool Host::hasJoined(const CSteamID &steamId)
{
    return m_connections.find(steamId) != m_connections.end();
}

void Host::invite(CSteamID steamId)
{
    BytesIO packet;
    packet.write((int)PacketInvitation);
    packet.write(PROTOCOL_VERSION);
    SteamNetworking()->SendP2PPacket(steamId, packet.data(), packet.size(), k_EP2PSendReliable, 0);
    EnterCriticalSection(&m_connectionsLock);
    m_invitations[steamId] = timeGetTime();
    LeaveCriticalSection(&m_connectionsLock);
}

bool Host::isInvited(const CSteamID &steamId)
{
    return m_invitations.find(steamId) != m_invitations.end();
}

void Host::receive(const CSteamID &sender, BytesIO &packet)
{
    // A short packet can leave type, chat length or zone coordinates uninitialised before use.
    int type;
    packet.read(type);
    std::map<CSteamID, Connection *>::iterator it = m_connections.find(sender);
    Connection *connection = nullptr;
    if (it != m_connections.end())
        connection = it->second;
    if (!connection)
        std::cout << "MESSAGE??? " << type << std::endl;

    switch (type) {
    case PacketLeave:
        disconnect(sender);
        break;
    case PacketVersionMismatch:
        m_invitations.erase(sender);
        break;
    case PacketJoin: {
        // The first unused player id, 1 through 10 (the player limit).
        int freeId = 1;
        while (m_world.creatureByID(freeId)) {
            if (++freeId > 10)
                return;
        }
        long long id = freeId;
        if (freeId) {
            if (m_connections[sender])
                delete m_connections[sender];
            Connection *created = new Connection;
            created->m_steamId = sender;
            created->m_id = id;
            m_connections[sender] = created;
            BytesIO reply;
            reply.write((int)PacketCreatureId);
            reply.write(id);
            send(sender, reply);
        }
        break;
    }
    case PacketEntityData:
        if (connection) {
            int size = 0;
            packet.read(size);
            if (size > 0) {
                BytesIO data;
                data.resize(size);
                packet.read(data.data(), size);
                decompress(data);
                long long id = 0;
                data.read(id);
                if (id == connection->m_id) {
                    Creature *creature = m_world.creatureByID(id);
                    bool existed = creature != nullptr;
                    if (!creature) {
                        creature = new Creature(id);
                        m_world.addCreature(creature);
                    }
                    decodeEntityData(data, creature->m_entityData);
                    if (!existed || creature->m_entityData.hp <= 0.0f) {
                        creature->m_smoothingTime = 0.0f;
                        creature->m_inventory.m_inventoryTabs.clear();
                        creature->m_drawnPosition = creature->m_entityData.position;
                        creature->m_drawnRotation = creature->m_entityData.rotation;
                        creature->m_previousDrawnPosition = creature->m_drawnPosition;
                        creature->m_previousDrawnRotation = creature->m_drawnRotation;
                    } else {
                        creature->m_smoothingTime = 60.0f;
                        creature->m_previousDrawnPosition = creature->m_drawnPosition;
                        creature->m_previousDrawnRotation = creature->m_drawnRotation;
                        creature->m_previousDrawnAttackRotation = creature->m_drawnAttackRotation;
                    }
                }
            }
        }
        break;
    case PacketInteraction:
        if (connection) {
            Interaction interaction;
            packet.read(interaction);
            Creature *creature = m_world.creatureByID(connection->m_id);
            if (creature)
                creature->m_interactions.push_back(interaction);
        }
        break;
    case PacketChat:
        if (connection) {
            int length;
            packet.read(length);
            if (length > 0) {
                std::vector<uint16_t> units(length);
                packet.read(units.data(), length * 2);
                std::wstring text = plasma::fromUtf16(units.data(), length);
                for (std::pair<const CSteamID, Connection *> &entry : m_connections) {
                    ChatMessage message;
                    message.text = text;
                    message.creatureId = connection->m_id;
                    entry.second->m_chatMessages.push_back(message);
                }
            }
        }
        break;
    case PacketHit:
        if (connection) {
            ServerUpdates::HitPacket hit;
            packet.read(hit);
            m_hitPacketList.push_back(hit);
        }
        break;
    case PacketSound:
        if (connection) {
            ServerUpdates::SoundPacket sound;
            packet.read(sound);
            m_soundPacketList.push_back(sound);
        }
        break;
    case PacketBuff:
        if (connection) {
            ServerUpdates::BuffPacket buff;
            packet.read(buff);
            m_buffPacketList.push_back(buff);
        }
        break;
    case PacketProjectile:
        if (connection) {
            ServerUpdates::BulletPacket projectile;
            packet.read(projectile);
            m_bulletPacketList.push_back(projectile);
        }
        break;
    case PacketZoneRequest:
        if (connection) {
            plasma::Vector<2, int> zone;
            packet.read(zone);
            connection->m_zones.push_back(zone);
        }
        break;
    }
}

void Host::send(const CSteamID &steamId, BytesIO &packet)
{
    if (steamId == g_localSteamId) {
        if (m_localClientConnection) {
            packet.seek(0);
            m_localClientConnection->send(g_localSteamId, packet);
        }
    } else {
        SteamNetworking()->SendP2PPacket(steamId, packet.data(), packet.size(), k_EP2PSendReliable, 0);
    }
}

void Host::stop()
{
    EnterCriticalSection(&m_connectionsLock);
    for (std::pair<const CSteamID, Connection *> &entry : m_connections) {
        if (entry.first != g_localSteamId) {
            int packet = 2;
            SteamNetworking()->SendP2PPacket(entry.first, &packet, 4, k_EP2PSendReliable, 0);
        }
        delete entry.second;
    }
    m_connections.clear();
    m_invitations.clear();
    m_hitPacketList.clear();
    m_buffPacketList.clear();
    m_bulletPacketList.clear();
    m_soundPacketList.clear();
    EnterCriticalSection(&m_receivedPacketsLock);
    for (std::pair<CSteamID, BytesIO *> &entry : m_receivedPackets)
        delete entry.second;
    m_receivedPackets.clear();
    LeaveCriticalSection(&m_receivedPacketsLock);
    m_world.removeCreatures();
    LeaveCriticalSection(&m_connectionsLock);
}

void Host::uninvite(CSteamID steamId)
{
    // Disconnects the player using the same cleanup steps as disconnect().
    EnterCriticalSection(&m_connectionsLock);
    m_invitations.erase(steamId);
    std::map<CSteamID, Connection *>::iterator it = m_connections.find(steamId);
    if (it != m_connections.end()) {
        Creature *creature = m_world.creatureByID(it->second->m_id);
        if (creature)
            m_world.removeCreature(creature);
        delete it->second;
        m_connections.erase(it);
    }
    LeaveCriticalSection(&m_connectionsLock);
    if (steamId != g_localSteamId) {
        int packet = 2;
        SteamNetworking()->SendP2PPacket(steamId, &packet, 4, k_EP2PSendReliable, 0);
    }
}

void Host::update(int milliseconds)
{
    // At most one second of world time per update.
    if (milliseconds > 1000)
        milliseconds = 1000;
    EnterCriticalSection(&m_connectionsLock);

    unsigned int receiveStart = timeGetTime();
    bool received = false;
    uint32 size;
    // The packets of the remote clients (channel 1).
    if (SteamNetworking()->IsP2PPacketAvailable(&size, 1)) {
        do {
            BytesIO packet;
            packet.resize(size);
            CSteamID sender;
            if (SteamNetworking()->ReadP2PPacket(packet.data(), size, &size, &sender, 1))
                receive(sender, packet);
        } while (SteamNetworking()->IsP2PPacketAvailable(&size, 1));
        received = true;
    }
    DWORD receiveTime = timeGetTime() - receiveStart;
    // Warn when receiving took more than 8 ms.
    if (receiveTime > 8)
        std::cout << "Host receive " << receiveTime << " | " << received << std::endl;

    EnterCriticalSection(&m_receivedPacketsLock);
    std::list<std::pair<CSteamID, BytesIO *>> packets = m_receivedPackets;
    m_receivedPackets.clear();
    LeaveCriticalSection(&m_receivedPacketsLock);
    for (std::pair<CSteamID, BytesIO *> entry : packets) {
        receive(entry.first, *entry.second);
        delete entry.second;
    }

    // Written, never read.
    static unsigned int lastUpdateTime = timeGetTime();
    unsigned int time = timeGetTime();
    lastUpdateTime = time;

    for (std::pair<const CSteamID, unsigned int> &invitation : m_invitations) {
        // An invitation is resent every 5 seconds until it connects.
        if (m_connections.find(invitation.first) == m_connections.end() && time - invitation.second > 5000) {
            invitation.second = time;
            BytesIO packet;
            packet.write((int)PacketInvitation);
            packet.write(PROTOCOL_VERSION);
            SteamNetworking()->SendP2PPacket(invitation.first, packet.data(), packet.size(), k_EP2PSendReliable, 0);
        }
    }

    {
        // Nothing is ever added to this list.
        std::list<CSteamID> disconnected;
        for (std::pair<const CSteamID, Connection *> &entry : m_connections) {
            Connection *connection = entry.second;
            bool first = true;
            std::map<long long, Creature::EntityData> entities;
            Creature *player = m_world.creatureByID(connection->m_id);
            if (!player)
                continue;

            // The creatures this player gets: the other players, the hostile ones within 270 blocks; of those not sent
            // before only one per tick, plus any with AppearanceFlagSummoned or AppearanceFlagMissionResident.
            for (Creature *creature : m_world.m_creatures) {
                if (creature->m_id == connection->m_id)
                    continue;
                if (creature->m_entityData.hostilityType &&
                    distanceExceeds(creature->m_entityData.position, player->m_entityData.position, 270.0f))
                    continue;
                if (connection->m_sentEntityData.find(creature->m_id) == connection->m_sentEntityData.end()) {
                    if (!first && !(creature->m_entityData.appearance.appearanceFlags &
                                    (AppearanceFlagSummoned | AppearanceFlagMissionResident)))
                        continue;
                    first = false;
                }
                entities[creature->m_id] = creature->m_entityData;
            }

            // What was sent before, without the creatures that are gone.
            std::map<long long, Creature::EntityData> sent;
            for (std::map<long long, Creature::EntityData>::iterator entity = connection->m_sentEntityData.begin();
                 entity != connection->m_sentEntityData.end(); ++entity) {
                if (m_world.creatureByID(entity->first))
                    sent[entity->first] = entity->second;
            }

            int timeOfDay = (int)m_world.m_state.m_time;
            int day = m_world.m_state.m_day;
            ServerUpdates updates = connection->m_serverUpdates;
            connection->m_serverUpdates = ServerUpdates {};
            ChatMessage message;
            if (connection->m_chatMessages.size()) {
                message = connection->m_chatMessages.front();
                connection->m_chatMessages.pop_front();
            }
            connection->m_sentEntityData = sent;

            for (std::map<long long, Creature::EntityData>::iterator entity = entities.begin();
                 entity != entities.end(); ++entity) {
                BytesIO packet;
                packet.write((int)PacketEntityData);
                BytesIO data;
                data.write(entity->first);
                if (connection->m_sentEntityData.find(entity->first) == connection->m_sentEntityData.end())
                    encodeEntityData(data, entity->second, entity->second, true);
                else
                    encodeEntityData(data, connection->m_sentEntityData[entity->first], entity->second, false);
                compressBytes(data);
                packet.write(data.size());
                packet.write(data.data(), data.size());
                send(entry.first, packet);
            }
            BytesIO packet;
            packet.write((int)PacketFullUpdateEnd);
            send(entry.first, packet);
            connection->m_sentEntityData = entities;

            packet.clear();
            packet.write((int)PacketTime);
            packet.write(day);
            packet.write(timeOfDay);
            send(entry.first, packet);

            packet.clear();
            packet.write((int)PacketWorldUpdate);
            BytesIO data;
            data.write(updates.m_cubeChangeList);
            data.write(updates.m_hitPacketList);
            data.write(updates.m_particlePacketList);
            data.write(updates.m_soundPacketList);
            data.write(updates.m_bulletPacketList);
            data.write((int)updates.m_staticEntityPacketList.size());
            for (std::list<ServerUpdates::Packet6>::iterator zoneObjects = updates.m_staticEntityPacketList.begin();
                 zoneObjects != updates.m_staticEntityPacketList.end(); ++zoneObjects) {
                data.write(zoneObjects->zone);
                data.write(zoneObjects->objects);
            }
            data.write((int)updates.m_itemListPacketList.size());
            for (std::list<ServerUpdates::ItemListPacket>::iterator zoneItems = updates.m_itemListPacketList.begin();
                 zoneItems != updates.m_itemListPacketList.end(); ++zoneItems) {
                data.write(zoneItems->zone);
                data.write(zoneItems->items);
            }
            data.write(updates.m_deliveredZoneList);
            data.write(updates.m_itemPickupPacketList);
            data.write(updates.m_killPacketList);
            data.write(updates.m_passivePacketList);
            data.write(updates.m_aggroPacketList);
            data.write(updates.m_buffPacketList);
            data.write(updates.m_worldEventList);
            data.write(updates.m_talkingCreatureList);
            compressBytes(data);
            packet.write(data.size());
            packet.write(data.data(), data.size());
            send(entry.first, packet);

            if (message.text.size()) {
                packet.clear();
                packet.write((int)PacketChat);
                packet.write(message.creatureId);
                std::vector<uint16_t> units = plasma::toUtf16(message.text);
                int length = (int)units.size();
                packet.write(length);
                packet.write(units.data(), length * 2);
                send(entry.first, packet);
            }
        }
        for (CSteamID steamId : disconnected)
            disconnect(steamId);
    }

    ServerUpdates receivedUpdates;
    getServerUpdates(receivedUpdates);
    m_world.enterZoneMeshes();
    m_world.applyEvents(receivedUpdates);
    int remaining = milliseconds;
    // Steps of at most 16 ms (the world step; Game::update's MaxWorldStepMs).
    while (remaining > 16) {
        m_world.updateStep(16.0f);
        remaining -= 16;
    }
    m_world.updateStep((float)remaining);
    m_world.inspectCreatures((float)milliseconds);
    m_world.updateZones();
    for (int event : m_world.m_serverUpdates.m_worldEventList) {
        if (event == WorldEventNewDay) {
            for (std::pair<const CSteamID, Connection *> &entry : m_connections)
                entry.second->m_sentEntityData.clear();
        }
    }
    addServerUpdates(m_world.m_serverUpdates);
    m_world.m_serverUpdates = ServerUpdates();
    m_world.leaveZoneMeshes();
    LeaveCriticalSection(&m_connectionsLock);
}

}
