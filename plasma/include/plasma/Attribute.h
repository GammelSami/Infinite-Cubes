// plasma::Attribute: an animatable property of a Keyable. A derived template (ContinuousAttribute,
// ContinuousArrayAttribute, DiscreteAttribute) stores the property's values ("elements") in a vector; element
// m_current is the value in use. Named sequences of keyframes refer to elements by index; a played sequence is
// copied to m_playback and applied frame by frame, either copying a key's element to the current one or
// interpolating between keys. The PLX files name these differently: an element is written as an "Attribute.frame"
// ("ArrayAttribute.frame"), a key's element as "Attribute.sequence.key.frame" and a key's frame as "...key.time".
#pragma once
#include <map>
#include <string>
#include <vector>
#include "plasma/Sequence.h"

class TextInterface;
class MeshInterface;
class TimelineMarkWidget;

namespace plasma {

class Attribute {
    friend class PLXStreamReader;
    friend class PLXStreamWriter;
    friend class ::TextInterface;      // Sets the colours' current element.
    friend class ::MeshInterface;      // Sets the colours' current element and change flag.
    friend class ::TimelineMarkWidget; // Matches the attribute names against the timeline rows.
    friend class SmoothMeshShape;      // Consumes the change flags, reads m_current.
public:
    // Template copy constructors create unnamed attributes.
    explicit Attribute(const std::string &name = "");

    // Element 1 keeps a copy of the current element: the value a played sequence starts from when its first key is
    // later than time 0 (Play), and a scratch copy (MeshShape::StorePositions, ContinuousArrayAttribute::
    // AppendElements). Every attribute starts with elements 0 and 1; element 0 is the one edited and drawn outside
    // playback (the editor's interfaces switch m_current back to it).
    enum {
        BASE_ELEMENT = 0,
        STORED_ELEMENT = 1
    };
    virtual ~Attribute();

    // The element vector, implemented by the templates.
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

    // Copies the animation state (playback, frame, sequences), not the name or the elements; the templates' implicit
    // assignment adds the elements.
    Attribute &operator=(const Attribute &other);

    int countSequences() const { return m_sequences.size(); }
    const std::map<std::wstring, Sequence *> &getSequences() const { return m_sequences; }

protected:
    std::map<std::wstring, Sequence *> m_sequences;
    Sequence m_playback; // The playing sequence's keys.
    int m_frame;
    int m_current; // Index of the element in use.
    std::string m_name;
    int m_lastElement; // Element last copied to the current one; -1 after interpolating.
    int m_keyIndex;    // Segment where the last search found the frame.
    int m_unused54;    // Zeroed by the constructor and otherwise unused.
    bool m_changed;
};

}
