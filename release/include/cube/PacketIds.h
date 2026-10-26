// ID catalog of the multiplayer packets: the int that leads every packet sent through Steam P2P (Host::send,
// Client::send). One numbering serves both directions. The wire field stays an int.
#pragma once

namespace cube {

enum PacketId {
    PacketInvitation = 0, // host -> client: an invitation, followed by the host's protocol version (Host::invite)
    PacketJoin =
        1, // client -> host: accept, ask for a creature id (Client::connect); the host answers PacketCreatureId
    PacketLeave =
        2, // client -> host: the client leaves (Host::receive disconnects it); Client::receive also takes it from an inviter (forgets the invitation)
    PacketEntityData = 3,    // either way: a creature's changed entity data, compressed (Host::update, Client::update)
    PacketCreatureId = 4,    // host -> client: the id of the client's own creature
    PacketFullUpdateEnd = 6, // host -> client: end of a full update; creatures not received since are removed
    PacketWorldUpdate = 7,   // host -> client: the tick's ServerUpdates lists, compressed
    PacketTime = 8,          // host -> client: the day and the time of day
    PacketInteraction = 9,   // client -> host: an Interaction of the client's creature
    PacketHit = 10,          // client -> host: a ServerUpdates::HitPacket
    PacketSound = 11,        // client -> host: a ServerUpdates::SoundPacket
    PacketBuff = 12,         // client -> host: a ServerUpdates::BuffPacket
    PacketProjectile = 13,   // client -> host: a ServerUpdates::BulletPacket
    PacketChat = 14,         // either way: a chat line (host -> client with the speaker's creature id)
    PacketZoneRequest = 15,  // client -> host: a zone (x, y) the client needs (Game::m_requestedZones)
    PacketVersionMismatch =
        24, // client -> host: the invitation's protocol version differs; the host drops the invitation
};

}
