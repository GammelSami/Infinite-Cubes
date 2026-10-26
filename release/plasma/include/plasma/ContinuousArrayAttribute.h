// ContinuousArrayAttribute stores animated arrays, such as mesh positions, colours and widths.
#pragma once
#include <iosfwd>
#include <map>
#include <string>
#include <vector>
#include "plasma/Attribute.h"

namespace plasma {

template<typename T> class ContinuousArrayAttribute : public Attribute {
    friend class CurveShape; // Reads the base element m_values[0] when attributes change.
    friend class MeshShape;  // Reads the base element's length and indices when setting faces or stretching positions.
    friend class PLXStreamReader; // Serialized ArrayAttribute size and frame chunks.
    friend class SmoothMeshShape; // Reads elements during subdivision and drawing updates.
    template<typename U>
    friend void readArrayAttribute(std::istream &, ContinuousArrayAttribute<U> *, std::map<int, std::string> &);

public:
    // Starts with two elements, each a copy of the initial array.
    ContinuousArrayAttribute(const std::string &name, const std::vector<T> &initial)
        : Attribute(name)
    {
        m_values.resize(2);
        m_values[0] = new std::vector<T>(initial);
        m_values[1] = new std::vector<T>(initial);
    }

    // Deletes every element's vector (see the bodies above).
    virtual ~ContinuousArrayAttribute()
    {
        for (int i = 0; i < (int)m_values.size(); i++)
            delete m_values[i];
    }

    virtual int size();
    virtual int duplicateElement();
    // Into the first element, not the current one.
    virtual void copyToCurrent(int element);
    virtual void removeElement(int element);
    virtual void copyFromCurrent(int element);
    // ContinuousAttribute's Bezier blend, entry by entry.
    virtual void interpolate(int before, int start, int end, int after, float startSmoothness, float endSmoothness,
                             float t, float weight0, float weight1, float weight2, float weight3);
    ContinuousArrayAttribute &operator=(const ContinuousArrayAttribute &other);

    // The element in use. Trivial accessor kept from plasma.
    std::vector<T> &value() { return *m_values[m_current]; }

    // Gives every element's array the same length.
    void resize(int size);

    // Appends the value to every element's array, and removes the entry at index from every element's array.
    void append(const T &value);
    void erase(int index);

    // The length of the arrays (of the first element), as an int.
    int length() { return (int)m_values.front()->size(); }

protected:
    std::vector<std::vector<T> *> m_values;
};

template<typename T> int ContinuousArrayAttribute<T>::size()
{
    return m_values.size();
}

template<typename T>
ContinuousArrayAttribute<T> &ContinuousArrayAttribute<T>::operator=(const ContinuousArrayAttribute &other)
{
    Attribute::operator=(other);
    for (int i = 0; i < (int)m_values.size(); i++)
        delete m_values[i];
    m_values.resize(other.m_values.size());
    for (int i = 0; i < (int)m_values.size(); i++)
        m_values[i] = new std::vector<T>(*other.m_values[i]);
    return *this;
}

// Adds an element: a copy of the current one, or an empty one when there is none.
template<typename T> int ContinuousArrayAttribute<T>::duplicateElement()
{
    if (m_values.empty())
        m_values.push_back(new std::vector<T>());
    else
        m_values.push_back(new std::vector<T>(*m_values[m_current]));
    return m_values.size() - 1;
}

template<typename T> void ContinuousArrayAttribute<T>::copyToCurrent(int element)
{
    if (element >= 0 && element < (int)m_values.size())
        *m_values[0] = *m_values[element];
}

template<typename T> void ContinuousArrayAttribute<T>::removeElement(int element)
{
    if (element >= 0 && element < (int)m_values.size()) {
        delete m_values[element];
        m_values.erase(m_values.begin() + element);
    }
}

template<typename T> void ContinuousArrayAttribute<T>::copyFromCurrent(int element)
{
    if (element >= 0 && element < (int)m_values.size())
        *m_values[element] = *m_values[m_current];
}

template<typename T>
void ContinuousArrayAttribute<T>::interpolate(int before, int start, int end, int after, float startSmoothness,
                                              float endSmoothness, float t, float weight0, float weight1, float weight2,
                                              float weight3)
{
    for (int i = 0; i < (int)m_values[start]->size(); i++) {
        const T &beforeValue = (*m_values[before])[i];
        const T &startValue = (*m_values[start])[i];
        const T &endValue = (*m_values[end])[i];
        // Cubic interpolation control curve: quarter-scaled neighbour tangents and chord, mixed by smoothness.
        T startTangent = (endValue - beforeValue) * 0.25f;
        T endTangent = ((*m_values[after])[i] - startValue) * 0.25f;
        T chord = (endValue - startValue) * 0.25f;
        T startControl = startValue + (startTangent * startSmoothness + chord * (1 - startSmoothness));
        T endControl = endValue - (endTangent * endSmoothness + chord * (1 - endSmoothness));
        (*m_values[m_current])[i] =
            startValue * weight0 + startControl * weight1 + endControl * weight2 + endValue * weight3;
    }
}

template<typename T> void ContinuousArrayAttribute<T>::resize(int size)
{
    for (int i = 0; i < (int)m_values.size(); i++)
        m_values[i]->resize(size);
}

template<typename T> void ContinuousArrayAttribute<T>::append(const T &value)
{
    for (int i = 0; i < (int)m_values.size(); i++)
        m_values[i]->push_back(value);
}

template<typename T> void ContinuousArrayAttribute<T>::erase(int index)
{
    for (int i = 0; i < (int)m_values.size(); i++)
        m_values[i]->erase(m_values[i]->begin() + index);
}

}
