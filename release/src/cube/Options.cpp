#include "cube/Noise.h"
#include "cube/Options.h"
#include <fstream>
#include <string>

namespace cube {

Options::Options()
{
    fullscreen = 1;
    resolutionX = 0;
    resolutionY = 0;
    refreshRate = 0;
    antiAliasing = 0;
    renderDistance = 50;
    cpuPerformance = 1;
    soundVolume = 100;
    musicVolume = 100;
    cameraSpeed = 50;
    cameraSmoothness = 80;
    invertY = 0;
    language = 0;
    minTimeStep = 0;
    smallGUI = 0;
    vsync = 1;
    showRarity = 1;
    guiScale = 1.0f;
    mapScale = 1.0f;
    musicLoop = 0;
    antiAliasingSamples = 8;
}

void Options::load()
{
    std::ifstream file("options.cfg");
    while (file.good()) {
        std::string key;
        file >> key;
        if (!file.good()) {
            break;
        }
        if (key == "fullscreen") {
            file >> fullscreen;
        }
        if (key == "resolutionX") {
            file >> resolutionX;
        }
        if (key == "resolutionY") {
            file >> resolutionY;
        }
        if (key == "refreshRate") {
            file >> refreshRate;
        }
        if (key == "antiAliasing") {
            file >> antiAliasing;
        }
        if (key == "renderDistance") {
            file >> renderDistance;
        }
        if (key == "cpuPerformance") {
            file >> cpuPerformance;
        }
        if (key == "soundVolume") {
            file >> soundVolume;
        }
        if (key == "musicVolume") {
            file >> musicVolume;
        }
        if (key == "cameraSpeed") {
            file >> cameraSpeed;
        }
        if (key == "cameraSmoothness") {
            file >> cameraSmoothness;
        }
        if (key == "language") {
            file >> language;
        }
        if (key == "invertY") {
            file >> invertY;
        }
        if (key == "minTimeStep") {
            file >> minTimeStep;
        }
        if (key == "smallGUI") {
            file >> smallGUI;
        }
        if (key == "vsync") {
            file >> vsync;
        }
        if (key == "showRarity") {
            file >> showRarity;
        }
        if (key == "musicLoop") {
            file >> musicLoop;
        }
        if (key == "guiScale") {
            file >> guiScale;
        }
        if (key == "mapScale") {
            file >> mapScale;
        }
        if (key == "antiAliasingSamples") {
            file >> antiAliasingSamples;
        }
    }
}

void Options::save()
{
    std::ofstream file("options.cfg");
    file << "fullscreen " << fullscreen << std::endl;
    file << "resolutionX " << resolutionX << std::endl;
    file << "resolutionY " << resolutionY << std::endl;
    file << "refreshRate " << refreshRate << std::endl;
    file << "antiAliasing " << antiAliasing << std::endl;
    file << "renderDistance " << renderDistance << std::endl;
    file << "cpuPerformance " << cpuPerformance << std::endl;
    file << "soundVolume " << soundVolume << std::endl;
    file << "musicVolume " << musicVolume << std::endl;
    file << "cameraSpeed " << cameraSpeed << std::endl;
    file << "cameraSmoothness " << cameraSmoothness << std::endl;
    file << "language " << language << std::endl;
    file << "invertY " << invertY << std::endl;
    file << "minTimeStep " << minTimeStep << std::endl;
    file << "smallGUI " << smallGUI << std::endl;
    file << "vsync " << vsync << std::endl;
    file << "showRarity " << showRarity << std::endl;
    file << "musicLoop " << musicLoop << std::endl;
    file << "guiScale " << guiScale << std::endl;
    file << "mapScale " << mapScale << std::endl;
    file << "antiAliasingSamples " << antiAliasingSamples << std::endl;
}

}
