// plasma::Keyable: a named object with animatable attributes (the base of Shape, Transformation and Display). It plays
// one named sequence at a time on all its attributes.
#pragma once
#include <list>
#include <string>
#include "plasma/NamedObject.h"

namespace plasma {

class Attribute;

// attributesChanged receives true for a complete rebuild after an edit or copy, or false when playback only moved
// animated values.
class Keyable : public NamedObject {
    friend class PLXStreamReader;

public:
    Keyable(Engine *engine, const std::wstring &name);
    virtual ~Keyable();

    // Boolean playback argument.
    virtual void attributesChanged(bool all) {}

    void addAttribute(Attribute *attribute);
    // Both return whether an attribute changed.
    bool advance(int frames);
    bool setFrame(int frame);
    void play(const std::wstring &sequenceName, int loopLength);
    void stop();
    int countSequences();

protected:
    friend class Widget; // Preserves m_changed while moving the object.
    friend class Node;   // Clears m_stepped and m_changed; reads m_attributes.
    std::list<Attribute *> m_attributes;
    bool m_playing;
    bool m_changed;              // Node::animate redraws and clears it
    bool m_stepped;              // set by Advance and setFrame, never cleared here
    std::wstring m_sequenceName; // Last sequence played.
};

}
