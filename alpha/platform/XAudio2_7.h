// XAudio2 2.7, the version of the DirectX SDK (June 2010) the game was written against. The engine is the
// COM server registered by the DirectX End-User Runtime (XAudio2_7.dll), so the game runs on Windows XP and
// later wherever that runtime is installed, and links no XAudio2 library. Current Windows SDKs only declare
// the later, incompatible interface (XAudio2 2.8+, Windows 8+), so this header declares the 2.7 one: every
// interface lists all of its methods in vtable order, as the 2.7 runtime expects. The structures are the
// SDK's; their natural x86 layout equals the SDK's packed one for every structure the game passes.
#pragma once
#include <windows.h>
#include <objbase.h>
#include <mmreg.h>

// Engine flags and limits used with this interface.
#define XAUDIO2_COMMIT_NOW 0
#define XAUDIO2_COMMIT_ALL 0
#define XAUDIO2_NO_LOOP_REGION 0
#define XAUDIO2_LOOP_INFINITE 255
#define XAUDIO2_DEFAULT_CHANNELS 0
#define XAUDIO2_DEFAULT_SAMPLERATE 0
#define XAUDIO2_DEFAULT_FREQ_RATIO 2.0f
#define XAUDIO2_DEBUG_ENGINE 0x0001
#define XAUDIO2_VOICE_NOPITCH 0x0002
#define XAUDIO2_VOICE_NOSRC 0x0004
#define XAUDIO2_VOICE_USEFILTER 0x0008
#define XAUDIO2_VOICE_MUSIC 0x0010
#define XAUDIO2_PLAY_TAILS 0x0020
#define XAUDIO2_END_OF_STREAM 0x0040
#define XAUDIO2_SEND_USEFILTER 0x0080

typedef enum XAUDIO2_WINDOWS_PROCESSOR_SPECIFIER {
    XAUDIO2_ANY_PROCESSOR = 0xffffffff,
    XAUDIO2_DEFAULT_PROCESSOR = XAUDIO2_ANY_PROCESSOR
} XAUDIO2_PROCESSOR;

typedef enum XAUDIO2_DEVICE_ROLE {
    NotDefaultDevice = 0x0,
    DefaultConsoleDevice = 0x1,
    DefaultMultimediaDevice = 0x2,
    DefaultCommunicationsDevice = 0x4,
    DefaultGameDevice = 0x8,
    GlobalDefaultDevice = 0xf,
    InvalidDeviceRole = ~GlobalDefaultDevice
} XAUDIO2_DEVICE_ROLE;

typedef enum XAUDIO2_FILTER_TYPE {
    LowPassFilter,
    BandPassFilter,
    HighPassFilter,
    NotchFilter
} XAUDIO2_FILTER_TYPE;

struct IXAudio2Voice;

typedef struct XAUDIO2_DEVICE_DETAILS {
    WCHAR DeviceID[256];
    WCHAR DisplayName[256];
    XAUDIO2_DEVICE_ROLE Role;
    WAVEFORMATEXTENSIBLE OutputFormat;
} XAUDIO2_DEVICE_DETAILS;

typedef struct XAUDIO2_VOICE_DETAILS {
    UINT32 CreationFlags;
    UINT32 InputChannels;
    UINT32 InputSampleRate;
} XAUDIO2_VOICE_DETAILS;

typedef struct XAUDIO2_SEND_DESCRIPTOR {
    UINT32 Flags;
    IXAudio2Voice *pOutputVoice;
} XAUDIO2_SEND_DESCRIPTOR;

typedef struct XAUDIO2_VOICE_SENDS {
    UINT32 SendCount;
    XAUDIO2_SEND_DESCRIPTOR *pSends;
} XAUDIO2_VOICE_SENDS;

typedef struct XAUDIO2_EFFECT_DESCRIPTOR {
    IUnknown *pEffect;
    BOOL InitialState;
    UINT32 OutputChannels;
} XAUDIO2_EFFECT_DESCRIPTOR;

typedef struct XAUDIO2_EFFECT_CHAIN {
    UINT32 EffectCount;
    XAUDIO2_EFFECT_DESCRIPTOR *pEffectDescriptors;
} XAUDIO2_EFFECT_CHAIN;

