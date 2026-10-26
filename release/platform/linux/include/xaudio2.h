// The XAudio2 2.8 interfaces the game uses, implemented over FAudio (the library Wine's and Proton's XAudio2 use).
// FAudio's structures share XAudio2's layout, so the buffer, state and format types are FAudio's own.
#pragma once
#include <windows.h>
#include <FAudio.h>

#define XAUDIO2_DEFAULT_PROCESSOR FAUDIO_DEFAULT_PROCESSOR
#define XAUDIO2_END_OF_STREAM FAUDIO_END_OF_STREAM
#define XAUDIO2_COMMIT_NOW FAUDIO_COMMIT_NOW
#define XAUDIO2_DEFAULT_FREQ_RATIO FAUDIO_DEFAULT_FREQ_RATIO
#define WAVE_FORMAT_PCM FAUDIO_FORMAT_PCM
#define WAVE_FORMAT_IEEE_FLOAT FAUDIO_FORMAT_IEEE_FLOAT
#define WAVE_FORMAT_EXTENSIBLE FAUDIO_FORMAT_EXTENSIBLE

typedef FAudioBuffer XAUDIO2_BUFFER;
typedef FAudioVoiceState XAUDIO2_VOICE_STATE;
typedef FAudioWaveFormatEx WAVEFORMATEX;
typedef FAudioWaveFormatExtensible WAVEFORMATEXTENSIBLE;

#define COINIT_MULTITHREADED 0
inline HRESULT CoInitializeEx(void *, DWORD) { return S_OK; }

#ifndef FAILED
#define FAILED(hr) (((HRESULT)(hr)) < 0)
#define SUCCEEDED(hr) (((HRESULT)(hr)) >= 0)
#endif

struct IXAudio2Voice {
    FAudioVoice *m_voice;

    explicit IXAudio2Voice(FAudioVoice *voice)
        : m_voice(voice)
    {}
    virtual ~IXAudio2Voice() {}
    HRESULT SetVolume(float volume, UINT32 operationSet = XAUDIO2_COMMIT_NOW)
    {
        return (HRESULT)FAudioVoice_SetVolume(m_voice, volume, operationSet);
    }
    void DestroyVoice()
    {
        FAudioVoice_DestroyVoice(m_voice);
        delete this;
    }
};

struct IXAudio2MasteringVoice : IXAudio2Voice {
    using IXAudio2Voice::IXAudio2Voice;
};

struct IXAudio2SourceVoice : IXAudio2Voice {
    using IXAudio2Voice::IXAudio2Voice;
    FAudioSourceVoice *source() { return (FAudioSourceVoice *)m_voice; }
    HRESULT Start(UINT32 flags = 0, UINT32 operationSet = XAUDIO2_COMMIT_NOW)
    {
        return (HRESULT)FAudioSourceVoice_Start(source(), flags, operationSet);
    }
    HRESULT Stop(UINT32 flags = 0, UINT32 operationSet = XAUDIO2_COMMIT_NOW)
    {
        return (HRESULT)FAudioSourceVoice_Stop(source(), flags, operationSet);
    }
    HRESULT SubmitSourceBuffer(const XAUDIO2_BUFFER *buffer, const void *bufferWma = nullptr)
    {
        return (HRESULT)FAudioSourceVoice_SubmitSourceBuffer(source(), buffer, (const FAudioBufferWMA *)bufferWma);
    }
    void GetState(XAUDIO2_VOICE_STATE *state, UINT32 flags = 0) { FAudioSourceVoice_GetState(source(), state, flags); }
    HRESULT SetFrequencyRatio(float ratio, UINT32 operationSet = XAUDIO2_COMMIT_NOW)
    {
        return (HRESULT)FAudioSourceVoice_SetFrequencyRatio(source(), ratio, operationSet);
    }
};

struct IXAudio2 {
    FAudio *m_audio;

    HRESULT CreateSourceVoice(IXAudio2SourceVoice **voice, const WAVEFORMATEX *format, UINT32 flags = 0,
                              float maxFrequencyRatio = XAUDIO2_DEFAULT_FREQ_RATIO, void *callback = nullptr,
                              const void *sendList = nullptr, const void *effectChain = nullptr)
    {
        FAudioSourceVoice *source = nullptr;
        HRESULT result = (HRESULT)FAudio_CreateSourceVoice(m_audio, &source, format, flags, maxFrequencyRatio,
                                                           (FAudioVoiceCallback *)callback,
                                                           (const FAudioVoiceSends *)sendList,
                                                           (const FAudioEffectChain *)effectChain);
        *voice = SUCCEEDED(result) ? new IXAudio2SourceVoice((FAudioVoice *)source) : nullptr;
        return result;
    }
    HRESULT CreateMasteringVoice(IXAudio2MasteringVoice **voice, UINT32 inputChannels = FAUDIO_DEFAULT_CHANNELS,
                                 UINT32 inputSampleRate = FAUDIO_DEFAULT_SAMPLERATE, UINT32 flags = 0,
                                 LPCWSTR deviceId = nullptr, const void *effectChain = nullptr,
                                 int category = FAudioStreamCategory_GameEffects)
    {
        FAudioMasteringVoice *master = nullptr;
        HRESULT result = (HRESULT)FAudio_CreateMasteringVoice8(m_audio, &master, inputChannels, inputSampleRate,
                                                               flags, nullptr, (const FAudioEffectChain *)effectChain,
                                                               (FAudioStreamCategory)category);
        *voice = SUCCEEDED(result) ? new IXAudio2MasteringVoice((FAudioVoice *)master) : nullptr;
        return result;
    }
    ULONG Release()
    {
        ULONG count = FAudio_Release(m_audio);
        delete this;
        return count;
    }
};

inline HRESULT XAudio2Create(IXAudio2 **xaudio2, UINT32 flags = 0, UINT32 processor = XAUDIO2_DEFAULT_PROCESSOR)
{
    FAudio *audio = nullptr;
    HRESULT result = (HRESULT)FAudioCreate(&audio, flags, processor);
    *xaudio2 = nullptr;
    if (SUCCEEDED(result)) {
        *xaudio2 = new IXAudio2;
        (*xaudio2)->m_audio = audio;
    }
    return result;
}
