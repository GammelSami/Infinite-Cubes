// Attribute is an animatable property of a Keyable. Named sequences refer to elements by index; playback copies a
// sequence into m_playback and applies it frame by frame, either selecting an element or interpolating between keys.
#pragma once
#include <map>
#include <string>
#include <vector>
#include "plasma/Sequence.h"

namespace plasma {

class Attribute {
    friend class PLXStreamReader; // Reads sequences and elements.
    friend class SmoothMeshShape; // Consumes change flags and reads m_current.
public:
    // The copy constructors of the templates pass no name.
    explicit Attribute(const std::string &name = "");

    // Element 0 is the one edited outside playback.
    enum {
        BASE_ELEMENT = 0,
        STORED_ELEMENT = 1
    };

    // The Continuous/Discrete templates' implicit destructors call it directly.
    virtual ~Attribute();

    // Element storage implemented by the derived templates.
    virtual int size() = 0;
    // Appends a copy of the current element (or a default one) and returns its index.
    virtual int duplicateElement() = 0;
    virtual void copyToCurrent(int element) = 0;
    virtual void removeElement(int element) = 0;
    virtual void copyFromCurrent(int element) = 0;
    // Sets the current element from the four elements around a segment (the keys before, at the start, at the end and
    // after it), with the cubic Bernstein weights of the segment's position t.
    virtual void interpolate(int before, int start, int end, int after, float startSmoothness, float endSmoothness,
                             float t, float weight0, float weight1, float weight2, float weight3) = 0;

    bool apply(Sequence &sequence, int frame);
    bool advanceSequence(Sequence &sequence, int frames);
    bool advance(int frames);
    bool setFrame(int frame);
    void stop();
    void play(const std::wstring &sequenceName, int loopLength);
    void deleteElement(int element);
    Sequence *newSequence(const std::wstring &name);
    void deleteSequence(const std::wstring &name);
    Sequence *findSequence(const std::wstring &name);
    // Copies the animation state (playback, frame, last element, sequences), not the name or the elements; the
    // templates' implicit assignment adds the elements.
    void operator=(const Attribute &other);

    int countSequences() const { return m_sequences.size(); }
    const std::map<std::wstring, Sequence *> &getSequences() const { return m_sequences; }

protected:
    std::map<std::wstring, Sequence *> m_sequences;
    Sequence m_playback; // the playing sequence's keys
    int m_frame;         // Play zeroes it, setFrame stores it
    int m_current;       // index of the element in use (the templates' value())
    std::string m_name;
    int m_lastElement; // element last copied to the current one; -1 after interpolating
    int m_keyIndex;    // segment where the last search found the frame (Play zeroes it)
    // Unused attribute state.
    int m_unused68;
    bool m_changed; // the constructor sets it
};

}