typedef struct XAUDIO2_FILTER_PARAMETERS {
    XAUDIO2_FILTER_TYPE Type;
    float Frequency;
    float OneOverQ;
} XAUDIO2_FILTER_PARAMETERS;

typedef struct XAUDIO2_BUFFER {
    UINT32 Flags;
    UINT32 AudioBytes;
    const BYTE *pAudioData;
    UINT32 PlayBegin;
    UINT32 PlayLength;
    UINT32 LoopBegin;
    UINT32 LoopLength;
    UINT32 LoopCount;
    void *pContext;
} XAUDIO2_BUFFER;

typedef struct XAUDIO2_BUFFER_WMA {
    const UINT32 *pDecodedPacketCumulativeBytes;
    UINT32 PacketCount;
} XAUDIO2_BUFFER_WMA;

typedef struct XAUDIO2_VOICE_STATE {
    void *pCurrentBufferContext;
    UINT32 BuffersQueued;
    UINT64 SamplesPlayed;
} XAUDIO2_VOICE_STATE;

typedef struct XAUDIO2_PERFORMANCE_DATA {
    UINT64 AudioCyclesSinceLastQuery;
    UINT64 TotalCyclesSinceLastQuery;
    UINT32 MinimumCyclesPerQuantum;
    UINT32 MaximumCyclesPerQuantum;
    UINT32 MemoryUsageInBytes;
    UINT32 CurrentLatencyInSamples;
    UINT32 GlitchesSinceEngineStarted;
    UINT32 ActiveSourceVoiceCount;
    UINT32 TotalSourceVoiceCount;
    UINT32 ActiveSubmixVoiceCount;
    UINT32 ActiveResamplerCount;
    UINT32 ActiveMatrixMixCount;
    UINT32 ActiveXmaSourceVoices;
    UINT32 ActiveXmaStreams;
} XAUDIO2_PERFORMANCE_DATA;

typedef struct XAUDIO2_DEBUG_CONFIGURATION {
    UINT32 TraceMask;
    UINT32 BreakMask;
    BOOL LogThreadID;
    BOOL LogFileline;
    BOOL LogFunctionName;
    BOOL LogTiming;
} XAUDIO2_DEBUG_CONFIGURATION;

struct IXAudio2EngineCallback {
    virtual void STDMETHODCALLTYPE OnProcessingPassStart() = 0;
    virtual void STDMETHODCALLTYPE OnProcessingPassEnd() = 0;
    virtual void STDMETHODCALLTYPE OnCriticalError(HRESULT error) = 0;
};

struct IXAudio2VoiceCallback {
    virtual void STDMETHODCALLTYPE OnVoiceProcessingPassStart(UINT32 bytesRequired) = 0;
    virtual void STDMETHODCALLTYPE OnVoiceProcessingPassEnd() = 0;
    virtual void STDMETHODCALLTYPE OnStreamEnd() = 0;
    virtual void STDMETHODCALLTYPE OnBufferStart(void *bufferContext) = 0;
    virtual void STDMETHODCALLTYPE OnBufferEnd(void *bufferContext) = 0;
    virtual void STDMETHODCALLTYPE OnLoopEnd(void *bufferContext) = 0;
    virtual void STDMETHODCALLTYPE OnVoiceError(void *bufferContext, HRESULT error) = 0;
};

