// Entity updates begin with a 64-bit field mask, followed by the bytes of each selected EntityData field. The helpers read packets, receive socket data and write changed fields.
#include <windows.h>
#include <string.h>
#include <list>
#include <utility>
#include "cube/Network.h"

namespace cube {

typedef std::list<std::pair<const void *, int>> EntityChunks;

// Reads a member when its bit is set in mask and moves on to the next bit. Returns the member's
// size when the bit was set (also when the data ran out).
template<class T> int readField(BytesIO &data, const unsigned long long &mask, int &bit, T &value)
{
    int size = 0;
    if (mask & (1ULL << bit)) {
        data.read(&value, sizeof(T));
        size = sizeof(T);
    }
    bit++;
    return size;
}

// Reads an entity update from a decompressed packet and returns its size.
int readEntityData(BytesIO &data, Creature::EntityData &entityData)
{
    unsigned long long mask = 0;
    data.read(&mask, sizeof(mask));
    int bit = 0;
    readField(data, mask, bit, entityData.position);
    readField(data, mask, bit, entityData.rotation);
    readField(data, mask, bit, entityData.velocity);
    readField(data, mask, bit, entityData.acceleration);
    readField(data, mask, bit, entityData.retreat);
    readField(data, mask, bit, entityData.headRotation);
    readField(data, mask, bit, entityData.flags);
    readField(data, mask, bit, entityData.hostilityType);
    readField(data, mask, bit, entityData.race);
    readField(data, mask, bit, entityData.currentAbility);
    readField(data, mask, bit, entityData.timeSinceAbility);
    readField(data, mask, bit, entityData.hitCombo);
    readField(data, mask, bit, entityData.timeSinceHit);
    readField(data, mask, bit, entityData.appearance);
    readField(data, mask, bit, entityData.binaryToggles);
    readField(data, mask, bit, entityData.rollTime);
    readField(data, mask, bit, entityData.stunTime);
    readField(data, mask, bit, entityData.actionLockTime);
    readField(data, mask, bit, entityData.slowedTime);
    readField(data, mask, bit, entityData.sprintTime);
    readField(data, mask, bit, entityData.revivalPenalty);
    readField(data, mask, bit, entityData.classType);
    readField(data, mask, bit, entityData.specialization);
    readField(data, mask, bit, entityData.charge);
    readField(data, mask, bit, entityData.finalHandsRotation);
    readField(data, mask, bit, entityData.initialHandsRotation);
    readField(data, mask, bit, entityData.attackRotation);
    readField(data, mask, bit, entityData.hp);
    readField(data, mask, bit, entityData.mp);
    readField(data, mask, bit, entityData.blockPower);
    readField(data, mask, bit, entityData.multipliers);
    readField(data, mask, bit, entityData.interactionState);
    readField(data, mask, bit, entityData.buildingModel);
    readField(data, mask, bit, entityData.level);
    readField(data, mask, bit, entityData.xp);
    readField(data, mask, bit, entityData.ownerId);
    readField(data, mask, bit, entityData.focusId);
    readField(data, mask, bit, entityData.rarity);
    readField(data, mask, bit, entityData.unused19c);
    readField(data, mask, bit, entityData.spawnSlot);
    readField(data, mask, bit, entityData.origin);
    // The serialized order places the later declared member first.
    readField(data, mask, bit, entityData.questZone);
    readField(data, mask, bit, entityData.speechExpression);
    readField(data, mask, bit, entityData.actionItem);
    readField(data, mask, bit, entityData.equipment);
    readField(data, mask, bit, entityData.name);
    readField(data, mask, bit, entityData.skillLevels);
    readField(data, mask, bit, entityData.manaCubeCount);
    return data.size();
}

// Queues the field's current bytes and sets its bit when it changed (or for a full update); the
// bit index moves on either way.
template<typename T>
bool writeField(EntityChunks &chunks, unsigned long long &mask, int &bit, const T &previous, const T &current,
                bool full)
{
    bool written = false;
    if (full || previous != current) {
        chunks.push_back(std::make_pair((const void *)&current, (int)sizeof(T)));
        mask |= 1ULL << bit;
        written = true;
    }
    bit++;
    return written;
}

// The same for an array field, compared element by element.
template<typename T, int N>
bool writeField(EntityChunks &chunks, unsigned long long &mask, int &bit, const T (&previous)[N], const T (&current)[N],
                bool full)
{
    bool written = false;
    bool changed = full;
    if (!changed) {
        for (int i = 0; i < N; i++) {
            if (previous[i] != current[i]) {
                changed = true;
                break;
            }
        }
    }
    if (changed) {
        chunks.push_back(std::make_pair((const void *)current, (int)sizeof(current)));
        mask |= 1ULL << bit;
        written = true;
    }
    bit++;
    return written;
}

// The same for the name, compared as a string.
bool writeField(EntityChunks &chunks, unsigned long long &mask, int &bit, const char (&previous)[16],
                const char (&current)[16], bool full)
{
    bool written = false;
    if (full || strcmp(previous, current) != 0) {
        chunks.push_back(std::make_pair((const void *)current, (int)sizeof(current)));
        mask |= 1ULL << bit;
        written = true;
    }
    bit++;
    return written;
}

// Receives length bytes; SOCKET_ERROR or 0 when the connection fails or closes first.
static int receiveAll(SOCKET socket, char *buffer, int length)
{
    int originalLength = length;
    while (length > 0) {
        int received = recv(socket, buffer, length, 0);
        if (received <= 0)
            return received;
        length -= received;
        buffer += received;
    }
    return originalLength;
}

// Receives the field when its bit is set; the bit index moves on unless the connection failed
// (SOCKET_ERROR).
template<typename T> int receiveField(SOCKET socket, const unsigned long long &mask, int &bit, T &value)
{
    int received = 0;
    if (mask & (1ULL << bit)) {
        received = receiveAll(socket, (char *)&value, sizeof(T));
        if (received == SOCKET_ERROR)
            return SOCKET_ERROR;
    }
    bit++;
    return received;
}

// The update's size, or 0 when the connection fails.
int receiveEntityData(SOCKET socket, Creature::EntityData &entityData)
{
    unsigned long long mask = 0;
    // A connection closed before the mask counts 0 bytes, not the mask's 8.
    int size = receiveAll(socket, (char *)&mask, sizeof(mask));
    if (size == SOCKET_ERROR)
        return 0;
    int bit = 0;
    int received;
    if ((received = receiveField(socket, mask, bit, entityData.position)) == SOCKET_ERROR)
        return 0;
    size += received;
    if ((received = receiveField(socket, mask, bit, entityData.rotation)) == SOCKET_ERROR)
        return 0;
    size += received;
    if ((received = receiveField(socket, mask, bit, entityData.velocity)) == SOCKET_ERROR)
        return 0;
    size += received;
    if ((received = receiveField(socket, mask, bit, entityData.acceleration)) == SOCKET_ERROR)
        return 0;
    size += received;
    if ((received = receiveField(socket, mask, bit, entityData.retreat)) == SOCKET_ERROR)
        return 0;
    size += received;
    if ((received = receiveField(socket, mask, bit, entityData.headRotation)) == SOCKET_ERROR)
        return 0;
    size += received;
    if ((received = receiveField(socket, mask, bit, entityData.flags)) == SOCKET_ERROR)
        return 0;
    size += received;
    if ((received = receiveField(socket, mask, bit, entityData.hostilityType)) == SOCKET_ERROR)
        return 0;
    size += received;
    if ((received = receiveField(socket, mask, bit, entityData.race)) == SOCKET_ERROR)
        return 0;
    size += received;
    if ((received = receiveField(socket, mask, bit, entityData.currentAbility)) == SOCKET_ERROR)
        return 0;
    size += received;
    if ((received = receiveField(socket, mask, bit, entityData.timeSinceAbility)) == SOCKET_ERROR)
        return 0;
    size += received;
    if ((received = receiveField(socket, mask, bit, entityData.hitCombo)) == SOCKET_ERROR)
        return 0;
    size += received;
    if ((received = receiveField(socket, mask, bit, entityData.timeSinceHit)) == SOCKET_ERROR)
        return 0;
    size += received;
    if ((received = receiveField(socket, mask, bit, entityData.appearance)) == SOCKET_ERROR)
        return 0;
    size += received;
    if ((received = receiveField(socket, mask, bit, entityData.binaryToggles)) == SOCKET_ERROR)
        return 0;
    size += received;
    if ((received = receiveField(socket, mask, bit, entityData.rollTime)) == SOCKET_ERROR)
        return 0;
    size += received;
    if ((received = receiveField(socket, mask, bit, entityData.stunTime)) == SOCKET_ERROR)
        return 0;
    size += received;
    if ((received = receiveField(socket, mask, bit, entityData.actionLockTime)) == SOCKET_ERROR)
        return 0;
    size += received;
    if ((received = receiveField(socket, mask, bit, entityData.slowedTime)) == SOCKET_ERROR)
        return 0;
    size += received;
    if ((received = receiveField(socket, mask, bit, entityData.sprintTime)) == SOCKET_ERROR)
        return 0;
    size += received;
    if ((received = receiveField(socket, mask, bit, entityData.revivalPenalty)) == SOCKET_ERROR)
        return 0;
    size += received;
    if ((received = receiveField(socket, mask, bit, entityData.classType)) == SOCKET_ERROR)
        return 0;
    size += received;
    if ((received = receiveField(socket, mask, bit, entityData.specialization)) == SOCKET_ERROR)
        return 0;
    size += received;
    if ((received = receiveField(socket, mask, bit, entityData.charge)) == SOCKET_ERROR)
        return 0;
    size += received;
    if ((received = receiveField(socket, mask, bit, entityData.finalHandsRotation)) == SOCKET_ERROR)
        return 0;
    size += received;
    if ((received = receiveField(socket, mask, bit, entityData.initialHandsRotation)) == SOCKET_ERROR)
        return 0;
    size += received;
    if ((received = receiveField(socket, mask, bit, entityData.attackRotation)) == SOCKET_ERROR)
        return 0;
    size += received;
    if ((received = receiveField(socket, mask, bit, entityData.hp)) == SOCKET_ERROR)
        return 0;
    size += received;
    if ((received = receiveField(socket, mask, bit, entityData.mp)) == SOCKET_ERROR)
        return 0;
    size += received;
    if ((received = receiveField(socket, mask, bit, entityData.blockPower)) == SOCKET_ERROR)
        return 0;
    size += received;
    if ((received = receiveField(socket, mask, bit, entityData.multipliers)) == SOCKET_ERROR)
        return 0;
    size += received;
    if ((received = receiveField(socket, mask, bit, entityData.interactionState)) == SOCKET_ERROR)
        return 0;
    size += received;
    if ((received = receiveField(socket, mask, bit, entityData.buildingModel)) == SOCKET_ERROR)
        return 0;
    size += received;
    if ((received = receiveField(socket, mask, bit, entityData.level)) == SOCKET_ERROR)
        return 0;
    size += received;
    if ((received = receiveField(socket, mask, bit, entityData.xp)) == SOCKET_ERROR)
        return 0;
    size += received;
    if ((received = receiveField(socket, mask, bit, entityData.ownerId)) == SOCKET_ERROR)
        return 0;
    size += received;
    if ((received = receiveField(socket, mask, bit, entityData.focusId)) == SOCKET_ERROR)
        return 0;
    size += received;
    if ((received = receiveField(socket, mask, bit, entityData.rarity)) == SOCKET_ERROR)
        return 0;
    size += received;
    if ((received = receiveField(socket, mask, bit, entityData.unused19c)) == SOCKET_ERROR)
        return 0;
    size += received;
    if ((received = receiveField(socket, mask, bit, entityData.spawnSlot)) == SOCKET_ERROR)
        return 0;
    size += received;
    if ((received = receiveField(socket, mask, bit, entityData.origin)) == SOCKET_ERROR)
        return 0;
    size += received;
    if ((received = receiveField(socket, mask, bit, entityData.questZone)) == SOCKET_ERROR)
        return 0;
    size += received;
    if ((received = receiveField(socket, mask, bit, entityData.speechExpression)) == SOCKET_ERROR)
        return 0;
    size += received;
    if ((received = receiveField(socket, mask, bit, entityData.actionItem)) == SOCKET_ERROR)
        return 0;
    size += received;
    if ((received = receiveField(socket, mask, bit, entityData.equipment)) == SOCKET_ERROR)
        return 0;
    size += received;
    if ((received = receiveField(socket, mask, bit, entityData.name)) == SOCKET_ERROR)
        return 0;
    size += received;
    if ((received = receiveField(socket, mask, bit, entityData.skillLevels)) == SOCKET_ERROR)
        return 0;
    size += received;
    if ((received = receiveField(socket, mask, bit, entityData.manaCubeCount)) == SOCKET_ERROR)
        return 0;
    return size + received;
}

// Appends an update to data: the mask, then the bytes of every field that differs between previous
// and current (or of all fields, when full). Returns data's new size.
int writeEntityData(BytesIO &data, const Creature::EntityData &previous, const Creature::EntityData &current, bool full)
{
    EntityChunks chunks;
    unsigned long long mask = 0;
    int bit = 0;
    writeField(chunks, mask, bit, previous.position, current.position, full);
    writeField(chunks, mask, bit, previous.rotation, current.rotation, full);
    writeField(chunks, mask, bit, previous.velocity, current.velocity, full);
    writeField(chunks, mask, bit, previous.acceleration, current.acceleration, full);
    writeField(chunks, mask, bit, previous.retreat, current.retreat, full);
    writeField(chunks, mask, bit, previous.headRotation, current.headRotation, full);
    writeField(chunks, mask, bit, previous.flags, current.flags, full);
    writeField(chunks, mask, bit, previous.hostilityType, current.hostilityType, full);
    writeField(chunks, mask, bit, previous.race, current.race, full);
    writeField(chunks, mask, bit, previous.currentAbility, current.currentAbility, full);
    writeField(chunks, mask, bit, previous.timeSinceAbility, current.timeSinceAbility, full);
    writeField(chunks, mask, bit, previous.hitCombo, current.hitCombo, full);
    writeField(chunks, mask, bit, previous.timeSinceHit, current.timeSinceHit, full);
    writeField(chunks, mask, bit, previous.appearance, current.appearance, full);
    // The toggles go as their 16-bit word.
    writeField(chunks, mask, bit, reinterpret_cast<const unsigned short &>(previous.binaryToggles),
               reinterpret_cast<const unsigned short &>(current.binaryToggles), full);
    writeField(chunks, mask, bit, previous.rollTime, current.rollTime, full);
    writeField(chunks, mask, bit, previous.stunTime, current.stunTime, full);
    writeField(chunks, mask, bit, previous.actionLockTime, current.actionLockTime, full);
    writeField(chunks, mask, bit, previous.slowedTime, current.slowedTime, full);
    writeField(chunks, mask, bit, previous.sprintTime, current.sprintTime, full);
    writeField(chunks, mask, bit, previous.revivalPenalty, current.revivalPenalty, full);
    writeField(chunks, mask, bit, previous.classType, current.classType, full);
    writeField(chunks, mask, bit, previous.specialization, current.specialization, full);
    writeField(chunks, mask, bit, previous.charge, current.charge, full);
    writeField(chunks, mask, bit, previous.finalHandsRotation, current.finalHandsRotation, full);
    writeField(chunks, mask, bit, previous.initialHandsRotation, current.initialHandsRotation, full);
    writeField(chunks, mask, bit, previous.attackRotation, current.attackRotation, full);
    writeField(chunks, mask, bit, previous.hp, current.hp, full);
    writeField(chunks, mask, bit, previous.mp, current.mp, full);
    writeField(chunks, mask, bit, previous.blockPower, current.blockPower, full);
    writeField(chunks, mask, bit, previous.multipliers, current.multipliers, full);
    writeField(chunks, mask, bit, previous.interactionState, current.interactionState, full);
    writeField(chunks, mask, bit, previous.buildingModel, current.buildingModel, full);
    writeField(chunks, mask, bit, previous.level, current.level, full);
    writeField(chunks, mask, bit, previous.xp, current.xp, full);
    writeField(chunks, mask, bit, previous.ownerId, current.ownerId, full);
    writeField(chunks, mask, bit, previous.focusId, current.focusId, full);
    writeField(chunks, mask, bit, previous.rarity, current.rarity, full);
    writeField(chunks, mask, bit, previous.unused19c, current.unused19c, full);
    writeField(chunks, mask, bit, previous.spawnSlot, current.spawnSlot, full);
    writeField(chunks, mask, bit, previous.origin, current.origin, full);
    writeField(chunks, mask, bit, previous.questZone, current.questZone, full);
    writeField(chunks, mask, bit, previous.speechExpression, current.speechExpression, full);
    writeField(chunks, mask, bit, previous.actionItem, current.actionItem, full);
    writeField(chunks, mask, bit, previous.equipment, current.equipment, full);
    writeField(chunks, mask, bit, previous.name, current.name, full);
    writeField(chunks, mask, bit, previous.skillLevels, current.skillLevels, full);
    writeField(chunks, mask, bit, previous.manaCubeCount, current.manaCubeCount, full);
    data.write(&mask, sizeof(mask));
    for (EntityChunks::iterator it = chunks.begin(); it != chunks.end(); ++it)
        data.write(it->first, it->second);
    return data.size();
}

}
