#pragma once
#include <list>
#include <map>
#include <string>
#include "XAudio2_7.h"
#include "cube/Audio.h"
#include "cube/Music.h"

namespace cube {

class Database;
class BytesIO;

HRESULT FindChunk(BytesIO &stream, DWORD fourcc, DWORD &size, DWORD &position);
HRESULT FindChunk(HANDLE file, DWORD fourcc, DWORD &size, DWORD &position);
HRESULT ReadChunkData(BytesIO &stream, void *buffer, DWORD size, DWORD offset);
HRESULT ReadChunkData(HANDLE file, void *buffer, DWORD size, DWORD offset);

// XAudio2 implementation of Audio. Sounds load from data2.db or individual RIFF/WAVE files, are cached by identifier, and play on separate source voices. It also streams two music tracks.
class XAudio2Engine : public Audio {
public:
    // A loaded sound owns a RIFF format chunk and an XAudio2 buffer referencing its data chunk. The destructor does not free buffer.pAudioData.
    struct Sound {
        Sound() {}
        virtual ~Sound() {}

        WAVEFORMATEXTENSIBLE format; // CreateSourceVoice's format
        XAUDIO2_BUFFER buffer;       // SubmitSourceBuffer's buffer
    };

    XAudio2Engine();

    virtual void cleanUp();
    // M_music plays fileName; m_secondMusic plays secondFileName or is stopped when it is empty.
    virtual bool playMusic(const std::string &fileName, const std::string &secondFileName, bool loop);
    virtual void update();
    virtual void stopMusic();
    virtual bool isMusicPlaying();
    virtual void setMusicVolume(float volume, float secondVolume);
    // For volume > 0: creates a source voice for the sound's format, sets the volume, submits the
    // buffer, starts it, sets the frequency ratio and adds the voice to m_voices.
    virtual void play(int sound, float volume, float pan, float frequencyRatio, bool flag);
    // Returns the id of name, loading it on first use: from m_database's blob of that key when
    // m_database is set, else from the file. Parses "RIFF" / "WAVE" / "fmt " / "data" and assigns the
    // next id.
    virtual int soundId(const std::string &name);
    virtual ~XAudio2Engine();

    // CoInitializeEx, XAudio2Create, CreateMasteringVoice and the two Musics; false on failure.
    bool initialize();

protected:
    IXAudio2 *m_xaudio2;                       // XAudio2Create (initialize); released by the destructor
    IXAudio2MasteringVoice *m_masteringVoice;  // CreateMasteringVoice (initialize)
    std::list<IXAudio2SourceVoice *> m_voices; // playing sound voices (play adds, cleanUp removes)
    std::map<int, Sound *> m_sounds;           // by id
    std::map<std::string, int> m_soundIds;     // by name
    Music *m_music;                            // (initialize; deleted by the destructor)
    Music *m_secondMusic;                      // (initialize; deleted by the destructor)
    Database *m_database;                      // "data2.db", opened by the constructor
    int m_lastSoundId;                         // soundId pre-increments it for each new sound
};

}
