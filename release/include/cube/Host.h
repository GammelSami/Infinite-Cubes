// Host runs the server side of a multiplayer session inside the game process. It owns the authoritative World, a
// generation thread, joined-player connections, invitations and received client packets. Packets for the local Client
// go through m_localClientConnection; remote packets use Steam P2P. Destruction stops and joins the thread and frees
// queued packets and connections, but does not delete m_localClientConnection.
#pragma once
#include <windows.h>
#include <list>
#include <map>
#include <string>
#include <thread>
#include <utility>
#include "steam/steam_api.h"
#include "plasma/Vector.h"
#include "cube/BytesIO.h"
#include "cube/Creature.h"
#include "cube/ServerUpdates.h"
#include "cube/World.h"

namespace cube {

class SpriteManager;

// One joined player, as the host sees it.
class Connection {
public:
    Connection()
        : m_id(0)
    {}

    // The player's creature id: the first of 1..10 that no creature has when the player joins.
    long long m_id;
    // CSteamID of the player (Host::receive stores the sender's ID).
    CSteamID m_steamId;
    // The events still to be sent to this player.
    ServerUpdates m_serverUpdates;
    // The zones the player asked for with packet 15: Host::addServerUpdates sends each loaded zone's contents and
    // clears the list.
    std::list<plasma::Vector<2, int>> m_zones;
    // Constructed and destroyed only: no code of Host (or of the program) adds to, reads or clears it.
    std::list<plasma::Vector<2, int>> m_unusedZones;
    // The chat lines still to be sent to this player: Host::receive queues a packet 14 for every player, Host::update
    // sends one per tick.
    std::list<ChatMessage> m_chatMessages;
    // Entity data last sent for each creature. update sends differences, or a complete record for a new creature.
    std::map<long long, Creature::EntityData> m_sentEntityData;
};

class Host {
public:
    // The receiver of the packets the host sends to the local player.
    class BaseLocalClientConnection {
    public:
        // Delivers the host's packet and sender ID to the local client.
        virtual void send(const CSteamID &sender, const BytesIO &packet) = 0;
    };

    // Forwards packets through the supplied member function. T must be complete where this template is instantiated.
    template<class T> class LocalClientConnection : public BaseLocalClientConnection {
    public:
        typedef void (T::*Method)(const CSteamID &sender, const BytesIO &packet);
        LocalClientConnection(T *client, Method method)
            : m_client(client),
              m_method(method)
        {}
        virtual void send(const CSteamID &sender, const BytesIO &packet) { (m_client->*m_method)(sender, packet); }

        T *m_client;
        Method m_method;
    };

    explicit Host(SpriteManager *spriteManager);
    virtual ~Host();

    // Queues a copy of a packet received from the local client with its sender (under m_receivedPacketsLock). Called
    // by cube::Client.
    void addPacket(const CSteamID &sender, const BytesIO &packet);
    // Appends the world's events of a tick to what each player still has to receive (particles and sounds only within
    // 256 blocks of the player's creature), and the contents of the zones the player asked for.
    void addServerUpdates(const ServerUpdates &serverUpdates);
    // Forgets the player: the invitation, the creature and the connection.
    void disconnect(CSteamID steamId);
    // Moves the hits, sounds, buffs and projectiles received from the players into serverUpdates.
    void getServerUpdates(ServerUpdates &serverUpdates);
    bool hasJoined(const CSteamID &steamId);
    void invite(CSteamID steamId);
    bool isInvited(const CSteamID &steamId);
    // Handles one packet of a client (the packet types are listed in Host.cpp).
    void receive(const CSteamID &sender, BytesIO &packet);
    // Sends a packet to a player: through m_localClientConnection for the user's own Steam ID, else through Steam P2P
    // (reliable, channel 0).
    void send(const CSteamID &steamId, BytesIO &packet);
    // Closes the session: tells every remote player (packet 2), deletes the connections, the invitations and
    // everything queued, and clears the world.
    void stop();
    // disconnect(), then tells the player (packet 2).
    void uninvite(CSteamID steamId);
    // Runs a host tick, at most 1000 ms: receives client packets, repeats invitations every five seconds, sends entity
    // data, time and events, then advances the world.
    void update(int milliseconds);

    // Replaces the local client connection with an adapter for client and method.
    template<class T> void setLocalClientConnection(T *client, void (T::*method)(const CSteamID &, const BytesIO &))
    {
        if (m_localClientConnection)
            delete m_localClientConnection;
        m_localClientConnection = new LocalClientConnection<T>(client, method);
    }

    // The host's world.
    World m_world;
    // Read by the generation thread; cleared by the destructor before joining.
    volatile bool
        m_running;        // Initially true; the generation thread tests it and the destructor clears it before joining.
    std::thread m_thread; // World generation thread.
    // Joined players keyed by Steam ID, guarded by m_connectionsLock.
    std::map<CSteamID, Connection *> m_connections;
    // Invited Steam IDs mapped to their invitation time from timeGetTime().
    std::map<CSteamID, unsigned int> m_invitations;
    std::list<ServerUpdates::HitPacket> m_hitPacketList;
    std::list<ServerUpdates::BuffPacket> m_buffPacketList;
    std::list<ServerUpdates::BulletPacket> m_bulletPacketList;
    std::list<ServerUpdates::SoundPacket> m_soundPacketList;
    BaseLocalClientConnection *m_localClientConnection;
    CRITICAL_SECTION m_receivedPacketsLock; // guards m_receivedPackets
    CRITICAL_SECTION m_connectionsLock;
    std::list<std::pair<CSteamID, BytesIO *>> m_receivedPackets;
};

}
