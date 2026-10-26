// ContinuousAttribute interpolates animated values between keys.
#pragma once
#include <iosfwd>
#include <map>
#include <string>
#include <vector>
#include "plasma/Attribute.h"

namespace plasma {

template<unsigned N, typename T> class Vector;
template<typename T> class Matrix;

template<typename T> class ContinuousAttribute : public Attribute {
    friend class PLXStreamReader;
    friend class PLXStreamWriter;
    // Engine.cpp's PLA attribute reader.
    template<typename A> friend void readAttribute(std::istream &, A *, std::map<int, std::string> &);

public:
    ContinuousAttribute(const std::string &name = "", const T &initial = T());
    ContinuousAttribute(const ContinuousAttribute &other);
    T &operator=(const T &value);

    T &value() { return m_values[m_current]; }

    virtual int size() { return (int)m_values.size(); }

    virtual int duplicateElement()
    {
        if (m_values.empty())
            m_values.push_back(T());
        else
            m_values.push_back(m_values[m_current]);
        return (int)(m_values.size() - 1);
    }
    virtual void copyToCurrent(int element);

    virtual void removeElement(int element)
    {
        if (element >= 0 && element < (int)m_values.size())
            m_values.erase(m_values.begin() + element);
    }

    virtual void copyFromCurrent(int element)
    {
        if (element >= 0 && element < (int)m_values.size())
            m_values[element] = m_values[m_current];
    }
    virtual void interpolate(int before, int start, int end, int after, float startSmoothness, float endSmoothness,
                             float t, float weight0, float weight1, float weight2, float weight3)
    {
        const T &beforeValue = m_values[before];
        const T &startValue = m_values[start];
        const T &endValue = m_values[end];
        const T &afterValue = m_values[after];
        // Bezier controls use quarter-length neighbor tangents and chord.
        T startTangent = (endValue - beforeValue) * 0.25f;
        T endTangent = (afterValue - startValue) * 0.25f;
        T chord = (endValue - startValue) * 0.25f;
        T startControl = startValue + (startTangent * startSmoothness + chord * (1 - startSmoothness));
        T endControl = endValue - (endTangent * endSmoothness + chord * (1 - endSmoothness));
        m_values[m_current] = startValue * weight0 + startControl * weight1 + endControl * weight2 + endValue * weight3;
    }

protected:
    std::vector<T> m_values;
};

template<typename T>
ContinuousAttribute<T>::ContinuousAttribute(const std::string &name, const T &initial)
    : Attribute(name)
{
    m_values.resize(2, initial);
}

template<typename T>
ContinuousAttribute<T>::ContinuousAttribute(const ContinuousAttribute &other)
    : Attribute()
{
    *this = other;
}

template<typename T> T &ContinuousAttribute<T>::operator=(const T &value)
{
    this->value() = value;
    return this->value();
}

template<typename T> void ContinuousAttribute<T>::copyToCurrent(int element)
{
    if (element >= 0 && element < (int)m_values.size())
        m_values[m_current] = m_values[element];
}

}
