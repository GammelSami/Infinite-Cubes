#pragma once

namespace cube {

// A kill event queued by the world and consumed by onKill. The unused word is not read there.
struct KillPacket {
    long long killerId;
    long long targetId;
    int targetRace;
    int xp; // the xp the kill gives
};

}
