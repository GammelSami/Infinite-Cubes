// Automated input for test runs (CUBE_TEST_SCRIPT); see TestScript.cpp for the commands.
#pragma once
#include <SDL3/SDL.h>
#include <string>
#include <vector>

class TestScript {
public:
    explicit TestScript(const char *fileName);
    // Runs the next command, pushing its events for this frame. Sets screenshotRequest to a file name when the frame
    // is to be saved. False at "quit".
    bool step(SDL_Window *window, std::string &screenshotRequest);

private:
    bool run(SDL_Window *window, const std::string &line, std::string &screenshotRequest);
    void moveTo(SDL_Window *window, float x, float y);
    void button(SDL_Window *window, Uint8 button, bool down);
    bool key(SDL_Window *window, const std::string &name, bool down);

    std::vector<std::string> m_lines;
    size_t m_next = 0;
    int m_wait = 0;
    std::vector<std::string> m_pending; // the steps that follow a click or key, one per frame
    std::string m_text;    // the text of the last "text" event, kept alive until SDL delivers it
    float m_x = 0.0f;
    float m_y = 0.0f;
};
