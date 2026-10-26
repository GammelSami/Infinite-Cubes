// XAudio2Engine plays sounds and music. The RIFF chunk helpers read either files or database blobs.
#include <windows.h>
#include <xaudio2.h>
#include "cube/XAudio2Engine.h"
#include "cube/BytesIO.h"
#include "cube/Database.h"

namespace cube {

enum ChunkId {
    fourccRIFF = 'FFIR',
    fourccDATA = 'atad',
    fourccFMT = ' tmf',
    fourccWAVE = 'EVAW'
};

Music::Music(IXAudio2 *xaudio2)
    : m_xaudio2(xaudio2),
      m_voice(0),
      m_playing(false),
      m_loop(false),
      m_endOfStream(false),
      m_stopPending(false),
      m_bufferIndex(0),
      m_volume(1.0f)
{}

XAudio2Engine::XAudio2Engine()
    : m_xaudio2(0),
      m_masteringVoice(0),
      m_music(0),
      m_secondMusic(0),
      m_database(0),
      m_lastSoundId(0)
{
    m_database = new Database;
    m_database->open("data2.db");
}

XAudio2Engine::~XAudio2Engine()
{
    cleanUp();
    for (std::map<int, Sound *>::iterator it = m_sounds.begin(); it != m_sounds.end(); ++it)
        delete it->second;
    delete m_music;
    delete m_secondMusic;
    if (m_masteringVoice)
        m_masteringVoice->DestroyVoice();
    if (m_xaudio2)
        m_xaudio2->Release();
    delete m_database;
}

// Finds the chunk fourcc in the RIFF data of stream: its size and the offset of its data.
HRESULT FindChunk(BytesIO &stream, DWORD fourcc, DWORD &dwChunkSize, DWORD &dwChunkDataPosition)
{
    stream.m_offset = 0;

    DWORD dwChunkType;
    DWORD dwChunkDataSize;
    DWORD dwRIFFDataSize = 0;
    DWORD bytesRead = 0;
    DWORD dwOffset = 0;

    while (!stream.atEnd()) {
        // Both stay uninitialized when the data ends early.
        stream.read(&dwChunkType, sizeof(DWORD));
        stream.read(&dwChunkDataSize, sizeof(DWORD));

        switch (dwChunkType) {
        case fourccRIFF:
            dwRIFFDataSize = dwChunkDataSize;
            dwChunkDataSize = 4;
            stream.skip(sizeof(DWORD));
            break;

        default:
            stream.m_offset += dwChunkDataSize;
        }

        dwOffset += sizeof(DWORD) * 2;

        if (dwChunkType == fourcc) {
            dwChunkSize = dwChunkDataSize;
            dwChunkDataPosition = dwOffset;
            return S_OK;
        }

        dwOffset += dwChunkDataSize;

        if (bytesRead >= dwRIFFDataSize)
            return S_FALSE;
    }

    return S_OK;
}

HRESULT FindChunk(HANDLE hFile, DWORD fourcc, DWORD &dwChunkSize, DWORD &dwChunkDataPosition)
{
    HRESULT hr = S_OK;
    if (INVALID_SET_FILE_POINTER == SetFilePointer(hFile, 0, NULL, FILE_BEGIN))
        return HRESULT_FROM_WIN32(GetLastError());

    DWORD dwChunkType;
    DWORD dwChunkDataSize;
    DWORD dwRIFFDataSize = 0;
    DWORD dwFileType;
    DWORD bytesRead = 0;
    DWORD dwOffset = 0;

    while (hr == S_OK) {
        DWORD dwRead;
        if (0 == ReadFile(hFile, &dwChunkType, sizeof(DWORD), &dwRead, NULL))
            hr = HRESULT_FROM_WIN32(GetLastError());

        if (0 == ReadFile(hFile, &dwChunkDataSize, sizeof(DWORD), &dwRead, NULL))
            hr = HRESULT_FROM_WIN32(GetLastError());

        switch (dwChunkType) {
        case fourccRIFF:
            dwRIFFDataSize = dwChunkDataSize;
            dwChunkDataSize = 4;
            if (0 == ReadFile(hFile, &dwFileType, sizeof(DWORD), &dwRead, NULL))
                hr = HRESULT_FROM_WIN32(GetLastError());
            break;

        default:
            if (INVALID_SET_FILE_POINTER == SetFilePointer(hFile, dwChunkDataSize, NULL, FILE_CURRENT))
                return HRESULT_FROM_WIN32(GetLastError());
        }

        dwOffset += sizeof(DWORD) * 2;

        if (dwChunkType == fourcc) {
            dwChunkSize = dwChunkDataSize;
            dwChunkDataPosition = dwOffset;
            return S_OK;
        }

        dwOffset += dwChunkDataSize;

        if (bytesRead >= dwRIFFDataSize)
            return S_FALSE;
    }

    return S_OK;
}

HRESULT ReadChunkData(BytesIO &stream, void *buffer, DWORD buffersize, DWORD bufferoffset)
{
    if (bufferoffset + buffersize > stream.m_data.size())
        return S_FALSE;
    stream.m_offset = bufferoffset;
    stream.read(buffer, buffersize);
    return S_OK;
}

HRESULT ReadChunkData(HANDLE hFile, void *buffer, DWORD buffersize, DWORD bufferoffset)
{
    HRESULT hr = S_OK;
    if (INVALID_SET_FILE_POINTER == SetFilePointer(hFile, bufferoffset, NULL, FILE_BEGIN))
        return HRESULT_FROM_WIN32(GetLastError());
    DWORD dwRead;
    if (0 == ReadFile(hFile, buffer, buffersize, &dwRead, NULL))
        hr = HRESULT_FROM_WIN32(GetLastError());
    return hr;
}

void XAudio2Engine::cleanUp()
{
    std::list<std::list<IXAudio2SourceVoice *>::iterator> finished;
    for (std::list<IXAudio2SourceVoice *>::iterator it = m_voices.begin(); it != m_voices.end(); ++it) {
        XAUDIO2_VOICE_STATE state;
        (*it)->GetState(&state);
        if (state.BuffersQueued == 0) {
            (*it)->DestroyVoice();
            finished.push_back(it);
        }
    }
    for (std::list<std::list<IXAudio2SourceVoice *>::iterator>::iterator it = finished.begin(); it != finished.end();
         ++it)
        m_voices.erase(*it);
}

bool XAudio2Engine::initialize()
{
    CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (FAILED(XAudio2Create(&m_xaudio2, 0, XAUDIO2_DEFAULT_PROCESSOR)))
        return false;
    if (FAILED(m_xaudio2->CreateMasteringVoice(&m_masteringVoice)))
        return false;
    m_music = new Music(m_xaudio2);
    m_secondMusic = new Music(m_xaudio2);
    return true;
}

bool XAudio2Engine::isMusicPlaying()
{
    return m_music->isPlaying();
}

void XAudio2Engine::play(int sound, float volume, float pan, float frequencyRatio, bool flag)
{
    if (volume <= 0.0f)
        return;
    Sound *cachedSound = m_sounds[sound];
    if (cachedSound) {
        IXAudio2SourceVoice *voice = 0;
        m_xaudio2->CreateSourceVoice(&voice, &cachedSound->format.Format);
        voice->SetVolume(volume);
        voice->SubmitSourceBuffer(&cachedSound->buffer);
        voice->Start(0);
        voice->SetVolume(volume);
        voice->SetFrequencyRatio(frequencyRatio);
        m_voices.push_back(voice);
    }
}

bool Music::playFile(const std::string &fileName, bool loop)
{
    if (m_voice) {
        stop();
        m_voice->DestroyVoice();
    }

    FILE *file = fopen(fileName.c_str(), "rb");
    if (!file)
        return false;

    if (ov_open(file, &m_vorbisFile, NULL, 0) < 0)
        return false;

    vorbis_info *info = ov_info(&m_vorbisFile, -1);

    WAVEFORMATEX format;
    memset(&format, 0, sizeof(format));
    format.cbSize = sizeof(format);
    format.nChannels = info->channels;
    format.wBitsPerSample = 16;
    format.nSamplesPerSec = info->rate;
    format.nAvgBytesPerSec = format.nSamplesPerSec * format.nChannels * 2;
    format.nBlockAlign = 2 * format.nChannels;
    format.wFormatTag = WAVE_FORMAT_PCM;

    DWORD position = 0;
    int section = 0;
    int read = 1;

    memset(m_buffers[m_bufferIndex], 0, BufferSize);

    while (read && position < BufferSize) {
        read = ov_read(&m_vorbisFile, (char *)m_buffers[m_bufferIndex] + position, BufferSize - position, 0, 2, 1,
                       &section);
        position += read;
    }

    if (FAILED(m_xaudio2->CreateSourceVoice(&m_voice, &format)))
        return false;

    XAUDIO2_BUFFER buffer = {0};
    buffer.pAudioData = m_buffers[m_bufferIndex];
    buffer.AudioBytes = BufferSize;

    if (FAILED(m_voice->SubmitSourceBuffer(&buffer)))
        return false;

    m_bufferIndex++;
    m_bufferIndex %= BufferCount;

    m_voice->Start(0);
    m_voice->SetVolume(m_volume);

    XAUDIO2_VOICE_STATE state;
    m_voice->GetState(&state);
    m_playing = state.BuffersQueued > 0;
    m_loop = loop;
    m_endOfStream = false;
    m_stopPending = false;
    return true;
}

bool XAudio2Engine::playMusic(const std::string &fileName, const std::string &secondFileName, bool loop)
{
    if (secondFileName.empty())
        m_secondMusic->stop();
    else
        m_secondMusic->playFile(secondFileName, loop);
    return m_music->playFile(fileName, loop);
}

void XAudio2Engine::setMusicVolume(float volume, float secondVolume)
{
    m_music->setVolume(volume);
    m_secondMusic->setVolume(secondVolume);
}

int XAudio2Engine::soundId(const std::string &name)
{
    std::map<std::string, int>::iterator found = m_soundIds.find(name);
    if (found != m_soundIds.end() && found->second != 0)
        return found->second;

    DWORD dwChunkSize;
    DWORD dwChunkPosition;

    if (m_database) {
        BytesIO stream;
        if (m_database->read(name, stream) && !stream.m_data.empty()) {
            stream.descramble();
            FindChunk(stream, fourccRIFF, dwChunkSize, dwChunkPosition);
            DWORD filetype = 0;
            ReadChunkData(stream, &filetype, sizeof(DWORD), dwChunkPosition);
            if (filetype == fourccWAVE) {
                Sound *sound = new Sound;
                FindChunk(stream, fourccFMT, dwChunkSize, dwChunkPosition);
                memset(&sound->format, 0, sizeof(sound->format));
                ReadChunkData(stream, &sound->format, dwChunkSize, dwChunkPosition);
                FindChunk(stream, fourccDATA, dwChunkSize, dwChunkPosition);
                BYTE *pDataBuffer = new BYTE[dwChunkSize];
                ReadChunkData(stream, pDataBuffer, dwChunkSize, dwChunkPosition);
                memset(&sound->buffer, 0, sizeof(sound->buffer));
                sound->buffer.AudioBytes = dwChunkSize;
                sound->buffer.pAudioData = pDataBuffer;
                sound->buffer.Flags = XAUDIO2_END_OF_STREAM;
                m_lastSoundId++;
                m_soundIds[name] = m_lastSoundId;
                m_sounds[m_lastSoundId] = sound;
                return m_lastSoundId;
            }
        }
        return 0;
    } else {
        std::wstring fileName(name.begin(), name.end());
        HANDLE hFile = CreateFileW(fileName.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
        if (hFile != INVALID_HANDLE_VALUE && SetFilePointer(hFile, 0, NULL, FILE_BEGIN) != INVALID_SET_FILE_POINTER) {
            FindChunk(hFile, fourccRIFF, dwChunkSize, dwChunkPosition);
            DWORD filetype;
            ReadChunkData(hFile, &filetype, sizeof(DWORD), dwChunkPosition);
            if (filetype == fourccWAVE) {
                Sound *sound = new Sound;
                FindChunk(hFile, fourccFMT, dwChunkSize, dwChunkPosition);
                memset(&sound->format, 0, sizeof(sound->format));
                ReadChunkData(hFile, &sound->format, dwChunkSize, dwChunkPosition);
                FindChunk(hFile, fourccDATA, dwChunkSize, dwChunkPosition);
                BYTE *pDataBuffer = new BYTE[dwChunkSize];
                ReadChunkData(hFile, pDataBuffer, dwChunkSize, dwChunkPosition);
                memset(&sound->buffer, 0, sizeof(sound->buffer));
                sound->buffer.AudioBytes = dwChunkSize;
                sound->buffer.pAudioData = pDataBuffer;
                sound->buffer.Flags = XAUDIO2_END_OF_STREAM;
                CloseHandle(hFile);
                m_lastSoundId++;
                m_soundIds[name] = m_lastSoundId;
                m_sounds[m_lastSoundId] = sound;
                return m_lastSoundId;
            }
        }
        return 0;
    }
}

void XAudio2Engine::stopMusic()
{
    m_music->stop();
    m_secondMusic->stop();
}

void Music::update()
{
    if (m_voice == 0)
        return;

    if (!m_playing)
        return;

    XAUDIO2_VOICE_STATE state;
    m_voice->GetState(&state);
    if (state.BuffersQueued < BufferCount - 1) {
        if (m_stopPending && !m_loop)
            m_voice->Stop(0);

        if (m_endOfStream && !m_loop)
            m_stopPending = true;

        memset(m_buffers[m_bufferIndex], 0, BufferSize);

        DWORD position = 0;
        int section = 0;
        int read = 1;

        while (read && position < BufferSize) {
            read = ov_read(&m_vorbisFile, (char *)m_buffers[m_bufferIndex] + position, BufferSize - position, 0, 2, 1,
                           &section);
            position += read;
        }

        if (!read && m_loop) {
            read = 1;
            ov_pcm_seek(&m_vorbisFile, 0);
            while (read && position < BufferSize) {
                read = ov_read(&m_vorbisFile, (char *)m_buffers[m_bufferIndex] + position, BufferSize - position, 0, 2,
                               1, &section);
                position += read;
            }
        } else if (!read && !m_loop) {
            m_endOfStream = true;
        }

        XAUDIO2_BUFFER buffer = {0};
        buffer.pAudioData = m_buffers[m_bufferIndex];
        if (m_endOfStream)
            buffer.Flags = XAUDIO2_END_OF_STREAM;
        buffer.AudioBytes = BufferSize;

        if (m_voice->SubmitSourceBuffer(&buffer) != S_OK)
            return;

        m_bufferIndex++;
        m_bufferIndex %= BufferCount;
    }
}

void XAudio2Engine::update()
{
    m_music->update();
    m_secondMusic->update();
}

}
