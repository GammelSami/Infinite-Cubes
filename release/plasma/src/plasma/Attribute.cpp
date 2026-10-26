#include <list>
#include "plasma/Attribute.h"
#include "plasma/EditFilter.h"

namespace plasma {

Attribute::Attribute(const std::string &name)
    : m_frame(0),
      m_current(0),
      m_name(name),
      m_lastElement(-1),
      m_keyIndex(0),
      m_unused68(0),
      m_changed(true)
{}

Attribute::~Attribute()
{
    for (std::map<std::wstring, Sequence *>::iterator it = m_sequences.begin(); it != m_sequences.end(); ++it)
        delete it->second;
}

void Attribute::operator=(const Attribute &other)
{
    m_playback = other.m_playback;
    m_frame = other.m_frame;
    m_lastElement = other.m_lastElement;
    for (std::map<std::wstring, Sequence *>::iterator it = m_sequences.begin(); it != m_sequences.end(); ++it)
        delete it->second;
    m_sequences.clear();
    for (std::map<std::wstring, Sequence *>::const_iterator it = other.m_sequences.begin();
         it != other.m_sequences.end(); ++it)
        m_sequences[it->first] = new Sequence(*it->second);
}

Sequence *Attribute::newSequence(const std::wstring &name)
{
    deleteSequence(name);
    Sequence *sequence = new Sequence;
    m_sequences[name] = sequence;
    return sequence;
}

Sequence *Attribute::findSequence(const std::wstring &name)
{
    std::map<std::wstring, Sequence *>::iterator it = m_sequences.find(name);
    if (it == m_sequences.end())
        return nullptr;
    return it->second;
}

void Attribute::play(const std::wstring &sequenceName, int loopLength)
{
    Sequence *sequence = findSequence(sequenceName);
    if (!sequence || !sequence->count())
        return;

    m_playback.clear();
    m_frame = 0;
    m_playback.m_loopLength = loopLength;
    m_keyIndex = 0;
    m_lastElement = -1;
    for (int i = 0; i < sequence->count(); i++) {
        Key &key = (*sequence)[i];
        if (loopLength <= 0 || key.frame < loopLength)
            m_playback.addKey(key.element, key.frame, key.smoothness);
    }
    // A sequence must start at time 0: start from the stored element there.
    if (m_playback.count() > 0 && m_playback[0].frame) {
        copyFromCurrent(STORED_ELEMENT);
        m_playback.addKey(STORED_ELEMENT, 0, m_playback[0].smoothness);
    }
}

void Attribute::deleteElement(int element)
{
    removeElement(element);
    for (std::map<std::wstring, Sequence *>::iterator it = m_sequences.begin(); it != m_sequences.end(); ++it) {
        Sequence *sequence = it->second;
        for (int i = 0; i < sequence->count(); i++) {
            if ((*sequence)[i].element > element)
                (*sequence)[i].element--;
        }
    }

    std::list<int> removed;
    for (int i = 0; i < m_playback.count(); i++) {
        if (m_playback[i].element == element)
            removed.push_back(i - removed.size());
    }
    for (std::list<int>::iterator it = removed.begin(); it != removed.end(); ++it)
        m_playback.removeKey(*it);
}

void Attribute::deleteSequence(const std::wstring &name)
{
    Sequence *sequence = findSequence(name);
    if (!sequence)
        return;
    m_sequences.erase(name);
    for (int i = 0; i < sequence->count(); i++)
        deleteElement((*sequence)[i].element);
    delete sequence;
}

bool Attribute::apply(Sequence &sequence, int frame)
{
    int count = sequence.count();
    if (count == 0)
        return false;

    // A single key: show its element.
    if (sequence.count() == 1) {
        if (m_lastElement == sequence[0].element)
            return false;
        copyToCurrent(sequence[0].element);
        m_lastElement = sequence[0].element;
        m_changed = true;
        return true;
    }
    if (frame < sequence[0].frame) {
        if (m_lastElement == sequence[0].element)
            return false;
        copyToCurrent(sequence[0].element);
        m_lastElement = sequence[0].element;
        m_changed = true;
        return true;
    }
    // After the last key of a sequence played once: show its element.
    if (frame >= sequence[count - 1].frame && !sequence.m_loopLength) {
        if (m_lastElement == sequence[count - 1].element)
            return false;
        copyToCurrent(sequence[count - 1].element);
        m_lastElement = sequence[sequence.count() - 1].element;
        m_changed = true;
        return true;
    }

    // Find the segment holding the frame, starting at the last one found; a looping sequence has one more segment,
    // from its last key back to the first.
    m_lastElement = -1;
    int segments = sequence.count() - 1;
    if (sequence.m_loopLength > 0)
        segments++;
    if (m_keyIndex >= segments)
        m_keyIndex = 0;
    for (int i = 0; i < segments; i++) {
        int index = (m_keyIndex + i) % segments;
        Key &start = sequence[index];
        int next = index + 1;
        if (sequence.m_loopLength > 0)
            next %= sequence.count();
        Key &end = sequence[next];
        if (start.element < 0 || end.element < 0)
            return false;
        if (start.element >= size() || end.element >= size())
            return false;

        int startFrame = start.frame;
        int endFrame = end.frame;
        if (sequence.m_loopLength > 0) {
            int loopStart = frame / sequence.m_loopLength * sequence.m_loopLength;
            startFrame += loopStart;
            endFrame += loopStart;
            if (index == segments - 1)
                endFrame += sequence.m_loopLength;
        }
        if (frame < startFrame || frame > endFrame)
            continue;

        float t = (float)(frame - startFrame) / (endFrame - startFrame);
        int before = index - 1;
        int after = index + 2;
        if (sequence.m_loopLength > 0) {
            before = (before + sequence.count()) % sequence.count();
            after %= sequence.count();
        }
        Key &previous = sequence[before];
        Key &following = sequence[after];
        if (previous.element < 0 || following.element < 0)
            return false;
        if (previous.element >= size() || following.element >= size())
            return false;

        float u = 1.0f - t;
        interpolate(previous.element, start.element, end.element, following.element, start.smoothness, end.smoothness,
                    t, u * u * u, 3.0f * t * u * u, 3.0f * t * t * u, t * t * t);
        m_keyIndex = index;
        m_changed = true;
        return true;
    }
    return false;
}

bool Attribute::setFrame(int frame)
{
    m_frame = frame;
    return apply(m_playback, frame);
}

void Attribute::stop()
{
    m_playback.clear();
    m_frame = 0;
}

// Used only by advance.
bool Attribute::advanceSequence(Sequence &sequence, int frames)
{
    if (!sequence.count())
        return false;
    int frame = m_frame;
    m_frame = frame + frames;
    return apply(sequence, frame);
}

bool Attribute::advance(int frames)
{
    bool changed = advanceSequence(m_playback, frames);
    if (!changed)
        m_playback.clear();
    return changed;
}

}
