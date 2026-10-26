// ContinuousAttribute interpolates values between animation keys. Transformation uses translation, pivot, deformation
// and rotation attributes; Display uses fill colour, stroke colour and blur radius.
#pragma once
#include <iosfwd>
#include <map>
#include <string>
#include <vector>
#include "plasma/Attribute.h"
#include "plasma/Matrix.h"
#include "plasma/Vector.h"

namespace plasma {

template<typename T> class ContinuousAttribute : public Attribute {
    friend class PLXStreamReader;
    // Legacy PLA reader accesses attribute values directly.
    template<typename A> friend void readAttribute(std::istream &, A *, std::map<int, std::string> &);

public:
    // Starts with two copies of the initial value.
    ContinuousAttribute(const std::string &name = "", const T &initial = T());

    // Returns the element currently in use.
    T &value() { return m_values[m_current]; }
    // Assigns the element in use and returns it.
    T &operator=(const T &value)
    {
        this->value() = value;
        return this->value();
    }

    virtual int size() { return m_values.size(); }

    virtual int duplicateElement()
    {
        if (m_values.empty())
            m_values.push_back(T());
        else
            m_values.push_back(m_values[m_current]);
        return m_values.size() - 1;
    }

    virtual void copyToCurrent(int element)
    {
        if (element >= 0 && element < (int)m_values.size())
            m_values[m_current] = m_values[element];
    }

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

    // A cubic Bezier segment from the start to the end element. Each key's control point lies a quarter along a blend,
    // by the key's smoothness, of the chord and the tangent through the neighbouring elements.
    virtual void interpolate(int before, int start, int end, int after, float startSmoothness, float endSmoothness,
                             float t, float weight0, float weight1, float weight2, float weight3)
    {
        const T &beforeValue = m_values[before];
        const T &startValue = m_values[start];
        const T &endValue = m_values[end];
        const T &afterValue = m_values[after];
        T startTangent = (endValue - beforeValue) * 0.25f;
        T endTangent = (afterValue - startValue) * 0.25f;
        T chord = (endValue - startValue) * 0.25f;
        T startControl = startValue + (startTangent * startSmoothness + chord * (1.0f - startSmoothness));
        T endControl = endValue - (endTangent * endSmoothness + chord * (1.0f - endSmoothness));
        T result = startValue * weight0 + startControl * weight1 + endControl * weight2 + endValue * weight3;
        m_values[m_current] = result;
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

}
