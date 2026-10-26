// Options holds the settings stored in options.cfg as name/value lines. The controls and options panels edit copies
// before applying them to Game.
#pragma once

namespace cube {

struct Options {
    Options();
    // Reads options.cfg when present, using lines of "name value" with option member names as keys.
    void load();
    // Writes "options.cfg".
    void save();

    int fullscreen;          // Initially 1.
    int resolutionX;         // Initially 0.
    int resolutionY;         // Initially 0.
    int refreshRate;         // Initially 0.
    int antiAliasing;        // Initially 0.
    int renderDistance;      // Initially 50.
    int cpuPerformance;      // Initially 1.
    int soundVolume;         // Initially 100.
    int musicVolume;         // Initially 100.
    int cameraSpeed;         // Initially 50.
    int cameraSmoothness;    // Initially 80.
    int invertY;             // Initially 0.
    int language;            // Initially 0.
    int minTimeStep;         // Initially 0.
    int smallGUI;            // Initially 0.
    int vsync;               // Initially 1.
    int showRarity;          // Initially 1.
    float guiScale;          // Initially 1.0.
    float mapScale;          // Initially 1.0.
    int musicLoop;           // Initially 0.
    int antiAliasingSamples; // Initially 8.
};

}