struct IXAudio2Voice {
    virtual void STDMETHODCALLTYPE GetVoiceDetails(XAUDIO2_VOICE_DETAILS *voiceDetails) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetOutputVoices(const XAUDIO2_VOICE_SENDS *sendList) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetEffectChain(const XAUDIO2_EFFECT_CHAIN *effectChain) = 0;
    virtual HRESULT STDMETHODCALLTYPE EnableEffect(UINT32 effectIndex, UINT32 operationSet = XAUDIO2_COMMIT_NOW) = 0;
    virtual HRESULT STDMETHODCALLTYPE DisableEffect(UINT32 effectIndex, UINT32 operationSet = XAUDIO2_COMMIT_NOW) = 0;
    virtual void STDMETHODCALLTYPE GetEffectState(UINT32 effectIndex, BOOL *enabled) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetEffectParameters(UINT32 effectIndex, const void *parameters,
                                                          UINT32 parametersByteSize,
                                                          UINT32 operationSet = XAUDIO2_COMMIT_NOW) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetEffectParameters(UINT32 effectIndex, void *parameters,
                                                          UINT32 parametersByteSize) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetFilterParameters(const XAUDIO2_FILTER_PARAMETERS *parameters,
                                                          UINT32 operationSet = XAUDIO2_COMMIT_NOW) = 0;
    virtual void STDMETHODCALLTYPE GetFilterParameters(XAUDIO2_FILTER_PARAMETERS *parameters) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetOutputFilterParameters(IXAudio2Voice *destinationVoice,
                                                                const XAUDIO2_FILTER_PARAMETERS *parameters,
                                                                UINT32 operationSet = XAUDIO2_COMMIT_NOW) = 0;
    virtual void STDMETHODCALLTYPE GetOutputFilterParameters(IXAudio2Voice *destinationVoice,
                                                             XAUDIO2_FILTER_PARAMETERS *parameters) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetVolume(float volume, UINT32 operationSet = XAUDIO2_COMMIT_NOW) = 0;
    virtual void STDMETHODCALLTYPE GetVolume(float *volume) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetChannelVolumes(UINT32 channels, const float *volumes,
                                                        UINT32 operationSet = XAUDIO2_COMMIT_NOW) = 0;
    virtual void STDMETHODCALLTYPE GetChannelVolumes(UINT32 channels, float *volumes) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetOutputMatrix(IXAudio2Voice *destinationVoice, UINT32 sourceChannels,
                                                      UINT32 destinationChannels, const float *levelMatrix,
                                                      UINT32 operationSet = XAUDIO2_COMMIT_NOW) = 0;
    virtual void STDMETHODCALLTYPE GetOutputMatrix(IXAudio2Voice *destinationVoice, UINT32 sourceChannels,
                                                   UINT32 destinationChannels, float *levelMatrix) = 0;
    virtual void STDMETHODCALLTYPE DestroyVoice() = 0;
};

struct IXAudio2SourceVoice : IXAudio2Voice {
    virtual HRESULT STDMETHODCALLTYPE Start(UINT32 flags = 0, UINT32 operationSet = XAUDIO2_COMMIT_NOW) = 0;
    virtual HRESULT STDMETHODCALLTYPE Stop(UINT32 flags = 0, UINT32 operationSet = XAUDIO2_COMMIT_NOW) = 0;
    virtual HRESULT STDMETHODCALLTYPE SubmitSourceBuffer(const XAUDIO2_BUFFER *buffer,
                                                         const XAUDIO2_BUFFER_WMA *bufferWma = NULL) = 0;
    virtual HRESULT STDMETHODCALLTYPE FlushSourceBuffers() = 0;
    virtual HRESULT STDMETHODCALLTYPE Discontinuity() = 0;
    virtual HRESULT STDMETHODCALLTYPE ExitLoop(UINT32 operationSet = XAUDIO2_COMMIT_NOW) = 0;
    virtual void STDMETHODCALLTYPE GetState(XAUDIO2_VOICE_STATE *voiceState) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetFrequencyRatio(float ratio, UINT32 operationSet = XAUDIO2_COMMIT_NOW) = 0;
    virtual void STDMETHODCALLTYPE GetFrequencyRatio(float *ratio) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetSourceSampleRate(UINT32 newSourceSampleRate) = 0;
};

struct IXAudio2SubmixVoice : IXAudio2Voice {};

struct IXAudio2MasteringVoice : IXAudio2Voice {};

