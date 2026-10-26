// Keyable registers attributes and applies animation keys.
#include "plasma/Keyable.h"
#include "plasma/Attribute.h"

namespace plasma {

Keyable::Keyable(Engine *engine, const std::wstring &name)
    : NamedObject(engine, name),
      m_playing(false),
      m_changed(false)
{}

Keyable::~Keyable() {}

void Keyable::addAttribute(Attribute *attribute)
{
    m_attributes.push_back(attribute);
}

int Keyable::countSequences()
{
    int count = 0;
    for (std::list<Attribute *>::iterator it = m_attributes.begin(); it != m_attributes.end(); ++it)
        count += (*it)->countSequences();
    return count;
}

void Keyable::play(const std::wstring &sequenceName, int loopLength)
{
    m_sequenceName = sequenceName;
    for (std::list<Attribute *>::iterator it = m_attributes.begin(); it != m_attributes.end(); ++it) {
        if ((*it)->findSequence(sequenceName)) {
            (*it)->play(sequenceName, loopLength);
            m_playing = true;
        }
    }
}

bool Keyable::setFrame(int frame)
{
    bool changed = false;
    m_stepped = true;
    for (std::list<Attribute *>::iterator it = m_attributes.begin(); it != m_attributes.end(); ++it) {
        if ((*it)->setFrame(frame))
            changed = true;
    }
    if (changed)
        attributesChanged(ATTRIBUTES_CHANGED_VALUES);
    return changed;
}

void Keyable::stop()
{
    m_playing = false;
    for (std::list<Attribute *>::iterator it = m_attributes.begin(); it != m_attributes.end(); ++it)
        (*it)->stop();
}

bool Keyable::advance(int frames)
{
    bool changed = false;
    m_stepped = true;
    if (m_playing) {
        for (std::list<Attribute *>::iterator it = m_attributes.begin(); it != m_attributes.end(); ++it) {
            if ((*it)->advance(frames))
                changed = true;
        }
        if (changed) {
            attributesChanged(ATTRIBUTES_CHANGED_VALUES);
            return changed;
        }
    }
    m_playing = false;
    return changed;
}

}
