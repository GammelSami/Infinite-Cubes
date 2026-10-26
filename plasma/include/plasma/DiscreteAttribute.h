// DiscreteAttribute stores values that switch at keys, including integers and wide strings.
#pragma once
#include <iosfwd>
#include <map>
#include <string>
#include <vector>
#include "plasma/Attribute.h"

class MeshOperation;

namespace plasma {

class Engine;
class TextShape;
class SmoothMeshShape;

template<typename T> class DiscreteAttribute : public Attribute {
    friend class PLXStreamReader;
    friend class PLXStreamWriter;
    friend class ::MeshOperation; // Forgets a deleted texture's id (ForgetTexture).

    // The legacy PLA reader and writer (Engine.cpp) index the values directly.
    friend void writePLATextShape(std::ostream &, TextShape *);
    friend SmoothMeshShape *__fastcall readPLAMesh(std::map<int, int> &, Engine *, std::istream &,
                                                   std::map<int, std::string> &);
    friend TextShape *__fastcall readPLATextShape(Engine *, std::istream &, std::map<int, std::string> &);
    friend void writePLAIntAttribute(std::ostream &, DiscreteAttribute<int> *);
    friend void readPLAIntAttribute(std::istream &, DiscreteAttribute<int> *, std::map<int, std::string> &);

public:
    // Starts with two copies of the initial value.
    DiscreteAttribute(const std::string &name = "", const T &initial = T());

    // Copies an unnamed attribute, including its elements and animation state.
    DiscreteAttribute(const DiscreteAttribute &other);

    // Replaces the element in use; used by Edit::InsertText and Node::SetText.
    T &operator=(const T &value);

    // The element in use.
    T &value() { return m_values[m_current]; }

    virtual int size() { return m_values.size(); }

    virtual int duplicateElement()
    {
        if (m_values.empty())
            m_values.push_back(T());
        else
            m_values.push_back(T(m_values[m_current]));
        return m_values.size() - 1;
    }

    virtual void copyToCurrent(int element)
    {
        if (element >= 0 && element < (int)m_values.size())
            m_values[m_current] = T(m_values[element]);
    }

    virtual void removeElement(int element)
    {
        if (element >= 0 && element < (int)m_values.size())
            m_values.erase(m_values.begin() + element);
    }

    virtual void copyFromCurrent(int element)
    {
        if (element >= 0 && element < (int)m_values.size())
            m_values[element] = T(m_values[m_current]);
    }

    // Inside a segment t is below 1, so this shows the start element.
    virtual void interpolate(int before, int start, int end, int after, float startSmoothness, float endSmoothness,
                             float t, float weight0, float weight1, float weight2, float weight3)
    {
        m_values[m_current] = t < 1.0f ? T(m_values[start]) : T(m_values[end]);
    }

protected:
    std::vector<T> m_values;
};

template<typename T>
DiscreteAttribute<T>::DiscreteAttribute(const std::string &name, const T &initial)
    : Attribute(name)
{
    m_values.resize(2, initial);
}

template<typename T>
DiscreteAttribute<T>::DiscreteAttribute(const DiscreteAttribute &other)
    : Attribute()
{
    *this = other;
}

template<typename T> T &DiscreteAttribute<T>::operator=(const T &value)
{
    this->value() = value;
    return this->value();
}

template<>
void DiscreteAttribute<int>::interpolate(int before, int start, int end, int after, float startSmoothness,
                                         float endSmoothness, float t, float weight0, float weight1, float weight2,
                                         float weight3);

}
