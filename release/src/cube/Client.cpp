#include "plasma/Utf16.h"
#include <windows.h>
#include <mmsystem.h>
// See cube/Client.h.
#include <iostream>
#include "cube/Client.h"
#include "cube/widgets/ChatWidget.h"
#include "cube/Game.h"
#include "cube/Host.h"
#include "cube/PacketIds.h"
#include "cube/World.h"
#include "plasma/Engine.h"
#include "plasma/EditFilter.h"

namespace cube {

static const int HostPacketTimeoutMs = 5000;
static const int InvitationLifetimeMs = 6000;
static const int EntitySendIntervalMs = 32;
static const int ZoneRequestIntervalMs = 1000;
static const int ClientUpdateWarningMs = 8;

// The user's own Steam ID, which is also the address of this process's host.
static CSteamID g_localSteamId;

Client::Client(Game *game)
    : m_runningCallbacks(true),
      m_sendFullEntityData(true),
      m_lastHostPacketTime(0),
      m_host(nullptr),
      m_game(game)
{
    InitializeCriticalSectionAndSpinCount(&m_receivedPacketsLock, 16);
    g_localSteamId = SteamUser()->GetSteamID();
    m_thread = std::thread([this] {
        while (m_runningCallbacks) {
            SteamAPI_RunCallbacks();
            uint32 size;
            while (SteamNetworking()->IsP2PPacketAvailable(&size, 0)) {
                BytesIO packet;
                packet.resize(size);
                CSteamID sender;
                if (SteamNetworking()->ReadP2PPacket(packet.data(), size, &size, &sender, 0))
                    addPacket(sender, packet);
                Sleep(0);
            }
            Sleep(16);
        }
    });
}

Client::~Client()
{
    connect(g_localSteamId);
    DeleteCriticalSection(&m_receivedPacketsLock);
    for (std::pair<CSteamID, BytesIO *> entry : m_receivedPackets)
        delete entry.second;
    m_runningCallbacks = false;
    if (m_thread.joinable())
        m_thread.join();
}

void Client::addPacket(const CSteamID &sender, const BytesIO &packet)
{
    std::pair<CSteamID, BytesIO *> entry(sender, new BytesIO(packet));
    EnterCriticalSection(&m_receivedPacketsLock);
    m_receivedPackets.push_back(entry);
    LeaveCriticalSection(&m_receivedPacketsLock);
}

void Client::addServerUpdates(const ServerUpdates &serverUpdates)
{
    if (!m_hostSteamId.IsValid())
        return;
    for (const ServerUpdates::HitPacket &hit : serverUpdates.m_hitPacketList) {
        if (hit.attackerId == m_game->m_world->m_localCreature->m_id)
            m_hitPacketList.push_back(hit);
    }
    for (const ServerUpdates::SoundPacket &sound : serverUpdates.m_soundPacketList) {
        if (sound.sourceId == m_game->m_world->m_localCreature->m_id)
            m_soundPacketList.push_back(sound);
    }
    for (const ServerUpdates::BuffPacket &buff : serverUpdates.m_buffPacketList) {
        if (buff.sourceId == m_game->m_world->m_localCreature->m_id)
            m_buffPacketList.push_back(buff);
    }
    for (const ServerUpdates::BulletPacket &projectile : serverUpdates.m_bulletPacketList) {
        if (projectile.ownerId == m_game->m_world->m_localCreature->m_id)
            m_bulletPacketList.push_back(projectile);
    }
}

void Client::connect(CSteamID steamId)
{
    if (m_hostSteamId.IsValid()) {
        BytesIO packet;
        packet.write((int)PacketLeave);
        send(m_hostSteamId, packet);
        m_invitations.erase(m_hostSteamId);
    }
    m_localEntityData = Creature::EntityData();
    m_sendFullEntityData = true;
    m_hostSteamId.Clear();
    m_receivedCreatureIds.clear();
    m_hitPacketList.clear();
    m_soundPacketList.clear();
    m_bulletPacketList.clear();
    m_buffPacketList.clear();
    m_game->m_world->retainLocalCreature();
    m_game->m_world->updateChanges();
    if (m_host)
        m_host->stop();

    BytesIO packet;
    packet.write((int)PacketJoin);
    send(steamId, packet);
    m_lastHostPacketTime = timeGetTime();
    m_hostSteamId = steamId;
    m_game->m_world->m_isClient = true;
}

void Client::disconnect()
{
    connect(g_localSteamId);
}

bool Client::isConnected()
{
    return m_hostSteamId.IsValid() && m_hostSteamId != g_localSteamId;
}

bool Client::isHost(const CSteamID &steamId)
{
    return m_hostSteamId == steamId;
}

bool Client::isInvitedBy(const CSteamID &steamId)
{
    return m_invitations.find(steamId) != m_invitations.end();
}

// Received packet types, identified by their leading integer: 0 invites the client and carries a protocol version; 2
// closes a host session or withdraws an invitation; 3 carries compressed creature entity changes; 4 assigns the local
// creature ID; 6 ends a full entity update; 7 carries compressed ServerUpdates; 8 sets the day and time; 14 carries
// the speaker ID and a length-prefixed chat line. Only invitations and disconnections are accepted from senders other
// than the host.
void Client::receive(const CSteamID &sender, BytesIO &packet)
{
    int type; // An empty packet leaves this read uninitialised.
    packet.read(type);
    if (sender != m_hostSteamId && type != PacketInvitation && type != PacketLeave)
        return;
    if (sender == m_hostSteamId)
        m_lastHostPacketTime = timeGetTime();

    switch (type) {
    case PacketInvitation: {
        int version = 0;
        packet.read(version);
        if (version != PROTOCOL_VERSION) {
            BytesIO reply;
            reply.write((int)PacketVersionMismatch);
            reply.write(PROTOCOL_VERSION);
            send(sender, reply);
            break;
        }
        if (!isInvitedBy(sender)) {
            m_game->m_interface.m_inGame.m_chatWidget->printMessage(
                L"You were invited by " + WidenString(SteamFriends()->GetFriendPersonaName(sender)) + L".\n",
                plasma::Vector<4, float>(1.0f, 0.5f, 0.2f, 1.0f));
        }
        m_invitations[sender] = timeGetTime();
        break;
    }
    case PacketLeave:
        if (sender == m_hostSteamId)
            connect(g_localSteamId);
        m_invitations.erase(sender);
        break;
    case PacketCreatureId: {
        long long id; // A short packet leaves this value uninitialised.
        packet.read(id);
        m_game->m_world->changeLocalCreatureId(id);
        m_sendFullEntityData = true;
        break;
    }
    case PacketEntityData: {
        int size = 0;
        packet.read(size);
        if (size <= 0)
            break;
        BytesIO data;
        data.resize(size);
        packet.read(data.data(), size);
        decompress(data);
        long long id = 0;
        data.read(id);
        if (id != m_game->m_world->m_localCreature->m_id) {
            m_receivedCreatureIds.insert(id);
            Creature *creature = m_game->m_world->creatureByID(id);
            bool created = false;
            if (!creature) {
                creature = new Creature(id);
                m_game->m_world->addCreature(creature);
                created = true;
            }
            decodeEntityData(data, creature->m_entityData);
            if (created || creature->m_entityData.hp <= 0.0f) {
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
        break;
    }
    case PacketFullUpdateEnd: {
        std::list<Creature *> removed;
        for (Creature *creature : m_game->m_world->m_creatures) {
            if (creature != m_game->m_world->m_localCreature &&
                m_receivedCreatureIds.find(creature->m_id) == m_receivedCreatureIds.end())
                removed.push_back(creature);
        }
        for (Creature *creature : removed)
            m_game->m_world->removeCreature(creature);
        m_receivedCreatureIds.clear();
        break;
    }
    case PacketChat: {
        ChatMessage message; // A short packet leaves the ID and length uninitialised.
        packet.read(message.creatureId);
        int length;
        packet.read(length);
        if (length > 0) {
            std::vector<uint16_t> units(length);
            packet.read(units.data(), length * 2);
            message.text = plasma::fromUtf16(units.data(), length);
            m_game->addChatMessage(message);
        }
        break;
    }
    case PacketTime: {
        int day; // A short packet leaves both values uninitialised.
        packet.read(day);
        int time;
        packet.read(time);
        if (m_game->m_world->m_state.m_day != day)
            m_game->m_world->onNewDay();
        m_game->m_world->m_state.m_day = day;
        m_game->m_world->m_state.m_time = time;
        break;
    }
    case PacketWorldUpdate: {
        int size = 0;
        packet.read(size);
        BytesIO data;
        data.resize(size);
        packet.read(data.data(), data.size());
        decompress(data);

        ServerUpdates updates;
        data.read(updates.m_cubeChangeList);
        data.read(updates.m_hitPacketList);
        data.read(updates.m_particlePacketList);
        data.read(updates.m_soundPacketList);
        data.read(updates.m_bulletPacketList);
        int objectListCount = 0;
        data.read(objectListCount);
        for (int i = 0; i < objectListCount; ++i) {
            ServerUpdates::Packet6 zoneObjects;
            data.read(zoneObjects.zone);
            data.read(zoneObjects.objects);
            updates.m_staticEntityPacketList.push_back(zoneObjects);
        }
        int itemListCount = 0;
        data.read(itemListCount);
        for (int i = 0; i < itemListCount; ++i) {
            ServerUpdates::ItemListPacket zoneItems;
            data.read(zoneItems.zone);
            data.read(zoneItems.items);
            updates.m_itemListPacketList.push_back(zoneItems);
        }
        data.read(updates.m_deliveredZoneList);
        data.read(updates.m_itemPickupPacketList);
        data.read(updates.m_killPacketList);
        data.read(updates.m_passivePacketList);
        data.read(updates.m_aggroPacketList);
        data.read(updates.m_buffPacketList);
        data.read(updates.m_worldEventList);
        data.read(updates.m_talkingCreatureList);

        // The game handles them with the events of its own world.
        m_game->m_serverUpdates.m_cubeChangeList.insert(m_game->m_serverUpdates.m_cubeChangeList.end(),
                                                        updates.m_cubeChangeList.begin(),
                                                        updates.m_cubeChangeList.end());
        m_game->m_serverUpdates.m_hitPacketList.insert(m_game->m_serverUpdates.m_hitPacketList.end(),
                                                       updates.m_hitPacketList.begin(), updates.m_hitPacketList.end());
        m_game->m_serverUpdates.m_particlePacketList.insert(m_game->m_serverUpdates.m_particlePacketList.end(),
                                                            updates.m_particlePacketList.begin(),
                                                            updates.m_particlePacketList.end());
        m_game->m_serverUpdates.m_soundPacketList.insert(m_game->m_serverUpdates.m_soundPacketList.end(),
                                                         updates.m_soundPacketList.begin(),
                                                         updates.m_soundPacketList.end());
        m_game->m_serverUpdates.m_bulletPacketList.insert(m_game->m_serverUpdates.m_bulletPacketList.end(),
                                                          updates.m_bulletPacketList.begin(),
                                                          updates.m_bulletPacketList.end());
        m_game->m_serverUpdates.m_itemListPacketList.insert(m_game->m_serverUpdates.m_itemListPacketList.end(),
                                                            updates.m_itemListPacketList.begin(),
                                                            updates.m_itemListPacketList.end());
        m_game->m_serverUpdates.m_staticEntityPacketList.insert(m_game->m_serverUpdates.m_staticEntityPacketList.end(),
                                                                updates.m_staticEntityPacketList.begin(),
                                                                updates.m_staticEntityPacketList.end());
        m_game->m_serverUpdates.m_deliveredZoneList.insert(m_game->m_serverUpdates.m_deliveredZoneList.end(),
                                                           updates.m_deliveredZoneList.begin(),
                                                           updates.m_deliveredZoneList.end());
        m_game->m_serverUpdates.m_itemPickupPacketList.insert(m_game->m_serverUpdates.m_itemPickupPacketList.end(),
                                                              updates.m_itemPickupPacketList.begin(),
                                                              updates.m_itemPickupPacketList.end());
        m_game->m_serverUpdates.m_killPacketList.insert(m_game->m_serverUpdates.m_killPacketList.end(),
                                                        updates.m_killPacketList.begin(),
                                                        updates.m_killPacketList.end());
        m_game->m_serverUpdates.m_passivePacketList.insert(m_game->m_serverUpdates.m_passivePacketList.end(),
                                                           updates.m_passivePacketList.begin(),
                                                           updates.m_passivePacketList.end());
        m_game->m_serverUpdates.m_aggroPacketList.insert(m_game->m_serverUpdates.m_aggroPacketList.end(),
                                                         updates.m_aggroPacketList.begin(),
                                                         updates.m_aggroPacketList.end());
        m_game->m_serverUpdates.m_buffPacketList.insert(m_game->m_serverUpdates.m_buffPacketList.end(),
                                                        updates.m_buffPacketList.begin(),
                                                        updates.m_buffPacketList.end());
        m_game->m_serverUpdates.m_worldEventList.insert(m_game->m_serverUpdates.m_worldEventList.end(),
                                                        updates.m_worldEventList.begin(),
                                                        updates.m_worldEventList.end());
        m_game->m_serverUpdates.m_talkingCreatureList.insert(m_game->m_serverUpdates.m_talkingCreatureList.end(),
                                                             updates.m_talkingCreatureList.begin(),
                                                             updates.m_talkingCreatureList.end());
        break;
    }
    }
}

void Client::send(const CSteamID &steamId, BytesIO &packet)
{
    if (steamId == g_localSteamId) {
        if (m_host)
            m_host->addPacket(g_localSteamId, packet);
    } else {
        SteamNetworking()->SendP2PPacket(steamId, packet.data(), packet.size(), k_EP2PSendReliable, 1);
    }
}

// Outgoing packet types: 1 joins; 2 leaves; 3 sends the local creature ID and compressed entity changes; 9 sends an
// interaction; 10, 11, 12 and 13 send hits, sounds, buffs and projectiles; 14 sends a length-prefixed chat line; 15
// requests a zone once per second; 24 replies to an incompatible invitation with the local protocol version.
void Client::update()
{
    EnterCriticalSection(&m_receivedPacketsLock);
    std::list<std::pair<CSteamID, BytesIO *>> packets(m_receivedPackets);
    m_receivedPackets.clear();
    LeaveCriticalSection(&m_receivedPacketsLock);
    for (std::pair<CSteamID, BytesIO *> entry : packets) {
        receive(entry.first, *entry.second);
        delete entry.second;
    }

    for (int event : m_game->m_world->m_serverUpdates.m_worldEventList) {
        if (event == WorldEventNewDay) {
            m_localEntityData = Creature::EntityData();
            m_sendFullEntityData = true;
        }
    }

    DWORD startTime = timeGetTime();
    static unsigned int lastSendTime = timeGetTime();
    unsigned int time = timeGetTime();
    if (time - m_lastHostPacketTime > HostPacketTimeoutMs) {
        connect(g_localSteamId);
        return;
    }

    std::list<CSteamID> expired;
    for (const std::pair<const CSteamID, unsigned int> &invitation : m_invitations) {
        if (time - invitation.second > InvitationLifetimeMs) {
            std::cout << "INVITATION TIMEOUT" << std::endl;
            expired.push_back(invitation.first);
        }
    }
    for (CSteamID &steamId : expired)
        m_invitations.erase(steamId);

    if (time - lastSendTime > EntitySendIntervalMs) {
        lastSendTime = time;
        std::list<CSteamID> unused;
        if (m_hostSteamId.IsValid()) {
            BytesIO entityData;
            Creature *creature = m_game->m_world->m_localCreature;
            entityData.write(creature->m_id);
            encodeEntityData(entityData, m_localEntityData, m_game->m_world->m_localCreature->m_entityData,
                             m_sendFullEntityData);
            m_sendFullEntityData = false;
            m_localEntityData = m_game->m_world->m_localCreature->m_entityData;
            compressBytes(entityData);

            BytesIO packet;
            packet.write((int)PacketEntityData);
            packet.write(entityData.size());
            packet.write(entityData.data(), entityData.size());
            send(m_hostSteamId, packet);

            std::wstring chatMessage;
            if (m_chatMessageList.size()) {
                chatMessage = m_chatMessageList.front();
                m_chatMessageList.pop_front();
            }
            size_t interactionCount = m_game->m_world->m_localCreature->m_interactions.size();
            Interaction interaction;
            if (interactionCount) {
                interaction = m_game->m_world->m_localCreature->m_interactions.front();
                m_game->m_world->m_localCreature->m_interactions.pop_front();
            }
            if (chatMessage.size()) {
                packet.clear();
                packet.write((int)PacketChat);
                std::vector<uint16_t> units = plasma::toUtf16(chatMessage);
                int length = (int)units.size();
                packet.write(length);
                packet.write(units.data(), length * 2);
                send(m_hostSteamId, packet);
            }
            if (interactionCount) {
                packet.clear();
                packet.write((int)PacketInteraction);
                packet.write(interaction);
                send(m_hostSteamId, packet);
            }
            for (ServerUpdates::HitPacket &hit : m_hitPacketList) {
                packet.clear();
                packet.write((int)PacketHit);
                packet.write(hit);
                send(m_hostSteamId, packet);
            }
            for (ServerUpdates::SoundPacket &sound : m_soundPacketList) {
                packet.clear();
                packet.write((int)PacketSound);
                packet.write(sound);
                send(m_hostSteamId, packet);
            }
            for (ServerUpdates::BuffPacket &buff : m_buffPacketList) {
                packet.clear();
                packet.write((int)PacketBuff);
                packet.write(buff);
                send(m_hostSteamId, packet);
            }
            for (ServerUpdates::BulletPacket &projectile : m_bulletPacketList) {
                packet.clear();
                packet.write((int)PacketProjectile);
                packet.write(projectile);
                send(m_hostSteamId, packet);
            }
            m_hitPacketList.clear();
            m_soundPacketList.clear();
            m_buffPacketList.clear();
            m_bulletPacketList.clear();

            static unsigned int lastIdTime = timeGetTime();
            unsigned int idTime = timeGetTime();
            if (idTime - lastIdTime > ZoneRequestIntervalMs) {
                lastIdTime = idTime;
                for (const plasma::Vector<2, int> &zone : m_game->m_requestedZones) {
                    packet.clear();
                    packet.write((int)PacketZoneRequest);
                    packet.write(zone);
                    send(m_hostSteamId, packet);
                }
            }
        }
    }

    DWORD elapsed = timeGetTime() - startTime;
    if (elapsed > ClientUpdateWarningMs)
        std::cout << elapsed << std::endl;
}

}
