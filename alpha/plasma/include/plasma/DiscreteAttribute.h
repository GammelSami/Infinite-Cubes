// DiscreteAttribute selects values without interpolation.
#pragma once
#include <iosfwd>
#include <map>
#include <string>
#include <vector>
#include "plasma/Attribute.h"

namespace plasma {

class Engine;
class TextShape;
class SmoothMeshShape;

template<typename T> class DiscreteAttribute : public Attribute {
    friend class PLXStreamReader;
    friend class PLXStreamWriter;
    friend class Engine; // the PLA readers (readTextShape, readSmoothMeshShape) Engine.cpp's PLA attribute reader.
    template<typename A> friend void readAttribute(std::istream &, A *, std::map<int, std::string> &);

public:
    DiscreteAttribute(const std::string &name = "", const T &initial = T());
    DiscreteAttribute(const DiscreteAttribute &other);
    T &operator=(const T &value);
    T &value() { return m_values[m_current]; }

    virtual int size() { return (int)m_values.size(); }

    virtual int duplicateElement()
    {
        if (m_values.empty())
            m_values.push_back(T());
        else
            m_values.push_back(T(m_values[m_current]));
        return (int)(m_values.size() - 1);
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

// Sets the current element.
template<typename T> T &DiscreteAttribute<T>::operator=(const T &value)
{
    this->value() = value;
    return this->value();
}

}