struct IXAudio2 : IUnknown {
    virtual HRESULT STDMETHODCALLTYPE GetDeviceCount(UINT32 *count) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetDeviceDetails(UINT32 index, XAUDIO2_DEVICE_DETAILS *deviceDetails) = 0;
    virtual HRESULT STDMETHODCALLTYPE Initialize(UINT32 flags = 0,
                                                 XAUDIO2_PROCESSOR processor = XAUDIO2_DEFAULT_PROCESSOR) = 0;
    virtual HRESULT STDMETHODCALLTYPE RegisterForCallbacks(IXAudio2EngineCallback *callback) = 0;
    virtual void STDMETHODCALLTYPE UnregisterForCallbacks(IXAudio2EngineCallback *callback) = 0;
    virtual HRESULT STDMETHODCALLTYPE CreateSourceVoice(IXAudio2SourceVoice **sourceVoice,
                                                        const WAVEFORMATEX *sourceFormat, UINT32 flags = 0,
                                                        float maxFrequencyRatio = XAUDIO2_DEFAULT_FREQ_RATIO,
                                                        IXAudio2VoiceCallback *callback = NULL,
                                                        const XAUDIO2_VOICE_SENDS *sendList = NULL,
                                                        const XAUDIO2_EFFECT_CHAIN *effectChain = NULL) = 0;
    virtual HRESULT STDMETHODCALLTYPE CreateSubmixVoice(IXAudio2SubmixVoice **submixVoice, UINT32 inputChannels,
                                                        UINT32 inputSampleRate, UINT32 flags = 0,
                                                        UINT32 processingStage = 0,
                                                        const XAUDIO2_VOICE_SENDS *sendList = NULL,
                                                        const XAUDIO2_EFFECT_CHAIN *effectChain = NULL) = 0;
    virtual HRESULT STDMETHODCALLTYPE CreateMasteringVoice(IXAudio2MasteringVoice **masteringVoice,
                                                           UINT32 inputChannels = XAUDIO2_DEFAULT_CHANNELS,
                                                           UINT32 inputSampleRate = XAUDIO2_DEFAULT_SAMPLERATE,
                                                           UINT32 flags = 0, UINT32 deviceIndex = 0,
                                                           const XAUDIO2_EFFECT_CHAIN *effectChain = NULL) = 0;
    virtual HRESULT STDMETHODCALLTYPE StartEngine() = 0;
    virtual void STDMETHODCALLTYPE StopEngine() = 0;
    virtual HRESULT STDMETHODCALLTYPE CommitChanges(UINT32 operationSet) = 0;
    virtual void STDMETHODCALLTYPE GetPerformanceData(XAUDIO2_PERFORMANCE_DATA *performanceData) = 0;
    virtual void STDMETHODCALLTYPE SetDebugConfiguration(const XAUDIO2_DEBUG_CONFIGURATION *debugConfiguration,
                                                         void *reserved = NULL) = 0;
};

// The 2.7 engine's COM classes and interface.
static const CLSID CLSID_XAudio2_7 = {0x5a508685, 0xa254, 0x4fba, {0x9b, 0x82, 0x9a, 0x24, 0xb0, 0x03, 0x06, 0xaf}};
static const CLSID CLSID_XAudio2_7_Debug = {0xdb05ea35, 0x0329, 0x4d4b, {0xa5, 0x3a, 0x6d, 0xea, 0xd0, 0x3d, 0x38, 0x52}};
static const IID IID_IXAudio2_7 = {0x8bcf1f58, 0x9fe7, 0x4583, {0x8a, 0xc6, 0xe2, 0xad, 0xc4, 0x65, 0xc8, 0xbb}};

// Creates and initializes the engine as the SDK's XAudio2Create does. COM must be initialized on the thread.
inline HRESULT XAudio2Create(IXAudio2 **xaudio2, UINT32 flags = 0, XAUDIO2_PROCESSOR processor = XAUDIO2_DEFAULT_PROCESSOR)
{
    IXAudio2 *engine;
    HRESULT result = CoCreateInstance((flags & XAUDIO2_DEBUG_ENGINE) ? CLSID_XAudio2_7_Debug : CLSID_XAudio2_7, NULL,
                                      CLSCTX_INPROC_SERVER, IID_IXAudio2_7, reinterpret_cast<void **>(&engine));
    if (SUCCEEDED(result)) {
        result = engine->Initialize(flags, processor);
        if (SUCCEEDED(result))
            *xaudio2 = engine;
        else
            engine->Release();
    }
    return result;
}
