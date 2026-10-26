#pragma once
#include <string>
#include <xaudio2.h>
#include <vorbis/vorbisfile.h>

namespace cube {

// Streams one Ogg Vorbis track through an XAudio2 source voice, decoding into a ring of three PCM buffers. XAudio2Engine owns two tracks.
class Music {
public:
    enum {
        BufferCount = 3,
        BufferSize = 0xa0000
    };

    explicit Music(IXAudio2 *xaudio2);
    virtual ~Music()
    {
        if (m_voice)
            m_voice->DestroyVoice();
    }

    // Stops the voice of a previous file, opens fileName, decodes the first buffer, creates a voice
    // for the stream's format, submits the buffer and starts it. False when the file or the voice
    // cannot be opened.
    bool playFile(const std::string &fileName, bool loop);
    // Stops the voice and closes the stream; the voice is kept.
    void stop()
    {
        if (m_voice) {
            m_voice->Stop();
            ov_clear(&m_vorbisFile);
            m_playing = false;
            m_endOfStream = false;
            m_stopPending = false;
        }
    }
    void setVolume(float volume)
    {
        m_volume = volume;
        if (m_voice)
            m_voice->SetVolume(volume);
    }
    bool isPlaying() const { return m_playing; }
    // While fewer than two buffers are queued: decodes and submits the next one, seeking back to the
    // start when looping.
    void update();

protected:
    unsigned char m_buffers[BufferCount][BufferSize]; // decoded 16-bit PCM, submitted in turn
    IXAudio2 *m_xaudio2;                              // the engine's XAudio2 (constructor)
    IXAudio2SourceVoice *m_voice;                     // created by playFile, destroyed by the destructor
    bool m_playing;                                   // the voice had buffers queued after the start; cleared by stop
    bool m_loop;                                      // The loop argument passed to playFile.
    // Set when the stream ends without looping; the last buffer uses XAUDIO2_END_OF_STREAM.
    bool m_endOfStream;
    // Set one refill after end of stream; the following refill stops the voice.
    bool m_stopPending;
    unsigned int m_bufferIndex;  // the next buffer to fill, modulo BufferCount
    float m_volume;              // 1.0f; setVolume
    OggVorbis_File m_vorbisFile; // the decoder state
};

}
