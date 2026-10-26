#pragma once
#include <winsock2.h>
#include <windows.h>
#include <list>
#include <string>
#include <vector>
#include "cube/ServerUpdates.h"
#include "plasma/Vector.h"

namespace cube {
class World;

// The first int of every packet between Server.exe and a client.
enum PacketId {
    PacketEntityData = 0,   // either way: a creature's entity data
    PacketEntityList = 1,   // server -> client: every visible creature's entity data (not sent by this server)
    PacketEntitiesEnd = 2,  // server -> client: after the tick's entity data
    PacketObjects = 3,      // server -> client: the world objects near the player (not sent by this server)
    PacketWorldUpdate = 4,  // server -> client: the tick's events, compressed
    PacketTime = 5,         // server -> client: the day and the time of day
    PacketInteraction = 6,  // client -> server: an Interaction
    PacketHit = 7,          // client -> server: a HitPacket
    PacketBuff = 8,         // client -> server: a BuffPacket
    PacketProjectile = 9,   // client -> server: a BulletPacket
    PacketChat = 10,        // either way: a chat line
    PacketZoneRequest = 11, // client -> server: a zone the client needs
    PacketRegionRequest =
        12, // client -> server: a region whose mission states the client needs (Server::update answers with its
            // 8 x 8 records)
    PacketSeed = 15,      // server -> client: the world seed
    PacketWelcome = 16,   // server -> client: the connection is accepted
    PacketHello = 17,     // client -> server: first packet, then the version
    PacketServerFull = 18 // server -> client: no room for another player
};

// The version a client must send after PacketHello; a server with another version answers with its
// own and closes the connection.
const int ProtocolVersion = 3;
const unsigned short ServerPort = 12345;
// Creature ids 1..10 are the players'.
const int MaxPlayers = 10;

// Connection owns per-player packet queues and their critical sections.
struct Connection {
    Connection();
    virtual ~Connection();

    // Alignment gap after the virtual pointer.
    SOCKET socket;
    long long creatureId;
    ServerUpdates updates;
    std::list<plasma::Vector<2, int>> zones;
    std::list<plasma::Vector<2, int>> regionRequests;
    struct ChatLine {
        long long creatureId;
        std::wstring text;
    };
    std::list<ChatLine> chat;
    CRITICAL_SECTION lock;
};
struct HitPacket;
struct BuffPacket;
struct BulletPacket;

// Server owns the dedicated server's socket and connection state. Packet queues are drained under the world lock.
class Server {
public:
    Server(World *world);
    virtual ~Server();
    void start();
    void addServerUpdates(const ServerUpdates &updates);
    // The world's seed and name as the server has them (World has the same pair of accessors); main's
    // generation thread compares them with the world's and sets the world up again when they differ.
    const std::string &getName() const { return m_name; }
    int getSeed() const { return m_seed; }
    // Moves client packets accumulated since the previous call to the supplied list under the world's region lock.
    void takeBuffs(std::list<BuffPacket> &buffs);
    void takeBullets(std::list<BulletPacket> &bullets);
    void takeHits(std::list<HitPacket> &hits);

    World *m_world;
    int m_seed;
    std::string m_name;
    SOCKET m_socket;
    std::vector<Connection *> m_connections;
    std::list<HitPacket> m_hits;
    std::list<BuffPacket> m_buffs;
    std::list<BulletPacket> m_projectiles;
    bool m_running;
    HANDLE m_acceptThread;
};

int receiveAll(SOCKET socket, char *buffer, int length, int flags);
}
