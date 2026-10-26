// ContinuousArrayAttribute stores an animated array attribute.
#pragma once
#include "plasma/Attribute.h"
#include <vector>

namespace plasma {

template<typename T> class ContinuousArrayAttribute : public Attribute {
public:
    // Starts with two elements, each a copy of the initial array.
    ContinuousArrayAttribute(const std::string &name, const std::vector<T> &initial)
        : Attribute(name)
    {
        m_values.resize(2);
        m_values[0] = new std::vector<T>(initial);
        m_values[1] = new std::vector<T>(initial);
    }

    // Deep-copies the other attribute's elements.
    ContinuousArrayAttribute(const ContinuousArrayAttribute &other);

    virtual ~ContinuousArrayAttribute()
    {
        for (int i = 0; i < (int)m_values.size(); i++)
            delete m_values[i];
    }

    virtual int size() { return (int)m_values.size(); }
    virtual int duplicateElement();
    virtual void copyToCurrent(int element);
    virtual void removeElement(int element);
    virtual void copyFromCurrent(int element);
    virtual void interpolate(int before, int start, int end, int after, float startSmoothness, float endSmoothness,
                             float t, float weight0, float weight1, float weight2, float weight3);
    // Replaces the elements with copies of the other attribute's.
    ContinuousArrayAttribute &operator=(const ContinuousArrayAttribute &other);

    std::vector<T> &value() { return *m_values[m_current]; }

    void append(const T &value);
    // Removes the array entry from every element.
    void erase(unsigned index);
    // Gives every element the same length.
    void resize(unsigned size);
    // The length of the arrays (the first element's).
    int length() { return (int)m_values.front()->size(); }
    std::vector<std::vector<T> *> m_values;
};

// Appends the value to every element's array.
template<typename T> void ContinuousArrayAttribute<T>::append(const T &value)
{
    for (int i = 0; i < (int)m_values.size(); i++)
        m_values[i]->push_back(value);
}

template<typename T> void ContinuousArrayAttribute<T>::erase(unsigned index)
{
    for (int i = 0; i < (int)m_values.size(); ++i)
        m_values[i]->erase(m_values[i]->begin() + index);
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

// Adds a copy of the current element, or an empty one when there is none.
template<typename T> int ContinuousArrayAttribute<T>::duplicateElement()
{
    if (m_values.empty())
        m_values.push_back(new std::vector<T>());
    else
        m_values.push_back(new std::vector<T>(*m_values[m_current]));
    return (int)(m_values.size() - 1);
}

template<typename T> void ContinuousArrayAttribute<T>::copyToCurrent(int element)
{
    // CopyToCurrent writes the first element.
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

// ContinuousAttribute's Bezier blend of the start and end elements, entry by entry. The Bezier
// control points lie a quarter of the chord (blended by the smoothness with a quarter of the
// neighbouring keys' tangent) from the keys.
template<typename T>
void ContinuousArrayAttribute<T>::interpolate(int before, int start, int end, int after, float startSmoothness,
                                              float endSmoothness, float t, float weight0, float weight1, float weight2,
                                              float weight3)
{
    for (int i = 0; i < (int)m_values[start]->size(); i++) {
        const T &from = (*m_values[start])[i];
        const T &to = (*m_values[end])[i];
        T chord = (to - from) * 0.25f;
        T endTangent = ((*m_values[after])[i] - from) * 0.25f;
        T endControl = to - (endTangent * endSmoothness + chord * (1 - endSmoothness));
        T startTangent = (to - (*m_values[before])[i]) * 0.25f;
        T startControl = startTangent * startSmoothness + chord * (1 - startSmoothness) + from;
        (*m_values[m_current])[i] = startControl * weight1 + from * weight0 + endControl * weight2 + to * weight3;
    }
}

template<typename T> void ContinuousArrayAttribute<T>::resize(unsigned size)
{
    for (int i = 0; i < (int)m_values.size(); i++)
        m_values[i]->resize(size);
}

template<typename T>
ContinuousArrayAttribute<T>::ContinuousArrayAttribute(const ContinuousArrayAttribute &other)
    : Attribute()
{
    *this = other;
}

}
