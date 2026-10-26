// Server keeps socket-player connections and distributes world events and requested zone contents to each player.
#pragma once
#include <windows.h>
#include <list>
#include <vector>
#include "plasma/Vector.h"
#include "cube/ServerUpdates.h"

namespace cube {

class World;

class Server {
public:
    // One connected player, with the same connection state as Host's Connection.
    struct Connection {
        virtual ~Connection();

        unsigned char m_unused08[8]; // no access in the program
        // The id of the player's creature.
        long long m_id;
        // The events still to be sent to this player.
        ServerUpdates m_serverUpdates;
        // The zones the player asked for; addServerUpdates sends each loaded one's contents. Guarded by m_lock.
        std::list<plasma::Vector<2, int>> m_zones;
        // A second list of the same type: addServerUpdates copies and clears it with m_zones and does not use the
        // copy; nothing adds to it.
        std::list<plasma::Vector<2, int>> m_clearedZones;
        unsigned char m_unused138[0x10]; // no access in the program
        // Guards the two zone lists against the connection's packet-receive thread.
        CRITICAL_SECTION m_lock;
    };

    virtual ~Server();

    // Appends the world's events of a tick to what each player still has to receive (particles and sounds only within
    // 200 blocks of the player's creature), and the contents of the zones the player asked for.
    void addServerUpdates(const ServerUpdates &serverUpdates);

    World *m_world;                 // the world the players are in
    unsigned char m_unused10[0x30]; // No access in the program.
    // A slot is 0 where a player has left.
    std::vector<Connection *> m_connections;
};

}
