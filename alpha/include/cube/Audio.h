#pragma once
#include <string>

namespace cube {
// Audio defines music streaming, sound loading and playback.
class Audio {
public:
    // Releases finished sound voices.
    virtual void cleanUp() = 0;
    // Starts one or two music streams; an empty second filename disables that stream.
    virtual bool playMusic(const std::string &fileName, const std::string &secondFileName, bool loop) = 0;
    // Refills music stream buffers.
    virtual void update() = 0;
    // Stops both music streams.
    virtual void stopMusic() = 0;
    virtual bool isMusicPlaying() = 0;
    // Sets the volume of each music stream.
    virtual void setMusicVolume(float volume, float secondVolume) = 0;
    // Plays a cached sound; XAudio2Engine ignores pan and flag.
    virtual void play(int sound, float volume, float pan, float frequencyRatio, bool flag) = 0;
    // Loads a sound once and returns its identifier, or zero on failure.
    virtual int soundId(const std::string &name) = 0;
};
}
