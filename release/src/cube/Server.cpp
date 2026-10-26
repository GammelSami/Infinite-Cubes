// Server distributes world updates to connected socket players.
#include "cube/Server.h"

#include "cube/Creature.h"
#include "cube/World.h"
#include "cube/Zone.h"

namespace cube {

void Server::addServerUpdates(const ServerUpdates &serverUpdates)
{
    for (int i = 0; i < (int)m_connections.size(); i++) {
        if (m_connections[i]) {
            m_connections[i]->m_serverUpdates.m_cubeChangeList.insert(
                m_connections[i]->m_serverUpdates.m_cubeChangeList.end(), serverUpdates.m_cubeChangeList.begin(),
                serverUpdates.m_cubeChangeList.end());
            m_connections[i]->m_serverUpdates.m_hitPacketList.insert(
                m_connections[i]->m_serverUpdates.m_hitPacketList.end(), serverUpdates.m_hitPacketList.begin(),
                serverUpdates.m_hitPacketList.end());

            // Sends particles and sounds only within 200 blocks of the player. Assumes the player's creature exists
            // and dereferences it without a null check.
            Creature *creature = m_world->creatureByID(m_connections[i]->m_id);
            for (std::list<ServerUpdates::ParticlePacket>::const_iterator particle =
                     serverUpdates.m_particlePacketList.begin();
                 particle != serverUpdates.m_particlePacketList.end(); ++particle) {
                plasma::Vector<3, Fixed> offset = particle->position - creature->m_entityData.position;
                plasma::Vector<3, float> blocks;
                for (int axis = 0; axis < 3; axis++)
                    blocks[axis] = (float)offset[axis];
                if (blocks.squaredLength() < 40000.0f)
                    m_connections[i]->m_serverUpdates.m_particlePacketList.push_back(*particle);
            }
            for (std::list<ServerUpdates::SoundPacket>::const_iterator sound = serverUpdates.m_soundPacketList.begin();
                 sound != serverUpdates.m_soundPacketList.end(); ++sound) {
                plasma::Vector<3, Fixed> offset = sound->position - creature->m_entityData.position;
                plasma::Vector<3, float> blocks;
                for (int axis = 0; axis < 3; axis++)
                    blocks[axis] = (float)offset[axis];
                if (blocks.squaredLength() < 40000.0f)
                    m_connections[i]->m_serverUpdates.m_soundPacketList.push_back(*sound);
            }

            m_connections[i]->m_serverUpdates.m_bulletPacketList.insert(
                m_connections[i]->m_serverUpdates.m_bulletPacketList.end(), serverUpdates.m_bulletPacketList.begin(),
                serverUpdates.m_bulletPacketList.end());
            m_connections[i]->m_serverUpdates.m_itemListPacketList.insert(
                m_connections[i]->m_serverUpdates.m_itemListPacketList.end(),
                serverUpdates.m_itemListPacketList.begin(), serverUpdates.m_itemListPacketList.end());
            m_connections[i]->m_serverUpdates.m_staticEntityPacketList.insert(
                m_connections[i]->m_serverUpdates.m_staticEntityPacketList.end(),
                serverUpdates.m_staticEntityPacketList.begin(), serverUpdates.m_staticEntityPacketList.end());
            m_connections[i]->m_serverUpdates.m_deliveredZoneList.insert(
                m_connections[i]->m_serverUpdates.m_deliveredZoneList.end(), serverUpdates.m_deliveredZoneList.begin(),
                serverUpdates.m_deliveredZoneList.end());
            m_connections[i]->m_serverUpdates.m_itemPickupPacketList.insert(
                m_connections[i]->m_serverUpdates.m_itemPickupPacketList.end(),
                serverUpdates.m_itemPickupPacketList.begin(), serverUpdates.m_itemPickupPacketList.end());
            m_connections[i]->m_serverUpdates.m_killPacketList.insert(
                m_connections[i]->m_serverUpdates.m_killPacketList.end(), serverUpdates.m_killPacketList.begin(),
                serverUpdates.m_killPacketList.end());
            m_connections[i]->m_serverUpdates.m_passivePacketList.insert(
                m_connections[i]->m_serverUpdates.m_passivePacketList.end(), serverUpdates.m_passivePacketList.begin(),
                serverUpdates.m_passivePacketList.end());
            m_connections[i]->m_serverUpdates.m_aggroPacketList.insert(
                m_connections[i]->m_serverUpdates.m_aggroPacketList.end(), serverUpdates.m_aggroPacketList.begin(),
                serverUpdates.m_aggroPacketList.end());
            m_connections[i]->m_serverUpdates.m_buffPacketList.insert(
                m_connections[i]->m_serverUpdates.m_buffPacketList.end(), serverUpdates.m_buffPacketList.begin(),
                serverUpdates.m_buffPacketList.end());
            m_connections[i]->m_serverUpdates.m_worldEventList.insert(
                m_connections[i]->m_serverUpdates.m_worldEventList.end(), serverUpdates.m_worldEventList.begin(),
                serverUpdates.m_worldEventList.end());
            m_connections[i]->m_serverUpdates.m_talkingCreatureList.insert(
                m_connections[i]->m_serverUpdates.m_talkingCreatureList.end(),
                serverUpdates.m_talkingCreatureList.begin(), serverUpdates.m_talkingCreatureList.end());

            // The zones the player asked for are taken under the connection's lock; their contents are sent where
            // loaded.
            EnterCriticalSection(&m_connections[i]->m_lock);
            std::list<plasma::Vector<2, int>> zones = m_connections[i]->m_zones;
            std::list<plasma::Vector<2, int>> unused = m_connections[i]->m_clearedZones;
            m_connections[i]->m_zones.clear();
            m_connections[i]->m_clearedZones.clear();
            LeaveCriticalSection(&m_connections[i]->m_lock);
            for (auto &position : zones) {
                m_world->enterZoneMeshes();
                Zone *zone = m_world->zone(position[0], position[1]);
                if (zone) {
                    m_connections[i]->m_serverUpdates.m_cubeChangeList.insert(
                        m_connections[i]->m_serverUpdates.m_cubeChangeList.end(), zone->m_cubeChanges.begin(),
                        zone->m_cubeChanges.end());
                    m_connections[i]->m_serverUpdates.m_itemListPacketList.push_back(
                        ServerUpdates::ItemListPacket(*zone));
                    m_connections[i]->m_serverUpdates.m_staticEntityPacketList.push_back(ServerUpdates::Packet6(*zone));
                    m_connections[i]->m_serverUpdates.m_deliveredZoneList.push_back(position);
                }
                m_world->leaveZoneMeshes();
            }
        }
    }
}

}
