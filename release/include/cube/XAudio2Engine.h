// XAudio2Engine implements Audio with cached RIFF/WAVE sounds and two streamed Music tracks. It loads sounds from
// m_database when set, otherwise from files, and creates one source voice per playback. cleanUp removes drained voices
// from m_voices.
#pragma once
#include <list>
#include <map>
#include <string>
#include <xaudio2.h>
#include "cube/Audio.h"
#include "cube/Music.h"

namespace cube {

class Database;

class XAudio2Engine : public Audio {
public:
    // A loaded sound: the RIFF "fmt " chunk and an XAUDIO2_BUFFER over the "data" chunk.
    struct Sound {
        Sound() {}
        virtual ~Sound() {}

        WAVEFORMATEXTENSIBLE format; // Sound format passed to CreateSourceVoice.
        XAUDIO2_BUFFER buffer;       // Sound buffer passed to SubmitSourceBuffer.
    };

    XAudio2Engine();
    virtual ~XAudio2Engine();

    // Destroys and forgets the voices with no buffer queued (GetState).
    virtual void cleanUp();
    // m_music plays fileName; m_secondMusic plays secondFileName or is stopped when it is empty. False without
    // m_music.
    virtual bool playMusic(const std::string &fileName, const std::string &secondFileName, bool loop);
    virtual void update();
    virtual void stopMusic();
    virtual bool isMusicPlaying();
    virtual void setMusicVolume(float volume, float secondVolume);
    // Creates the mastering voice if needed; for volume > 0 creates a source voice for the sound's format, sets the
    // volume, submits the buffer, starts it, sets the frequency ratio and adds it to m_voices.
    virtual void play(int sound, float volume, float pan, float frequencyRatio, bool flag);
    // Returns the id of name, loading it on first use: from m_database's blob of that key when m_database is set, else
    // from the file (CreateFileW). Parses "RIFF" / "WAVE" / "fmt " / "data" and assigns the next id (++m_lastSoundId).
    virtual int soundId(const std::string &name);

    // CoInitializeEx, XAudio2Create (processor 1), CreateMasteringVoice (category 6) and the two Musics; false on
    // failure.
    bool initialize();

protected:
    IXAudio2 *m_xaudio2;                       // XAudio2Create (initialize); released by the destructor
    IXAudio2MasteringVoice *m_masteringVoice;  // CreateMasteringVoice (initialize, play)
    std::list<IXAudio2SourceVoice *> m_voices; // playing sound voices (play adds, cleanUp removes)
    std::map<int, Sound *> m_sounds;           // Loaded sounds keyed by ID.
    std::map<std::string, int> m_soundIds;     // Sound IDs keyed by name.
    Music *m_music;                            // (initialize; deleted by the destructor)
    Music *m_secondMusic;                      // (initialize; deleted by the destructor)
    // Optional sound-blob database used by soundId; initially null and never set in this implementation. Deleted by
    // the destructor.
    Database *m_database;
    int m_lastSoundId; // 0; soundId pre-increments it for each new sound
};

}
