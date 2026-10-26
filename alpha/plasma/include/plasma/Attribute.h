// Attribute stores animated values and their key sequence.
#pragma once
#include <map>
#include <string>
#include <vector>
#include "plasma/Sequence.h"

namespace plasma {

class Attribute {
    friend class PLXStreamReader;
    friend class PLXStreamWriter;
    friend class SmoothMeshShape;

public:
    explicit Attribute(const std::string &name = "");
    enum {
        BASE_ELEMENT = 0,
        STORED_ELEMENT = 1
    };
    virtual ~Attribute();
    virtual int size() = 0;
    virtual int duplicateElement() = 0;
    virtual void copyToCurrent(int element) = 0;
    virtual void removeElement(int element) = 0;
    virtual void copyFromCurrent(int element) = 0;
    virtual void interpolate(int before, int start, int end, int after, float startSmoothness, float endSmoothness,
                             float t, float weight0, float weight1, float weight2, float weight3) = 0;

    bool showFrame(const std::wstring &sequenceName, int frame);
    bool apply(Sequence &sequence, int frame);
    bool advanceSequence(Sequence &sequence, int frames);
    bool advance(int frames);
    bool setFrame(int frame);
    void stop();
    void play(const std::wstring &sequenceName, int loopLength);
    int setKey(const std::wstring &sequenceName, int frame, float smoothness);
    void deleteElement(int element);
    Sequence *newSequence(const std::wstring &name);
    void deleteSequence(const std::wstring &name);
    Sequence *findSequence(const std::wstring &name);
    void operator=(const Attribute &other);

    int countSequences() const { return (int)m_sequences.size(); }
    const std::map<std::wstring, Sequence *> &getSequences() const { return m_sequences; }

protected:
    std::map<std::wstring, Sequence *> m_sequences;
    Sequence m_playback;
    int m_frame;
    int m_current;
    std::string m_name;
    int m_lastElement;
    int m_keyIndex;
    int m_unused44; // zeroed by the constructor; nothing reads it (as the 2012 engine's m_unused54)
    bool m_changed;
};

}
