// Client is the player's side of a multiplayer session. A background thread runs Steam callbacks and queues packets
// received on P2P channel 0. The local host queues packets directly through Host::LocalClientConnection. The game
// thread processes received packets, keeps the entity data last sent to the host and forwards queued hits, sounds,
// buffs, projectiles and chat messages. Packet formats are documented in Client.cpp.
#pragma once
#include <windows.h>
#include <list>
#include <map>
#include <set>
#include <string>
#include <thread>
#include <utility>
#include "steam/steam_api.h"
#include "cube/BytesIO.h"
#include "cube/Creature.h"
#include "cube/ServerUpdates.h"

namespace cube {

class Game;
class Host;

class Client {
public:
    explicit Client(Game *game);
    virtual ~Client();

    // Queues a copy of a received packet with its sender (under the lock).
    void addPacket(const CSteamID &sender, const BytesIO &packet);
    // Nothing while no host is joined.
    void addServerUpdates(const ServerUpdates &serverUpdates);
    void connect(CSteamID steamId);
    void disconnect();
    bool isConnected();
    bool isHost(const CSteamID &steamId);
    bool isInvitedBy(const CSteamID &steamId);
    // Handles one packet of the host or of an inviting user (the packet types are listed in Client.cpp).
    void receive(const CSteamID &sender, BytesIO &packet);
    // Sends a packet to steamId: through Steam P2P (reliable, channel 1), or straight into the host of this process
    // when steamId is the user.
    void send(const CSteamID &steamId, BytesIO &packet);
    void update();

    // Read by the receive thread (loops while set); cleared by the destructor before joining m_thread.
    volatile bool m_runningCallbacks; // Initially true; cleared before the receive thread is joined.
    std::thread m_thread;             // Background packet receive thread.
    std::list<ServerUpdates::HitPacket> m_hitPacketList;
    std::list<ServerUpdates::SoundPacket> m_soundPacketList;
    std::list<ServerUpdates::BuffPacket> m_buffPacketList;
    std::list<ServerUpdates::BulletPacket> m_bulletPacketList;
    // Outgoing chat messages.
    std::list<std::wstring> m_chatMessageList;
    CSteamID m_hostSteamId;
    Creature::EntityData m_localEntityData;
    bool m_sendFullEntityData;         // Initially 1.
    unsigned int m_lastHostPacketTime; // Initially 0.
    std::set<long long> m_receivedCreatureIds;
    std::map<CSteamID, unsigned int> m_invitations;
    Host *m_host;                           // The game's host, supplied by Game's constructor.
    CRITICAL_SECTION m_receivedPacketsLock; // Critical-section spin count.
    std::list<std::pair<CSteamID, BytesIO *>> m_receivedPackets;
    Game *m_game;
};

}
