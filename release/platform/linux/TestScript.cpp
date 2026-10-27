// A script for automated test runs (CUBE_TEST_SCRIPT=<file>): one command per line, one command per frame, sent as SDL
// events to the window so that they take the same path as real input. Coordinates are window coordinates.
//   wait <frames>              do nothing for that many frames
//   move <x> <y>               move the cursor
//   click <x> <y> [right]      move there, press a mouse button one frame later and release it the frame after
//   key <name>                 press and, one frame later, release a key (SDL key names: "Return", "A", "Escape")
//   down <name> / up <name>    press or release a key
//   text <text>                type text
//   screenshot <file>          save the next frame as a BMP file
//   quit                       end the program
// Lines starting with # are comments.
#include "TestScript.h"

#include <fstream>
#include <iostream>
#include <sstream>

TestScript::TestScript(const char *fileName)
{
    std::ifstream file(fileName);
    std::string line;
    while (std::getline(file, line)) {
        if (!line.empty() && line[0] != '#') {
            m_lines.push_back(line);
        }
    }
    std::cout << "test script " << fileName << ": " << m_lines.size() << " commands" << std::endl;
}

void TestScript::moveTo(SDL_Window *window, float x, float y)
{
    SDL_Event event;
    SDL_zero(event);
    event.type = SDL_EVENT_MOUSE_MOTION;
    event.motion.timestamp = SDL_GetTicksNS();
    event.motion.windowID = SDL_GetWindowID(window);
    event.motion.x = x;
    event.motion.y = y;
    event.motion.xrel = x - m_x;
    event.motion.yrel = y - m_y;
    SDL_PushEvent(&event);
    m_x = x;
    m_y = y;
}

void TestScript::button(SDL_Window *window, Uint8 button, bool down)
{
    SDL_Event event;
    SDL_zero(event);
    event.type = down ? SDL_EVENT_MOUSE_BUTTON_DOWN : SDL_EVENT_MOUSE_BUTTON_UP;
    event.button.timestamp = SDL_GetTicksNS();
    event.button.windowID = SDL_GetWindowID(window);
    event.button.button = button;
    event.button.down = down;
    event.button.clicks = 1;
    event.button.x = m_x;
    event.button.y = m_y;
    SDL_PushEvent(&event);
}

bool TestScript::key(SDL_Window *window, const std::string &name, bool down)
{
    SDL_Scancode scancode = SDL_GetScancodeFromName(name.c_str());
    if (scancode == SDL_SCANCODE_UNKNOWN) {
        std::cout << "test script: unknown key " << name << std::endl;
        return false;
    }
    SDL_Event event;
    SDL_zero(event);
    event.type = down ? SDL_EVENT_KEY_DOWN : SDL_EVENT_KEY_UP;
    event.key.timestamp = SDL_GetTicksNS();
    event.key.windowID = SDL_GetWindowID(window);
    event.key.scancode = scancode;
    event.key.key = SDL_GetKeyFromScancode(scancode, SDL_KMOD_NONE, false);
    event.key.down = down;
    SDL_PushEvent(&event);
    return true;
}

bool TestScript::step(SDL_Window *window, std::string &screenshotRequest)
{
    if (m_wait > 0) {
        m_wait--;
        return true;
    }
    if (!m_pending.empty()) {
        std::string pending = m_pending.front();
        m_pending.erase(m_pending.begin());
        return run(window, pending, screenshotRequest);
    }
    if (m_next >= m_lines.size()) {
        return true;
    }
    return run(window, m_lines[m_next++], screenshotRequest);
}

bool TestScript::run(SDL_Window *window, const std::string &line, std::string &screenshotRequest)
{
    std::istringstream stream(line);
    std::string command;
    stream >> command;
    if (command == "wait") {
        stream >> m_wait;
    } else if (command == "move") {
        float x, y;
        stream >> x >> y;
        moveTo(window, x, y);
    } else if (command == "click") {
        float x, y;
        std::string which;
        stream >> x >> y >> which;
        // The game takes the pointer's position once per frame, after the events.
        moveTo(window, x, y);
        which = which == "right" ? "right" : "left";
        m_pending = {"press " + which, "release " + which};
    } else if (command == "press" || command == "release") {
        std::string which;
        stream >> which;
        button(window, which == "right" ? SDL_BUTTON_RIGHT : SDL_BUTTON_LEFT, command == "press");
    } else if (command == "key") {
        std::string name;
        std::getline(stream >> std::ws, name);
        if (key(window, name, true)) {
            m_pending = {"up " + name};
        }
    } else if (command == "down" || command == "up") {
        std::string name;
        std::getline(stream >> std::ws, name);
        key(window, name, command == "down");
    } else if (command == "text") {
        std::getline(stream >> std::ws, m_text);
        SDL_Event event;
        SDL_zero(event);
        event.type = SDL_EVENT_TEXT_INPUT;
        event.text.timestamp = SDL_GetTicksNS();
        event.text.windowID = SDL_GetWindowID(window);
        event.text.text = m_text.c_str();
        SDL_PushEvent(&event);
    } else if (command == "screenshot") {
        stream >> screenshotRequest;
    } else if (command == "quit") {
        return false;
    } else {
        std::cout << "test script: unknown command " << line << std::endl;
    }
    return true;
}
