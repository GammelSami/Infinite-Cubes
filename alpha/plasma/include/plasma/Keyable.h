// Keyable registers attributes and applies animation keys.
#pragma once
#include <list>
#include <set>
#include <string>
#include "plasma/NamedObject.h"

namespace plasma {

class Attribute;
enum AttributeChangeReason {
    ATTRIBUTES_CHANGED_VALUES = 0,
    ATTRIBUTES_CHANGED_ALL = 1
};

class Keyable : public NamedObject {
    friend class PLXStreamReader;
    friend class PLXStreamWriter;

public:
    Keyable(Engine *engine, const std::wstring &name);
    virtual ~Keyable();
    virtual void attributesChanged(int reason) {}

    void addAttribute(Attribute *attribute);
    bool advance(int frames);
    bool setFrame(int frame);
    void play(const std::wstring &sequenceName, int loopLength);
    void stop();
    int countSequences();

protected:
    friend class Widget;
    friend class Node;
    std::list<Attribute *> m_attributes;
    bool m_playing;
    bool m_changed;
    bool m_stepped;
    std::wstring m_sequenceName;
};

}
