// Audio is the game's sound interface. XAudio2Engine implements sound loading and playback, voice cleanup and two
// simultaneous music streams.
#pragma once
#include <string>

namespace cube {

class Audio {
public:
    virtual ~Audio() {}

    // Releases finished sound voices.
    virtual void cleanUp() {}
    // Starts the music file and, when secondFileName is not empty, a second stream alongside it.
    virtual bool playMusic(const std::string &fileName, const std::string &secondFileName, bool loop) { return true; }
    // Refills streaming music buffers.
    virtual void update() {}
    // Stops both music streams.
    virtual void stopMusic() {}
    // The first music stream's playing flag.
    virtual bool isMusicPlaying() { return false; }
    // Sets each music stream's volume.
    virtual void setMusicVolume(float volume, float secondVolume) {}
    // Plays a loaded sound with the given volume, pan and playback rate. XAudio2Engine ignores pan and flag.
    virtual void play(int sound, float volume, float pan, float frequencyRatio, bool flag) = 0;
    // Loads a sound by name once and returns its id, 0 on failure.
    virtual int soundId(const std::string &name) = 0;
};

}
