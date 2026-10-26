// Music streams Ogg Vorbis audio through an XAudio2 source voice using three decoded PCM buffers, with optional
// looping.
#pragma once
#include "cube/MusicIds.h"
#include <string>
#include <xaudio2.h>
#include <vorbis/vorbisfile.h>

namespace cube {

class Music {
public:
    explicit Music(IXAudio2 *xaudio2)
        : m_xaudio2(xaudio2),
          m_voice(nullptr),
          m_playing(false),
          m_loop(false),
          m_endOfStream(false),
          m_stopPending(false),
          m_bufferIndex(0),
          m_volume(1.0f)
    {}
    virtual ~Music()
    {
        if (m_voice)
            m_voice->DestroyVoice();
    }

    // Stops and destroys the voice of a previous file, opens fileName, decodes the first buffer, creates a voice for
    // the stream's format, submits the buffer and starts it. False when the file or the voice cannot be opened.
    bool playFile(const std::string &fileName, bool loop);
    // Stops the voice and closes the stream; the voice is kept.
    void stop()
    {
        if (m_voice) {
            m_voice->Stop(0);
            ov_clear(&m_vorbisFile);
            m_playing = false;
            m_endOfStream = false;
            m_stopPending = false;
        }
    }
    // Stores the volume and applies it to the voice.
    void setVolume(float volume)
    {
        m_volume = volume;
        if (m_voice)
            m_voice->SetVolume(volume);
    }
    bool isPlaying() const { return m_playing; }
    // While fewer than two buffers are queued: decodes and submits the next one, seeking back to the start when
    // looping.
    void update();

protected:
    unsigned char m_buffers[MusicBufferCount][MusicBufferSize]; // decoded 16-bit PCM, submitted in turn
    IXAudio2 *m_xaudio2;                                        // the engine's XAudio2 (constructor)
    IXAudio2SourceVoice *m_voice;                               // created by playFile, destroyed by the destructor
    bool m_playing;
    bool m_loop; // playFile's loop argument; update seeks back to 0 at the end
    // Set by update when the stream has ended and m_loop is false (the last buffer is submitted with
    // XAUDIO2_END_OF_STREAM).
    bool m_endOfStream;
    // Set by update one refill after m_endOfStream; the next refill then stops the voice.
    bool m_stopPending;
    unsigned int m_bufferIndex;  // the next buffer to fill, modulo 3
    float m_volume;              // 1.0f; setVolume
    OggVorbis_File m_vorbisFile; // Decoder state storage.
};

}
