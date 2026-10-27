#pragma once
#include <string>

namespace cube {
class Game;
class World;
}

// Chat commands for testing and balancing: /help lists them. The world's time is the host's, so /time goes to the host
// as a chat line; the others change the local player at once. Nothing of /skills and /cooldowns is saved.
namespace mod {

struct Cheats {
    bool allSkills = false;   // /skills all: every skill at its most points (effectivePoints)
    bool noCooldowns = false; // /cooldowns off: the player's skills are ready at once
};
Cheats &cheats();

// Game::handleKeyDown, for a line typed into the chat: true when it was a command of the mod, answered in the chat.
bool chatCommand(cube::Game *game, const std::wstring &line);
// Host::receive, for a chat line of a player: true when it was a command for the host, with the reply for that player.
// Only the host's own player (local) may change the world.
bool hostCommand(cube::World &world, const std::wstring &line, bool local, std::wstring &reply);

}
