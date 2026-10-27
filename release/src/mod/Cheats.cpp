#include "mod/Cheats.h"
#include "mod/ClassData.h"
#include "mod/Progress.h"
#include "cube/Creature.h"
#include "cube/Game.h"
#include "cube/World.h"
#include "cube/WorldDimensions.h"
#include "cube/widgets/ChatWidget.h"
#include <algorithm>
#include <cwctype>
#include <iomanip>
#include <sstream>
#include <vector>

namespace mod {

namespace {

const wchar_t *HelpLines[] = {
    L"/time day|night|morning|evening|HH:MM  set the time of day (host only)",
    L"/skills all|off  every skill at its most points, for now",
    L"/cooldowns off|on  skills without cooldown, for now",
    L"/level N  set the alpha level",
    L"/xp N  gain experience",
    L"/gold N  gain gold",
    L"/heal  full hit points and mana",
};

std::vector<std::wstring> words(const std::wstring &line)
{
    std::wstringstream stream(line);
    std::vector<std::wstring> result;
    std::wstring word;
    while (stream >> word) {
        std::transform(word.begin(), word.end(), word.begin(), [](wchar_t c) { return (wchar_t)std::towlower(c); });
        result.push_back(word);
    }
    return result;
}

void say(cube::Game *game, const std::wstring &text)
{
    game->m_interface.m_inGame.m_chatWidget->printMessage(text + L"\n", plasma::Vector<3, unsigned char>(160, 220, 255));
}

// A whole number; false for anything else.
bool number(const std::wstring &text, int &value)
{
    std::wstringstream stream(text);
    return (stream >> value) && stream.eof();
}

// The on/off argument: "on"/"all"/none switch on, "off" off; false for anything else.
bool toggle(const std::vector<std::wstring> &args, const wchar_t *on, bool current, bool &value)
{
    if (args.size() < 2) {
        value = !current;
        return true;
    }
    if (args[1] == on || args[1] == L"on") {
        value = true;
        return true;
    }
    if (args[1] == L"off") {
        value = false;
        return true;
    }
    return false;
}

// "day", "night", "morning", "evening", "HH" or "HH:MM" as milliseconds after midnight; -1 for anything else.
int timeOfDay(const std::wstring &text)
{
    if (text == L"day" || text == L"noon")
        return 12 * cube::MillisecondsPerHour;
    if (text == L"night" || text == L"midnight")
        return 0;
    if (text == L"morning")
        return 7 * cube::MillisecondsPerHour;
    if (text == L"evening")
        return 19 * cube::MillisecondsPerHour;
    std::wstringstream stream(text);
    int hours = -1;
    int minutes = 0;
    wchar_t colon = L':';
    if (!(stream >> hours))
        return -1;
    if (!stream.eof() && (!(stream >> colon >> minutes) || colon != L':' || !stream.eof()))
        return -1;
    if (hours < 0 || hours > 24 || minutes < 0 || minutes > 59 || (hours == 24 && minutes > 0))
        return -1;
    return (hours % 24) * cube::MillisecondsPerHour + minutes * 60000;
}

}

Cheats &cheats()
{
    static Cheats state;
    return state;
}

bool chatCommand(cube::Game *game, const std::wstring &line)
{
    std::vector<std::wstring> args = words(line);
    if (args.empty())
        return false;
    const std::wstring &command = args[0];
    cube::Creature *player = game->m_world->m_localCreature;
    if (command == L"/help") {
        for (const wchar_t *help : HelpLines)
            say(game, help);
    } else if (command == L"/time") {
        // The host owns the time and sends it to every player; it answers.
        game->m_client.m_chatMessageList.push_back(line);
    } else if (command == L"/skills") {
        if (!toggle(args, L"all", cheats().allSkills, cheats().allSkills)) {
            say(game, L"Usage: /skills all|off");
            return true;
        }
        say(game, cheats().allSkills ? L"All skills unlocked for now." : L"Skills back to the learned points.");
    } else if (command == L"/cooldowns") {
        bool on = !cheats().noCooldowns;
        if (!toggle(args, L"on", on, on)) {
            say(game, L"Usage: /cooldowns off|on");
            return true;
        }
        cheats().noCooldowns = !on;
        say(game, on ? L"Cooldowns on." : L"Cooldowns off for now.");
    } else if (command == L"/level" || command == L"/xp" || command == L"/gold") {
        int value = 0;
        if (args.size() < 2 || !number(args[1], value) || !player || !player->m_playerData) {
            say(game, L"Usage: " + command + L" N");
            return true;
        }
        Progress &progress = player->m_playerData->modProgress;
        if (command == L"/level") {
            progress.level = (std::max)(1, (std::min)(value, 1000));
            progress.xp = 0;
            say(game, L"Level " + std::to_wstring(progress.level) + L", skill points: " +
                          std::to_wstring(skillPoints(player)) + L" (X)");
        } else if (command == L"/xp") {
            addExperience(player, (std::max)(0, value));
            say(game, L"Experience +" + std::to_wstring((std::max)(0, value)) + L".");
        } else {
            player->m_inventory.m_gold = (std::max)(0, player->m_inventory.m_gold + value);
            say(game, L"Gold: " + std::to_wstring(player->m_inventory.m_gold));
        }
    } else if (command == L"/heal") {
        if (!player)
            return true;
        player->m_entityData.hp = player->maxHP();
        player->m_entityData.mp = 1.0f;
        say(game, L"Healed.");
    } else {
        return false;
    }
    return true;
}

bool hostCommand(cube::World &world, const std::wstring &line, bool local, std::wstring &reply)
{
    std::vector<std::wstring> args = words(line);
    if (args.empty() || args[0] != L"/time")
        return false;
    if (!local) {
        reply = L"Only the host can change the time.";
        return true;
    }
    // "/time set 10:00" as well as "/time 10:00".
    size_t argument = args.size() > 1 && args[1] == L"set" ? 2 : 1;
    int time = argument < args.size() ? timeOfDay(args[argument]) : -1;
    if (time < 0) {
        reply = L"Usage: /time day|night|morning|evening|HH:MM";
        return true;
    }
    world.m_state.m_time = (float)time;
    std::wstringstream text;
    text << L"Time set to " << std::setw(2) << std::setfill(L'0') << time / cube::MillisecondsPerHour << L":"
         << std::setw(2) << time % cube::MillisecondsPerHour / 60000 << L".";
    reply = text.str();
    return true;
}

}
